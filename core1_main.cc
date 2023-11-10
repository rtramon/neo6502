
#include "m6821.h"
#include "mos65C02.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <ctype.h>
#include <stdio.h>

extern uint32_t ticks;

//
// function to perform emulated peripheral functions
//
void core1_main() {

  uint32_t start = time_us_32();

  while (true) {
    // handle keyboard input
    if (tud_cdc_available()) {
      // read the character
      // uint32_t key = tud_cdc_read_char();
      int key = getchar();

      // check for emulator commands
      switch (key & 0xff) {
      case 0x12: // control-R
        puts("RESET");
        gpio_put(GP_RESET, 0);
        sleep_ms(10);
        gpio_put(GP_RESET, 1);
        break;
      case 0x13: // control-S
        break;
      case 0x14: // control-T
        printf("Freq:%fMhz\n", (float)ticks / (time_us_32() - start));
        start = time_us_32();
        ticks = 0;
        break;

      default:
        // pass through 6502
        if (!(regKBDCR & 0x80)) {
          regKBDCR |= 0x80; // set IRQA
          regKBD =
              toupper(key) | 0x80; // apple 1 expect upper bit set on incoming
        }
        break;
      }
    }
  }
}