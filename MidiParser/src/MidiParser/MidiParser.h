#pragma once

#include <string>
#include <fstream>

enum class MidiEventStatus {
    Error = -1,
    Success,
    End
};

class MidiParser {
private:
    std::fstream m_Stream;

    uint16_t m_Format, m_TrackCount, m_Division;
public:
    MidiParser() = default;
    MidiParser(const std::string& file);

    ~MidiParser();

    bool Open(const std::string& file);
private:
    bool ReadFile();
    bool ReadTrack();
    MidiEventStatus ReadEvent();

    int32_t ReadVariableLengthValue();  // Return -1 if invalid
public:
    inline uint16_t GetFormat() { return m_Format; }
    inline uint16_t GetDivision() { return m_Division; }
    inline uint16_t GetTrackCount() { return m_TrackCount; }
private:
    // ReadBytes() reads type T from file and converts to big endian
    template<typename T>
    T ReadBytes(T* destination = nullptr) {
        static_assert(std::is_integral<T>(), "Type T is not an integer!");
    
        uint8_t* numBuff = (uint8_t*)&destination;  // Type punning :)
        uint8_t buffer[sizeof(T)];
    
        m_Stream.read((char*)buffer, sizeof(T));
    
        for (int i = 0; i < sizeof(T); i++)
            numBuff[i] = buffer[sizeof(T) - 1 - i];
    
        return *(T*)numBuff;
    }

    inline void ReadBytes(const void* buffer, size_t size) {
        m_Stream.read((char*)buffer, size);
    }
};
