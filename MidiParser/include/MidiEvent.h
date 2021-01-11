#pragma once

#include <algorithm>
#include <vector>

enum class EventCategory : uint8_t {
    Midi,
    Meta = 0xff,
    SysEx = 0xf0,
    EndSysEx = 0xf7
};

enum MetaEventType : uint8_t {
    SequenceNumber = 0x00,
    Text = 0x01,
    Copyright = 0x02,
    TrackName = 0x03,
    InstrumentName = 0x04,
    Lyric = 0x05,
    Marker = 0x06,
    CuePoint = 0x07,
    ProgramName = 0x08,
    DeviceName = 0x09,
    TrackComment = 0xf0,
    PortChange = 0x21,
    ChannelPrefix = 0x22,
    EndOfTrack = 0x2f,
    Tempo = 0x51,
    TimeSignature = 0x58,
    KeySignature = 0x59
};

enum MidiEventType : uint8_t {
    None = 0x00,

    NoteOff = 0x80,
    NoteOn = 0x90,
    PolyAfter = 0xa0,
    ControlChange = 0xb0,
    ProgramChange = 0xc0,  // Type of instrument
    ChannelAfterTouch = 0xd0,
    PitchBend = 0xe0
};

class Event {
public:
    Event(uint32_t tick, float time) : m_Tick(tick), m_Time(time) {}
    virtual ~Event() {}

    virtual inline uint8_t GetType() const = 0;
    virtual inline EventCategory GetCategory() const = 0;

    inline uint32_t GetTick() const { return m_Tick; }
    inline float GetTime() const { return m_Time; }
protected:
    uint32_t m_Tick;
    float m_Time;
};

class MetaEvent : public Event {
public:
    friend class MidiParser;

    MetaEvent(uint32_t tick, float time, MetaEventType metaType, std::vector<uint8_t>&& data)
        : Event(tick, time), m_MetaType(metaType), m_Data(std::move(data)) {}

    MetaEvent(const MetaEvent& other)
        : Event(other.m_Tick, other.m_Time), m_MetaType(other.m_MetaType), m_Data(other.m_Data) {}

    MetaEvent(MetaEvent&& other) noexcept
        : Event(other.m_Tick, other.m_Time), m_MetaType(other.m_MetaType), m_Data(std::move(other.m_Data)) {

        other.m_Tick = 0;
        other.m_Time = 0;
    }

    virtual ~MetaEvent() override = default;

    MetaEvent& operator=(const MetaEvent& other) {
        if (&other != this) {
            m_Tick = other.m_Tick;
            m_Time = other.m_Time;
            m_MetaType = other.m_MetaType;
            m_Data = other.m_Data;
        }

        return *this;
    }

    MetaEvent& operator=(MetaEvent&& other) noexcept {
        if (&other != this) {
            m_Tick = other.m_Tick;
            m_Time = other.m_Time;
            m_MetaType = other.m_MetaType;
            m_Data = std::move(other.m_Data);

            other.m_Tick = 0;
            other.m_Time = 0;
        }

        return *this;
    }

    inline uint8_t GetType() const override { return m_MetaType; }
    inline EventCategory GetCategory() const override { return EventCategory::Meta; }

    inline size_t GetSize() const { return m_Data.size(); }
    inline uint8_t* Data() const { return (uint8_t*)m_Data.data(); }

    uint8_t operator[](size_t index) { return m_Data[index]; }
protected:
    MetaEventType m_MetaType;
    std::vector<uint8_t> m_Data;
};

class MidiEvent : public Event {
public:
    friend class MidiParser;

    MidiEvent(uint32_t tick, float time, MidiEventType type, uint8_t channel, uint8_t dataA, uint8_t dataB)
        : Event(tick, time), m_MidiEventType(type), m_Channel(channel), m_DataA(dataA), m_DataB(dataB) {}

    inline uint8_t GetType() const override { return m_MidiEventType; }
    inline EventCategory GetCategory() const override { return EventCategory::Midi; }

    inline uint8_t GetChannel() const { return m_Channel; }
    inline uint8_t GetDataA() const { return m_DataA; }
    inline uint8_t GetDataB() const { return m_DataB; }
protected:
    MidiEventType m_MidiEventType;
    uint8_t m_Channel;
    uint8_t m_DataA;
    uint8_t m_DataB;
};
