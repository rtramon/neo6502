#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "mos65C02.h"

// external functions
void core1_main();

int main() {

  bi_decl(bi_program_description("NEO6502 System Emulator v0.01"));
  set_sys_clock_khz(250000, true);
  // set_sys_clock_khz(300000,
  // true); // higher then 255Mhz only works using copy_to_ram

  stdio_init_all();
  sleep_ms(2000); // wait for USB to finish setup

  // start service core
  multicore_launch_core1(core1_main);

  puts("\nNEO6502 Memory Emulator v0.01");

  init6502();
  reset6502();
  while (1) {
    run6502();
  }
}
