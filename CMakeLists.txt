cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(test
  main.cc
  mos65C02.cc
  m6821.cc
  rom.cc
  ewoz.cc
  core1_main.cc
)

target_include_directories( test PRIVATE
    $(CMAKE_CURRENT_SOURCE_DIR)/PicoDVI/software/include   
) 

pico_enable_stdio_usb(test 1)
#pico_enable_stdio_uart(test 1)
#target_compile_definitions(test PRIVATE
#        PICO_DEFAULT_UART=0
#        PICO_DEFAULT_UART_TX_PIN=28
#        PICO_DEFAULT_UART_RX_PIN=29
#        PICO_DEFAULT_UART_BAUD_RATE=115200
#        USE_UART
#) 

pico_add_extra_outputs(test)

# We have a lot in SRAM4 (particularly TMDS LUT) but don't need much stack on
# core 1. Probably even 256 bytes would be fine.
target_compile_definitions(test PRIVATE
	DVI_DEFAULT_SERIAL_CONFIG=pico_neo6502_cfg
    DVI_MONOCHROME_TMDS=1
	DVI_VERTICAL_REPEAT=2
    #  PICO_CORE1_STACK_SIZE=0x200
	)


target_link_libraries(test 
    pico_stdlib 
    pico_multicore
    pico_util
    libdvi
)

add_compile_options(-Wall
 -O2 -mcpu=cortex-m0plus
 )

#pico_set_binary_type(test copy_to_ram)

add_subdirectory(PicoDVI/software/libdvi)

