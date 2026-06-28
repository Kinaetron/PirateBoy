cmake_minimum_required(VERSION 3.20)
project(PirateBoy LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL)
set(SDL3_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include)

add_library(PirateBoyCore STATIC
    src/cpu.c
    include/cpu.h
    include/memory.h
)
target_include_directories(PirateBoyCore PUBLIC include)

add_executable(PirateBoy
    src/main.c
)
target_include_directories(PirateBoy PRIVATE include)
target_link_libraries(PirateBoy
    PRIVATE
        PirateBoyCore
        SDL3::SDL3
)

add_library(unity STATIC external/Unity/src/unity.c)
target_include_directories(unity PUBLIC external/Unity/src)

add_executable(PirateBoyTests
    tests/cpu/opcode.c
)
target_link_libraries(PirateBoyTests
    PRIVATE
        PirateBoyCore
        unity
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    src/main.c
    src/cpu.c
    include/cpu.h
    include/memory.h
)

enable_testing()
add_test(NAME PirateBoyTests COMMAND PirateBoyTests)