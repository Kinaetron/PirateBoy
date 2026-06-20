cmake_minimum_required(VERSION 3.20)
project(PirateBoy LANGUAGES C)

set(CMAKE_C_STANDARD 99)

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL)

add_executable(PirateBoy 
    src/main.c
)

target_link_libraries(PirateBoy
    PRIVATE 
        SDL3::SDL3
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    src/main.c
)


