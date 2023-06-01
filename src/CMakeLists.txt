add_executable(Chip8 WIN32)

target_sources(Chip8 PRIVATE 
    Program.cpp
    Chip8.cpp
    Opcode.cpp)

target_include_directories(Chip8 PRIVATE include)

target_link_libraries(Chip8 PRIVATE sfml-system sfml-window sfml-graphics sfml-audio sfml-main imgui)

set_target_properties(Chip8 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)