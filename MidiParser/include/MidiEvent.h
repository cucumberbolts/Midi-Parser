#pragma once

#include <stdint.h>

enum class MidiEventType : uint8_t {
    None = 0x00,
    NoteOff = 0x80,
    NoteOn = 0x90,
    PolyAfter = 0xa0,
    ControlChange = 0xb0,
    ProgramChange = 0xc0,  // Type of instument
    ChannelAfterTouch = 0xd0,
    PitchBend = 0xe0,

    Meta = 0xff,
    SysEx = 0xf0,
    EndSysEx = 0xf7
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

enum class MetaEventType : uint8_t {
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
    MidiEventType Type;

    Event(uint32_t tick, MidiEventType type) : Tick(tick), Type(type) {}
    virtual ~Event() {}
};

class TempoEvent : public Event {

};

class MetaEvent : public Event {
public:
    MetaEventType MetaType;
    uint8_t* Data = nullptr;
    uint32_t Size = 0;

    MetaEvent(uint32_t tick, MetaEventType type, uint8_t* data, uint32_t size)
        : Event(tick, MidiEventType::Meta), MetaType(type), Data(data), Size(size) {}

    ~MetaEvent() override {
        delete[] Data;
    }
};

class MidiEvent : public Event {
public:
    uint8_t DataA;
    uint8_t DataB;

    uint32_t DurationTicks = 0;  // Duration of this event in ticks (ticks until next event)
    float DurationSeconds = 0.f;

    MidiEvent(MidiEventType type)
        : Event(0, type), DataA(0), DataB(0) {}

    MidiEvent(uint32_t tick, MidiEventType type, uint8_t dataA, uint8_t dataB)
        : Event(0, type), DataA(dataA), DataB(dataB) {}
};
