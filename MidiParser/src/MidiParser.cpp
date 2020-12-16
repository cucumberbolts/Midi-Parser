#include "MidiParser.h"
#include "Endian.h"
#include "MidiEvent.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <type_traits>

#define MThd 0x4d546864 // The string "MThd" in hexadecimal
#define MTrk 0x4d54726b // The string "MTrk" in hexadecimal
#define HEADER_SIZE 6   // The size of the MIDI header (always 6)

#define DEFAULT_TEMPO 1000000 // One million microseconds per quarter note or 60 bpm

#define VERIFY(x, msg) if (!(x)) { Error(msg); }
#define ERROR(msg) Error(msg);

MidiParser::MidiParser(const std::string& file) {
    Open(file);
}

bool MidiParser::Open(const std::string& file) {
    m_ReadPosition = 0;
    m_TrackList.clear();
    m_TempoList.clear();

    std::fstream input(file, std::ios_base::binary | std::ios_base::in);
    if (!input) {
        ERROR("Could not open file " + file);
        return false;
    }

    input.seekg(0, input.end);
    size_t size = input.tellg();
    input.seekg(0, input.beg);
    m_Data.resize(size);

    input.read((char*)m_Data.data(), size);
    input.close();

    ReadFile();

    return m_ErrorStatus;
}

std::pair<uint32_t, uint32_t> MidiParser::GetDurationMinutesAndSeconds() {
    return { (uint32_t)(m_Duration / 1000000 / 60), (uint32_t)(m_Duration / 1000000 % 60) };
}

bool MidiParser::ReadFile() {
    // The following section parses the MIDI header
    uint32_t mthd = ReadInteger<uint32_t>();
    uint32_t headerSize = ReadInteger<uint32_t>();
    ReadInteger(&m_Format);
    ReadInteger(&m_TrackCount);
    ReadInteger(&m_Division);

    VERIFY(mthd == MThd, "Invalid MIDI header: expected string \"MThd\"");
    VERIFY(headerSize == HEADER_SIZE, "Invalid MIDI header: header size is not 6");
    VERIFY(m_Format < 3, "Invalid MIDI header: invalid MIDI format");
    VERIFY(m_Division != 0, "Invalid MIDI header: division is 0");

    VERIFY(!(m_Division & 0x8000), "Division mode not supported");  // Checks if the left-most bit is not 1
    VERIFY(m_Format != 2, "Type 2 MIDI format not supported");

    m_TrackList.reserve(m_TrackCount);  // Reserves memory

    // This parses the tracks
    for (int i = 0; i < m_TrackCount; i++)
        ReadTrack();

    // Calculates the time stamps for each tempo
    m_Duration = 0;  // First tempo's duration/tick should be 0
    if (m_TempoList[0].m_Tick != 0)
        m_TempoList.emplace(m_TempoList.begin(), 0, DEFAULT_TEMPO);

    for (auto it = m_TempoList.begin(); it != m_TempoList.end(); it++) {
        it->m_Time = m_Duration;
        auto next = std::next(it);
        uint32_t nextTempoTick = (next == m_TempoList.end() ? m_TotalTicks : next->m_Tick);
        uint32_t ticks = nextTempoTick - it->m_Tick;
        m_Duration += TicksToMicroseconds(ticks, it->m_Tempo);
    }
#if 0
    // Calculates the note durations
    TempoEvent* tempo = &m_TempoList[0];

    for (MidiTrack& track : m_TrackList) {
        for (int i = 0; i < track.GetEventCount(); i++) {
            if (track[i]->Type() == MetaEventType::Tempo)
                tempo = (TempoEvent*)track[i];

            if (track[i]->Type() != MidiEventType::NoteOn)
                continue;
            if (((MidiEvent*)track[i])->m_DataB == 0)
                continue;

            MidiEvent* noteOn = (MidiEvent*)track[i];
            MidiEvent* noteOff = nullptr;

            // Lmao what is this
            for (int x = 1; x < track.GetEventCount(); x++)
                if (track[x]->GetCategory() == EventCategory::Midi)
                    if (track[x]->Type() == MidiEventType::NoteOff || ((MidiEvent*)track[x])->GetDataB() == 0)
                        if (((MidiEvent*)track[x])->GetDataA() == noteOn->GetDataA())
                            noteOff = (MidiEvent*)track[x];

            float start = tempo->GetTime() + TicksToMicroseconds(noteOn->m_Tick - tempo->GetTick(), tempo->GetTempo());
            float end = tempo->GetTime() + TicksToMicroseconds(noteOff->m_Tick - tempo->GetTick(), tempo->GetTempo());

            noteOn->m_Duration = (end - start) / 1000000.f;
            noteOn->m_Start = start / 1000000.f;
        }
    }
#endif
    return m_ErrorStatus;
}

bool MidiParser::ReadTrack() {
    VERIFY(ReadInteger<uint32_t>() == MTrk, "Invalid track: expected string \"MTrk\"");

    uint32_t size = ReadInteger<uint32_t>();  // Size of track chunk in bytes
    MidiTrack& track = AddTrack(size);

    // Reads each event in the track
    for (MidiEventStatus s = MidiEventStatus::Success; s == MidiEventStatus::Success; )
        s = ReadEvent(track);

    // Sets the duration of the MIDI file in ticks
    if (track.m_TotalTicks > m_TotalTicks)
        m_TotalTicks = track.m_TotalTicks;

    return m_ErrorStatus;
}

MidiParser::MidiEventStatus MidiParser::ReadEvent(MidiTrack& track) {
    uint32_t deltaTime = ReadVariableLengthValue();  // Ticks since last event
    MidiEventType eventType = (MidiEventType)ReadInteger<uint8_t>();
    EventCategory eventCategory = eventType >= 0xf0 ? (EventCategory)eventType : EventCategory::Midi;
    track.m_TotalTicks += deltaTime;

    if (eventCategory == EventCategory::Meta) {  // Meta event
        m_RunningStatus = MidiEventType::None;

        MetaEventType metaType = (MetaEventType)ReadInteger<uint8_t>();
        int32_t metaLength = ReadVariableLengthValue();

        if (metaType == MetaEventType::EndOfTrack)
            return MidiEventStatus::End;

        uint8_t* data = new uint8_t[metaLength];
        ReadBytes(data, metaLength);

        if (metaType == MetaEventType::Tempo)
            m_TempoList.emplace_back(track.m_TotalTicks, CalculateTempo(data, metaLength));

        track.AddEvent<MetaEvent>(track.m_TotalTicks, metaType, data, metaLength);

        return MidiEventStatus::Success;
    } else if (eventCategory == EventCategory::SysEx) {  // Ignore SysEx events
        m_RunningStatus = MidiEventType::None;

        ERROR("SysEx events not supported yet");
    } else {  // Midi event
        uint8_t a;
        if (eventType < 0x80) {
            a = eventType;
            eventType = m_RunningStatus;
        } else {
            m_RunningStatus = eventType;
            ReadInteger(&a);
        }

        uint8_t channel = eventType & 0x0f;
        MidiEvent* event = track.AddEvent<MidiEvent>(track.m_TotalTicks, eventType, channel, a, 0);

        switch (eventType - channel) {
            // These have 2 bytes of data
            case MidiEventType::NoteOff:
            case MidiEventType::NoteOn:
            case MidiEventType::PolyAfter:
            case MidiEventType::ControlChange:
            case MidiEventType::PitchBend:
            {
                ReadInteger(&event->m_DataB);
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
                std::ostringstream ss;
                ss << "Unrecognized event type: " << std::hex << (int)eventType;
                ERROR(ss.str());
                return MidiEventStatus::Error;
            }
        }

        return MidiEventStatus::Success;
    }

    return MidiEventStatus::Error;
}

inline int32_t MidiParser::ReadVariableLengthValue() {
    int32_t value = 0;

    for (int i = 0; i < 16; i++) {
        uint8_t byte = ReadInteger<uint8_t>();
        value += (byte & 0b01111111);
        if (!(byte & 0b10000000))  // If the left bit is 0 (end of value)
            return value;
        value <<= 7;
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

    *destination = *(T*)(m_Data.data() + m_ReadPosition);
    m_ReadPosition += sizeof(T);

    if constexpr (Endian::Little)
        *destination = Endian::FlipEndian(*destination);

    return *destination;
}

inline void MidiParser::ReadBytes(void* buffer, size_t size) {
    memcpy(buffer, m_Data.data() + m_ReadPosition, size);
    m_ReadPosition += size;
}

inline uint64_t MidiParser::TicksToMicroseconds(uint32_t ticks, uint32_t tempo) {
    return ticks / (uint64_t)m_Division * tempo;
}

inline uint32_t MidiParser::CalculateTempo(uint8_t* data, size_t size) {
    VERIFY(size == 3, "Invalid tempo!");  // Data should be 24 bits or 3 bytes

    if constexpr (Endian::Little)
        return Endian::FlipEndian(*(uint32_t*)data) >> 8;
    else
        return *(uint32_t*)data >> 8;
}

void MidiParser::Error(const std::string& msg) {
    std::cout << msg << "\n";
    m_ErrorStatus = false;
}
