project(MidiParser VERSION 1.0)

add_library(
    MidiParser
    "src/MidiParser/MidiParser.h"
    "src/MidiParser/MidiParser.cpp"
    "src/MidiParser/MidiTrack.h"
    "src/MidiParser/MidiTrack.cpp"
    "src/MidiParser/MidiEvent.h"
    "src/MidiParser/Instruments.h"

    "src/MidiUtilities/MidiUtilities.h"
)

set_target_properties(MidiParser PROPERTIES CXX_STANDARD 17)

add_subdirectory(vendor/fmt)
target_link_libraries(MidiParser PRIVATE fmt)

target_include_directories(
    MidiParser
    PRIVATE
    "vendor/fmt/include"
)

target_include_directories(
    MidiParser
    PUBLIC
    "src/"
)
