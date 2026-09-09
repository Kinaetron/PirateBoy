cmake_minimum_required(VERSION 3.20)
project(PirateBoy LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL)
set(SDL3_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL/include)

add_library(cjson STATIC
    external/cJSON/cJSON.c
)

target_include_directories(cjson PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cJSON
)

add_library(PirateBoyCore STATIC
    src/cpu.c
    src/timer.c
    src/interrupt.c
    include/cpu.h
    include/timer.h
    include/interrupt.h
    include/memory.h
)

target_include_directories(PirateBoyCore PUBLIC include)

add_library(PirateBoyMemory STATIC src/memory.c)
target_include_directories(PirateBoyMemory PUBLIC include)

add_library(PirateBoyMemoryConformance STATIC src/memory_conformance.c)
target_include_directories(PirateBoyMemoryConformance PUBLIC include)

add_executable(PirateBoy
    src/main.c
)
target_include_directories(PirateBoy PRIVATE include)
target_link_libraries(PirateBoy
    PRIVATE
        PirateBoyCore
        PirateBoyMemory
        SDL3::SDL3
)

add_library(unity STATIC external/Unity/src/unity.c)
target_include_directories(unity PUBLIC external/Unity/src)

add_executable(PirateBoyTests
    tests/cpu/opcode_tests.c
)

target_compile_definitions(PirateBoyTests PRIVATE
    SM83_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/files"
)

target_link_libraries(PirateBoyTests
    PRIVATE
        PirateBoyCore
        PirateBoyMemoryConformance
        unity
        cjson
)

add_custom_command(TARGET PirateBoy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying SDL3 from: $<TARGET_FILE:SDL3::SDL3>"
    COMMAND ${CMAKE_COMMAND} -E echo "Copying SDL3 to: $<TARGET_FILE_DIR:PirateBoy>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:PirateBoy>
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    src/main.c
    src/cpu.c
    src/timer.c
    src/memory.c
    src/memory_conformance.c
    src/interrupt.c
    include/cpu.h
    include/timer.h
    include/memory.h
    include/interrupt.h
)

enable_testing()
add_test(NAME PirateBoyTests COMMAND PirateBoyTests)