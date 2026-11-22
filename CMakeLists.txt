cmake_minimum_required(VERSION 3.16)
project(blockfall VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SDL2 REQUIRED)

add_executable(blockfall
    src/main.cpp
    src/app.cpp
    src/game.cpp
    src/piece.cpp
    src/painter.cpp
    src/font.cpp
    src/effects.cpp
)

if(MSVC)
    target_compile_options(blockfall PRIVATE /W4)
else()
    target_compile_options(blockfall PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(TARGET SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(blockfall PRIVATE SDL2::SDL2main)
    endif()
    target_link_libraries(blockfall PRIVATE SDL2::SDL2)
else()
    target_include_directories(blockfall PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(blockfall PRIVATE ${SDL2_LIBRARIES})
endif()
