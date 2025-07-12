# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(cracky C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(PICO_TINYUSB_HOST 1)

# Add executable. Default name is the project name, version 0.1

add_executable(cracky 
    cracky.cpp Ntsc.cpp Sound.cpp ScanKeys.cpp
    Vram.cpp Sprite.cpp Chars.cpp Print.cpp Math.cpp
    Destruction.cpp Main.cpp Man.cpp Monster.cpp
    Movable.cpp Stage.cpp Stages.cpp Status.cpp
)

pico_set_program_name(cracky "cracky")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(cracky 0)
pico_enable_stdio_usb(cracky 0)

# Add the standard library to the build
target_link_libraries(cracky
    pico_stdlib
    hardware_pwm
    hardware_dma
    tinyusb_host
    tinyusb_board
)

# Add the standard include files to the build
target_include_directories(cracky PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PICO_SDK_PATH}/lib/tinyusb/src
    ${PICO_SDK_PATH}/lib/tinyusb/src/class/hid
    ${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/rp2040
)

pico_add_extra_outputs(cracky)
