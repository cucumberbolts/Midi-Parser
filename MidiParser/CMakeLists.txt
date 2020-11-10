project("MidiParser")

file(
    GLOB_RECURSE
    SOURCES
    "src/*.cpp"
    "include/*.h"
    "include/MidiUtilities/*.h"
)

add_library(${PROJECT_NAME} ${SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/" FILES ${SOURCES})

set_target_properties(MidiParser PROPERTIES CXX_STANDARD 17)

add_subdirectory(vendor/fmt)
target_link_libraries(MidiParser PRIVATE fmt)

target_include_directories(
    MidiParser
    PUBLIC
    "include/"
)
