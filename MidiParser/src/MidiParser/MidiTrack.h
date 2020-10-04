#pragma once

#include "MidiEvent.h"

#include <vector>

class MidiTrack {
private:
    friend class MidiParser;
    std::vector<MidiEvent> m_Events;
    size_t m_Size;
public:
    void AddEvent(MidiEvent event);

    inline size_t Size() { return m_Events.size(); }  // For debugging
    inline size_t SizeInBytes() { return m_Size; }

    MidiEvent& operator[](size_t index) { return m_Events[index]; }
};
