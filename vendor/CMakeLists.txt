find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.5.1
    GIT_SHALLOW TRUE 
)
FetchContent_MakeAvailable(SFML)

add_library(imgui STATIC)
target_sources(imgui PRIVATE
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui-SFML.cpp)
target_include_directories(imgui PUBLIC imgui/include)
target_link_libraries(imgui PRIVATE sfml-system sfml-window sfml-graphics opengl32)