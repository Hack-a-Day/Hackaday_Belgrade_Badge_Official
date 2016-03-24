;*****************************************************************************************	
;	TIMER0  INTERRUPT (vector = 0x18 or 0x1018)												
;	Services: - 8x16 LED display (points to Buffer or BufferPause, determined by Flag.0		
;			  - 5 keys (debouncer and edge detection)										
;			  - Brightness inc/dec with physiological linearization							
;			  - Pause flag (sets/resets Flag.0) if Flag.5 is reset							
;			  - Sleep/wake up																
;	Default interrupt frequency:  12M/(78*128) = 1202 Hz									
;	That gives about 150 Hz display refresh rate											
;	Register Brightness (0...15) affects duty cycle only									
;	The best way to affect the display refresh rate is to edit table "bright.tab"			
;	It is also possible to change T0 prescaler setting (very coarse, in *2 steps)			
;	NOTE: This is not the only interrupt, as INT0 is used to wake up the					
;		  device from sleep. As the device actually sleeps in interrupt						
;		  routine, and external INT0 (which is used to wake up the device					
;		  from sleep) can have only the high priority, this TMR2 interrupt					
;		  must have the same priority level, or interrupt nesting has to be					
;		  disabled (otherwise, FAST registers will be lost during wake-up)					
;*****************************************************************************************	

	banksel	Buffer			; all banked modes will address bank 7 in this routine

; ignore interrupt if it was INT0											
	btfsc	INTCON,INT0IF,access
	goto	it.was.int0
; brightness																

	movff	TBLPTRL,FSR0temp+0	; save TBLPTR
	movff	TBLPTRH,FSR0temp+1

	movlw	high bright.tab
	movwf	TBLPTRH,access
	movf	Brightness,w,access	; Brightness = 0...15
	andlw	0x0f				; just in case
	movwf	Brightness,access	; Brightness repaired
	addwf	WREG,w,access		; W = Brightness*2 = 0...30

	btg		Flag,4,access		; bit 4 set: INT in 2nd cycle (OFF period)
	btfsc	Flag,4,access
	incf	WREG,f,access		; if INT is in 2nd cycle (OFF period), point to 2nd byte

	addlw	low bright.tab
	movwf	TBLPTRL,access
	ifc
	incf	TBLPTRH,f,access	; if TBLPTRL overflowed

	tblrd*						; read T0 timing from table
	movff	TABLAT,TMR0L		; TMR0L <-------------

	movff	FSR0temp+0,TBLPTRL	; restore TBLPTR
	movff	FSR0temp+1,TBLPTRH

	btfsc	Flag,4,access
	setf	LATA,access			; in OFF period, turn off andode drivers
	btfsc	Flag,4,access
	goto	exit2				; in OFF period, nothing else should be executed
							; first cycle (ON period)
	movff	FSR0L,FSR0temp+0	; save FSR0
	movff	FSR0H,FSR0temp+1	; save FSR0

; display refresh															
	btfss	Flag,7,access		; bit 7 set = common cathode
	bra		column.anode

;---------------------------------------------------------------------------
;						COLUMN CATHODE DRIVER								
;																			

	scf							; set carry, as Bitmask2 is inverse
	rlcf	BitMask2,f,access	; Shift Left BitMask2      <--- 11111011 <--- 1

	incf	BitMask,w,access
	andlw	0x07
	movwf	BitMask,access		; BitMask count up 0...7
	ifz
	bcf		BitMask2,0,access	; bit reconstruction BitMask2

	lfsr	FSR0,Buffer+8		; frame buffer pointer
	btfsc	Flag,0				; bit 0 set = pause
	lfsr	FSR0,BufferPause+8	; pause buffer pointer
	addwf	FSR0L,f,access
	onebit	0
	onebit	1
	onebit	2
	onebit	3
	onebit	4
	onebit	5
	onebit	6
	onebit	7

	movlw	-.8
	addwf	FSR0L,f,access
	onebit	0
	onebit	1
	onebit	2
	onebit	3
	onebit	4
	onebit	5
	onebit	6
	onebit	7

; Sets one anode, depended on BitMask
	movf	BitMask2,w,access
	movwf	LATA,access
	addlw	.2
	ifz
	bsf		Flag,2,access		; 150 Hz full scan handshaking flag (should be after last anode!)

	goto	column.any

;---------------------------------------------------------------------------
;						COLUMN ANODE DRIVER									
;																			
column.anode
; rotates with no carry BitMask (slower than single RLNCF, but more safe)
	scf							; set carry, as Bitmask2 is inverse
	rlcf	BitMask2,f,access	; Shift Left BitMask2      <--- 11111011 <--- 1
	rcf							; clear Carry
	rrcf	BitMask,f,access	; Shift Right BitMask      <--- 00000100 <--- 0
	bnz		no.reconstruction
	bsf		BitMask,7,access	; bit reconstruction BitMask
	bcf		BitMask2,0,access	; bit reconstruction BitMask2
no.reconstruction
	lfsr	FSR0,Buffer+.15		; frame buffer pointer
	btfsc	Flag,0				; bit 0 set = pause
	lfsr	FSR0,BufferPause+.15	; pause buffer pointer

	variable xx
xx=0
	while xx<.16
		bcf		sdo,ACCESS
		movf	BitMask,w,ACCESS
		andwf	POSTDEC0,w
		ifnz
		bsf		sdo,ACCESS		; Transfer "BitMask" state in POSTDEC0 to "sdo" pin
		bcf		clk,ACCESS		; ^^^\___	generate "clk" pulse
		bsf		clk,ACCESS		; ___/^^^	shift out one bit
xx+=1
	endw

; Sets one anode, depended on BitMask
	movf	BitMask2,w,access
	movwf	LATA,access
	addlw	.2
	ifz
	bsf		Flag,2,access		; 150 Hz full scan handshaking flag (should be after last anode!)
column.any
;																			
;---------------------------------------------------------------------------

; Tests keys in every 8th pass (150 Hz rate)
	btfsc	BitMask2,0,access
	goto	no.test.0
							; only if BitMask2 = 11111110 (anode 1 on)
					; *** key 0 test (up)
	rcf
	btfsc	keyint,access
	scf							; port pin state transferred to Carry
	rlcf	Rotor0,f,access		; shift in
	movf	Rotor0,w,access
	addlw	2					; if Rotor0 = 11111110 (just pressed), W will be 0
	ifz
	bsf		KeyEdge,0,access	; if key0 just pressed, set EDGE flag
					; *** key 1 test (left)
	rcf
	btfsc	keys,access
	scf							; port pin state transferred to Carry
	rlcf	Rotor1,f,access		; shift in
	movf	Rotor1,w,access
	addlw	2					; if Rotor1 = 11111110 (just pressed), W will be 0
	ifz
	bsf		KeyEdge,1,access	; if key1 just pressed, set EDGE flag
	ifz
	bcf		Flag,0,access	; reset pause flag
	bra		no.test.x
no.test.0
;		
	btfsc	BitMask2,1,access
	goto	no.test.1
							; only if BitMask2 = 11111101 (anode 2 on)
					; *** key 2 test (up)
	rcf
	btfsc	keys,access
	scf							; port pin state transferred to Carry
	rlcf	Rotor2,f,access		; shift in
	movf	Rotor2,w,access
	addlw	2					; if Rotor2 = 11111110 (just pressed), W will be 0
	ifz
	bsf		KeyEdge,2,access	; if key2 just pressed, set EDGE flag
	ifz
	bcf		Flag,0,access	; reset pause flag
	bra		no.test.x
no.test.1
;		
	btfsc	BitMask2,2,access
	goto	no.test.2
							; only if BitMask2 = 11111011 (anode 3 on)
					; *** key 3 test (down)
	rcf
	btfsc	keys,access
	scf							; port pin state transferred to Carry
	rlcf	Rotor3,f,access		; shift in
	movf	Rotor3,w,access
	addlw	2					; if Rotor2 = 11111110 (just pressed), W will be 0
	ifz
	bsf		KeyEdge,3,access	; if key3 just pressed, set EDGE flag
	ifz
	bcf		Flag,0,access	; reset pause flag
	bra		no.test.x
no.test.2
;		
	btfsc	BitMask2,3,access
	goto	no.test.x
							; only if BitMask2 = 11110111 (anode 4 on)
					; *** key 4 test (right) and Brightness +/- keys
	rcf
	btfsc	keys,access
	scf							; port pin state transferred to Carry
	rlcf	Rotor4,f,access		; shift in
	movf	Rotor4,w,access
	addlw	2					; if Rotor2 = 11111110 (just pressed), W will be 0
	ifz
	bsf		KeyEdge,4,access	; if key4 just pressed, set EDGE flag
	ifz
	bcf		Flag,0,access	; reset pause flag
;		
					; *** keys 1+4+2 or 1+4+3 test (brightness +/-)
	btfsc	Rotor1,0,access
	bra		no.test.x			; if key LEFT not pressed
	btfsc	Rotor4,0,access
	bra		no.test.x			; if key RIGHT not pressed

	btfss	KeyEdge,2,access
	bra		no.bright.up
								; brightness +
	movf	Brightness,w,access
	addlw	-.15
	ifnc						; if <15...
	incf	Brightness,f,access	; ...then inc Brightness

	clrf	EEADR,access		; *** Brightness je na addr 0
	movff	Brightness,EEDATA	; *** load data
	movlw	b'00000100'			; ***
	movwf	EECON1,access		; *** Allow write to EEPROM
	movlw	0x55				; ***
	movwf	EECON2,access		; *** first WR password
	movlw	0xAA				; ***
	movwf	EECON2,access		; *** seconf WR password
	bsf		EECON1,WR,access	; *** WRITE TO EEPROM

	bra		no.test.14

no.bright.up
	btfss	KeyEdge,3,access
	bra		no.test.x
								; brightness -
	movf	Brightness,f,access	; cp 0
	ifnz						; if >0...
	decf	Brightness,f,access	; ...then dec Brightness

	clrf	EEADR,access		; *** Brightness je na addr 0
	movff	Brightness,EEDATA	; *** load data
	movlw	b'00000100'			; ***
	movwf	EECON1,access		; *** Allow write to EEPROM
	movlw	0x55				; ***
	movwf	EECON2,access		; *** first WR password
	movlw	0xAA				; ***
	movwf	EECON2,access		; *** seconf WR password
	bsf		EECON1,WR,access	; *** WRITE TO EEPROM
no.test.14
	clrf	KeyEdge,access
no.test.x
; IF KEY0 (UP) JUST PRESSED, GO PAUSE OR SLEEP MODE

	btfsc	Flag,5,access	; bit 5 set = Only two steps for key 0, without pause
	bra		pause.mode		; if pause not allowed, act as if it was pause already

	btfsc	Flag,0,access		; pause flag
	bra		pause.mode
					; NO PAUSE MODE HERE ---								
	btfsc	KeyEdge,0,access
	bsf		Flag,0,access			; if key 0 just pressed, set pause flag
	bcf		KeyEdge,0,access	; clear KEY0 flag
	goto	no.test				; go on

pause.mode			; PAUSE MODE HERE ---									
	btfss	KeyEdge,0,access
	bra		no.test				; if not just test, skip sleeping routine
	bcf		KeyEdge,0,access	; clear KEY0 flag
	bcf		Flag,0,access		; clear pause flag
	call	sleeping.time		; GO SLEEP !!!

; NO SLEEP OR RETURNED FROM SLEEP 												
no.test
; RX service																	

	btfsc	RXFlag,0
	bra		rx.enabled			; if RX enabled
	movf	RCREG,w,access		; kill PIR1,RCIF flag
	bra		no.rx				; if RX disabled
rx.enabled
	btfss	PIR1,RCIF,access	; is RX register empty?
	bra		no.rx				; if RX register is empty
								; RX register is full
	movf	RCREG,w,access		; this resets PIR1,RCIF flag
	btfsc	RXFlag,1,access		; bit 6 set: RX header in progress
	bra		header.in.progress
	btfsc	RXFlag,2,access		; bit 3 set: RX in progress
	bra		in.progress
								; nothing is in progress, only '[' will be accepted
	xorlw	'['
	bnz		rx.exit				; it was byte outside verified reception, ignore it
	clrf	RXserial+0,access
	clrf	RXserial+1,access
	bsf		RXFlag,1,access		; bit 6 set: RX header in progress (open header reception cycle)
	bra		preset.pat			; after '['

header.in.progress
	xorlw	']'
	bz		rx.ser.terminator	; "]" is serial number terminator

	movf	RCREG,w,access
	addlw	-'0'
	addlw	-.10
	ifc
	clrf	RXpatience,access	; byte is not ASCII digit, terminate
	bc		preset.pat			; byte is not ASCII digit, terminate

	serialx2					; *2
	movff	RXserial+0,InnerInt
	movff	RXserial+1,OuterInt
	serialx2					; *4
	serialx2					; *8
	movf	InnerInt,w,access
	addwf	RXserial+0,f,access
	movf	OuterInt,w,access
	addwfc	RXserial+1,f,access	; *10

	movf	RCREG,w,access
	andlw	0x0f				; convert ascii digit to binary
	addwf	RXserial+0,f,access	; *10 + received byte
	ifc
	incf	RXserial+1,f,access
	bra		preset.pat			; after one digit

rx.ser.terminator
	bcf		RXFlag,1,access		; bit 6 clr: terminate RX header progress
	bcf		RXFlag,2,access		; bit 3 clr: default RX not in progress

	clrf	EEADR,access		; ***

	movf	RXserial+0,w,access
	xorwf	MYserial+0,w,access
	bnz		preset.pat			; serial number doesn't match
	movf	RXserial+1,w,access
	xorwf	MYserial+1,w,access
	bnz		preset.pat			; serial number doesn't match

	clrf	RXptr,access		; clr RX pointer
	bsf		RXFlag,2,access		; bit 3 set: RX in progress (open RX cycle)
	bra		preset.pat			; after serial number recognized

in.progress
	incf	Received,f,access
	bz		RXbuf.ovf

	lfsr	FSR0,0x600
	movff	RXptr,FSR0L			; load FSR0 with RX buffer pointer
	incf	RXptr,f,access		; RXptr ready for next byte
	movff	RCREG,POSTINC0		; WRITE TO RAM
	clrf	INDF0				; put 0x00 terminator

	incf	EEADR,f,access		; *** adv addr (1...255)
	incf	EEADR,w,access		; *** test if EEADR=255 (keep at least 1 by for terminator)
	bz		preset.pat			; *** if overflow (>254 chars)	
	movff	RCREG,EEDATA		; *** load data
	movlw	b'00000100'			; ***
	movwf	EECON1,access		; *** Allow write to EEPROM
	movlw	0x55				; ***
	movwf	EECON2,access		; *** first WR password
	movlw	0xAA				; ***
	movwf	EECON2,access		; *** seconf WR password
	bsf		EECON1,WR,access	; *** WRITE TO EEPROM
preset.pat
	movlw	.240
	movwf	RXpatience,access	; preset patience to 200 ms
	bra		rx.exit
RXbuf.ovf
	decf	Received,f,access	; restore Received max
no.rx
	movf	RXpatience,f,access
	bnz		still.rxpat

	bcf		EECON1,WREN,access	; *** disable further eeprom writes
	bcf		RXFlag,2,access		; bit 3 clr: terminate RX progress
	bcf		RXFlag,1,access		; bit 6 clr: terminate RX header progress
	clrf	RXptr,access		; clr RX pointer
	bra		rx.exit
still.rxpat
	decf	RXpatience,f,access
	bnz		rx.exit
	bsf		Flag,6,access		; bit 6 set = Display message received
	incf	EEADR,f,access		; *** adv addr (1...255)
	clrf	EEDATA,access		; *** load terminator (0)
	movlw	0x55				; ***
	movwf	EECON2,access		; *** first WR password
	movlw	0xAA				; ***
	movwf	EECON2,access		; *** seconf WR password
	bsf		EECON1,WR,access	; *** WRITE TO EEPROM
	bcf		EECON1,WREN,access	; *** disable further eeprom writes
rx.exit

; Exit interrupt routine														
	movff	FSR0temp+0,FSR0L		; restore FSR0
	movff	FSR0temp+1,FSR0H		; restore FSR0
exit2
	bsf		Flag,1,access			; 150x8 Hz handshaking flag
	bcf		INTCON,TMR0IF,access
	retfie	FAST
;	
it.was.int0
	bcf		INTCON,INT0IF,access	; reset INT0IF flag also, to avoid int retrigering
	retfie	FAST

;***********************************************************************************
;	Puts all hardware in low power mode, enables wake up on INT0, and sleeps		
;	On INT0 reenables all hardware drivers and resumes								
;***********************************************************************************

sleeping.time
; all drivers off																
	setf	LATA,access
	bsf		vdd,access				; disable shift reg voltage supply
; wait for 200 ms keyint = off													
	movlw	.7					; initialize 400 ms timing
	call	wait.keyint.released	; now it is released for sure (400 ms)
	bcf		INTCON,INT0IF,access	; reset INT0IF flag also, to avoid false wake up
	bsf		INTCON,INT0IE,access	; enable INT0IE (but not GIE!) to wake up from sleep
	bcf		INTCON,TMR0IE,access	; disable Timer 0 interrupt
; sleep																			
	sleep					; Zzzzzzzz... Rrrrrrrrrr... AHRRRrrrr... Zzzzzzzz...
; wake up																		
	nop
	bcf		INTCON,INT0IE,access	; disable INT0IE
	bcf		vdd,access				; reenable shift reg voltage supply
	bcf		INTCON,INT0IF,access	; reset INT0IF flag, to avoid int retrigering
	bsf		INTCON,TMR0IE,access	; reenable Timer 0 interrupt
; wait for 50 ms keyint = off													
	movlw	.2					; initialize 50 ms timing
wait.keyint.released
	movwf	OutPlusInt,access
	clrf	InnerInt,access
	clrf	OuterInt,access
debounce.off
	btfss	keyint,access
	bra		wait.keyint.released		; because it is still on
	decfsz	InnerInt,f,access
	bra		debounce.off
	decfsz	OuterInt,f,access
	bra		debounce.off
	decfsz	OutPlusInt,f,access
	bra		debounce.off
									; now it is released for sure
	return

;																

					; Brightness linearization table
bright0		equ		.3
bright1		equ		.4
bright2		equ		.5
bright3		equ		.6
bright4		equ		.8
bright5		equ		.10
bright6		equ		.12
bright7		equ		.15
bright8		equ		.19
bright9		equ		.24
bright10	equ		.30
bright11	equ		.37
bright12	equ		.45
bright13	equ		.54
bright14	equ		.65
bright15	equ		.77

bright.tab
	db	.256-bright0, .256-(.78-bright0)
	db	.256-bright1, .256-(.78-bright1)
	db	.256-bright2, .256-(.78-bright2)
	db	.256-bright3, .256-(.78-bright3)
	db	.256-bright4, .256-(.78-bright4)
	db	.256-bright5, .256-(.78-bright5)
	db	.256-bright6, .256-(.78-bright6)
	db	.256-bright7, .256-(.78-bright7)
	db	.256-bright8, .256-(.78-bright8)
	db	.256-bright9, .256-(.78-bright9)
	db	.256-bright10,.256-(.78-bright10)
	db	.256-bright11,.256-(.78-bright11)
	db	.256-bright12,.256-(.78-bright12)
	db	.256-bright13,.256-(.78-bright13)
	db	.256-bright14,.256-(.78-bright14)
	db	.256-bright15,.256-(.78-bright15)


