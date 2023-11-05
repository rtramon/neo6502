//
// Author: Rien Matthijsse
//

#ifndef _MOS65C02_h
#define _MOS65C02_h

// NEO6502 board v1.0
#define GP_RESET 26 // RESB(40) <-- UEXT pin 3
#define GP_CLOCK 21 // PHI2
#define GP_RW 11    // RW#

#define GP_BUZZ 20

// mux bus enable pins
//                                2         1         0
//                              21098765432109876543210
// 8 - 10
constexpr uint32_t en_MASK = 0b00000000000011100000000;
constexpr uint32_t en_NONE = 0b00000000000011100000000;
constexpr uint32_t en_A0_7 = 0b00000000000011000000000;
constexpr uint32_t en_A8_15 = 0b00000000000010100000000;
constexpr uint32_t en_D0_7 = 0b00000000000001100000000;

constexpr uint32_t A0_7_OE = 1ul << 8;  // gpio for a0-7 output enable
constexpr uint32_t A8_15_OE = 1ul << 9; // gpio for a8-15 output enable
constexpr uint32_t D0_7_OE = 1ul << 10; // gpio for D0-7 output enable
constexpr uint32_t CLOCK_MASK = 1ul << GP_CLOCK;

#define RESET_LOW false
#define RESET_HIGH true

#define ENABLE_LOW false
#define ENABLE_HIGH true

#define CLOCK_LOW false
#define CLOCK_HIGH true

#define RW_READ true
#define RW_WRITE false

#define DATA_OUTPUT 0xff
#define DATA_INPUT 0

#define MEMORY_SIZE 0x10000 // 64k

void init6502();
void reset6502();
void tick6502();
void run6502();

#endif
