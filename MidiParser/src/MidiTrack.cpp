#include "MidiTrack.h"

#define MIDI_EVENT_SIZE 3  // The size of one MIDI event (in the file)

MidiTrack::MidiTrack(size_t sizeBytes) : m_SizeBytes(sizeBytes) {
    ReserveBytes(sizeBytes);
}

inline void MidiTrack::ReserveBytes(size_t sizeBytes) {
    m_EventList.reserve(sizeBytes / MIDI_EVENT_SIZE);
}

inline void MidiTrack::ReserveEvents(size_t eventCount) {
    m_EventList.reserve(eventCount);
}

MidiEvent& MidiTrack::AddEvent(uint32_t tick, MidiEventType type, uint8_t channel, uint8_t dataA, uint8_t dataB) {
    return m_EventList.emplace_back(tick, type, channel, dataA, dataB);
}
