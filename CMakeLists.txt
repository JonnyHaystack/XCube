cmake_minimum_required(VERSION 3.12)

set(PICO_SDK_PATH "~/source/pico-sdk-xinput/")

include(pico_sdk_import.cmake)

project(XCube CXX C ASM)

pico_sdk_init()

# Adapted from https://cliutils.gitlab.io/modern-cmake/chapters/projects/submodule.html
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
else()
    message("Git not found or not a git repo")
endif()

include_directories(
    include
    lib/joybus-pio/include
    lib/tusb_xinput
)

add_executable(XCube
    src/main.cpp
    src/filters.cpp
    src/xinput.cpp
    src/hid.cpp
    lib/joybus-pio/src/GamecubeConsole.cpp
    lib/joybus-pio/src/joybus.c
    lib/tusb_xinput/xinput_host.c
)

pico_generate_pio_header(XCube ${CMAKE_CURRENT_LIST_DIR}/lib/joybus-pio/src/joybus.pio)

target_link_libraries(XCube
    pico_stdlib
    hardware_resets
    hardware_irq
    hardware_pio
    tinyusb_host
    tinyusb_board
    pico_multicore
)

pico_add_extra_outputs(XCube)
