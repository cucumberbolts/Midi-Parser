#include "MidiTrack.h"

void MidiTrack::AddEvent(MidiEvent event) {
    m_Events.push_back(event);
}
