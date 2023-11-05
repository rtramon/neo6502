;
; Builtin Test for Memory
; syntax for xa65 assembler 
;
* = $0600
DSP	=	$d012
DSPCR	=	$d013

RESET:
	CLD
	LDX #$FF
	TXS
	JSR DISPLAY_INIT
LP:
	JSR PRINTSTART
        JSR TEST_MEM
	JMP LP
	
TEST_MEM:
      LDA #$00
             TAY
              TAX
L0604:        INY
              TYA
FILL:         STA $00,X
              INX
              CPX #$FF
              BNE FILL
CHECK:        
              LDA #$00
              TAX
              TYA
CHECK1:        
              CMP $00,X
              BNE FAIL
              INX
              CPX #$FF
              BNE CHECK1
              CPY #$FF
              BNE L0604
              JSR PRINTOK
	RTS
FAIL:          
	LDA #'E'
	STA DSP
	LDA #'R'
	STA DSP
	LDA #10		; newline
	STA DSP
        RTS

DISPLAY_INIT:
	LDA	#$02
	STA	DSPCR
	RTS

PRINTSTART:
        LDX #0
LOOP1:
        LDA _START,X
        BEQ END
        STA DSP
        INX
        JMP LOOP1
END:
        RTS

_START: .byt 10, "Start ", 0

PRINTOK:
        LDX #0
LOOP2:
        LDA _OK,X 
        BEQ END2
        STA DSP
        INX
        JMP LOOP2
END2:
        RTS
_OK:    .byt "OK", 10, 0

