project("Example")

add_executable(
    ${PROJECT_NAME}
    "src/Main.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

target_link_libraries(${PROJECT_NAME} PRIVATE MidiParser)
