cmake_minimum_required(VERSION 3.20)
project(PirateBoy LANGUAGES C)

set(CMAKE_C_STANDARD 99)

add_executable(PirateBoy 
    src/main.c
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    src/main.c
)


