
	LIST P=18F25K50
	#include <p18f25k50.inc>
	#include <macros.inc>

 extern Flag, KeyEdge, BitMask, Rotor0, Rotor1, Rotor2, Rotor3, cont, tx.byte, tx.ascnum
 extern Buffer, Buffer2, BufferPause, rnd, Ma0, Ma1, Ma2, Ma3, Mc0, Mc1, Mc2, Mc3
 extern	RXptr, Received, RXpatience, RXFlag, MYserial

demo_ram	udata_acs

FlagDemo	res	1	; bit 0 set = long pause (display only)
					; bit 1 set = next char immediate (display only)
					; bit 2 set = showing final result (tetris)
					; bit 3 set = key action, bit 3 clr = no action or vertical action
Inner 		res 1	; inner loop counter (used outside int routine)
Outer		res 1	; outer loop counter (used outside int routine)
Uniform		res 1	; 150Hz freerunning uniform counter for timing functions
Tempw		res 1	; Temporary W storage
Speed		res 1	; 150Hz divider, global timing
Block		res 1	; current block number 0 to 6, 0=I, 1=J, 2=L, 3=O, 4=S, 5=Z, 6=T
CurrentBlock res 4	; bitmapped current block in current angle and current BlockX, bits 7-0
BlockX		res 1	; Current block X position, 1 to 8
BlockY		res 1	; Current block Y position, 0 (which is -1) to 15 (which is +14)
BlockA		res 1	; Current block angle, 0 to 3
Temp.BlockX res 1	; Temporary, in case the command was forbidden
Temp.BlockA res 1	; Temporary, in case the command was forbidden
Block.count res 1	; block counter, for speed increasing
Speed.index res 1	; Speed, 0 = slowest, 15 = fastest
liqflag		res	1	; bit 0 set = move X, bit 1 set = move Y
Xtest		res	1	; X pos for test if the pixel is occupied
Ytest		res	1	; Y pos for test if the pixel is occupied
WhoAmI		res	1	; Accelerometer Device ID (0x68 or 0xE5, otherwise 0x00)
DispSpeed	res	1	; Display period (number of display refresh steps between disp scrolls)
DispPtr		res	1	; Display text low pointer (high pointer is always 6)
ColCount	res	1	; Display column step counter
Score		res	2	; Tetris score (binary)
Bin			res 2	; lo,hi: Binary number for bin2bcd conversion
Bcd			res	2	; 7654 tens 3210 ones, 7654 thousands 3210 hundreds: converted BCD number
AccX		res 2	; Accelerometer X
AccY		res 2	; Accelerometer Y
AccZ		res 2	; Accelerometer Z

Disp.text	equ	0x600	; display ASCII 256 chars
;	
access	equ	0
banked	equ	1

democode	code

;*******************************************************************************
;						D E M O   F A R M	 									
;*******************************************************************************

demo.farm
	bcf		RXFlag,0,access	; disable RX
	bsf		Flag,5,access	; bit 5 set = Only two steps for key 0, without pause

	movlw	low (inimes)
	movwf	TBLPTRL
	movlw	high (inimes)
	movwf	TBLPTRH
	lfsr	FSR0,Buffer
	call	print.rom		; move .16 bytes from TBLPTR to FSR0
test.keys
	btfsc	KeyEdge,1,access
	goto	accelerometer
	btfsc	KeyEdge,2,access
	goto	display
	btfsc	KeyEdge,4,access
	goto	tetris
	btfsc	KeyEdge,3,access
	call	id
	bra		test.keys

;																				
move2up				; shift buffer2 up
	movff	Buffer2+.1,Buffer2+.0
	movff	Buffer2+.2,Buffer2+.1
	movff	Buffer2+.3,Buffer2+.2
	movff	Buffer2+.4,Buffer2+.3
	movff	Buffer2+.5,Buffer2+.4
	movff	Buffer2+.6,Buffer2+.5
	movff	Buffer2+.7,Buffer2+.6
	movff	Buffer2+.8,Buffer2+.7
	movff	Buffer2+.9,Buffer2+.8
	movff	Buffer2+.10,Buffer2+.9
	movff	Buffer2+.11,Buffer2+.10
	movff	Buffer2+.12,Buffer2+.11
	movff	Buffer2+.13,Buffer2+.12
	movff	Buffer2+.14,Buffer2+.13
	movff	Buffer2+.15,Buffer2+.14
	decf	ColCount,f,access	; decrement shift count
	return

;																				
print.rom					; move .16 bytes from TBLPTR to FSR0
	movlw	.16
go.pr
	tblrd*+
	movff	TABLAT,POSTINC0
	decfsz	WREG,f,access
	bra		go.pr
	return

;;;;;;;;;;
inimes				; initial message: D-A-T-I 
	db	b'00011000',b'00010100'
	db	b'00010100',b'00010100'
	db	b'00011000',b'00000000'
	db	b'01000111',b'10100010'
	db	b'10100010',b'11100010'
	db	b'10100010',b'00001000'
	db	b'00001000',b'00001000'
	db	b'00001000',b'00001000'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

id							; TX 3-digit or 4-digit ASCII serial number
							; including leading and trailing spaces
	clrf	KeyEdge,access

	movlw	' '
	call	tx.byte			; TX leading space

	movff	MYserial+0,Bin+0
	movff	MYserial+1,Bin+1
	call	bin2bcd			; convert serial number to BCD

	swapf	Bcd+1,w,access
	andlw	0x0f			; isolate thousands
	ifnz
	call	tx.ascnum		; TX thousands

	movf	Bcd+1,w,access
	call	tx.ascnum		; TX hundreds

	swapf	Bcd+0,w,access
	call	tx.ascnum		; TX tens

	movf	Bcd+0,w,access
	call	tx.ascnum		; TX ones

	movlw	' '
	call	tx.byte			; TX trailing space
	return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	#include <tetris.inc>	; tetris demo game
	#include <accel.inc>	; accelerator demo
	#include <display.inc>	; display demo
	#include <chargen.inc>	; character bitmaps, for display demo

	END


