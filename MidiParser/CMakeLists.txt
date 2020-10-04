project(MidiParser VERSION 1.0)

add_library(MidiParser
    "src/MidiParser/MidiParser.h"
    "src/MidiParser/MidiParser.cpp"
    "src/MidiParser/MidiTrack.h"
    "src/MidiParser/MidiTrack.cpp"
    "src/MidiParser/MidiEvent.h"
)

target_include_directories(
    MidiParser
    PUBLIC
    "src/"
)

