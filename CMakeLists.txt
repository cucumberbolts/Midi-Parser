cmake_minimum_required(VERSION 3.8)

project(MidiParser)

add_subdirectory(MidiParser)
add_subdirectory(Example)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Example)
