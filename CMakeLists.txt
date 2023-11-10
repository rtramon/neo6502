cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)
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
pico_enable_stdio_usb(test 1)
# pico_enable_stdio_uart(test 1)
pico_add_extra_outputs(test)

target_compile_definitions(test PRIVATE
	DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG}
	DVI_VERTICAL_REPEAT=1
	DVI_N_TMDS_BUFFERS=3
	DVI_MONOCHROME_TMDS=1
)

include_directories(
    $(CMAKE_CURRENT_SOURCE_DIR)/PicoDVI/software/include   
) 

target_link_libraries(test 
    pico_stdlib 
    pico_multicore
    libdvi
)

add_compile_options(-Wall
 -O2 -mcpu=cortex-m0plus
 )

pico_set_binary_type(test copy_to_ram)

add_subdirectory(PicoDVI/software/libdvi)

