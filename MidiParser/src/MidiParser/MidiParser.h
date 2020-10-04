#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "MidiTrack.h"

class MidiParser {
private:
    std::fstream m_Stream;
    uint16_t m_Format, m_TrackCount, m_Division;
    std::vector<MidiTrack> m_TrackList;
public:
    enum class Mode : uint8_t {
        Read,
        Write
    };
public:
    MidiParser() = default;
    MidiParser(const std::string& file, Mode mode);

    ~MidiParser();

    bool Open(const std::string& file, Mode mode);

    inline uint16_t GetFormat() { return m_Format; }
    inline uint16_t GetDivision() { return m_Division; }
    inline uint16_t GetTrackCount() { return m_TrackCount; }

    MidiTrack& operator[](size_t index) { return m_TrackList[index]; }
private:
    enum class MidiEventStatus : int8_t {
        Error = -1,
        Success,
        End
    };
private:
    bool ReadFile();
    bool ReadTrack();
    MidiEventStatus ReadEvent(MidiTrack& track);

    MidiTrack& AddTrack();

    int32_t ReadVariableLengthValue();  // Return -1 if invalid
private:
    // Reads type T from file and converts to big endian
    template<typename T>
    T ReadBytes(T* destination = nullptr) {
        static_assert(std::is_integral<T>(), "Type T is not an integer!");

        if constexpr (sizeof(T) == 1) {
            T number;
            m_Stream.read((char*)&number, 1);
            return number;
        } else {
            uint8_t* numBuff = (uint8_t*)&destination;  // Type punning :)
            uint8_t buffer[sizeof(T)];

            m_Stream.read((char*)buffer, sizeof(T));

            for (int i = 0; i < sizeof(T); i++)
                numBuff[i] = buffer[sizeof(T) - 1 - i];

            return *(T*)numBuff;
        }
    }

    inline void ReadBytes(const void* buffer, size_t size) {
        m_Stream.read((char*)buffer, size);
    }
};
