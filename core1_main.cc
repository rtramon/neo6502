
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
    if (!(regKBDCR & 0x80) && (tud_cdc_available())) {
      regKBDCR |= 0x80; // set IRQA

      // read the character
      uint32_t key = tud_cdc_read_char();

      // check for emulator commands
      switch (key & 0xff) {
      case 0x12: // control-R
        puts("RESET");
        reset6502();
        break;
      case 0x13: // control-S
        break;
      case 0x14: // control-T
        printf("Freq:%fMhz\n", (float)ticks / (time_us_32() - start));
        break;

      default:
        // pass through 6502

        regKBD =
            toupper(key) | 0x80; // apple 1 expect upper bit set on incoming
        break;
      }
    }
  }
}