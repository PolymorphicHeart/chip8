cmake_minimum_required (VERSION 3.12)
set (CMAKE_C_STANDARD 11)

set (SOURCES
    "src/entry.c"
    "src/chip8.c"
    "src/beep.c")

set (INCLUDES
    "vendor/raylib/src"
    "./src")

project                    (chip8)
add_executable             (chip8 ${SOURCES})
target_include_directories (chip8 PUBLIC ${INCLUDES})
target_link_libraries      (chip8 raylib)

add_subdirectory(vendor/raylib)