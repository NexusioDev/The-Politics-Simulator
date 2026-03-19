cmake_minimum_required(VERSION 4.1)
project(The_Politics_Simulator)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DUNICODE -D_UNICODE)

include_directories("C:/Users/rando/Documents/Libarys/glm")
include_directories("C:/Users/rando/Documents/Libarys/glew-2.3.1/include")

link_directories("C:/Users/rando/Documents/Libarys/glew-2.3.1/lib/Release/x64")

add_executable(The_Politics_Simulator WIN32
        main.cpp
        stb_image.h
        stb_truetype.h
        GameState.h
        GameState.cpp
        text/TextRenderer.h
        text/TextRenderer.cpp
        ui/UIElements.h
        ui/UIColors.h)

# Und füge für den MSVC-Linker das Subsystem hinzu:
if(MSVC)
    set_target_properties(The_Politics_Simulator PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

target_link_libraries(The_Politics_Simulator glew32s.lib opengl32.lib)
