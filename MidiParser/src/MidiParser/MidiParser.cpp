#include "MidiParser.h"
#include "MidiEvent.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#define MThd 0x4d546864  // The string "MThd" in hexadecimal
#define MTrk 0x4d54726b  // The string "MTrk" in hexadecimal
#define HEADER_SIZE 6    // The size of the MIDI header

MidiParser::MidiParser(const std::string& file) {
    Open(file);
}

MidiParser::~MidiParser() {
    Close();
}

bool MidiParser::Open(const std::string& file) {
    m_Stream.open(file, std::ios_base::binary | std::ios_base::in);

    if (!m_Stream) {
        std::cout << "Could not open file!\n";
        return false;
    }

    Reset();

    ReadFile();

    Close();
    return true;
}

void MidiParser::Close() {
    m_Stream.close();
}

uint32_t MidiParser::GetDurationSeconds() {
    uint64_t microseconds = 0;

    // Default is 60 bpm if no tempo is specified
    if (m_TempoMap.empty()) {
        m_TempoMap[0] = 1000000;
        return m_TotalTicks / m_Division * 1000000;
    }

    for (auto it = m_TempoMap.begin(); it != m_TempoMap.end(); it++) {
        auto nextTempo = std::find_if(m_TempoMap.begin(), m_TempoMap.end(),
            [tick = it->first](const std::pair<uint32_t, uint32_t>& x) -> bool {
                return x.first > tick;
            }
        );
        uint32_t nextTick = (nextTempo == m_TempoMap.end() ? m_TotalTicks : nextTempo->first);

        uint32_t ticks = nextTick - it->first;
        uint32_t quarterNotes = ticks / m_Division;
        microseconds += (uint64_t)quarterNotes * it->second;
    }

    return microseconds * 0.000001;
}

std::pair<uint32_t, uint32_t> MidiParser::GetDuration() {
    uint32_t seconds = GetDurationSeconds() + 1;  // Round up
    uint32_t minutes = (seconds - (seconds % 60)) / 60;

    return { minutes, seconds % 60 };
}

void MidiParser::Reset() {
    m_TotalTicks = 0;
    m_TrackList.clear();
    m_TempoMap.clear();
}

bool MidiParser::ReadFile() {
    // The following section parses the MIDI header
    if (ReadBytes<uint32_t>() != MThd)
        return false;
    if (ReadBytes<uint32_t>() != HEADER_SIZE)
        return false;

    ReadBytes(&m_Format);
    ReadBytes(&m_TrackCount);
    ReadBytes(&m_Division);

    if (m_Format < 0 || m_Format > 2)
        return false;
    if (m_Division == 0)
        return false;

    m_TrackList.reserve(m_TrackCount);

    // This parses the tracks
    switch (m_Format) {
        case 1:
        {
            for (int i = 0; i < m_TrackCount; i++)
                if (!ReadTrack())
                    return false;
            break;
        }
        default:
        {
            std::cout << "MIDI format not supported yet!\n";
            return false;
        }
    }

    return true;
}

bool MidiParser::ReadTrack() {
    if (ReadBytes<uint32_t>() != MTrk) {
        std::cout << "Invalid track!\n";
        return false;
    }

    uint32_t trackSize = ReadBytes<uint32_t>();  // Size of track chunk in bytes
    MidiTrack& track = AddTrack();
    track.m_Size = trackSize;
    track.m_Events.reserve(trackSize / sizeof(MidiEvent));  // Reserve an approximate size

    // Reads each event in the track
    MidiEventStatus status = MidiEventStatus::Success;
    while (status == MidiEventStatus::Success)
        status = ReadEvent(track);

    // Sets the duration of the MIDI file in ticks
    if (track.m_TotalTicks > m_TotalTicks)
        m_TotalTicks = track.m_TotalTicks;

    if (status == MidiEventStatus::Error) {
        std::cout << "Error parsing MIDI file!\n";
        return false;
    }

    return true;
}

MidiParser::MidiEventStatus MidiParser::ReadEvent(MidiTrack& track) {
    uint32_t deltaTime = ReadVariableLengthValue();
    MidiEventType eventType = static_cast<MidiEventType>(ReadBytes<uint8_t>());

    uint8_t a;
    if ((int)eventType < 0x80) {
        if (m_RunningStatus == MidiEventType::None) {
            std::cout << "Error: running status with no previous command! eventType: " << (int)eventType << "\n";
            return MidiEventStatus::Error;
        }
        // Meta (0xff), SysEx (can be 0xf0 or 0xf7)
        if ((int)m_RunningStatus >= 0xf0) {
            std::cout << "Error: running status cannot be a meta or sysex event.\n";
            return MidiEventStatus::Error;
        }

        a = (uint8_t)eventType;
        eventType = (MidiEventType)m_RunningStatus;
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

        if (metaType == MetaEventType::Tempo) {
            uint32_t tempo;  // Store tempo here
            tempo = data[2] | (data[1] << 8) | (data[0] << 16); // Convert endian and store in uint32_t
            m_TempoMap[track.m_TotalTicks] = tempo;
        }

        delete[] data;

        return MidiEventStatus::Success;
    } else if (eventType == MidiEventType::SysEx) {  // Ignore SysEx events for now 'cause I dunno what they do...
        m_RunningStatus = MidiEventType::None;

        uint8_t data = ReadBytes<uint8_t>();
        while (data != (uint8_t)MidiEventType::EndSysEx)
            data = ReadBytes<uint8_t>();
    } else {  // Midi event
        m_RunningStatus = eventType;

        MidiEvent event(eventType, a, 0);

        switch (eventType) {
            // These have 1 byte of data
            case MidiEventType::ProgramChange:
            case MidiEventType::ChannelAfterTouch:
            {
                break;
            }
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
            default:
            {
                std::cout << std::hex;
                std::cout << "Error! Unrecognized event type: " << (uint32_t)eventType << "\n";
                std::cout << std::dec;
                return MidiEventStatus::Error;
            }
        }
        track.AddEvent(event);

        return MidiEventStatus::Success;
    }

    return MidiEventStatus::Error;
}

MidiTrack& MidiParser::AddTrack() {
    return m_TrackList.emplace_back();
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

    return -1;
}

template<typename T>
T MidiParser::ReadBytes(T* destination) {
    static_assert(std::is_integral<T>(), "Type T is not an integer!");

    T number = 0;
    if (!destination)
        destination = &number;

    if constexpr (sizeof(T) == 1) {
        *destination = m_Stream.get();
    } else {
        uint8_t* numBuff = (uint8_t*)destination;  // Type punning :)
        uint8_t buffer[sizeof(T)];

        m_Stream.read((char*)buffer, sizeof(T));

        for (int i = 0; i < sizeof(T); i++)
            numBuff[i] = buffer[sizeof(T) - 1 - i];
    }
    return *destination;
}

void MidiParser::ReadBytes(const void* buffer, size_t size) {
    m_Stream.read((char*)buffer, size);
}
