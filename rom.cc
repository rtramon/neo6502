#include <stdint.h>

uint8_t mem_test[] = {0xa9, 0,    0xa8, 0xaa, 0xc8, 0x98, 0x95, 0,
                      0xe8, 0xe0, 0xff, 0xd0, 0xF9, 0xA9, 00,   0xAA,
                      0x98, 0xD5, 00,   0xD0, 0x0C, 0xE8, 0xE0, 0xFF,
                      0xD0, 0xF7, 0xC0, 0xFF, 0xD0, 0xE6, 0x4C, 0x00,
                      0x06, 0xEA, 0x4C, 0x00, 0x06};

/* Assembly*/
#if 0
0600                             * = $0600
0600  A9 00        L0600         LDA #$00
0602  A8                         TAY
0603  AA                         TAX
0604  C8           L0604         INY
0605  98                         TYA
0606  95 00        FILL          STA $00,X
0608  E8                         INX
0609  E0 FF                      CPX #$FF
060B  D0 F9                      BNE $0606
060D               CHECK         
060D  A9 00                      LDA #$00
060F  AA                         TAX
0610  98                         TYA
0611               CHECK1        
0611  D5 00                      CMP $00,X
0613  D0 0C                      BNE $0621
0615  E8                         INX
0616  E0 FF                      CPX #$FF
0618  D0 F7                      BNE $0611
061A  C0 FF                      CPY #$FF
061C  D0 E6                      BNE $0604
061E  4C 00 06                   JMP $0600
0621               FAIL          
0621  EA                         NOP
0622  4C 00 06                   JMP $0600
#endif

void load_rom() {
  extern uint8_t mem[];
  for (int i = 0; i < sizeof(mem_test); i++)
    mem[0x0600 + i] = mem_test[i];

  // setup reset vector
  mem[0xFFFC] = 0;
  mem[0xFFFD] = 0x06;
}