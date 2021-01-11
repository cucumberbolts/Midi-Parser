#include "MidiParser.h"
#include "Endian.h"
#include "MidiEvent.h"

#include <fstream>
#include <iostream>
#include <sstream>

#define MThd 0x4d546864 // The string "MThd" in hexadecimal
#define MTrk 0x4d54726b // The string "MTrk" in hexadecimal
#define HEADER_SIZE 6   // The size of the MIDI header (always 6)

#define DEFAULT_TEMPO 500000 // Five hundred thousand microseconds per quarter note or 120 bpm

#define VERIFY(x, msg) if (!(x)) { Error(msg); }
#define ERROR(msg) Error(msg);

MidiParser::MidiParser(const std::string& file) {
    Open(file);
}

bool MidiParser::Open(const std::string& file) {
    m_ReadPosition = 0;
    m_TrackList.clear();

    std::fstream input(file, std::ios_base::binary | std::ios_base::in);
    if (!input) {
        ERROR("Could not open file " + file);
        return false;
    }

    input.seekg(0, input.end);
    size_t size = input.tellg();
    input.seekg(0, input.beg);

    if (size > m_Data.size())
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
    uint32_t mthd = ReadInteger();
    uint32_t headerSize = ReadInteger();
    m_Format = ReadShort();
    m_TrackCount = ReadShort();
    m_Division = ReadShort();

    VERIFY(mthd == MThd, "Invalid MIDI header: expected string \"MThd\"");
    VERIFY(headerSize == HEADER_SIZE, "Invalid MIDI header: header size is not 6");
    VERIFY(m_Format < 3, "Invalid MIDI header: invalid MIDI format");
    VERIFY(m_Division != 0, "Invalid MIDI header: division is 0");

    VERIFY(!(m_Division & 0x8000), "Division mode not supported");  // Checks if the left-most bit is not 1
    VERIFY(m_Format != 2, "Type 2 MIDI format not supported");

    return m_ErrorStatus;

    if (!m_ErrorStatus)
        return false;

    m_TrackList.reserve(m_TrackCount);  // Reserves memory

    // This parses the tracks
    for (int i = 0; i < m_TrackCount; i++)
        if (!ReadTrack())
            break;

    return m_ErrorStatus;
}

bool MidiParser::ReadTrack() {
    VERIFY(ReadInteger() == MTrk, "Invalid track: expected string \"MTrk\"");

    uint32_t size = ReadInteger();  // Size of track chunk in bytes
    MidiTrack& track = m_TrackList.emplace_back(size * 8);  // 8 is a good number I guess

    if (m_ReadPosition + size > m_Data.size()) {
        ERROR("Invalid track size");
        return false;
    }

    MidiEventType runningStatus = MidiEventType::None;

    // Reads each event in the track
    for (MidiEventStatus s = MidiEventStatus::Success; s == MidiEventStatus::Success; )
        s = ReadEvent(track, runningStatus);

    // Sets the duration of the MIDI file in ticks
    if (track.m_TotalTicks > m_TotalTicks)
        m_TotalTicks = track.m_TotalTicks;

    return m_ErrorStatus;
}

MidiParser::MidiEventStatus MidiParser::ReadEvent(MidiTrack& track, MidiEventType& runningStatus) {
    uint32_t deltaTime = ReadVariableLengthValue();  // Ticks since last event
    MidiEventType eventType = (MidiEventType)ReadByte();
    EventCategory eventCategory = eventType >= 0xf0 ? (EventCategory)eventType : EventCategory::Midi;
    track.m_TotalTicks += deltaTime;

    if (eventCategory == EventCategory::Meta) {  // Meta event
        runningStatus = MidiEventType::None;

        MetaEventType metaType = (MetaEventType)ReadByte();
        int32_t metaLength = ReadVariableLengthValue();

        if (metaType == MetaEventType::EndOfTrack)
            return MidiEventStatus::End;

        std::vector<uint8_t> data(metaLength);
        ReadBytes(data.data(), metaLength);

        track.AppendEvent<MetaEvent>(track.m_TotalTicks, 0.0f, metaType, std::move(data));

        return MidiEventStatus::Success;
    } else if (eventCategory == EventCategory::SysEx) {  // Ignore SysEx events
        runningStatus = MidiEventType::None;

        ERROR("SysEx events not supported yet");
        return MidiEventStatus::Error;
    } else {  // Midi event
        uint8_t a, b;
        if (eventType < 0x80) {
            a = eventType;
            eventType = runningStatus;
        } else {
            runningStatus = eventType;
            a = ReadByte();
        }

        uint8_t channel = eventType & 0x0f;

        switch (eventType - channel) {
            // These have 2 bytes of data
            case MidiEventType::NoteOn:
            {
                b = ReadByte();
                if (b == 0)
                    eventType = MidiEventType::NoteOff;
                break;
            }
            case MidiEventType::NoteOff:
            case MidiEventType::PolyAfter:
            case MidiEventType::ControlChange:
            case MidiEventType::PitchBend:
            {
                b = ReadByte();
                break;
            }
            // These have 1 byte of data
            case MidiEventType::ProgramChange:
            case MidiEventType::ChannelAfterTouch:
            {
                break;
            }
            // Error
            default:
            {
                std::ostringstream ss;
                ss << "Unrecognized event type: " << std::hex << "0x" << (int)eventType;
                ERROR(ss.str());
                return MidiEventStatus::Error;
            }
        }

        track.AppendEvent<MidiEvent>(track.m_TotalTicks, 0.0f, eventType, channel, a, b);

        return MidiEventStatus::Success;
    }

    return MidiEventStatus::Error;
}

inline int32_t MidiParser::ReadVariableLengthValue() {
    int32_t value = 0;

    for (int i = 0; i < 16; i++) {
        uint8_t byte = ReadByte();
        value += (byte & 0b01111111);
        if (!(byte & 0b10000000))  // If the left bit is 0 (end of value)
            return value;
        value <<= 7;
    }

    ERROR("Unable to read variable length value");
    return -1;
}

inline uint8_t MidiParser::ReadByte() {
    uint8_t number = *(uint8_t*)(m_Data.data() + m_ReadPosition);
    m_ReadPosition += sizeof(uint8_t);

    return number;
}

inline uint16_t MidiParser::ReadShort() {
    uint16_t number = *(uint16_t*)(m_Data.data() + m_ReadPosition);
    m_ReadPosition += sizeof(uint16_t);

    if constexpr (Endian::Little)
        number = Endian::FlipEndian(number);

    return number;
}

inline uint32_t MidiParser::ReadInteger() {
    uint32_t number = *(uint32_t*)(m_Data.data() + m_ReadPosition);
    m_ReadPosition += sizeof(uint32_t);

    if constexpr (Endian::Little)
        number = Endian::FlipEndian(number);

    return number;
}

inline void MidiParser::ReadBytes(uint8_t* buffer, size_t size) {
    std::copy(m_Data.data() + m_ReadPosition, m_Data.data() + m_ReadPosition + size, buffer);
    m_ReadPosition += size;
}

inline float MidiParser::TicksToMicroseconds(uint32_t ticks, uint32_t tempo) {
    return ticks / (float)m_Division * tempo;
}

void MidiParser::Error(const std::string& msg) {
    std::cout << "Error: " << msg << "\n";
    m_ErrorStatus = false;
}
