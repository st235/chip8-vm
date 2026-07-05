cmake_minimum_required(VERSION 3.10)

project(CHIP-8_Virtual_Machine VERSION 0.9.1 LANGUAGES C)

option(ENABLE_DEBUG "Enables debug logging." OFF)

add_library(chip8vm-core
    src/opcodes.c
    src/rom.c
    src/vm.c
)

target_include_directories(chip8vm-core PUBLIC include)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
    GIT_TAG "main"
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SDL3)

add_executable(chip8vm
    main.c
)

add_dependencies(chip8vm
    SDL3::SDL3
    chip8vm-core
)

target_link_libraries(chip8vm PRIVATE
    SDL3::SDL3
    chip8vm-core
)

if(ENABLE_DEBUG)
    message(STATUS "Enabling debug logging...")
    target_compile_definitions(chip8vm-core PUBLIC _CHIP8_DEBUG)
endif()

target_compile_definitions(chip8vm PRIVATE
    _CHIP8_VERSION="${CMAKE_PROJECT_VERSION}"
)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX .js)
    set_target_properties(chip8vm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs"
        LINK_FLAGS "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS'] -s USE_SDL=3"
    )
endif()
