#include "Endian.h"
#include "MidiParser.h"
#include "MidiEvent.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include <fmt/core.h>

#define MThd 0x4d546864    // The string "MThd" in hexadecimal
#define MTrk 0x4d54726b    // The string "MTrk" in hexadecimal
#define HEADER_SIZE 6      // The size of the MIDI header (always 6)
#define MIDI_EVENT_SIZE 3  // The size of one MIDI event

#define BIND_ERROR_FN(fn) [this](const std::string& msg) { this->fn(msg); }

#define VERIFY(x, msg) if (!(x)) { CallError(msg); }
#define ERROR(msg) CallError(msg);

MidiParser::MidiParser(const std::string& file) : m_ErrorCallback(BIND_ERROR_FN(DefaultErrorCallback)) {
    Open(file);
}

MidiParser::MidiParser(const std::string& file, ErrorCallbackFunc callback) : m_ErrorCallback(callback) {
    Open(file);
}

bool MidiParser::Open(const std::string& file) {
    size_t size = std::filesystem::file_size(file);
    m_Buffer = new uint8_t[size];

    std::fstream input(file, std::ios_base::binary | std::ios_base::in);
    if (!input) {
        ERROR(fmt::format("Could not open file {}!", file));
        return false;
    }
    input.read((char*)m_Buffer, size);
    input.close();

    m_Position = 0;
    m_TrackList.clear();
    m_TempoMap.clear();

    VERIFY(ReadFile(), fmt::format("Failed to parse file {}!", file));
    delete[] m_Buffer;

    return m_ErrorStatus;
}

uint32_t MidiParser::GetDurationSeconds() {
    // Default is 60 bpm if no tempo is specified
    if (m_TempoMap.empty()) {
        m_TempoMap[0] = 1000000;  // 1000000 ticks per quarter note = 60 bpm
        return m_TotalTicks / m_Division;
    }

    uint64_t microseconds = 0;
    for (auto tempo = m_TempoMap.begin(); tempo != m_TempoMap.end(); tempo++) {
        auto nextTempo = std::next(tempo);
        // If another tempo event exists, use that, otherwise, use total ticks
        uint32_t nextTempoTick = nextTempo == m_TempoMap.end() ? m_TotalTicks : nextTempo->first;
        // Number of ticks this tempo lasts for
        uint32_t ticks = nextTempoTick - tempo->first;
        // Convert to microseconds
        microseconds += ticks / m_Division * tempo->second;
    }

    return (uint32_t)(microseconds / 1000000);
}

std::pair<uint32_t, uint32_t> MidiParser::GetDuration() {
    uint32_t seconds = GetDurationSeconds();
    uint32_t minutes = (seconds - (seconds % 60)) / 60;

    return { minutes, seconds % 60 };
}

bool MidiParser::ReadFile() {
    // The following section parses the MIDI header
    uint32_t mthd = ReadInteger<uint32_t>();
    uint32_t headerSize = ReadInteger<uint32_t>();
    ReadInteger(&m_Format);
    ReadInteger(&m_TrackCount);
    ReadInteger(&m_Division);

    VERIFY(mthd == MThd, "Invalid MIDI header: Expected string \"MThd\"");
    VERIFY(headerSize == HEADER_SIZE, "Invalid MIDI header: Header size is not 6");
    VERIFY(m_Format < 3, "Invalid MIDI header: Invalid MIDI format");
    VERIFY(m_Division != 0, "Invalid MIDI header: Division is 0");
    VERIFY(m_Division & 0b10000000, "Division mode not supported");

    m_TrackList.reserve(m_TrackCount);  // Reserves memory

    // This parses the tracks
    VERIFY(m_Format != 2, "Type 2 MIDI format not supported");  // Type 2 MIDI files not supported

    for (int i = 0; i < m_TrackCount; i++)
        ReadTrack();

    return m_ErrorStatus;
}

bool MidiParser::ReadTrack() {
    VERIFY((ReadInteger<uint32_t>() == MTrk), "Invalid track, Expected string \"MTrk\"");

    MidiTrack& track = AddTrack();
    track.m_Size = ReadInteger<uint32_t>();  // Size of track chunk in bytes
    track.m_Events.reserve(track.m_Size / MIDI_EVENT_SIZE);  // Reserve an approximate size

    // Reads each event in the track
    for (MidiEventStatus s = MidiEventStatus::Success; s == MidiEventStatus::Success; s = ReadEvent(track));

    // Sets the duration of the MIDI file in ticks
    if (track.m_TotalTicks > m_TotalTicks)
        m_TotalTicks = track.m_TotalTicks;

    return m_ErrorStatus;
}

MidiParser::MidiEventStatus MidiParser::ReadEvent(MidiTrack& track) {
    uint32_t deltaTime = ReadVariableLengthValue();
    MidiEventType eventType = static_cast<MidiEventType>(ReadInteger<uint8_t>());
    track.m_TotalTicks += deltaTime;

    uint8_t a;
    if ((int)eventType < 0x80) {
        a = (uint8_t)eventType;
        eventType = m_RunningStatus;
    } else {
        if ((eventType != MidiEventType::Meta) && (eventType != MidiEventType::SysEx)) {
            m_RunningStatus = eventType;
            ReadInteger(&a);
        }
    }

    if (eventType == MidiEventType::Meta) {  // Meta event
        m_RunningStatus = MidiEventType::None;

        MetaEventType metaType = static_cast<MetaEventType>(ReadInteger<uint8_t>());
        int32_t metaLength = ReadVariableLengthValue();

        if (metaType == MetaEventType::EndOfTrack)
            return MidiEventStatus::End;

        uint8_t* data = new uint8_t[metaLength];
        ReadBytes(data, metaLength);

        MetaEvent* metaEvent = new MetaEvent(track.m_TotalTicks, metaType, data, metaLength);
        track.AddEvent(metaEvent);

        if (metaType == MetaEventType::Tempo && Endian::Little)
            m_TempoMap[track.m_TotalTicks] = Endian::FlipEndian(*(uint32_t*)metaEvent->Data) >> 8;

        return MidiEventStatus::Success;
    } else if (eventType == MidiEventType::SysEx) {  // Ignore SysEx events
        m_RunningStatus = MidiEventType::None;

        uint8_t data = ReadInteger<uint8_t>();
        while (data != (uint8_t)MidiEventType::EndSysEx)
            ReadInteger<uint8_t>(&data);
    } else {  // Midi event
        m_RunningStatus = eventType;

        MidiEvent* midiEvent = new MidiEvent(track.m_TotalTicks, eventType, a, 0);

        switch (eventType) {
            // These have 2 bytes of data
            case MidiEventType::NoteOff:
            case MidiEventType::NoteOn:
            case MidiEventType::PolyAfter:
            case MidiEventType::ControlChange:
            case MidiEventType::PitchBend:
            {
                ReadInteger(&midiEvent->DataB);
                break;
            }
            // These have 1 byte of data
            case MidiEventType::ProgramChange:
            case MidiEventType::ChannelAfterTouch:
            {
                break;
            }
            default:
            {
                ERROR(fmt::format("Unrecognized event type: {:x}", eventType));
                return MidiEventStatus::Error;
            }
        }
        track.AddEvent(midiEvent);

        return MidiEventStatus::Success;
    }

    return MidiEventStatus::Error;
}

int32_t MidiParser::ReadVariableLengthValue() {
    int32_t value = 0;

    for (int i = 0; i < 16; i++) {
        uint8_t byte = ReadInteger<uint8_t>();  // Read 1 more byte
        value += (byte & 0b01111111);  // The left bit isn't data so discard it
        if (!(byte & 0b10000000))  // If the left bit is 0 (that means it's the end of value)
            return value;
        value <<= 7;  // Shift 7 because the left bit isn't data
    }

    ERROR("Unable to read variable length value");
    return -1;
}

template<typename T>
T MidiParser::ReadInteger(T* destination) {
    static_assert(std::is_integral<T>(), "Type T is not an integer!");

    T number;
    if (destination == nullptr)
        destination = &number;
    *destination = 0;

    *destination = *(T*)(m_Buffer + m_Position);
    m_Position += sizeof(T);

    if constexpr (Endian::Little)
        *destination = Endian::FlipEndian(*destination);

    return *destination;
}

void MidiParser::ReadBytes(void* buffer, size_t size) {
    memcpy(buffer, m_Buffer + m_Position, size);
    m_Position += size;
}

float MidiParser::TicksToSeconds(uint32_t startTick, uint32_t endTick) {
    auto tempo = std::find_if(m_TempoMap.rbegin(), m_TempoMap.rend(),
        [startTick](const std::pair<uint32_t, uint32_t>& tempo) -> bool {
            return tempo.first <= startTick;
        }
    );

    double microseconds = ((double)(endTick - startTick) / m_Division) * tempo->second;
    return microseconds / 1000000.f;
}

void MidiParser::CallError(const std::string& msg) {
    m_ErrorCallback(msg);
    m_ErrorStatus = false;
}

void MidiParser::DefaultErrorCallback(const std::string& msg) {
    fmt::print(msg);
}
