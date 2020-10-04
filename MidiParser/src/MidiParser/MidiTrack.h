#pragma once

#include "MidiEvent.h"

#include <vector>

class EventIterator {
private:
    using ValueType = MidiEvent;
    using PointerType = MidiEvent*;
    using ReferenceType = MidiEvent&;
public:
    EventIterator(PointerType pointer)
        : m_Pointer(pointer) {}

    EventIterator& operator++() {
        m_Pointer++;
        return *this;
    }

    EventIterator& operator++(int) {
        EventIterator iterator = *this;
        ++(*this);
        return iterator;
    }

    EventIterator& operator--() {
        m_Pointer--;
        return *this;
    }

    EventIterator& operator--(int) {
        EventIterator iterator = *this;
        --(*this);
        return iterator;
    }

    ReferenceType operator[](int index) {
        return *(m_Pointer + index);
    }

    PointerType operator->() {
        return m_Pointer;
    }

    ReferenceType operator*() {
        return *m_Pointer;
    }

    bool operator==(const EventIterator& other) const {
        return m_Pointer == other.m_Pointer;
    }

    bool operator!=(const EventIterator& other) const {
        return !(*this == other);
    }
private:
    PointerType m_Pointer;
};

class MidiTrack {
public:
    using Iterator = EventIterator;
private:
    friend class MidiParser;
    std::vector<MidiEvent> m_Events;
    size_t m_Size;
public:
    void AddEvent(MidiEvent event);

    inline size_t Size() { return m_Events.size(); }  // For debugging
    inline size_t SizeInBytes() { return m_Size; }

    Iterator begin() { return Iterator(m_Events.data()); }
    Iterator end() { return Iterator(m_Events.data() + m_Events.size()); }

    MidiEvent& operator[](size_t index) { return m_Events[index]; }
};
