
serial	equ	.1234

;                     ____    ____                 				
;             MCLR  >|    \__/    |X  B7  PGD					
;      ANODE 1  A0  <|            |<  B6  PGC, SW3...6 in 		
;      ANODE 2  A1  <|            |>  B5  Vdd ON/OFF			
;      ANODE 3  A2  <|            |X  B4  AUX data				
;      ANODE 4  A3  <|            |>  B3  cathode driver DATA   
;      ANODE 5  A4  <|            |>  B2  AUX clock				
;      ANODE 6  A5  <|   PIC18LF  |>  B1  cathode driver CLOCK	
;              Vss  <|    25K50   |<  B0  KEY INT				
;      ANODE 8  A7  <|            |=  Vdd            			
;      ANODE 7  A6  <|            |=  Vss            			
;  USB V detect C0  >|            |<  C7  RXD IR in				
;  CCP2 carrier C1  <|            |>  C6  TXD IR out			
;  LED out/ANin C2  X|            |X  C5  USB D+     			
;             Vcap  =|____________|X  C4  USB D-     			
;																
;	FOLLOWING DEFINITIONS ARE LOCATED IN FILE MASCROS.INC:		
;																
;	keyint			PORTB,0		key "INT" in					
;	clk				LATB,1		cathode driver CLOCK			
;	accscl			LATB,2		AUX clock out latch				
;	accscl.port		PORTB,2		AUX clock in port				
;	accscl.ts		TRISB,2		AUX clock TRIS bit				
;	sdo				LATB,3		cathode driver DATA				
;	accsda			LATB,4		AUX data out latch				
;	accsda.port		PORTB,4		AUX data in port				
;	accsda.ts		TRISB,4		AUX data TRIS bit				
;	vdd				LATB,5		Vdd ON/OFF switch				
;	keys			PORTB,6		PGC / SW3...6 input				
;	usbv			PORTC,0		USB V detect					
;																

	LIST P=18LF25K50
	#include <p18lf25k50.inc>
	#include <macros.inc>

;*******************************************************************************
;			CONFIGURATION REGISTERS (not used if firmware is bootloaded!!!)		
;*******************************************************************************

 config PLLSEL = PLL3X      ; 3x clock multiplier
 config CFGPLLEN = ON       ; PLL Enabled
 config CPUDIV = NOCLKDIV   ; 1:1 mode
 config LS48MHZ = SYS48X8   ; System clock at 48 MHz, USB clock divider is set to 8
 config FOSC = INTOSCIO     ; Internal oscillator
 config PCLKEN = OFF        ; Primary oscillator disabled
 config FCMEN = OFF         ; Fail-Safe Clock Monitor disabled
 config IESO = OFF          ; Oscillator Switchover mode disabled
 config nPWRTEN = ON        ; Power up timer enabled
 config BOREN = OFF         ; BOR disabled
 config nLPBOR = OFF        ; Low-Power Brown-out Reset disabled
 config WDTEN = OFF         ; WDT disabled in hardware (SWDTEN ignored)
 config CCP2MX = RC1        ; CCP2 input/output is multiplexed with RC1
 config PBADEN = OFF        ; PORTB<5:0> pins are configured as digital I/O on Reset
 config MCLRE = ON          ; MCLR pin enabled; RE3 input disabled
 config STVREN = OFF        ; Stack full/underflow will not cause Reset
 config LVP = OFF           ; Single-Supply ICSP disabled
 config XINST = OFF         ; Instruction set extension and Indexed Addressing mode disabled
 config DEBUG = OFF         ; Bkgnd debugger disabled, RB6 and RB7 configured as gp I/O pins
 config CP0 = OFF           ; Block 0 is not code-protected
 config CP1 = OFF           ; Block 1 is not code-protected
 config CPB = OFF           ; Boot block is not code-protected
 config CPD = OFF           ; Data EEPROM is not code-protected
 config WRT0 = OFF          ; Block 0 (0800-1FFFh) is not write-protected
 config WRT1 = OFF          ; Block 1 (2000-3FFFh) is not write-protected
 config WRTC = OFF          ; Configuration registers (300000-3000FFh) are not write-protected
 config WRTB = OFF          ; Boot block (0000-7FFh) is not write-protected
 config WRTD = OFF          ; Data EEPROM is not write-protected
 config EBTR0 = OFF         ; Block 0 is not protected from table reads executed in other blocks
 config EBTRB = OFF         ; Boot block is not protected from table reads executed in other blocks

 global Flag, KeyEdge, BitMask, Rotor0, Rotor1, Rotor2, Rotor3, rnd, cont, tx.byte
 global Buffer, Buffer2, BufferPause, Ma0, Ma1, Ma2, Ma3, Mc0, Mc1, Mc2, Mc3, tx.ascnum
 global	RXptr, Received, RXpatience, RXFlag, MYserial


access	equ	0
banked	equ	1

;*******************************************************************************
;							DATA  MEMORY										
;*******************************************************************************

kernel_ram	udata_acs	0

Brightness	res	1	; display ON time, 0...15, used for dimming (15 = max brightness)
					; Brightness should be at address 0 (cleared by POR only!)

Flag	res 1	; bit 0 set = pause mode
				; bit 1 set = Timer interrupt (1200 Hz) handshaking (set only)
				; bit 2 set = full scan (150 Hz) handshaking (set only)
				; bit 3 set = 
				; bit 4 set = INT in second cycle (OFF period)
				; bit 5 set = Only two steps for key 0, without pause
				; bit 6 set = Display message received
				; bit 7 set = column cathode

RXFlag	res 1	; bit 0 set = RX enable
				; bit 1 set = RX header reception in progress
				; bit 2 set = RX in progress
				; bit 3 set = 
				; bit 4 set = 
				; bit 5 set = 
				; bit 6 set = 
				; bit 7 set = 

KeyEdge		res 1	; bits 3,2,1,0 are flags, set when keys 3,2,1,0 are JUST pressed
					; must be cleared in user routine
					; bit 0 set if key 0 (INT) just pressed
					; bit 1 set if key 1 (left) just pressed
					; bit 2 set if key 2 (up) just pressed
					; bit 3 set if key 3 (down) just pressed
					; bit 4 set if key 4 (right) just pressed

BitMask		res 1	; 10000000...00000001, shift right (CA) or 0...7 row counter (CC)
BitMask2	res 1	; 11111110...01111111, shift left
T0period	res	1	; total (OFF+ON) period for T0
Rotor0		res 1	; used for key 0 (INT) debouncer
Rotor1		res 1	; used for key 1 (left) debouncer
Rotor2		res 1	; used for key 2 (up) debouncer
Rotor3		res 1	; used for key 3 (down) debouncer
Rotor4		res 1	; used for key 4 (right) debouncer
Ma0			res 1	; rnd seed
Ma1			res 1
Ma2			res 1
Ma3			res 1
Mc0			res 1	; rnd acc
Mc1			res 1
Mc2			res 1
Mc3			res 1
InnerInt	res 1	; inner loop counter
OuterInt	res 1	; outer loop counter
OutPlusInt	res 1	; one more outer loop counter
FSR0temp	res	2	; temporary FSR0 during INT
RXptr		res	1	; low RXD buffer pointer, high is always =6
Received	res 1	; Received chars counter, 0...255 in interrupt RX buffer service routine
RXpatience	res	1	; Patience counter, preset to 240 (200ms) when byte received, countdown to 0
RXserial	res	2	; received serial number BINARY, ready for comparison
MYserial	res	2	; serial number from ROM addr 0x000E

banked_ram	udata	0x0700

Buffer		res .16	; display buffer, upper row first, bit 7 = left column, bit set = LED on
Buffer2		res .16	; aux buffer (not displayed by interrupt display refresh)
BufferPause	res .16	; aux buffer (displayed only during pause)

;*******************************************************************************
;							C  O  D  E											
;*******************************************************************************

	code	Offset+0x0000
	goto	main

;*******************************************************************************
;				TIMER2 AND EXT0 INTERRUPT AND SERIAL NUMBER						
;*******************************************************************************

	org		Offset+0x0008		; T2 int 800 Hz
	bra		interrupt_8

	org		Offset+0x000E		; serial number
	dw		serial

interrupt_8
	#include <int.inc>

;*******************************************************************************
;							M A I N												
;*******************************************************************************

main
;								INTERNAL OSCILLATOR
	movlw	b'01111100'		; Sleep mode on SLEEP instr, 16 MHz internal osc
	movwf	OSCCON,access
	movlw	b'00010011'
	movwf	OSCCON2,access

;								PORTS

	banksel	ANSELA			; banked SFR page
	clrf	ANSELA,banked	; all inputs digital
	clrf	ANSELB,banked	; all inputs digital
	movlw	b'00000100'
	movwf	ANSELC,banked	; all inputs digital except C2 (AN14)
	banksel	0				; banked page 0

	movlw	b'00000000'		; pull-ups enabled, int0 on falling edge of PORTB,0
	movwf	INTCON2,access
	movlw	b'01000000'
	movwf	WPUB,access		; PORTB,6 PULL UP ENABLED

	movlw	b'11111111'
	movwf	LATA,access
	movlw	b'00100000'
	movwf	LATB,access
	movlw	b'00000100'
	movwf	LATC,access

	movlw	b'00000000'		; RA all outputs except USBV and EXTRA LED
	movwf	TRISA,access
	movlw	b'01000001'		; RB all outputs except keys
	movwf	TRISB,access
	movlw	b'10111001'		; RC all outputs
	movwf	TRISC,access

; ----------------------- timer0
	movlw	b'11000110'		; T0 on, 8 bit, prescaler=128
	movwf	T0CON,access
	movlw	.78				; int on  78*128t = 9984T  (1202 Hz)
	movwf	T0period,access

; ----------------------- timer2 and PWM2
	movlw	b'00000101'		; T2 on, prescaler 1:4, no postscaler
	movwf	T2CON,access
	movlw	.75-1			; 12MHz/(4*75) = 40KHz carrier
	movwf	PR2,access

	movlw	b'00001100'		; PWM mode
	movwf	CCP2CON,access
	movlw	b'00100110'		; period = .38
	movwf	CCPR2L,access
	clrf	CCPR2H,access

; ----------------------- UART
	movlw	b'00100000'
	movwf	TXSTA1,access	; reg TXREG, BRGH=0
	movlw	b'10010000'
	movwf	RCSTA1,access	; reg RCREG
	movlw	b'00011000'
	movwf	BAUDCON1,access	; TX inverted, brg16=1
	movlw	high .5000
	movwf	SPBRGH1,access	; 600 baud
	movlw	low .5000
	movwf	SPBRG1,access	; 600 baud

;-------------------------------- initialization over

	bcf		vdd,access				; enable shift reg voltage supply

	lfsr	FSR0,1			; Brightness is skipped, that's why it is at addr 0
goclr1
	xorwf	INDF0,w,access	; uses 0x000-0x1FF for randomization and clears banks
	clrf	POSTINC0		; clears ram banks 0 and 1
	btfss	FSR0H,1,access
	bra		goclr1
	movwf	Ma0,access		; randomizes seed 0
goclr2
	xorwf	INDF0,w,access	; uses 0x200-0x3FF for randomization and clears banks
	clrf	POSTINC0		; clears ram banks 2 and 3
	btfsc	FSR0H,1,access
	bra		goclr2
	movwf	Ma1,access		; randomizes seed 1
goclr3
	xorwf	INDF0,w,access	; uses 0x400-0x5FF for randomization and clears banks
	clrf	POSTINC0		; clears ram banks 4 and 5
	btfss	FSR0H,1,access
	bra		goclr3
	movwf	Ma2,access		; randomizes seed 2
goclr4
	xorwf	INDF0,w,access	; uses 0x600-0x7FF for randomization and clears banks
	clrf	POSTINC0		; clears ram banks 6 and 7
	btfsc	FSR0H,1,access
	bra		goclr4
	movwf	Ma3,access		; randomizes seed 3

	banksel	Buffer			; all banked modes will address bank 7
	clrf	TBLPTRU,access
	bsf		Flag,5,access	; bit 5 set = Only two steps for key 0, without pause

	movlw	.8
	movwf	Brightness,access	; Brightness initialized

;							serial number start								
		if Offset == 0
	movlw	0x0E				; serial number defined in kernel
	movwf	TBLPTRL,access
	clrf	TBLPTRH,access
	tblrd*+
	movff	TABLAT,MYserial+0	; lo byte
	tblrd*
	movff	TABLAT,MYserial+1	; hi byte
		else
	movlw	low (Offset+0x000E)		; serial number defined in kernel
	movwf	TBLPTRL,access
	movlw	high (Offset+0x000E)	; serial number defined in kernel
	movwf	TBLPTRH,access
	tblrd*+
	movff	TABLAT,MYserial+0	; lo byte
	tblrd*-
	movf	MYserial+0,w,access
	iorwf	TABLAT,w,access
	bz		get.from.boot			; if serial number defined in kernel is 0x0000
	movf	MYserial+0,w,access
	andwf	TABLAT,w,access
	addlw	1
	bnz		get.hi					; if serial number defined in kernel is not 0xFFFF
get.from.boot
	movlw	0x0E				; serial number defined in bootloader
	movwf	TBLPTRL,access
	clrf	TBLPTRH,access
	tblrd*+
	movff	TABLAT,MYserial+0	; lo byte
	tblrd*
get.hi
	movff	TABLAT,MYserial+1	; hi byte
		endif
;								EEPROM read									

	clrf	EECON1,access		; Disable write to EEPROM
	setf	EEADR,access		; EEADR = -1

	call	ee.rd				; read addr 0 (Brightness)
	andlw	0x0F
	movwf	Brightness,access
						; load display text
	lfsr	FSR0,0x600			; Display buffer
	call	ee.rd				; read first byte only from EE addr 1
	movwf	POSTINC0,access

	addlw	0
	bz		def.txt				; if first char = terminator, go load def text
	addlw	1
	bz		def.txt				; if first char = 0xFF (empty EE), go load def text

	movlw	.254				; one was read, 254 more left
	movwf	BitMask,access		; temporarily borrowed register
go.rd.ee
	call	ee.rd
	movwf	POSTINC0,access
	decfsz	BitMask,f,access
	bra		go.rd.ee
	bra		exit.ee
						; load default text from rom
def.txt
	lfsr	FSR0,0x600			; initialize dest
	movlw	low greeting
	movwf	TBLPTRL,access		; initialize src
	movlw	high greeting
	movwf	TBLPTRH,access		; initialize src
transfer.greeting
	tblrd*+						; read ACSII byte from ROM
	movf	TABLAT,w,access
	movwf	POSTINC0
	bnz		transfer.greeting
exit.ee							; message terminator 0 found

;																			

	call	disp.detect			; auto detect display type (affects Flag,7)

	movlw	b'11100000'		; enable INTERRUPTS, enable timer0 int
	movwf	INTCON,access
	goto	cont

;***********************************************************************************;
;***********************************************************************************;
ee.rd
	incf	EEADR,f,access		; *** adv addr (1...255)
	bsf		EECON1,RD,access	; initiate READ
	movf	EEDATA,w,access		; read data
	return

;***********************************************************************************;
disp.detect
	movlw	0x7F
	movwf	LATA			; switch on anode 7 only
	movlw	b'00111001'		; AN14, enable ADC
	movwf	ADCON0,access
	movlw	b'00000000'		; Vref+ to AVdd, Vref- to AVss
	movwf	ADCON1,access
	movlw	b'00111110'		; Left just, 20 Tad, FOSC/64
	movwf	ADCON2,access

		bcf		sdo,ACCESS		; serial data low (shift reg outputs 2...15 off)
	variable xx
xx=0
	while xx<.15
		bcf		clk,ACCESS		; ^^^\___	generate "clk" pulse
		bsf		clk,ACCESS		; ___/^^^	shift out one bit
xx+=1
	endw

	bsf		sdo,ACCESS		; serial data high (shift reg output 1 on)
	bcf		clk,ACCESS		; ^^^\___	generate "clk" pulse
	bsf		clk,ACCESS		; ___/^^^	shift out one bit

	call	ad.convert		; first ADC
	movwf	BitMask,access	; temporarily borrowed register

	bcf		sdo,ACCESS		; serial data low (shift reg output 1 off,output 2 on)
	bcf		clk,ACCESS		; ^^^\___	generate "clk" pulse
	bsf		clk,ACCESS		; ___/^^^	shift out one bit

	call	ad.convert		; second ADC

	setf	LATA,access		; switch off all anodes
	bcf		ADCON0,access	; disable ADC

	subwf	BitMask,w,access	; conv1-conv2
	ifnc
	bsf		Flag,7,access	; set column cathode flag if conv2>conv1
	return

ad.convert
	bsf		ADCON0,GO,access	; start conv
wait.conv
	btfsc	ADCON0,GO,access	; test ready/bysy flag
	bra		wait.conv
	movf	ADRESH,w,access		; read 8-bit result to W
	return

;***********************************************************************************;
tx.ascnum
	andlw	0x0f			; isolate digit
	iorlw	0x30
tx.byte
	btfss	TXSTA,TRMT
	bra		tx.byte		; jer tx reg full
	movwf	TXREG
	return

;***********************************************************************************;
;   32-bit pseudorandom generator													;
;   every time it is envoked, it performs SEED * 0x41c64e6d + 0x00006073 ---> SEED	;
;   At the end, it performs  Ma0 xor Ma1 plus Ma2 xor Ma3 xor TMR2 ---> w			;
;   where: Ma0 = lowest byte of SEED, Ma3 = highest byte							;
;   input:  Ma0,Ma1,Ma2,Ma3 (which is SEED), output: w, trashes: Mc0,Mc1,Mc2,Mc3	;
;   Execution time:  80T including call/ret											;
;***********************************************************************************;
rnd
;	A3 A2 A1 A0 * 41 c6 4e 6d = C3 C2 C1 C0 

	movf	Ma0,w,access		; a0
	mullw	0x6d				; b0
	movff	PRODL,Mc0
	movff	PRODH,Mc1			; 1

	movf	Ma0,w,access		; a0
	mullw	0x4e				; b1
	movf	PRODL,w,access
	addwf	Mc1,f,access		; +lo
	ifc
	incf	PRODH,f,access		; c
	movff	PRODH,Mc2			; 2

	movf	Ma1,w,access		; a1
	mullw	0x6d				; b0
	movf	PRODL,w,access
	addwf	Mc1,f,access		; +lo
	movf	PRODH,w,access
	addwfc	Mc2,w,access		; +hi c
	clrf	Mc3,access
	ifc
	incf	Mc3,f,access		; c
	movwf	Mc2,access			; 3

	movf	Ma0,w,access		; a0
	mullw	0xc6				; b2
	movf	PRODL,w,access
	addwf	Mc2,f,access		; +lo
	movf	PRODH,w,access
	addwfc	Mc3,f,access		; +hi c   4

	movf	Ma2,w,access		; a2
	mullw	0x6d				; b0
	movf	PRODL,w,access
	addwf	Mc2,f,access		; +lo
	movf	PRODH,w,access
	addwfc	Mc3,f,access		; +hi c   5

	movf	Ma1,w,access		; a1
	mullw	0x4e				; b1
	movf	PRODL,w,access
	addwf	Mc2,f,access		; +lo
	movf	PRODH,w,access
	addwfc	Mc3,f,access		; +hi c   6

	movf	Ma0,w,access		; a0
	mullw	0x41				; b3
	movf	PRODL,w,access
	addwf	Mc3,f,access		; +lo     7

	movf	Ma3,w,access		; a0
	mullw	0x6d				; b0
	movf	PRODL,w,access
	addwf	Mc3,f,access		; +lo     8

	movf	Ma1,w,access		; a0
	mullw	0xc6				; b2
	movf	PRODL,w,access
	addwf	Mc3,f,access		; +lo     9

	movf	Ma2,w,access		; a0
	mullw	0x4e				; b1
	movf	PRODL,w,access
	addwf	Mc3,f,access		; +lo     10

;                  |      A0 B0    1
;                  |   A0 B1       2
;                  |   A1 B0       3
;                  |A0 B2          4
;                  |A2 B0          5
;                  |A1 B1          6
;               |A0 B3             7
;               |A3 B0             8
;               |A1 B2             9
;               |A2 B1             10

;	C3 C2 C1 C0 + 00 00 60 73 = A3 A2 A1 A0 

	clrf	Ma3,access
	clrf	Ma2,access
	movlw	0x60
	movwf	Ma1,access
	movlw	0x73
	movwf	Ma0,access

	movf	Mc0,w,access
	addwf	Ma0,f,access

	movf	Mc1,w,access
	addwfc	Ma1,f,access

	movf	Mc2,w,access
	addwfc	Ma2,f,access

	movf	Mc3,w,access
	addwfc	Ma3,f,access

	movf	Ma0,w,access	; extra:  Ma0 ^ Ma1 + Ma2 ^ Ma3 ---> w
	xorwf	Ma1,w,access
	addwf	Ma2,w,access
	xorwf	Ma3,w,access
	xorwf	TMR2,w,access	; 8-bit output: w

	return

;																					
greeting

 db "<h>Welcome to<H> Hackaday Belgrade<h> conference    "
 db	"<I>H<i> <I>A<i> <I>C<i> <I>K<i> <I>A<i> <I>D<i> <I>A<i> <I>Y<I> "
 db "<v>Welcome to<V> Hackaday Belgrade<v> conference    "
 db	"<I>H<i> <I>A<i> <I>C<i> <I>K<i> <I>A<i> <I>D<i> <I>A<i> <I>Y<I> "

; db "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 01234567890 !@#$%^&*()_+-= \':[]{},./<>?",0x32,0x3b,0

;																					
cont

	END


