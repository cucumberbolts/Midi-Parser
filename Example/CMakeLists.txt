add_executable(
    Main
    src/Main.cpp
)

target_link_libraries(Main PRIVATE MidiParser)
