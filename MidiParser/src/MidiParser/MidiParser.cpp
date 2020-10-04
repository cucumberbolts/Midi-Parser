#include "MidiParser.h"
#include "MidiEvent.h"

#include <iostream>

#define MThd 0x4d546864  // The string "MThd" in hexadecimal
#define MTrk 0x4d54726b  // The string "MTrk" in hexadecimal

MidiParser::MidiParser(const std::string& file, Mode mode) {
    if (!Open(file, mode))
        std::cout << "Failed!\n";
}

MidiParser::~MidiParser() {
    m_Stream.close();
}

bool MidiParser::Open(const std::string& file, Mode mode) {
    if (mode == Mode::Read) {
        m_Stream.open(file, std::ios_base::binary | std::ios_base::in);

        if (!m_Stream) {
            std::cout << "Could not open file!\n";
            return false;
        }

        ReadFile();
    } else {
        std::cout << "Writing not availible yet :(\n";
        return false;
    }

    return true;
}

bool MidiParser::ReadFile() {
    // The following section parses the MIDI header
    bool validHeader = true;

    if (ReadBytes<uint32_t>() != MThd)
        validHeader = false;
    if (ReadBytes<uint32_t>() != 6)
        validHeader = false;

    m_Format = ReadBytes<uint16_t>();
    m_TrackCount = ReadBytes<uint16_t>();
    m_Division = ReadBytes<uint16_t>();

    if (m_Format < 0 || m_Format > 2)
        validHeader = false;
    if (m_Division == 0)
        validHeader = false;

    if (!validHeader) {
        std::cout << "Invalid MIDI header!\n";
        return false;
    }

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
            break;
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

    MidiEventStatus status = MidiEventStatus::Success;
    while (status == MidiEventStatus::Success)
        status = ReadEvent(track);

    if (status == MidiEventStatus::Error) {
        std::cout << "Error parsing the midi file!\n";
        return false;
    }

    return true;
}

MidiParser::MidiEventStatus MidiParser::ReadEvent(MidiTrack& track) {
    uint32_t deltaTime = ReadVariableLengthValue();
    MidiEventType eventType = static_cast<MidiEventType>(ReadBytes<uint8_t>());

    if (eventType == MidiEventType::Meta) {  // Meta event
        uint8_t metaType = ReadBytes<uint8_t>();
        int32_t metaLength = ReadVariableLengthValue();

        if (metaType == (uint8_t)MetaEventType::EndOfTrack)
            return MidiEventStatus::End;

        uint8_t* data = new uint8_t[metaLength];
        ReadBytes(data, metaLength);

        delete[] data;

        return MidiEventStatus::Success;
    } else {  // Midi event
        MidiEvent event(eventType, 0, 0);

        switch (eventType) {
            // These have 1 byte of data
            case MidiEventType::ProgramChange:
            case MidiEventType::ChannelAfterTouch:
            {
                ReadBytes<>(&event.DataA);
                break;
            }
            // These have 2 bytes of data
            case MidiEventType::NoteOff:
            case MidiEventType::NoteOn:
            case MidiEventType::PolyAfter:
            case MidiEventType::ControlChange:
            case MidiEventType::PitchBend:
            {
                ReadBytes(&event.DataA);
                ReadBytes(&event.DataB);
                break;
            }
            default:
            {
                std::cout << "Error parsing MIDI events! ";
                std::cout << "Event type: " << std::hex << (uint32_t)eventType << "\n";
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
        int8_t byte;
        m_Stream.read((char*)&byte, 1);  // Read 1 more byte
        value += (byte & 0b01111111);  // The left bit isn't data so discard it
        if (!(byte & 0b10000000))  // If the left bit is 0 (that means it's the end of value)
            return value;
        value <<= 7;  // Shift 7 because the left bit isn't data
    }

    return -1;
}
