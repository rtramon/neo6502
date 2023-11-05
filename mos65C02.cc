
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include <stdio.h>

#include "m6821.h"
#include "mos65C02.h"

// #define DELAY_FACTOR_SHORT() asm volatile("nop\nnop\nnop\nnop\n");
#define DELAY_FACTOR_SHORT() asm volatile("nop\nnop\nnop\nnop\n");
// #define DELAY_FACTOR_SHORT() sleep_ms(100);

// #define DELAY_FACTOR_LONG()  asm
// volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
#define DELAY_FACTOR_LONG() asm volatile("nop\nnop\nnop\nnop\nnop\nnop\n");

// # of clock cycles to keep reset pin low
#define RESET_COUNT 4

//  64k RAM
uint8_t mem[MEMORY_SIZE];

// mask used for the mux address/data bus: GP0-7
constexpr uint32_t BUS_MASK = 0xFF;

void load_rom();

inline __attribute__((always_inline)) void ClockHigh() {
  gpio_set_mask(1ul << GP_CLOCK);
}
inline __attribute__((always_inline)) void ClockLow() {
  gpio_clr_mask(1ul << GP_CLOCK);
}

inline __attribute__((always_inline)) void setReset(bool reset) {
  gpio_put(GP_RESET, reset);
}

inline __attribute__((always_inline)) uint8_t getData() {
  // output enable  for data0-data7 first
  gpio_clr_mask(D0_7_OE);            // enable d0-d7 latch
  gpio_set_mask(A0_7_OE | A8_15_OE); // disable latch a0-a7 and a8-a15
  // asm volatile("nop\nnop\nnop\n");
  asm volatile("dmb\n");
  uint8_t data = gpio_get_all() & BUS_MASK;

  return data;
}

inline __attribute__((always_inline)) void putData(uint8_t data) {
  gpio_clr_mask(D0_7_OE);            // enable d0-d7 latch
  gpio_set_mask(A0_7_OE | A8_15_OE); // disable latch a0-a7 and a8-a15

  // set databus direction to output
  gpio_set_dir_masked(BUS_MASK, BUS_MASK);
  // asm volatile("nop\n");
  gpio_put_masked(BUS_MASK, (uint32_t)data);
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
  for (int i = 0; i < 8; i++)
    gpio_set_input_hysteresis_enabled(i, false);

  // ADDRESS
  // DATA
  gpio_init_mask(BUS_MASK);
  // initialize databus for input
  gpio_set_dir_masked(BUS_MASK, 0);

  // populate memory
  for (int i = 0; i < sizeof(mem); i++)
    mem[i] = 0xEA;

  load_rom();
}

void reset6502() {
  gpio_set_mask(D0_7_OE | A0_7_OE | A8_15_OE); // disable all latch
  gpio_set_dir_masked(BUS_MASK, 0);            // data gpio input

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

  ClockLow();

  // read A0-7
  // set output enable of required latch first, to use
  // clocks for usefull work
  gpio_clr_mask(A0_7_OE);            // enable a0-a7 latch
  gpio_set_mask(D0_7_OE | A8_15_OE); // disable latch d0-d7 and a8-a15
  // set databus gpios as inputs
  gpio_set_dir_masked(BUS_MASK, 0);
  ClockHigh();

  asm volatile("nop\nnop\n");

  // read A0 - A7
  uint32_t allbits = gpio_get_all();

  // prepare to read A8-15
  gpio_clr_mask(A8_15_OE);          // enable a8-a15 latch
  gpio_set_mask(D0_7_OE | A0_7_OE); // disable latch d0-d7 and a0-a7

  // do a0-a7 reading now to reduce nop's
  uint16_t address = allbits & BUS_MASK;
  bool rw = allbits & (1ul << GP_RW);

  asm volatile("nop\n");

  address |= (gpio_get_all() & BUS_MASK) << 8;

  // do RW action
  if (rw) {
    // RW_READ:
    switch (address) {
      // case 0x0600:
      //   printf("Start, ");
      //   data = mem[address];

      //   break;

      // case 0x061F:
      //   puts("OK");
      //   data = mem[address];

      //   break;

      // case 0x0624:
      //   puts("FAIL");
      //   data = mem[address];

      //   break;

    case DSP:
      // if (regDSPCR & 0x02) {
      data = regDSP;
      // regDSPCR &= 0x7F;
      // } else {
      // data = regDSPDIR;
      // }
      break;

    case DSPCR:
      data = regDSPCR;
      break;

    default:
      data = mem[address];
      break;
    }

    putData(data);

    // printf("R %04X %02X\n", address, data);
  }

  else {
    // RW_WRITE:
    data = getData();

    switch (address) {
    case DSP:
      // if (regDSPCR & 0x02)
      putchar(data);
      // else
      // regDSPDIR = data;
      break;

    case DSPCR:
      regDSPCR = data;
      break;

    default:
      mem[address] = data;
      break;
    }
    // printf("W %04X %02X\n", address, data);
  }
}

void run6502() {
  constexpr uint32_t TIME_PERIOD = 5000 * 1000;
  uint32_t tick = 0;
  uint32_t start = time_us_32();

  while (tick < 10 * 1000 * 1000) {
    tick6502();
    tick++;
  }

  printf("tick: %d  freq:%fMhz\n", tick, (float)tick / (time_us_32() - start));
}
