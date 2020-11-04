#pragma once

#include <fstream>
#include <unordered_map>
#include <string>
#include <tuple>
#include <vector>

#include "MidiTrack.h"

class MidiParser {
private:
    std::fstream m_Stream;
    std::vector<MidiTrack> m_TrackList;
    std::unordered_map<uint32_t, uint32_t> m_TempoMap;

    uint16_t m_Format = 0, m_TrackCount = 0, m_Division = 0;
    uint32_t m_TotalTicks = 0;  // Duration of MIDI file in ticks

    MidiEventType m_RunningStatus = MidiEventType::None;  // Current running status
public:
    MidiParser() = default;
    MidiParser(const std::string& file);

    ~MidiParser();

    bool Open(const std::string& file);
    inline void Close() { m_Stream.close(); }

    inline uint16_t GetFormat() const { return m_Format; }
    inline uint16_t GetDivision() const { return m_Division; }
    inline uint16_t GetTrackCount() const { return m_TrackCount; }

    uint32_t GetDurationSeconds();
    std::pair<uint32_t, uint32_t> GetDuration();

    MidiTrack& operator[](size_t index) { return m_TrackList[index]; }
private:
    enum class MidiEventStatus : int8_t {
        Error = -1,
        Success,
        End
    };
private:
    void ResetValues();  // Resets values to default

    bool ReadFile();
    bool ReadTrack();
    MidiEventStatus ReadEvent(MidiTrack& track);  // Reads a single event

    MidiTrack& AddTrack();

    int32_t ReadVariableLengthValue();  // Returns -1 if invalid

    // Reads type T from file and converts to big endian
    template<typename T>
    T ReadBytes(T* destination = nullptr);
    inline void ReadBytes(const void* buffer, size_t size);

    inline uint64_t TicksToMicroseconds(uint32_t ticks, uint32_t tempo) { return ticks / m_Division * tempo; }
};
