#include "Endian.h"
#include "MidiParser.h"
#include "MidiEvent.h"

#include <array>
#include <filesystem>
#include <fstream>

#include <fmt/core.h>

static std::array<MidiEvent*, 127> s_ActiveNotes = { nullptr };  // All the unfinished note on events

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
    m_TempoList.clear();

    ReadFile();
    delete[] m_Buffer;

    return m_ErrorStatus;
}

std::pair<uint32_t, uint32_t> MidiParser::GetDuration() {
    uint32_t seconds = GetDurationSeconds();
    uint32_t minutes = (seconds - (seconds % 60)) / 60;

    return std::make_pair(minutes, seconds % 60);
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

    m_Duration = 0;  // First tempo's duration should be 0
    for (int i = 0; i < m_TempoList.size(); i++) {
        m_TempoList[i].Time = m_Duration;
        // If another tempo event exists, use that, otherwise, use total ticks
        uint32_t nextTempoTick = (i == m_TempoList.size() - 1 ? m_TotalTicks : m_TempoList[i + 1].Tick);
        // Number of ticks this tempo lasts for
        uint32_t ticks = nextTempoTick - m_TempoList[i].Tick;
        // Convert to microseconds
        m_Duration += ticks / (uint64_t)m_Division * m_TempoList[i].Tempo;
    }

    for (MidiTrack& track : m_TrackList) {
        for (int i = 0; i < track.m_EventList.size(); i++) {
            MidiEvent* noteOn = &track.m_EventList[i];
            MidiEvent* noteOff = noteOn->NoteOff;

            if (noteOn->Type() != MidiEventType::NoteOn || noteOn->DataB == 0)
                continue;

            if (noteOff == nullptr) {
                ERROR("Could not find the end to note!");
                return false;
            }

            float start = 0, end = 0;
            for (int x = 0; x < m_TempoList.size(); ++x) {
                if ((x == m_TempoList.size() - 1) || (m_TempoList[x + 1].Tick >= noteOn->Tick)) {
                    uint32_t tick = noteOn->Tick - m_TempoList[x].Tick;
                    start = m_TempoList[x].Time + TicksToMicroseconds(tick, m_TempoList[x].Tempo);
                    break;
                }
            }

            for (int x = 0; x < m_TempoList.size(); ++x) {
                if ((x == m_TempoList.size() - 1) || (m_TempoList[x + 1].Tick >= noteOff->Tick)) {
                    uint32_t tick = noteOff->Tick - m_TempoList[x].Tick;
                    end = m_TempoList[x].Time + TicksToMicroseconds(tick, m_TempoList[x].Tempo);
                    break;
                }
            }
            noteOn->Duration = (end - start) / 1000000.f;
            noteOn->Start = start / 1000000.f;
        }
    }

    return m_ErrorStatus;
}

bool MidiParser::ReadTrack() {
    VERIFY(ReadInteger<uint32_t>() == MTrk, "Invalid track, Expected string \"MTrk\"");

    MidiTrack& track = AddTrack();
    track.m_Size = ReadInteger<uint32_t>();  // Size of track chunk in bytes
    track.m_EventList.reserve(track.m_Size / MIDI_EVENT_SIZE);  // Reserve an approximate size

    // Reads each event in the track
    for (MidiEventStatus s = MidiEventStatus::Success; s == MidiEventStatus::Success; s = ReadEvent(track));

    // Sets the duration of the MIDI file in ticks
    if (track.m_TotalTicks > m_TotalTicks)
        m_TotalTicks = track.m_TotalTicks;

    return m_ErrorStatus;
}

MidiParser::MidiEventStatus MidiParser::ReadEvent(MidiTrack& track) {
    uint32_t deltaTime = ReadVariableLengthValue();
    MidiEventType eventType = (MidiEventType)ReadInteger<uint8_t>();
    EventCategory eventCategory = eventType >= 0xf0 ? (EventCategory)eventType : EventCategory::Midi;
    track.m_TotalTicks += deltaTime;

    if (eventCategory == EventCategory::Meta) {  // Meta event
        m_RunningStatus = MidiEventType::None;

        MetaEventType metaType = static_cast<MetaEventType>(ReadInteger<uint8_t>());
        int32_t metaLength = ReadVariableLengthValue();

        if (metaType == MetaEventType::EndOfTrack)
            return MidiEventStatus::End;

        uint8_t* data = new uint8_t[metaLength];
        ReadBytes(data, metaLength);

        if (metaType == MetaEventType::Tempo) {
            uint32_t tempo = Endian::Little ? Endian::FlipEndian(*(uint32_t*)data) >> 8 : *(uint32_t*)data >> 8;
            m_TempoList.emplace_back(track.m_TotalTicks, tempo);
            return MidiEventStatus::Success;
        }

        MetaEvent metaEvent(track.m_TotalTicks, metaType, data, metaLength);

        return MidiEventStatus::Success;
    } else if (eventCategory == EventCategory::SysEx) {  // Ignore SysEx events
        m_RunningStatus = MidiEventType::None;

        fmt::print("Warning: SysEx event!\n");

        uint8_t data = ReadInteger<uint8_t>();
        while (data != (int)EventCategory::EndSysEx)
            ReadInteger<uint8_t>(&data);
    } else {  // Midi event
        uint8_t a = 0;
        if (eventType < 0x80) {
            a = eventType;
            eventType = m_RunningStatus;
        } else {
            m_RunningStatus = eventType;
            ReadInteger(&a);
        }

        MidiEvent& event = track.AddEvent(track.m_TotalTicks, eventType, a, 0);

        switch (eventType) {
            // These have 2 bytes of data
            case MidiEventType::NoteOff:
            {
                ReadInteger(&event.DataB);
                s_ActiveNotes[event.DataA]->NoteOff = &event;
                s_ActiveNotes[event.DataA] = nullptr;
                break;
            }
            case MidiEventType::NoteOn:
            {
                ReadInteger(&event.DataB);
                if (event.DataB > 0) {
                    s_ActiveNotes[event.DataA] = &event;
                } else {
                    s_ActiveNotes[event.DataA]->NoteOff = &event;
                    s_ActiveNotes[event.DataA] = nullptr;
                }
                break;
            }
            case MidiEventType::PolyAfter:
            case MidiEventType::ControlChange:
            case MidiEventType::PitchBend:
            {
                ReadInteger(&event.DataB);
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
inline T MidiParser::ReadInteger(T* destination) {
    static_assert(std::is_integral<T>(), "Type T is not an integer!");

    T number;
    if (destination == nullptr)
        destination = &number;

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

void MidiParser::CallError(const std::string& msg) {
    m_ErrorCallback(msg);
    m_ErrorStatus = false;
}

void MidiParser::DefaultErrorCallback(const std::string& msg) {
    fmt::print(msg);
}
