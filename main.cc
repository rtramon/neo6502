#include "hardware/gpio.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/vreg.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include <cstring>
#include <ctype.h>
#include <stdio.h>

#include "m6821.h"
#include "mos65C02.h"

// PicoDVI definitions
#include "PicoDVI/software/include/common_dvi_pin_configs.h"
#include "dvi.h"
extern "C" {
#include "dvi_serialiser.h"
#include "tmds_encode.h"
}

#define FONT_CHAR_WIDTH 8
#define FONT_CHAR_HEIGHT 8
#define FONT_N_CHARS 256
#define FONT_FIRST_ASCII 0
#include "font_ibm_8x8.h"

// DVDD 1.2V (1.1V seems ok too)
#define FRAME_WIDTH 640
#if DVI_VERTICAL_REPEAT == 2
#define FRAME_HEIGHT (240)
#else
#define FRAME_HEIGHT (480)

#endif
#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

struct dvi_inst dvi0;
struct semaphore dvi_start_sem;

#define CHAR_COLS (FRAME_WIDTH / FONT_CHAR_WIDTH)
#define CHAR_ROWS (FRAME_HEIGHT / FONT_CHAR_HEIGHT)

char charbuf[CHAR_ROWS * CHAR_COLS];

// function declaration
void video_cls();

static inline void __not_in_flash_func(prepare_scanline)(const char *chars,
                                                         uint y) {
  static uint8_t scanbuf[FRAME_WIDTH / 8];
  // First blit font into 1bpp scanline buffer, then encode scanbuf into tmdsbuf
  for (uint i = 0; i < CHAR_COLS; ++i) {
    uint c = chars[i + y / FONT_CHAR_HEIGHT * CHAR_COLS];
    scanbuf[i] = font_8x8[(c - FONT_FIRST_ASCII) +
                          (y % FONT_CHAR_HEIGHT) * FONT_N_CHARS];
  }
  uint32_t *tmdsbuf;
  queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
  tmds_encode_1bpp((const uint32_t *)scanbuf, tmdsbuf, FRAME_WIDTH);
  queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
}

void __not_in_flash_func(core1_scanline_callback)() {
  static uint y = 1;
  prepare_scanline(charbuf, y);
  y = (y + 1) % FRAME_HEIGHT;
}

//
// function to perform emulated peripheral functions
//
void __not_in_flash_func(core1_main)() {

  dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
  sem_acquire_blocking(&dvi_start_sem);
  dvi_start(&dvi0);

  uint32_t start = time_us_32();
  extern uint32_t ticks;

  while (true) {

    // handle keyboard input
    // if (uart_is_readable(uart0)) {
    if (tud_cdc_available()) {
      // read the character
      int key = getchar();

      // check for emulator commands
      switch (key & 0xff) {
      case 0x12: // control-R
        puts("RESET");
        gpio_put(GP_RESET, 0);
        sleep_ms(10);
        gpio_put(GP_RESET, 1);

        // clear the display
        video_cls();
        break;
      case 0x13: // control-S
        break;
      case 0x14: // control-T
        printf("Freq:%fMhz\n", (float)ticks / (time_us_32() - start));
        start = time_us_32();
        ticks = 0;
        break;

      default:
        // pass key to 6502
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

int __not_in_flash_func(main)() {

  bi_decl(bi_program_description("NEO6502 System Emulator v0.01"));
  vreg_set_voltage(VREG_VSEL);
  sleep_ms(10);

  set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

  stdio_init_all();

  sleep_ms(2000); // wait for USB to finish setup

  puts("\nNEO6502 Memory Emulator v0.01");
  printf("CPU Freq: %d Mhz\n", DVI_TIMING.bit_clk_khz / 1000);

  printf("Configuring DVI\n");
  dvi0.timing = &DVI_TIMING;
  dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG; // pico_neo6502_cfg;
  dvi0.scanline_callback = core1_scanline_callback;
  dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

  printf("Prepare first scanline\n");
  for (int i = 0; i < CHAR_ROWS * CHAR_COLS; ++i)
    charbuf[i] = ' ';
  prepare_scanline(charbuf, 0);

  // start service core
  printf("Core 1 Startup \n");
  sem_init(&dvi_start_sem, 0, 1);
  hw_set_bits(&bus_ctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS);

  multicore_launch_core1(core1_main);

  sem_release(&dvi_start_sem);

  init6502();
  reset6502();
  while (1) {
    run6502();
  }
}

uint8_t video_x = 0;
uint8_t video_y = 0;

void __not_in_flash_func(video_putchar)(uint8_t data) {
  if (data > 31) {
    charbuf[video_y * CHAR_COLS + video_x] = data;
    video_x++;
    // charbuf[video_y * CHAR_COLS + video_x] = 95; // underscore to mark cursor
  } else {
    // special character handling
    if (data == 8) { // backspace
      charbuf[video_y * CHAR_COLS + video_x] = ' ';
      if (video_x > 0)
        video_x--;
    } else if ((data == '\r') || (data == '\n')) {
      // remove cursor
      // charbuf[video_y * CHAR_COLS + video_x] = ' ';

      video_y++;
      video_x = 0;
      // charbuf[video_y * CHAR_COLS + video_x] = 95; // underscore for cursor
    }
  }
  if (video_x >= CHAR_COLS) {
    video_x = 0;
    video_y++;
  }

  if (video_y >= CHAR_ROWS) {
    // copy all characters one line up
    memmove(charbuf, charbuf + CHAR_COLS, CHAR_COLS * (CHAR_ROWS - 1));
    // clear bottom line
    memset(charbuf + CHAR_COLS * (CHAR_ROWS - 1), 32, CHAR_COLS); // 32 == space

    video_y--;
  }
}

void video_cls() {
  video_x = 0;
  video_y = 0;
  for (int i = 0; i < CHAR_COLS * CHAR_ROWS; i++)
    charbuf[i] = ' ';

  // memset(charbuf, 32, CHAR_COLS * CHAR_ROWS); // fill with 32 (space)
}
