#pragma once

#include "MidiEvent.h"

#include <vector>

class MidiTrack {
private:
    friend class MidiParser;
    std::vector<MidiEvent> m_EventList;
    size_t m_Size;  // Size in bytes
    uint32_t m_TotalTicks;  // Amount of ticks the track lasts for
public:
    MidiTrack() : m_Size(0), m_TotalTicks(0) {}

    MidiEvent& AddEvent(uint32_t tick, MidiEventType type, uint8_t dataA, uint8_t dataB);

    inline uint32_t TotalTicks() const { return m_TotalTicks; }
    inline size_t GetEventCount() const { return m_EventList.size(); }
    inline size_t SizeBytes() const { return m_Size; }

    MidiEvent operator[](size_t index) { return m_EventList[index]; }
};
