#include "MidiTrack.h"

MidiTrack::~MidiTrack() {
    for (auto event : m_Events)
        delete event;
}

void MidiTrack::AddEvent(Event* event) {
    m_Events.push_back(event);
}
