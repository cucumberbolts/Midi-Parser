#pragma once

#include <algorithm>

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
    ProgramChange = 0xc0,  // Type of instument
    ChannelAfterTouch = 0xd0,
    PitchBend = 0xe0
};

enum ControlChange : uint8_t {
    HBANK = 0x00,
    LBANK = 0x20,

    HDATA = 0x06,
    LDATA = 0x26,

    HNRPN = 0x63,
    LNRPN = 0x62,

    HRPN = 0x65,
    LRPN = 0x64,

    ModulationWheel = 0x01,
    Breath = 0x02,
    Foot = 0x04,
    PortamentoTime = 0x05,
    Volume = 0x07,
    Pan = 0x0a,
    Expression = 0x0b,
    Sustain = 0x40,
    Portamento = 0x41,
    Sostenuto = 0x42,
    SoftPedal = 0x43,
    HarmonicContent = 0x47,
    ReleaseTime = 0x48,
    AttackTime = 0x49,

    Brightness = 0x4a,
    PortamentoControl = 0x54,
    ReverbSend = 0x5b,
    ChorusSend = 0x5d,
    VariationSend = 0x5e,

    AllSoundsOff = 0x78,
    ResetAllControl = 0x79,
    LocalOff = 0x7a,
    AllNotesOff = 0x7b
};

class Event {
public:
    Event(uint32_t tick) : m_Tick(tick) {}
    virtual ~Event() {}

    virtual inline uint8_t GetType() const = 0;
    virtual inline EventCategory GetCategory() const = 0;

    virtual inline uint32_t GetTick() const { return m_Tick; }
protected:
    uint32_t m_Tick;
};

class MetaEvent : public Event {
public:
    friend class MidiParser;

    MetaEvent(uint32_t tick, MetaEventType metaType, uint8_t* data, size_t size)
        : Event(tick), m_MetaType(metaType), m_Data(data), m_Size(size) {}

    MetaEvent(const MetaEvent& other)
        : Event(other.m_Tick), m_MetaType(other.m_MetaType), m_Size(other.m_Size) {
    
        m_Data = new uint8_t[m_Size];
        std::copy(other.m_Data, other.m_Data + other.m_Size, m_Data);
    }

    MetaEvent(MetaEvent&& other) noexcept
        : Event(other.m_Tick), m_MetaType(other.m_MetaType), m_Data(other.m_Data), m_Size(other.m_Size) {
        
        other.m_Size = 0;
        other.m_Data = nullptr;
    }

    virtual ~MetaEvent() override {
        delete[] m_Data;
    }

    MetaEvent& operator=(const MetaEvent& other) {
        if (&other != this) {
            if (m_Size != other.m_Size) {
                m_Size = other.m_Size;

                delete[] m_Data;
                m_Data = new uint8_t[m_Size];
            }

            std::copy(other.m_Data, other.m_Data + other.m_Size, m_Data);
        }

        return *this;
    }

    MetaEvent& operator=(MetaEvent&& other) noexcept {
        if (&other != this) {
            delete[] m_Data;

            m_Size = other.m_Size;
            m_Data = other.m_Data;

            other.m_Size = 0;
            other.m_Data = nullptr;
        }

        return *this;
    }

    inline uint8_t GetType() const override { return m_MetaType; }
    inline EventCategory GetCategory() const override { return EventCategory::Meta; }

    inline size_t GetSize() const { return m_Size; }
    inline uint8_t* Data() const { return m_Data; }

    uint8_t operator[](size_t index) { return m_Data[index]; }
protected:
    MetaEventType m_MetaType;
    uint8_t* m_Data = nullptr;
    size_t m_Size = 0;
};

class TempoEvent : public MetaEvent {
public:
    friend class MidiParser;

    TempoEvent(uint32_t tick, uint32_t tempo, uint8_t* data = nullptr)
        : MetaEvent(tick, MetaEventType::Tempo, data, 3), m_Tempo(tempo) {

        if (data == nullptr)
            m_Size = 0;
    }

    TempoEvent(const TempoEvent& other) 
        : MetaEvent(other.m_Tick, MetaEventType::Tempo, nullptr, other.m_Size), m_Tempo(other.m_Tempo) {
        
        m_Data = new uint8_t[m_Size];
        std::copy(other.m_Data, other.m_Data + other.m_Size, m_Data);
    }

    TempoEvent(TempoEvent&& other) noexcept
        : MetaEvent(other.m_Tick, other.m_MetaType, other.m_Data, other.m_Size), m_Time(other.m_Time), m_Tempo(other.m_Tempo) {

        other.m_Size = 0;
        other.m_Data = nullptr;

        other.m_Time = 0;
        other.m_Tempo = 0;
    }

    ~TempoEvent() override = default;

    TempoEvent& operator=(const TempoEvent& other) {
        if (&other != this) {
            std::copy(other.m_Data, other.m_Data + other.m_Size, m_Data);
            m_Size = other.m_Size;
        }

        return *this;
    }

    TempoEvent& operator=(TempoEvent&& other) noexcept {
        if (&other != this) {
            delete[] m_Data;

            m_Size = other.m_Size;
            m_Data = other.m_Data;

            m_Time = other.m_Time;
            m_Tempo = other.m_Tempo;

            other.m_Size = 0;
            other.m_Data = nullptr;

            other.m_Time = 0;
            other.m_Tempo = 0;
        }

        return *this;
    }

    inline uint32_t GetTempo() const { return m_Tempo; }
    inline uint64_t GetTime() const { return m_Time; }
private:
    uint64_t m_Time = 0;
    uint32_t m_Tempo = 0;
};

class MidiEvent : public Event {
public:
    friend class MidiParser;

    MidiEvent(uint32_t tick, MidiEventType type, uint8_t channel, uint8_t dataA, uint8_t dataB)
        : Event(tick), m_MidiEventType(type), m_Channel(channel), m_DataA(dataA), m_DataB(dataB) {}

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

class NoteOnEvent : public MidiEvent {
public:
    friend class MidiParser;

    NoteOnEvent(uint32_t tick, uint8_t channel, uint8_t noteNum, uint8_t velocity)
        : MidiEvent(tick, MidiEventType::NoteOn, channel, noteNum, velocity) {}

    inline uint8_t GetNote() const { return m_DataA; }
    inline uint8_t GetNoteVelocity() const { return m_DataB; }

    inline float GetTimeStamp() const { return m_Start; }
    inline float GetDuration() const { return m_Duration; }
private:
    float m_Start = 0.f;  // Time between beginning of track and beginning of note (seconds)
    float m_Duration = 0.f;  // Duration of note in (seconds)
};
