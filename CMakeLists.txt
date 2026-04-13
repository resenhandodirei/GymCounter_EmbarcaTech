cmake_minimum_required(VERSION 3.13)

# Define o board ANTES do pico_sdk_init — obrigatorio para BitDogLab (RP2040)
set(PICO_BOARD pico CACHE STRING "Board type")

# Importa o Pico SDK
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(gym_counter C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(gym_counter
    src/main.c
    src/ssd1306.c
)

target_include_directories(gym_counter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gym_counter
    pico_stdlib
    hardware_i2c
    hardware_gpio
)

# Gera .uf2 para drag-and-drop na BitDogLab
pico_add_extra_outputs(gym_counter)

# Saida serial USB (opcional, para debug)
pico_enable_stdio_usb(gym_counter 1)
pico_enable_stdio_uart(gym_counter 0)