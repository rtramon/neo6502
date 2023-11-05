     * = $0600
DSP	EQU	$d012
DSPCR	EQU	$d013

RESET
	CLD
	LDX #$FF
	TXS
	JSR DISPLAY_INIT
LP
	JSR TEST_MEM
	JMP LP
	
TEST_MEM      LDA #$00
              TAY
              TAX
L0604         INY
              TYA
FILL          STA $00,X
              INX
              CPX #$FF
              BNE FILL
CHECK         
              LDA #$00
              TAX
              TYA
CHECK1        
              CMP $00,X
              BNE FAIL
              INX
              CPX #$FF
              BNE CHECK1
              CPY #$FF
              BNE L0604
	RTS
FAIL          
	LDA #'E
	JSR PUTCHAR
	LDA #'R
	JSR PUTCHAR
	LDA #10		; newline
	JSR PUTCHAR
        RTS

DISPLAY_INIT
	LDA	#$02
	STA	DSPCR
	RTS

PUTCHAR
	STA	DSP
	RTS
