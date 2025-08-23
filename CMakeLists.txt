cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

set(PROJECT_NAME XCube)

project(${PROJECT_NAME} CXX C ASM)

pico_sdk_init()

set(FAMILY rp2040)
set(BOARD pico_sdk)

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

add_compile_definitions(
    GC_DATA_PIN=$ENV{GC_DATA_PIN}
)

if (DEFINED $ENV{GC_3V3_PIN})
    add_compile_definitions(
        GC_3V3_PIN=$ENV{GC_3V3_PIN}
    )
endif ()

include_directories(
    include
    lib/joybus-pio/include
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lib/tusb_xinput xinput_host)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/filters.cpp
    src/xinput.cpp
    src/hid.cpp
    lib/joybus-pio/src/GamecubeConsole.cpp
    lib/joybus-pio/src/joybus.c
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/joybus-pio/src/joybus.pio)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_multicore
    hardware_resets
    hardware_irq
    hardware_pio
    tinyusb_host
    tinyusb_board
    xinput_host
)

pico_add_extra_outputs(${PROJECT_NAME})
