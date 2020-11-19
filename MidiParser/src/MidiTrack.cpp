#include "MidiTrack.h"

void MidiTrack::AddEvent(uint32_t tick, MidiEventType type, uint8_t dataA, uint8_t dataB) {
    m_Events.emplace_back(tick, type, dataA, dataB);
}
