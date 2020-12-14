#pragma once

#include "MidiEvent.h"

#include <vector>

class MidiTrack {
public:
    MidiTrack() : m_SizeBytes(0) {}
    MidiTrack(size_t sizeBytes);

    inline void ReserveBytes(size_t sizeBytes);
    inline void ReserveEvents(size_t eventCount);

    MidiEvent& AddEvent(uint32_t tick, MidiEventType type, uint8_t channel, uint8_t dataA, uint8_t dataB);

    inline uint32_t TotalTicks() const { return m_TotalTicks; }
    inline size_t GetEventCount() const { return m_EventList.size(); }
    inline size_t GetSizeBytes() const { return m_SizeBytes; }

    MidiEvent& operator[](size_t index) { return m_EventList[index]; }

    std::vector<MidiEvent>::iterator begin() { return m_EventList.begin(); }
    std::vector<MidiEvent>::iterator end() { return m_EventList.end(); }

    std::vector<MidiEvent>::reverse_iterator rbegin() { return m_EventList.rbegin(); }
    std::vector<MidiEvent>::reverse_iterator rend() { return m_EventList.rend(); }

    std::vector<MidiEvent>::const_iterator cbegin() { return m_EventList.cbegin(); }
    std::vector<MidiEvent>::const_iterator cend() { return m_EventList.cend(); }
private:
    friend class MidiParser;

    std::vector<MidiEvent> m_EventList;
    size_t m_SizeBytes;  // Size in bytes
    uint32_t m_TotalTicks = 0;  // Amount of ticks the track lasts for
};
