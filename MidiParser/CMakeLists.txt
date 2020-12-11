project("MidiParser")

set(SOURCES
    "src/MidiParser.cpp"
    "src/MidiTrack.cpp"
    "src/Endian.h"
    "include/Instruments.h"
    "include/MidiEvent.h"
    "include/MidiParser.h"
    "include/MidiTrack.h"
    "include/MidiUtilities/MidiUtilities.h"
)

add_library(${PROJECT_NAME} ${SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/" FILES ${SOURCES})

set_target_properties(MidiParser PROPERTIES CXX_STANDARD 17)

target_include_directories(
    MidiParser
    PUBLIC
    "include/"
)
