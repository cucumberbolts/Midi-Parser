#include "MidiParser.h"
#include "MidiEvent.h"

#include <algorithm>

#include <fmt/core.h>

#define MThd 0x4d546864  // The string "MThd" in hexadecimal
#define MTrk 0x4d54726b  // The string "MTrk" in hexadecimal
#define HEADER_SIZE 6    // The size of the MIDI header (always 6)

#define BIND_ERROR_FN(fn) [this](const std::string& msg) { this->fn(msg); }

#define VERIFY(x, msg) if (!x) { CallError(msg); }
#define ERROR(msg) CallError(msg);

MidiParser::MidiParser(const std::string& file) {
    m_ErrorCallback = BIND_ERROR_FN(DefaultErrorCallback);
    Open(file);
}

MidiParser::MidiParser(const std::string& file, ErrorCallbackFunc callback) : m_ErrorCallback(callback) {
    Open(file);
}

bool MidiParser::Open(const std::string& file) {
    m_Stream.open(file, std::ios_base::binary | std::ios_base::in);

    VERIFY(m_Stream, fmt::format("Could not open file {}!", file));

    m_TotalTicks = 0;
    m_TrackList.clear();
    m_TempoMap.clear();

    VERIFY(ReadFile(), fmt::format("Failed to read file {}!", file));

    Close();
    return m_ErrorStatus;
}

uint32_t MidiParser::GetDurationSeconds() {
    // Default is 60 bpm if no tempo is specified
    if (m_TempoMap.empty())
        return TicksToMicroseconds(m_TotalTicks, 1000000);  // 1000000 ticks per quarter note is 60 bpm

    uint64_t microseconds = 0;

    for (auto it = m_TempoMap.begin(); it != m_TempoMap.end(); it++) {
        auto nextTempo = std::find_if(m_TempoMap.begin(), m_TempoMap.end(),
            [tick = it->first](const std::pair<uint32_t, uint32_t>& x) -> bool {
                return x.first > tick;
            }
        );
        // If another tempo event exists, use that, otherwise, use total ticks
        uint32_t nextTick = (nextTempo == m_TempoMap.end() ? m_TotalTicks : nextTempo->first);

        uint32_t ticks = nextTick - it->first;
        microseconds += TicksToMicroseconds(ticks, it->second);
    }

    return microseconds / 1000000;
}

std::pair<uint32_t, uint32_t> MidiParser::GetDuration() {
    uint32_t seconds = GetDurationSeconds();
    uint32_t minutes = (seconds - (seconds % 60)) / 60;

    return { minutes, seconds % 60 };
}

bool MidiParser::ReadFile() {
    // The following section parses the MIDI header
    uint32_t mthd = ReadBytes<uint32_t>();
    uint32_t headerSize = ReadBytes<uint32_t>();
    ReadBytes(&m_Format);
    ReadBytes(&m_TrackCount);
    ReadBytes(&m_Division);

    VERIFY((mthd == MThd), "Invalid MIDI header: Expected string MThd");
    VERIFY((headerSize == HEADER_SIZE), "Invalid MIDI header: Header size is not 6");
    VERIFY((m_Format > 0 && m_Format < 3), "Invalid MIDI header: Invalid MIDI format");
    VERIFY((m_Division != 0), "Invalid MIDI header: Division is 0");
    VERIFY((m_Division & 0b10000000), "Division mode not supported");

    m_TrackList.reserve(m_TrackCount);  // Reserves memory

    // This parses the tracks
    switch (m_Format) {
        case 1:
        {
            for (int i = 0; i < m_TrackCount; i++)
                ReadTrack();
            break;
        }
        default:
        {
            ERROR("MIDI format not supported yet!");
        }
    }

    return m_ErrorStatus;
}

bool MidiParser::ReadTrack() {
    VERIFY((ReadBytes<uint32_t>() == MTrk), "Invalid track");

    MidiTrack& track = AddTrack();
    track.m_Size = ReadBytes<uint32_t>();  // Size of track chunk in bytes
    track.m_Events.reserve(track.m_Size / sizeof(MidiEvent));  // Reserve an approximate size

    // Reads each event in the track
    MidiEventStatus status = MidiEventStatus::Success;
    while (status == MidiEventStatus::Success)
        status = ReadEvent(track);

    // Sets the duration of the MIDI file in ticks
    if (track.m_TotalTicks > m_TotalTicks)
        m_TotalTicks = track.m_TotalTicks;

    return m_ErrorStatus;
}

MidiParser::MidiEventStatus MidiParser::ReadEvent(MidiTrack& track) {
    uint32_t deltaTime = ReadVariableLengthValue();
    MidiEventType eventType = static_cast<MidiEventType>(ReadBytes<uint8_t>());

    uint8_t a;
    if ((int)eventType < 0x80) {
        VERIFY((m_RunningStatus != MidiEventType::None), fmt::format("Running status set to nothing. Event type: {:x}", eventType));
        VERIFY(((int)m_RunningStatus < 0xf0), "Error: running status cannot be a meta or sysex event");

        a = (uint8_t)eventType;
        eventType = m_RunningStatus;
    } else {
        m_RunningStatus = eventType;
        if ((eventType != MidiEventType::Meta) && (eventType != MidiEventType::SysEx))
            ReadBytes(&a);
    }

    track.m_TotalTicks += deltaTime;

    if (eventType == MidiEventType::Meta) {  // Meta event
        m_RunningStatus = MidiEventType::None;

        MetaEventType metaType = (MetaEventType)ReadBytes<uint8_t>();
        int32_t metaLength = ReadVariableLengthValue();

        if (metaType == MetaEventType::EndOfTrack)
            return MidiEventStatus::End;

        uint8_t* data = new uint8_t[metaLength];
        ReadBytes(data, metaLength);

        if (metaType == MetaEventType::Tempo)
            m_TempoMap[track.m_TotalTicks] = data[2] | (data[1] << 8) | (data[0] << 16);  // Convert endian

        delete[] data;

        return MidiEventStatus::Success;
    } else if (eventType == MidiEventType::SysEx) {  // Ignore SysEx events for now 'cause I dunno what they do...
        m_RunningStatus = MidiEventType::None;

        uint8_t data = ReadBytes<uint8_t>();
        while (data != (uint8_t)MidiEventType::EndSysEx)
            ReadBytes<uint8_t>(&data);
    } else {  // Midi event
        m_RunningStatus = eventType;

        MidiEvent event(eventType, a, 0);

        switch (eventType) {
            // These have 2 bytes of data
            case MidiEventType::NoteOff:
            case MidiEventType::NoteOn:
            case MidiEventType::PolyAfter:
            case MidiEventType::ControlChange:
            case MidiEventType::PitchBend:
            {
                ReadBytes(&event.DataB);
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
                ERROR(fmt::format("Error! Unrecognized event type: {:x}", eventType));
                return MidiEventStatus::Error;
            }
        }
        track.AddEvent(event);

        return MidiEventStatus::Success;
    }

    return MidiEventStatus::Error;
}

int32_t MidiParser::ReadVariableLengthValue() {
    int32_t value = 0;

    for (int i = 0; i < 16; i++) {
        uint8_t byte = m_Stream.get(); // Read 1 more byte
        value += (byte & 0b01111111);  // The left bit isn't data so discard it
        if (!(byte & 0b10000000))  // If the left bit is 0 (that means it's the end of value)
            return value;
        value <<= 7;  // Shift 7 because the left bit isn't data
    }

    ERROR("Error reading variable length value");
    return -1;
}

template<typename T>
T MidiParser::ReadBytes(T* destination) {
    static_assert(std::is_integral<T>(), "Type T is not an integer!");

    T number;
    if (destination == nullptr)
        destination = &number;
    *destination = 0;

    if constexpr (sizeof(T) == 1) {
        *destination = m_Stream.get();
    } else {
        uint8_t buffer[sizeof(T)];
        m_Stream.read((char*)buffer, sizeof(T));

        for (int i = 0; i < sizeof(T); i++)
            *destination += buffer[sizeof(T) - 1 - i] << (i * 8);
    }
    return *destination;
}

void MidiParser::ReadBytes(const void* buffer, size_t size) {
    m_Stream.read((char*)buffer, size);
}

void MidiParser::CallError(const std::string& msg) {
    m_ErrorCallback(msg);
    m_ErrorStatus = false;
}

void MidiParser::DefaultErrorCallback(const std::string& msg) {
    fmt::print(msg);
}
