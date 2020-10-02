#include "MidiParser.h"

#include <iostream>

MidiParser::MidiParser(const std::string& file) {
    if (Open(file))
        std::cout << "Success!\n";
    else
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
}

bool MidiParser::Read() {
    // The following section parses the MIDI header
    bool validHeader = true;

    if (ReadBytes<unsigned int>() != 0x4d546864)
        validHeader = false;
    if (ReadBytes<unsigned int>() != 6)
        validHeader = false;

    m_Format = ReadBytes<unsigned short>();
    m_Division = ReadBytes<unsigned short>();
    m_TrackCount = ReadBytes<unsigned short>();

    if (m_Format < 0 || m_Format > 2)
        validHeader = false;
    if (m_Division == 0)
        validHeader = false;

    if (!validHeader) {
        std::cout << "Invalid MIDI header!\n";
        return false;
    }
}
