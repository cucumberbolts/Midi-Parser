#include "MidiTrack.h"

#define MIDI_EVENT_SIZE 3  // The approximate size of one MIDI event (in the file)

MidiTrack::MidiTrack(size_t sizeBytes) : m_Capacity(sizeBytes) {
    ReserveBytes(sizeBytes);
}

MidiTrack::~MidiTrack() {
    for (int i = 0; i < m_Indicies.size(); i++)
        ((Event*)&m_Data[m_Indicies[i]])->~Event();
    ::operator delete(m_Data, m_Capacity);
}

void MidiTrack::ReserveBytes(size_t sizeBytes) {
    if (m_Data != nullptr) {  // Checks if m_Data has already been initialized
        uint8_t* newData = new uint8_t[sizeBytes];

        std::copy(m_Data, m_Data + m_Capacity, newData);
        delete[] m_Data;

        m_Capacity = sizeBytes;
        m_Data = newData;
    } else {
        m_Capacity = sizeBytes;
        m_Data = new uint8_t[sizeBytes];
    }
    m_Indicies.reserve(sizeBytes / MIDI_EVENT_SIZE);
}

void MidiTrack::ReserveEvents(size_t eventCount) {
    ReserveBytes(eventCount * MIDI_EVENT_SIZE);
    m_Indicies.reserve(eventCount);
}
