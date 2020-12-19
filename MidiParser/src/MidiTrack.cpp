#include "MidiTrack.h"

#define MIDI_EVENT_SIZE 3  // The approximate size of one MIDI event (in the file)

MidiTrack::MidiTrack(size_t sizeBytes) : m_Capacity(sizeBytes) {
    ReserveBytes(sizeBytes);
}

MidiTrack::MidiTrack(const MidiTrack& other)
    : m_PushIndex(other.m_PushIndex), m_Capacity(other.m_Capacity), m_Indicies(std::move(other.m_Indicies)), m_TotalTicks(other.m_TotalTicks) {

    m_Data = new uint8_t[m_Capacity];
    std::copy(other.m_Data, other.m_Data + other.m_Capacity, m_Data);
}

MidiTrack::MidiTrack(MidiTrack&& other) noexcept
    : m_Data(other.m_Data), m_PushIndex(other.m_PushIndex), m_Capacity(other.m_Capacity), m_Indicies(std::move(other.m_Indicies)), m_TotalTicks(other.m_TotalTicks) {

    other.m_Data = nullptr;
    other.m_PushIndex = 0;
    other.m_Capacity = 0;
    other.m_TotalTicks = 0;
}

MidiTrack::~MidiTrack() {
    for (int i = 0; i < m_Indicies.size(); i++)
        ((Event*)&m_Data[m_Indicies[i]])->~Event();  // Calls the destructor for each event
    delete[] m_Data;
}

MidiTrack& MidiTrack::operator=(const MidiTrack& other) {
    if (&other != this) {
        if (m_Capacity != other.m_Capacity) {
            m_Capacity = other.m_Capacity;

            delete[] m_Data;
            m_Data = new uint8_t[m_Capacity];
        }

        std::copy(other.m_Data, other.m_Data + other.m_Capacity, m_Data);

        m_PushIndex = other.m_PushIndex;
        m_Indicies = other.m_Indicies;
        m_TotalTicks = other.m_TotalTicks;
    }

    return *this;
}

MidiTrack& MidiTrack::operator=(MidiTrack&& other) noexcept {
    if (&other != this) {
        delete[] m_Data;

        m_Data = other.m_Data;
        m_PushIndex = other.m_PushIndex;
        m_Capacity = other.m_Capacity;
        m_Indicies = std::move(other.m_Indicies);
        m_TotalTicks = other.m_TotalTicks;

        other.m_Data = nullptr;
        other.m_PushIndex = 0;
        other.m_Capacity = 0;
        other.m_TotalTicks = 0;
    }

    return *this;
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
