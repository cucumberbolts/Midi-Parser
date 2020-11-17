#pragma once

#include "MidiEvent.h"

#include <vector>

class MidiTrack {
private:
    friend class MidiParser;
    std::vector<Event*> m_Events;
    size_t m_Size;  // Size in bytes
    uint32_t m_TotalTicks;  // Amount of ticks the track lasts for
public:
    MidiTrack() : m_Size(0), m_TotalTicks(0) {}
    ~MidiTrack();

    void AddEvent(Event* event);

    inline uint32_t TotalTicks() const { return m_TotalTicks; }
    inline size_t GetEventCount() const { return m_Events.size(); }
    inline size_t SizeBytes() const { return m_Size; }

    Event* operator[](size_t index) { return m_Events[index]; }
};
