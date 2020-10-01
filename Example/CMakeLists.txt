project(Main VERSION 1.0)

add_executable(
    Main
    src/Main.cpp
)

target_link_libraries(Main PRIVATE MidiParser)
