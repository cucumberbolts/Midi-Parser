#pragma once

#include <string>
#include <fstream>

class MidiParser {
private:
    std::fstream m_Stream;

    unsigned short m_Format, m_Division, m_TrackCount;
public:
    MidiParser() = default;
    MidiParser(const std::string& file);

    ~MidiParser();

    bool Open(const std::string& file);

    bool Read();

    inline unsigned short GetFormat() { return m_TrackCount; }
    inline unsigned short GetDivision() { return m_Division; }
    inline unsigned short GetTrackCount() { return m_TrackCount; }
private:
    // ReadBytes() reads type T from file and converts to big endian
    template<typename T>
    T ReadBytes(T* number = nullptr) {
        static_assert(std::is_integral<T>(), "Type T is not an integer!");

        unsigned char* numBuff = (unsigned char*)&number;
        unsigned char buffer[sizeof(T)];

        m_Stream.read((char*)buffer, sizeof(T));

        for (int i = 0; i < sizeof(T); i++)
            numBuff[i] = buffer[sizeof(T) - i - 1];

        return *(T*)numBuff;
    }
};
