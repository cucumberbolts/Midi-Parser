project(MidiParser VERSION 1.0)

add_library(MidiParser
    "src/MidiParser/MidiParser.h"
    "src/MidiParser/MidiParser.cpp"
)

target_include_directories(
    MidiParser
    PUBLIC
    "src/"
)

