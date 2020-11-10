#pragma once

#include "MidiEvent.h"

#include <vector>

class MidiTrack {
private:
    friend class MidiParser;
    std::vector<MidiEvent> m_Events;
    size_t m_Size;  // Size in bytes
    uint32_t m_TotalTicks;  // Amount of ticks the track lasts for
public:
    MidiTrack() : m_Size(0), m_TotalTicks(0) {}

    void AddEvent(MidiEvent event);

    inline uint32_t TotalTicks() const { return m_TotalTicks; }
    inline size_t EventCount() const { return m_Events.size(); }
    inline size_t SizeBytes() const { return m_Size; }

    MidiEvent& operator[](size_t index) { return m_Events[index]; }
};
