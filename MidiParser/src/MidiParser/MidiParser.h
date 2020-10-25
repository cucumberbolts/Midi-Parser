#pragma once

#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "MidiTrack.h"

class TrackIterator {
private:
    using ValueType = MidiTrack;
    using PointerType = MidiTrack*;
    using ReferenceType = MidiTrack&;
public:
    TrackIterator(PointerType pointer)
        : m_Pointer(pointer) {}

    TrackIterator& operator++() {
        m_Pointer++;
        return *this;
    }

    TrackIterator& operator++(int) {
        TrackIterator iterator = *this;
        ++(*this);
        return iterator;
    }

    TrackIterator& operator--() {
        m_Pointer--;
        return *this;
    }

    TrackIterator& operator--(int) {
        TrackIterator iterator = *this;
        --(*this);
        return iterator;
    }

    ReferenceType operator[](int index) {
        return *(m_Pointer + index);
    }

    PointerType operator->() {
        return m_Pointer;
    }

    ReferenceType operator*() {
        return *m_Pointer;
    }

    bool operator==(const TrackIterator& other) const {
        return m_Pointer == other.m_Pointer;
    }

    bool operator!=(const TrackIterator& other) const {
        return !(*this == other);
    }
private:
    PointerType m_Pointer;
};

class MidiParser {
public:
    using Iterator = TrackIterator;
private:
    std::fstream m_Stream;
    uint16_t m_Format, m_TrackCount, m_Division;
    std::vector<MidiTrack> m_TrackList;

    MidiEventType m_RunningStatus = MidiEventType::None;  // Current running status
public:
    MidiParser() = default;
    MidiParser(const std::string& file);

    ~MidiParser();

    bool Open(const std::string& file);

    inline uint16_t GetFormat() { return m_Format; }
    inline uint16_t GetDivision() { return m_Division; }
    inline uint16_t GetTrackCount() { return m_TrackCount; }

    uint32_t GetDurationSeconds();
    std::pair<uint32_t, uint32_t> GetDuration();

    Iterator begin() { return Iterator(m_TrackList.data()); }
    Iterator end() { return Iterator(m_TrackList.data() + m_TrackList.size()); }

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
    int32_t ReadVariableLengthValue();  // Return -1 if invalid

    MidiTrack& AddTrack();

    // Reads type T from file and converts to big endian
    template<typename T>
    T ReadBytes(T* destination = nullptr);

    inline void ReadBytes(const void* buffer, size_t size);
};
