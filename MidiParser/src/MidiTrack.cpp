#include "MidiTrack.h"

MidiEvent& MidiTrack::AddEvent(uint32_t tick, MidiEventType type, uint8_t dataA, uint8_t dataB) {
    return m_EventList.emplace_back(tick, type, dataA, dataB);
}
