
Buffer3		equ	0x730	; aux buffer (used to stabilize output)
						; Everything is prepared in Buffer3 and then it is moved to Buffer
BittabR		equ	0x740	; bit table for bit position conversion
Dots		equ	0x748	; X,Y dot coordinates (table length 2x.44)

;*******************************************************************************
;*******************************************************************************
;*******************************************************************************
;					A C C E L E R O M E T E R	  (MAIN LOOP)					
;*******************************************************************************
;*******************************************************************************
;*******************************************************************************

accelerometer
	clrf	EECON1,access		; Disable write to EEPROM
	bcf		RXFlag,0,access	; disable RX
	bsf		Flag,5,access	; bit 5 set = Only two steps for key 0, without pause
	lfsr	FSR0,BittabR
	movlw	.8
	movwf	Inner,access
	movlw	0x80				; 10000000
bittab8
	movwf	POSTINC0			; preset BittabR to 10000000 01000000 00100000... 00000001
	rrncf	WREG,f,access
	decfsz	Inner,f,access
	bra		bittab8

	clrf	KeyEdge,access
	banksel	Buffer2				; all BANKED modes will address bank 7

	call	snif.accel
	movlw	.4
	movwf	Inner,access
waitacc
;	sync150
	decfsz	Inner,f,access
	bra		waitacc
	call	snif.accel

	call	reset.i2c
	call	createtable

liquid.farm
	call	get.accel		; *** read g
	call	div.clip		; adjust result
							; dice shaking test			
	movf	AccX,w,access
	addwf	AccY,w,access
	bc		g.ovf
	addwf	AccZ,w,access
	bc		g.ovf
	addlw	-.220			; dice switch threshold
	ifnc
	goto		not.ovf		; if no switch to dice
g.ovf						; switch to dice
	call	rndarea

	call	clrbuf2
	call	rnd
	andlw	7
	addlw	-6
	bc		g.ovf			; >5, rnd again
	addlw	7				; restore+1, w contains dice number 1...6

	btfss	WREG,0,access
	bra		no1
	bsf		Buffer2+3,3,banked	; central dot, existing only on numbers 1, 3, 5
	bsf		Buffer2+3,4,banked
	bsf		Buffer2+4,3,banked
	bsf		Buffer2+4,4,banked
no1
	rcf
	rrcf	WREG,f,access		; /2
	bz		drawed.num			; if it was 1

	decf	WREG,f,access
	bz		draw2.num			; if it was 2 or 3
	decf	WREG,f,access
	bz		draw4.num			; if it was 4 or 5
								; if it was 6
	bsf		Buffer2+0,3,banked	; upper middle dot (6)
	bsf		Buffer2+0,4,banked
	bsf		Buffer2+1,3,banked
	bsf		Buffer2+1,4,banked
	bsf		Buffer2+6,3,banked	; lower middle dot (6)
	bsf		Buffer2+6,4,banked
	bsf		Buffer2+7,3,banked
	bsf		Buffer2+7,4,banked
draw4.num
	bsf		Buffer2+0,6,banked	; upper left dot (6, 4)
	bsf		Buffer2+0,7,banked
	bsf		Buffer2+1,6,banked
	bsf		Buffer2+1,7,banked
	bsf		Buffer2+6,0,banked	; lower right dot (6, 4)
	bsf		Buffer2+6,1,banked
	bsf		Buffer2+7,0,banked
	bsf		Buffer2+7,1,banked
draw2.num
	bsf		Buffer2+0,0,banked	; upper right dot (6, 4, 2)
	bsf		Buffer2+0,1,banked
	bsf		Buffer2+1,0,banked
	bsf		Buffer2+1,1,banked
	bsf		Buffer2+6,6,banked	; lower left dot (6, 4, 2)
	bsf		Buffer2+6,7,banked
	bsf		Buffer2+7,6,banked
	bsf		Buffer2+7,7,banked
drawed.num
;	
	movlw	.128			; pixels to clear
	movwf	Mc0,access
	clrf	Mc2,access	; 7-bit pseudornd register (X=X*17+7)
clear.dice
	sync150					; sync 150 Hz
	movf	Mc2,w,access
	mullw	.17
	addlw	.7
	andlw	0x7f
	movwf	Mc2,access

	lfsr	FSR2,BittabR
	movf	Mc2,w,access
	andlw	.7
	addwf	FSR2L,f,access		; mask bit in [INDF2]
				
	rrncf	Mc2,w,access
	rrncf	WREG,f,access
	rrncf	WREG,f,access
	andlw	0x0f
	lfsr	FSR0,Buffer
	addwf	FSR0L,f,access

	movf	INDF2,w,access
	comf	WREG,f,access
	andwf	INDF0,f				; reset pixel
;	
	call	ior.bufer2.buffer	; IOR buffers
	decfsz	Mc0,f,access
	bra		clear.dice
							; final clear
	call	clrbuf2
	call	ior.bufer2.buffer	; final IOR buffers
;	
	movlw	.200
	movwf	Mc1,access
hold.dice
	sync150					; sync 150 Hz
							; test for shaking before waiting time elapsed
	call	get.accel		; *** read g
	call	div.clip		; adjust result
	movf	AccX,w,access
	addwf	AccY,w,access
	bc		not.ovf
	addwf	AccZ,w,access
	bc		not.ovf
	addlw	-.220			; dice switch threshold
	bc		not.ovf			; don't wait

	decfsz	Mc1,f,access
	bra		hold.dice
not.ovf
;												
;					noise						
;												
	movf	AccY,w,access
	addlw	-.50			; other angle limit for noise add
	bnc		nonoisex
	movf	AccX,w,access
	addlw	-.5				; angle limit for noise add
	bc		nonoisex

	call	rnd
	andlw	0x0f
	bnz		nonoisex
							; add noise x
	movlw	.10				; noise value
	addwf	AccX,f,access
	btg		AccX+1,0,access	; toggle angle
nonoisex
	movf	AccX,w,access
	addlw	-.50			; other angle limit for noise add
	bnc		nonoisey
	movf	AccY,w,access
	addlw	-.5				; angle limit for noise add
	bc		nonoisey

	call	rnd
	andlw	0x0f
	bnz		nonoisey
							; add noise y
	movlw	.10				; noise value
	addwf	AccY,f,access
	btg		AccY+1,0,access	; toggle angle
nonoisey
;	
;	sync150x8				; sync 150x8 Hz
	call	drawdots		; CLS and draw dots from [Dots] table with coordinates

	bcf		liqflag,0		; if set, then move X
	call	rnd
	andlw	0x0f
	subwf	AccX,w,access	; probability of moving across X axis
	ifc						; if C set, then move pixel accross X axis
	bsf		liqflag,0		; flag move X

	bcf		liqflag,1		; if set, then move Y
	call	rnd
	andlw	0x0f
	subwf	AccY,w,access	; probability of moving across Y axis
	ifc						; if C set, then move pixel accross X axis
	bsf		liqflag,1		; flag move Y
					; move random dot coordinates X/Y +/- if new location is free
findfree
	call	rnd
	andlw	b'00111111'
	addlw	-.44
	bc		findfree		; if >=44
	addlw	.44				; restore
	addwf	WREG,w,access	; x2
	addlw	low (Dots+Offset)	; Dots + 2 * rnd(44)
	movwf	FSR0L
	movlw	high (Dots+Offset)
	movwf	FSR0H

	btfss	liqflag,0		; if set, then move X
	bra		nomovex
	btfss	AccX+1,0
	call	moveXleft		; move X left
	btfsc	AccX+1,0
	call	moveXright		; move X right
nomovex
	movf	POSTINC0,f,access	; dummy
	btfss	liqflag,1		; if set, then move Y
	bra		nomovey
	btfss	AccY+1,0
	call	moveYback		; move Y back
	btfsc	AccY+1,0
	call	moveYfront		; move Y front
nomovey
	goto	liquid.farm

dottable	; X,Y coordinates for start position (when accelerometer pgm started)
			; Note: 16-bit hex is big endian representation, lo byte should be first
			; but here it doesn't look so
	dw		       0x0501,0x0601,0x0701,0x0801,0x0901,0x0a01
	dw		0x0402,0x0502,0x0602,0x0702,0x0802,0x0902,0x0a02,0x0b02
	dw		0x0403,0x0503,0x0603,0x0703,0x0803,0x0903,0x0a03,0x0b03
	dw		0x0404,0x0504,0x0604,0x0704,0x0804,0x0904,0x0a04,0x0b04
	dw		0x0405,0x0505,0x0605,0x0705,0x0805,0x0905,0x0a05,0x0b05
	dw		       0x0506,0x0606,0x0706,0x0806,0x0906,0x0a06

;	movff	AccX+0,Buffer3+0
;	movff	AccX+1,Buffer3+1
;	movff	AccY+0,Buffer3+2
;	movff	AccY+1,Buffer3+3
;	movff	AccZ+0,Buffer3+4
;	movff	AccZ+1,Buffer3+5
;																			
;					 S U B S												
;																			

;							
moveXleft
	movf	INDF0,w
	ifz
	return					; left border
	decf	POSTINC0,w
	movwf	Xtest,access
	movff	POSTDEC0,Ytest
	call	is.it.free		; is there free pixel on new location?
	ifnc
	decf	INDF0,f			; move left
	return
moveXright
	movf	INDF0,w
	xorlw	7
	ifz
	return					; right border
	incf	POSTINC0,w
	movwf	Xtest,access
	movff	POSTDEC0,Ytest
	call	is.it.free		; is there free pixel on new location?
	ifnc
	incf	INDF0,f			; move right
	return
moveYback
	movf	INDF0,w
	xorlw	0xf
	ifz
	return					; lower border
	incf	POSTDEC0,w
	movwf	Ytest,access
	movff	POSTINC0,Xtest
	call	is.it.free		; is there free pixel on new location?
	ifnc
	incf	INDF0,f			; move down
	return
moveYfront
	movf	INDF0,w
	ifz
	return					; upper border
	decf	POSTDEC0,w
	movwf	Ytest,access
	movff	POSTINC0,Xtest
	call	is.it.free		; is there free pixel on new location?
	ifnc
	decf	INDF0,f			; move up
	return
;						
is.it.free				; test if there is a dot on Xtest, Ytest, set C if occupied
	lfsr	FSR1,Dots
	movlw	.44				; dots to test
	movwf	Block,access

test44dots
	movf	POSTINC1,w
	cpfseq	Xtest,access
	bra		different
	movf	INDF1,w
	cpfseq	Ytest,access
	bra		different
	scf						; match found, set C
	return
different

	movf	POSTINC1,f		; dummy
	decfsz	Block,f,access
	bra		test44dots

	rcf						; match NOT found, reset C
	return

;					
createtable			;		copy table from rom to ram
	movlw	low (dottable+Offset)
	movwf	TBLPTRL,access
	movlw	high (dottable+Offset)
	movwf	TBLPTRH,access
	lfsr	FSR0,Dots
	movlw	.44*2					; number of dots*2
	movwf	Inner,access
go44x2
	tblrd*+
	movff	TABLAT,POSTINC0
	decfsz	Inner,f,access
	bra		go44x2
	return
;					

drawdots			;		clear screen and draw all dots
	lfsr	FSR1,Buffer3
	call	clrx			; clrbuf3

	lfsr	FSR0,Dots
	movlw	.44					; number of dots
	movwf	Inner,access
godraw44
	lfsr	FSR2,BittabR
	lfsr	FSR1,Buffer3
	movf	POSTINC0,w			; get X
	addwf	FSR2L,f,access		; bitmask is now in [INDF2]
	movf	POSTINC0,w			; get Y
	addlw	low Buffer3
	movwf	FSR1L				; FSR1 points to dot in ram
	movf	INDF2,w,access
	iorwf	INDF1,f				; put dot in place

	decfsz	Inner,f,access
	bra		godraw44

	lfsr	FSR1,Buffer3		; Buffer3 ---> Buffer
	lfsr	FSR2,Buffer
	call	move.fsr1.fsr2

	return
;					
div.clip			; scale down accelerometer output, ABS and clip it to 255
;	divide			
	call	div2
	call	div2
	call	div2
	call	div2
	call	div2

	movf	WhoAmI,w,access
	xorlw	0xE5			; ADXL345 ID
	bnz		nomult8
							; multiply by 8, as ADXL345 has lower sensitivity
	rcf
	rlcf	AccX+0,f,access
	rcf
	rlcf	AccX+0,f,access
	rcf
	rlcf	AccY+0,f,access
	rcf
	rlcf	AccY+0,f,access
	rcf
	rlcf	AccZ+0,f,access
nomult8
;						ABS X, clip to 255			
	btfss	AccX+1,2,access		; sign bit
	bra		pozitx				; if sign +

	negf	AccX+0,access		; if sign -
	movf	AccX+1,w,access
	andlw	3					; isolate bits 8 and 9
	xorlw	3					; test if it was =XXXXXX11 (8 bit range)
	ifnz
	setf	AccX+0,access		; if >255 then =255
	movlw	1
	movwf	AccX+1,access		; new sign -
	bra		okx
pozitx
	movf	AccX+1,w,access		; dummy, test if =0
	andlw	3
	ifnz
	setf	AccX+0,access		; if >255 then =255
	clrf	AccX+1,access
okx
;						ABS Y, clip to 255			
	btfss	AccY+1,2,access		; sign bit
	bra		pozity				; if sign +

	negf	AccY+0,access		; if sign -
	movf	AccY+1,w,access
	andlw	3					; isolate bits 8 and 9
	xorlw	3					; test if it was =XXXXXX11 (8 bit range)
	ifnz
	setf	AccY+0,access		; if >255 then =255
	movlw	1
	movwf	AccY+1,access		; new sign -
	bra		oky
pozity
	movf	AccY+1,w,access		; dummy, test if =0
	andlw	3
	ifnz
	setf	AccY+0,access		; if >255 then =255
	clrf	AccY+1,access
oky
;						ABS Z, clip to 255			
	btfss	AccZ+1,2,access		; sign bit
	bra		pozitz				; if sign +

	negf	AccZ+0,access		; if sign -
	movf	AccZ+1,w,access
	andlw	3					; isolate bits 8 and 9
	xorlw	3					; test if it was =XXXXXX11 (8 bit range)
	ifnz
	setf	AccZ+0,access		; if >255 then =255
	movlw	1
	movwf	AccZ+1,access		; new sign -
	bra		okz
pozitz
	movf	AccZ+1,w,access		; dummy, test if =0
	andlw	3
	ifnz
	setf	AccZ+0,access		; if >255 then =255
	clrf	AccZ+1,access
okz
	return

;								
div2							; divide /2
	rcf
	rrcf	AccX+1,f,access
	rrcf	AccX+0,f,access
	rcf
	rrcf	AccY+1,f,access
	rrcf	AccY+0,f,access
	rcf
	rrcf	AccZ+1,f,access
	rrcf	AccZ+0,f,access
	return
;								
rndarea
	lfsr	FSR0,Buffer3
	movlw	.16
	movwf	Inner,access
rnda.16
	call	rnd
	movwf	Tempw,access
	call	rnd
	andwf	Tempw,w,access
	movwf	POSTINC0

	decfsz	Inner,f,access
	bra		rnda.16
	return

;																			

sladdr	equ		b'11010000'
sladdr2	equ		0xA6			; 0x3A if SDO/Alt Address is high

writreg		macro regdata
	movlw	regdata
	call	tx.i2c.ack
			endm
;																			
reset.i2c:
	bcf		accsda,ACCESS	; default output lo (pseudo open drain)
	bcf		accscl,ACCESS	; default output lo (pseudo open drain)
	call	t38
	call	i2c.stop		; stop bit
	bcf		accscl.ts,ACCESS	; MCU clock pin low
	call	t38
	call	i2c.stop		; stop bit
	call	t38
	return

;				A C C E L E R O M E T E R 									

snif.accel				; auto detect accelerometer type
	bcf		accsda,ACCESS	; default output lo (pseudo open drain)
	bcf		accscl,ACCESS	; default output lo (pseudo open drain)

;				test if it is ADXL345						
	call	i2c.start	; start
	writreg	sladdr2		; request for slave #0xA6
	writreg	00h			; addr 0x00 (Device ID)
	call	i2c.start	; restart
	writreg	sladdr2+1	; request for slave #0xA6+1 (+1 because this is RD) 
	call	get.data
	call	i2c.nack	; put NACK
	call	i2c.stop	; stop
	xorlw	0xE5
	movlw	0xE5
	ifz
	movwf	WhoAmI,access	; if =0xE5, then store
;				test if it is GY521							
	call	i2c.start	; start
	writreg	sladdr		; request for slave #0xD0
	writreg	0x75		; addr 0x75 (Device ID)
	call	i2c.start	; restart
	writreg	sladdr+1	; request for slave #0xD0+1 (+1 because this is RD) 
	call	get.data
	call	i2c.nack	; put NACK
	call	i2c.stop	; stop
	xorlw	0x68
	movlw	0x68
	ifz
	movwf	WhoAmI,access	; if =0xD0, then store
	return

;																			

get.accel:					; XYZ data ---> AccX,AccY,AccZ
	bcf		accsda,ACCESS	; default output lo (pseudo open drain)
	bcf		accscl,ACCESS	; default output lo (pseudo open drain)

	movf	WhoAmI,w,access
	xorlw	0x68
	ifz
	goto	gy521		; if =0xD0, then it's GY521

	movf	WhoAmI,w,access
	xorlw	0xE5
	bz		adxl345		; if =0xE5, then it's ADXL345
	return				; otherwise no accelerometer

;																					
;																					

adxl345
	call	i2c.start	; start
	writreg	sladdr2		; request for slave #0xA6
	writreg	2ch			; addr 0x2c (Data Rate and Power Management)
	writreg	b'00001101'	; data for Data Rate
	writreg	b'00001000'	; data for Power Management
;	call	i2c.stop	; stop
;	
	call	i2c.start	; start
	writreg	sladdr2		; request for slave #0xA6
	writreg	31h			; addr 0x31 (Data Format)
	writreg	b'00001011'	; data for Data Format
;	call	i2c.stop	; stop
;	
	call	i2c.start	; start
	writreg	sladdr2		; request for slave #0xA6
	writreg	32h			; addr 0x32 (data accel X low byte)

	call	i2c.start	; restart
	writreg	sladdr2+1	; request for slave #0xA6+1 (+1 because this is RD) 
	call	get.data
	movwf	AccY,ACCESS
	call	i2c.ack			; put ACK
	call	get.data
	movwf	AccY+1,ACCESS
	call	i2c.ack			; put ACK

	call	get.data
	xorlw	0xff
	movwf	AccX,ACCESS
	call	i2c.ack			; put ACK
	call	get.data
	xorlw	0xff
	movwf	AccX+1,ACCESS
	call	i2c.ack			; put ACK

	call	get.data
	movwf	AccZ,ACCESS
	call	i2c.ack			; put ACK
	call	get.data
	movwf	AccZ+1,ACCESS
	call	i2c.nack		; put NACK
	call	i2c.stop		; stop

	return

;																					
;																					

gy521
	call	i2c.start	; start
	writreg	sladdr		; request for slave #0xD0
	writreg	0x6b		; Power Management
	writreg	b'00001001'	; Reset, No sleep, Temp Disabled, Internal 8MHz osc
	writreg	b'00000111'	; Gyroscope sleep
	call	i2c.stop		; stop
;	
	call	i2c.start	; start
	writreg	sladdr		; request for slave #0xD0
	writreg	0x1a		; addr 0x1a (Config)
	writreg	b'00000110'	; 1a data CONFIG (low pass filter)
	writreg	b'00001000'	; 1b data GYRO_CONFIG (sensitivity) 500 degrees/s
	writreg	b'00011000'	; 1c data ACCEL_CONFIG (sensitivity) 16 g
	call	i2c.stop		; stop
;	
	call	i2c.start	; start
	writreg	sladdr		; request for slave #0xD0
	writreg	0x3b		; 0x3b start of acc data X high
	call	i2c.start	; restart
	writreg	sladdr+1	; request for slave #0xD0 + READ bit

	call	get.data
	movwf	AccX+1,ACCESS
	call	i2c.ack			; put ACK
	call	get.data
	movwf	AccX,ACCESS
	call	i2c.ack			; put ACK

	call	get.data
	movwf	AccY+1,ACCESS
	call	i2c.ack			; put ACK
	call	get.data
	movwf	AccY,ACCESS
	call	i2c.ack			; put ACK

	call	get.data
	movwf	AccZ+1,ACCESS
	call	i2c.ack			; put ACK
	call	get.data
	movwf	AccZ,ACCESS
	call	i2c.nack		; put NACK

	call	i2c.stop		; stop
	return

;																				
;				I 2 C   S U B S													
;																				

datahi			macro
	bsf		accsda.ts,ACCESS	; MCU data pin hi imp
	call	t38
					endm
datalo			macro
	bcf		accsda.ts,ACCESS	; MCU data pin low
	call	t38
					endm
clockhi			macro
	bsf		accscl.ts,ACCESS	; MCU clock pin hi imp
	call	t38
					endm
clocklo			macro
	bcf		accscl.ts,ACCESS	; MCU clock pin low
	call	t38
					endm

;													
i2c.start
	datahi
	clockhi
	datalo
	clocklo
	return
;				
i2c.stop
	datalo
	clockhi
	datahi
	return
;				
i2c.tx.w7
	rlcf	WREG,f,ACCESS
	bc		i2c.nack
i2c.ack				; ack (bit=0 transmit)
	datalo
	clockhi
	clocklo
	return
i2c.nack			; nack (bit=1 transmit)
	datahi
	clockhi
	clocklo
	return
;				
tx.i2c.ack			; transmit w7w6w5w4w3w2w1w0, then get ack
	call	i2c.tx.w7
	call	i2c.tx.w7
	call	i2c.tx.w7
	call	i2c.tx.w7
	call	i2c.tx.w7
	call	i2c.tx.w7
	call	i2c.tx.w7
	call	i2c.tx.w7
get.ack				; read 1 bit ---> carry
	datahi
	clockhi
	rcf							; acknowledged (bit 0)
	btfsc	accsda.port,ACCESS	; test data pin
	scf							; not acknowledged (bit 1)
	clocklo
	return
;				
get.data			; read 8 bits ---> w7w6w5w4w3w2w1w0
	movlw	8
	movwf	Outer,ACCESS	; loop bit count
loopab2					; \
	call	get.ack			; get bit ---> carry
	rlcf	WREG,f,ACCESS

	decfsz	Outer,f,ACCESS
	bra		loopab2		; /
	return
;				
t38:				; 30t incl call/ret, 2.5 us
	clrf	Inner,ACCESS
	bsf		Inner,3,ACCESS	; =8
got30
	decfsz	Inner,f,ACCESS
	bra		got30
	return

;									

ior.bufer2.buffer3		; buffer2 or buffer3 ---> buffer3 (optimized for speed)
	iorbuf3	.0
	iorbuf3	.1
	iorbuf3	.2
	iorbuf3	.3
	iorbuf3	.4
	iorbuf3	.5
	iorbuf3	.6
	iorbuf3	.7
	iorbuf3	.8
	iorbuf3	.9
	iorbuf3	.10
	iorbuf3	.11
	iorbuf3	.12
	iorbuf3	.13
	iorbuf3	.14
	iorbuf3	.15
	return

