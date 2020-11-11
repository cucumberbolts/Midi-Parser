#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <tuple>
#include <vector>

#include "MidiTrack.h"
#include "Instruments.h"

class MidiParser {
public:
    using ErrorCallbackFunc = std::function<void(const std::string&)>;
private:
    uint8_t* m_Buffer = nullptr;
    size_t m_Position = 0;

    std::vector<MidiTrack> m_TrackList;
    std::unordered_map<uint32_t, uint32_t> m_TempoMap;

    uint16_t m_Format = 0, m_TrackCount = 0, m_Division = 0;
    uint32_t m_TotalTicks = 0;  // Duration of MIDI file in ticks

    MidiEventType m_RunningStatus = MidiEventType::None;  // Current running status

    ErrorCallbackFunc m_ErrorCallback;
    bool m_ErrorStatus = true;  // True if no error
public:
    MidiParser() = default;
    MidiParser(ErrorCallbackFunc callback) : m_ErrorCallback(callback) {}
    MidiParser(const std::string& file);
    MidiParser(const std::string& file, ErrorCallbackFunc callback);

    ~MidiParser() = default;

    bool Open(const std::string& file);

    inline uint16_t GetFormat() const { return m_Format; }
    inline uint16_t GetDivision() const { return m_Division; }
    inline uint16_t GetTrackCount() const { return m_TrackCount; }

    uint32_t GetDurationSeconds();
    std::pair<uint32_t, uint32_t> GetDuration();

    MidiTrack& operator[](size_t index) { return m_TrackList[index]; }

    inline void SetErrorCallback(ErrorCallbackFunc callback) { m_ErrorCallback = callback; }
private:
    enum class MidiEventStatus : int8_t {
        Error,
        Success,
        End
    };
private:
    bool ReadFile();
    bool ReadTrack();
    MidiEventStatus ReadEvent(MidiTrack& track);  // Reads a single event

    MidiTrack& AddTrack() { return m_TrackList.emplace_back(); }

    int32_t ReadVariableLengthValue();  // Returns -1 if invalid

    // Reads type T from file and converts to little endian if necessary
    template<typename T>
    T ReadInteger(T* destination = nullptr);
    inline void ReadBytes(void* buffer, size_t size);
    inline uint8_t ReadByte();

    inline uint64_t TicksToMicroseconds(uint32_t ticks, uint32_t tempo) {
        return (uint32_t)((double)(ticks / m_Division) * tempo);
    }

    inline void CallError(const std::string& msg);
    inline void DefaultErrorCallback(const std::string& msg);
};
