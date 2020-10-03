#include "MidiParser.h"
#include "MidiEvent.h"

#include <iostream>

MidiParser::MidiParser(const std::string& file) {
    if (!Open(file))
        std::cout << "Failed!\n";
}

MidiParser::~MidiParser() {
    m_Stream.close();
}

bool MidiParser::Open(const std::string& file) {
    m_Stream.open(file, std::ios_base::binary | std::ios_base::in);

    if (!m_Stream) {
        std::cout << "could not open file!\n";
        return false;
    }

    ReadFile();

    return true;
}

bool MidiParser::ReadFile() {
    // The following section parses the MIDI header
    bool validHeader = true;

    if (ReadBytes<uint32_t>() != 0x4d546864)
        validHeader = false;
    if (ReadBytes<uint32_t>() != 6)
        validHeader = false;

    m_Format = ReadBytes<uint16_t>();
    std::cout << "Format: " << m_Format << std::endl;
    m_TrackCount = ReadBytes<uint16_t>();
    std::cout << "Track Count: " << m_TrackCount << std::endl;
    m_Division = ReadBytes<uint16_t>();
    std::cout << "Division: " << m_Division << std::endl;

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
    if (ReadBytes<uint32_t>() != 0x4D54726B) {
        std::cout << "Invalid track!\n";
        return false;
    }

    uint32_t trackSize = ReadBytes<uint32_t>();  // Size of track chunk in bytes

    std::cout << "Track size: " << trackSize << std::endl;

    MidiEventStatus status = MidiEventStatus::Success;
    while (status == MidiEventStatus::Success) {
        status = ReadEvent();
    }

    if (status == MidiEventStatus::Error) {
        std::cout << "Error parsing the midi file!\n";
        return false;
    }

    return true;
}

MidiEventStatus MidiParser::ReadEvent() {
    int deltaTime = ReadVariableLengthValue();
    std::cout << "deltaTime: " << deltaTime << "\n";
    uint8_t eventType = ReadBytes<uint8_t>();
    std::cout << "Event Type: " << std::hex << (int)eventType << "\n";

    if (eventType == 0xff) {  // Meta event
        std::cout << "Meta Event! ";
        uint8_t metaType = ReadBytes<uint8_t>();
        int metaLength = ReadVariableLengthValue();

        if (metaType == (uint8_t)MetaEventType::EndOfTrack) {
            std::cout << "End of track!\n";
            return MidiEventStatus::End;
        }

        uint8_t* data = new uint8_t[metaLength];
        ReadBytes(data, metaLength);

        std::cout << "data\n";
        for (int i = 0; i < metaLength; i++)
            std::cout << (int)data[i] << "\n";

        delete[] data;

        return MidiEventStatus::Success;
    } else {  // Midi event
        std::cout << "Midi Event! ";

        MidiEventType midiEventType = (MidiEventType)eventType;

        MidiEvent event(midiEventType, 0, 0);

        switch (midiEventType) {
            case MidiEventType::ProgramChange:
            case MidiEventType::ChannelAfterTouch:
            {
                event.DataA = ReadBytes<uint8_t>();

                std::cout << (int)event.DataA << "\n\n";
                break;
            }
            case MidiEventType::NoteOff:
            case MidiEventType::NoteOn:
            case MidiEventType::PolyAfter:
            case MidiEventType::ControlChanges:
            case MidiEventType::PitchBend:
            {
                event.DataA = ReadBytes<uint8_t>();
                event.DataB = ReadBytes<uint8_t>();

                std::cout << (int)event.DataA << " ";
                std::cout << (int)event.DataB << "\n\n";
                break;
            }
            default:
            {
                std::cout << "Error parsing MIDI events! ";
                std::cout << "Event type: " << (uint32_t)midiEventType << "\n";
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
        char byte;
        m_Stream.read(&byte, 1);  // Read 1 more byte
        value += (byte & 0b01111111);  // The left bit isn't data so discard it
        if (!(byte & 0b10000000))  // If the left bit is 0 (that means it's the end of value)
            return value;
        value <<= 7;  // Shift 7 because the left bit isn't data
    }

    return -1;
}
