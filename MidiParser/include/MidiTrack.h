#pragma once

#include "MidiEvent.h"

#include <vector>

class MidiTrack {
public:
    MidiTrack() = default;
    MidiTrack(size_t sizeBytes);

    ~MidiTrack();

    void ReserveBytes(size_t sizeBytes);
    void ReserveEvents(size_t eventCount);

    // T is the event type
    template<typename T, typename... Args>
    T* AddEvent(Args&&... args) {
        static_assert(std::is_base_of<Event, T>::value, "T must be an event!");

        if (m_PushIndex + sizeof(T) > m_Capacity)
            ReserveBytes(m_Capacity * 2);

        Event* event = new(m_Data + m_PushIndex) T(std::forward<Args>(args)...);
        m_Indicies.push_back(m_PushIndex);
        m_PushIndex += sizeof(T);
        return (T*)event;
    }

    inline uint32_t TotalTicks() const { return m_TotalTicks; }
    inline size_t GetEventCount() const { return m_Indicies.size(); }
    inline size_t GetSizeBytes() const { return m_PushIndex; }

    Event* operator[](size_t index) { return (Event*)(m_Data + m_Indicies[index]); }
private:
    friend class MidiParser;

    uint8_t* m_Data = nullptr;
    uint32_t m_PushIndex = 0;
    size_t m_Capacity = 0;  // Size in bytes
    std::vector<uint32_t> m_Indicies;

    uint32_t m_TotalTicks = 0;  // Amount of ticks the track lasts for
};
