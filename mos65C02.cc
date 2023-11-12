#include <ctype.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "m6821.h"
#include "mos65C02.h"

#define DELAY_FACTOR_SHORT() asm volatile("nop\nnop\nnop\nnop\n");

#define DELAY_FACTOR_LONG() asm volatile("nop\nnop\nnop\nnop\nnop\nnop\n");

// # of clock cycles to keep reset pin low
#define RESET_COUNT 4

//  64k RAM
uint8_t mem[MEMORY_SIZE];
uint32_t ticks;

// mask used for the mux address/data bus: GP0-7
constexpr uint32_t BUS_MASK = 0xFF;

// external declarations
void load_rom();
void video_putchar(uint8_t);

inline __attribute__((always_inline)) void ClockHigh() {
  gpio_set_mask(1ul << GP_CLOCK);
}
inline __attribute__((always_inline)) void ClockLow() {
  gpio_clr_mask(1ul << GP_CLOCK);
}

inline __attribute__((always_inline)) void setReset(bool reset) {
  gpio_put(GP_RESET, reset);
}

// initialise the 65C02
void init6502() {
  // CLOCK
  gpio_init(GP_CLOCK);
  gpio_set_dir(GP_CLOCK, GPIO_OUT);
  ClockHigh();

  // RESET
  gpio_init(GP_RESET);
  gpio_set_dir(GP_RESET, GPIO_OUT);
  setReset(RESET_LOW);

  // RW
  gpio_init(GP_RW);
  gpio_set_dir(GP_RW, GPIO_IN);

  // BUS ENABLE
  gpio_init_mask(en_MASK);
  gpio_set_dir_out_masked(en_MASK); // enable as output
  gpio_set_mask(D0_7_OE | A0_7_OE |
                A8_15_OE); // disable latch d0-7 and a0-a7 and a8-a15

  // speed up gpio input by disabling input hysteresis on databus gpios
  // for (int i = 0; i < 8; i++)
  // gpio_set_input_hysteresis_enabled(i, false);

  // ADDRESS
  // DATA
  gpio_init_mask(BUS_MASK);
  // initialize databus for input
  gpio_set_dir_in_masked(BUS_MASK);

  // populate memory
  for (int i = 0; i < sizeof(mem); i++)
    mem[i] = 0xEA;

  ticks = 0;

  load_rom();
  init6821();
}

void reset6502() {
  gpio_set_mask(D0_7_OE | A0_7_OE | A8_15_OE); // disable all latch
  gpio_set_dir_in_masked(BUS_MASK);            // data gpio input

  ClockHigh();

  setReset(RESET_LOW);

  for (int i = 0; i < RESET_COUNT; i++) {
    DELAY_FACTOR_LONG();
    ClockLow();
    DELAY_FACTOR_LONG();
    ClockHigh();
  }
  DELAY_FACTOR_LONG();
  setReset(RESET_HIGH);

  puts("RESET released");
}

inline __attribute__((always_inline)) void tick6502() {
  uint8_t data;

  // read A7-15
  // set output enable of required latch first, to use
  // clocks for usefull work
  gpio_clr_mask(A8_15_OE | CLOCK_MASK); // enable a0-a7 latch
  gpio_set_mask(D0_7_OE | A0_7_OE);     // disable latch d0-d7 and a8-a15

  // set databus gpios as inputs
  gpio_set_dir_in_masked(BUS_MASK);

  // increment tick counter here, avoiding useless nop's
  ticks++;

  // read A8 - A15
  uint32_t allbits = gpio_get_all();

  // prepare to read A0-7
  // disable latch d0-d7 and a0-a7 and set clock high
  gpio_set_mask(D0_7_OE | A8_15_OE | CLOCK_MASK);
  gpio_clr_mask(A0_7_OE); // enable a8-a15 latch

  // do a0-a7 reading now to reduce nop's
  uint16_t address = (allbits & BUS_MASK) << 8;
  bool rw = allbits & (1ul << GP_RW);

  gpio_clr_mask(D0_7_OE); // enable d0-d7 latch

  address |= (gpio_get_all() & BUS_MASK);

  // setup latches for databus operation
  // gpio_clr_mask(D0_7_OE);            // enable d0-d7 latch
  gpio_set_mask(A0_7_OE | A8_15_OE); // disable latch a0-a7 and a8-a15

  // do RW action
  if (rw) {
    // RW_READ:
    if ((address >= KBD) && (address <= DSPCR)) {

      switch (address) {
      case DSP:
        if (regDSPCR & 0x02) {
          data = regDSP;
          regDSPCR &= 0x7F;
        } else {
          data = regDSPDIR;
        }

        break;

      case DSPCR:
        data = regDSPCR;
        break;

      case KBD:
        if (regKBDCR & 0x02) {
          data = regKBD;
          regKBDCR &= 0x7F; // clear IRQA
        } else
          data = regKBDDIR;

        break;

      case KBDCR:
        data = regKBDCR;
        break;

      default:
        data = mem[address];
        break;
      }

    } else
      data = mem[address];

    // set databus direction to output
    gpio_set_dir_out_masked(BUS_MASK);
    gpio_put_masked(BUS_MASK, (uint32_t)data);

    // printf("R %04X %02X\n", address, data);

  } else {
    // RW_WRITE:
    uint32_t data = (uint8_t)gpio_get_all();

    if ((address >= KBD) && (address <= DSPCR)) {

      switch (address) {
      case DSP:
        if (regDSPCR & 0x02) {
          // if (data == '\r')
          // putchar('\n');
          // putchar(data);

          // display output
          video_putchar(data);
        } else
          regDSPDIR = data;
        break;

      case DSPCR:
        regDSPCR = data;
        break;

      case KBD:
        if (regKBDCR & 0x02) {
          regKBD = data;
        } else {
          regKBDDIR = data;
        }
        break;

      case KBDCR:
        regKBDCR = data & 0x7F;
        break;
      }
    } else
      mem[address] = data;

    // printf("W %04X %02X\n", address, data);
  }
}

void __not_in_flash_func(run6502)() {
  while (1) {
    tick6502();
  }
}
