#pragma once

#include <stdint.h>

enum class EventCategory : uint8_t {
    Midi,
    Meta = 0xff,
    SysEx = 0xf0,
    EndSysEx = 0xf7
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

enum class ControlChange : uint8_t {
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

class Event {
public:
    uint32_t Tick;
    EventCategory Category;

    Event(uint32_t tick, EventCategory category) : Tick(tick), Category(category) {}
    virtual ~Event() {}

    virtual uint8_t Type() const = 0;
};

class MetaEvent : public Event {
public:
    MetaEventType MetaType;
    uint8_t* Data = nullptr;
    uint32_t Size = 0;

    MetaEvent(uint32_t tick, MetaEventType metaType, uint8_t* data, uint32_t size)
        : Event(tick, EventCategory::Meta), MetaType(metaType), Data(data), Size(size) {}

    ~MetaEvent() override {
        delete[] Data;
    }

    uint8_t Type() const override { return MetaType; }
};

class TempoEvent : public Event {
public:
    uint64_t Time = 0;
    uint32_t Tempo;

    TempoEvent(uint32_t tick, uint32_t tempo)
        : Event(tick, EventCategory::Meta), Tempo(tempo) {}

    uint8_t Type() const override { return MetaEventType::Tempo; }
};

class MidiEvent : public Event {
public:
    MidiEventType MidiType;
    uint8_t DataA;
    uint8_t DataB;

    float Start = 0.f;  // Time between beginning of track and beginning of note (seconds)
    float Duration = 0.f;  // Duration of note in (seconds)

    MidiEvent(uint32_t tick, MidiEventType type, uint8_t dataA, uint8_t dataB)
        : Event(tick, EventCategory::Midi), MidiType(type), DataA(dataA), DataB(dataB) {}

    uint8_t Type() const override { return MidiType; }

    bool IsMatchingNoteOff(Event* event) {
        if (Type() == MidiEventType::NoteOn && DataB > 0 && (event->Type() == MidiEventType::NoteOn || event->Type() == MidiEventType::NoteOff))
            if (((MidiEvent*)event)->DataB == 0)
                return DataA == ((MidiEvent*)event)->DataA;
        return false;
    }
};
