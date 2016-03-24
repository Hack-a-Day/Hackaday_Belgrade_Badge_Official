
Offset	equ	0x0000

#define	keyint		PORTB,0		; key "INT" in
#define	clk			LATB,1		; cathode driver CLOCK
#define	accscl		LATB,2		; AUX clock out latch
#define	accscl.port	PORTB,2		; AUX clock in port
#define	accscl.ts	TRISB,2		; AUX clock TRIS bit
#define	sdo			LATB,3		; cathode driver DATA
#define	accsda		LATB,4		; AUX data out latch
#define	accsda.port	PORTB,4		; AUX data in port
#define	accsda.ts	TRISB,4		; AUX data TRIS bit
#define	vdd			LATB,5		; Vdd ON/OFF switch
#define	keys		PORTB,6		; PGC / SW3...6 input
#define	usbv		PORTC,0		; USB V detect

;******************************************************************
;	system flags

ifz		macro
	btfsc	STATUS,Z,ACCESS
		endm
ifnz	macro
	btfss	STATUS,Z,ACCESS
		endm
ifc		macro
	btfsc	STATUS,C,ACCESS
		endm
ifnc	macro
	btfss	STATUS,C,ACCESS
		endm
rcf		macro
	bcf		STATUS,C,ACCESS
		endm
scf		macro
	bsf		STATUS,C,ACCESS
		endm

;******************************************************************

; macro sync150
sync150	macro
	btfss	Flag,2,ACCESS		; 150 Hz handshaking sync
	bra		$-2
	bcf		Flag,2,ACCESS		; reset sync flag
		endm

; macro sync150x8
sync150x8	macro
	btfss	Flag,1,ACCESS		; 150x8 Hz handshaking sync
	bra		$-2
	bcf		Flag,1,ACCESS		; reset sync flag
		endm

;******************************************************************

onebit	macro	bitpos
	bcf		sdo,ACCESS
	btfsc	INDF0,bitpos
	bsf		sdo,ACCESS		; Transfer "BitMask" state in POSTDEC0 to "sdo" pin
	bcf		clk,ACCESS		; ^^^\___	generate "clk" pulse
	bsf		clk,ACCESS		; ___/^^^	shift out one bit
		endm

;******************************************************************

; macro iorbuf
iorbuf	macro	bufaddress
	movf	Buffer2+bufaddress,w,BANKED
	iorwf	Buffer+bufaddress,f,BANKED
		endm

;******************************************************************

; macro iorbuf3
iorbuf3	macro	buf3address
	movf	Buffer2+buf3address,w,BANKED
	iorwf	Buffer3+buf3address,f,BANKED
		endm

;******************************************************************

; macro overlap
overlap	macro	overaddress
	movf	Buffer2+overaddress,w,BANKED
	andwf	Buffer+overaddress,w,BANKED
	bnz		ovl.found
		endm

;******************************************************************

; macro test.new
test.new	macro
	movf	RXpatience,f,access


		endm

;******************************************************************

serialx2	macro
	rcf
	rlcf	RXserial+0,f,access
	rlcf	RXserial+1,f,access
	bc		preset.pat			; number overflow, ignore it
		endm
;******************************************************************
