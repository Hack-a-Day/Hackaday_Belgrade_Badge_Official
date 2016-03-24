;*******************************************************************************
;*******************************************************************************
;*******************************************************************************
;						D I S P L A Y	 										
;*******************************************************************************
;*******************************************************************************
;*******************************************************************************

; [0]...[65535] Serial Number header
; <h> horizontal
; <H> horizontal broad
; <v> vertical
; <V> vertical broad
; <l> lateral
; <i> immidiate short
; <I> Immidiate long
; <1>...<9> pause (*250ms)

display
	clrf	EECON1,access		; Disable write to EEPROM
	bsf		Flag,5,access		; bit 5 set = Only two steps for key 0, without pause
	clrf	KeyEdge,access
	bsf		RXFlag,0,access		; enable RX

	movlw	.14
	movwf	DispSpeed,access	; def display speed
	clrf	DispPtr,access
	call	clrbuf
	call	clrbuf2

;					GO DISPLAY

	goto	start.message.h1

;					HORIZONTAL 1 MODE													
start.message.h1
	bcf		Flag,6,access	; bit 6 set = Display message received
	lfsr	FSR0,Disp.text	; initialize read ptr

next.char.h1				; \\\\\\\\\\\\\\\\
	bcf		FlagDemo,1		; bit 1 set = next char immediate
	movlw	.6
	movwf	ColCount,access

	call	clrbuf2
gnc.h1
	btfsc	FlagDemo,1,access	; bit 1 set = next char immediate
	goto	gnc.h2.spec

	movf	POSTINC0,w		; get next char
	bz		start.message.h1	; if end of message detected

	call	spec.case		; special case: possible <x>
	bnc		nospec.h1
	dcfsnz	WREG,f,access
	goto	next.char.h1
	dcfsnz	WREG,f,access
	goto	next.char.h2
	dcfsnz	WREG,f,access
	goto	next.char.v1
	dcfsnz	WREG,f,access
	goto	next.char.v2
	dcfsnz	WREG,f,access
	goto	next.char.l
nospec.h1
	addlw	0
	bz		gnc.h1			; this 0 is not terminator, but pause or immediate was executed
	addlw	-0x20			; ASCII " " offset
	mullw	.10				; bytes/char
	movf	PRODL,w,access
	addlw	low	(charg1)
	movwf	TBLPTRL			; low charg ptr
	movf	PRODH,w,access
	ifc
	addlw	1				; addc
	addlw	high (charg1)
	movwf	TBLPTRH			; hi charg ptr

	lfsr	FSR1,Buffer2+4	; start of next char in Buffer2 (defines vert text pos)
	movlw	.9
go9txt.h1
	TBLRD*+
	movff	TABLAT,POSTINC1	; transfer char bitmap from char gen to buffer2
	decfsz	WREG,f,access
	bra		go9txt.h1

next.column.h1			; \\\\\\\\
	test.new
	movf	DispSpeed,w,access	; this determines moving message speed
	addlw	3				; additional slow down in horizontal mode
wait.disp.h1
	call	sync150cond
	decfsz	WREG,f,access
	bra		wait.disp.h1		; wait for period
;	
	lfsr	FSR1,Buffer
	lfsr	FSR2,Buffer2

	movlw	.16
moveleft16.h1
	rcf
	rlcf	POSTINC2,f
	rlcf	POSTINC1,f		; shift left buffer2 to buffer
	decfsz	WREG,f,access
	bra		moveleft16.h1

	btfsc	Flag,6,access		; bit 6 set = Display message received
	bra		start.message.h1

	decfsz	ColCount,f,access
	bra		next.column.h1; ////////
							; one ASCII char passed, go next char
	call	comm.i			; exec short or long pause if it was coomand <i> or <I>
	bra		next.char.h1	; ////////////////

;					HORIZONTAL 2 MODE													
gnc.h2.spec
	movlw	.8
	movwf	ColCount,access
	bra		gnc.h2

start.message.h2
	lfsr	FSR0,Disp.text	; initialize read ptr

next.char.h2				; \\\\\\\\\\\\\\\\
	bcf		FlagDemo,1		; bit 1 set = next char immediate
	movlw	.8
	movwf	ColCount,access

	call	clrbuf2
gnc.h2
	movf	POSTINC0,w		; get next char
	bz		start.message.h2	; if end of message detected

	call	spec.case		; special case: possible <x>
	bnc		nospec.h2
	dcfsnz	WREG,f,access
	goto	next.char.h1
	dcfsnz	WREG,f,access
	goto	next.char.h2
	dcfsnz	WREG,f,access
	goto	next.char.v1
	dcfsnz	WREG,f,access
	goto	next.char.v2
	dcfsnz	WREG,f,access
	goto	next.char.l
nospec.h2
	addlw	0
	bz		gnc.h2			; this 0 is not terminator, but pause or immediate was executed

	addlw	-0x20			; ASCII " " offset
	mullw	.14				; bytes/char
	movf	PRODL,w,access
	addlw	low	(charg3)
	movwf	TBLPTRL			; low charg ptr
	movf	PRODH,w,access
	ifc
	addlw	1				; addc
	addlw	high (charg3)
	movwf	TBLPTRH			; hi charg ptr

	lfsr	FSR1,Buffer2+1	; start of next char in Buffer2 (defines vert text pos)
	movlw	.14
go14txt.h2
	TBLRD*+
	movff	TABLAT,POSTINC1	; transfer char bitmap from char gen to buffer2
	decfsz	WREG,f,access
	bra		go14txt.h2

	call	shlbuf			; wait DispSpeed+3 cycles and shift left buffer
	call	shlbuf

next.column.h2			; \\\\\\\\
	movf	DispSpeed,w,access	; this determines moving message speed
	addlw	3				; additional slow down in horizontal mode
wait.disp.h2
	call	sync150cond
	decfsz	WREG,f,access
	bra		wait.disp.h2		; wait for period
;	
	lfsr	FSR1,Buffer
	lfsr	FSR2,Buffer2

	movlw	.16
moveleft16.h2
	rcf
	rlcf	POSTINC2,f
	rlcf	POSTINC1,f		; shift left buffer2 to buffer
	decfsz	WREG,f,access
	bra		moveleft16.h2

	btfsc	Flag,6,access		; bit 6 set = Display message received
	goto	start.message.h1

	decfsz	ColCount,f,access
	bra		next.column.h2; ////////
							; one ASCII char passed, go next char
	call	comm.i			; exec short or long pause if it was coomand <i> or <I>
	bra		next.char.h2	; ////////////////

;					VERTICAL 1 MODE														

start.message.v1
	lfsr	FSR0,Disp.text	; initialize read ptr

next.char.v1				; \\\\\\\\\\\\\\\\
	bcf		FlagDemo,1		; bit 1 set = next char immediate
	movlw	.11
	movwf	ColCount,access

	call	clrbuf2
gnc.v1
	btfsc	FlagDemo,1,access	; bit 1 set = next char immediate
	goto	gnc.h2.spec

	movf	POSTINC0,w		; get next char
	bz		start.message.v1	; if end of message detected

	call	spec.case		; special case: possible <x>
	bnc		nospec.v1
	dcfsnz	WREG,f,access
	goto	next.char.h1
	dcfsnz	WREG,f,access
	goto	next.char.h2
	dcfsnz	WREG,f,access
	goto	next.char.v1
	dcfsnz	WREG,f,access
	goto	next.char.v2
	dcfsnz	WREG,f,access
	goto	next.char.l
nospec.v1
	addlw	0
	bz		gnc.v1			; this 0 is not terminator, but pause or immediate was executed
	addlw	-0x20			; ASCII " " offset
	mullw	.10				; bytes/char
	movf	PRODL,w,access
	addlw	low	(charg1)
	movwf	TBLPTRL			; low charg ptr
	movf	PRODH,w,access
	ifc
	addlw	1				; addc
	addlw	high (charg1)
	movwf	TBLPTRH			; hi charg ptr

	lfsr	FSR1,Buffer2+.1	; start of next char in Buffer2
	movlw	.9
go9txt.v1
	TBLRD*+
	movff	TABLAT,POSTINC1	; transfer char bitmap from char gen to buffer2
	decfsz	WREG,f,access
	bra		go9txt.v1
;	
	movf	Buffer2+.8,w,banked
	ifz
	decf	ColCount,f,access	; if lower row blank, decrement shift count
	movf	Buffer2+.8,w,banked
	ifz
	decf	ColCount,f,access	; if lower row blank, decrement shift count
	movf	Buffer2+.1,w,banked
	ifz
	call	move2up			; if upper row blank, shift buffer2 up
	movf	Buffer2+.1,w,banked
	ifz
	call	move2up			; if upper row blank, shift buffer2 up

;	
next.row.v1				; \\\\\\\\
	movf	DispSpeed,w,access	; this determines moving message speed
wait.disp.v1
	call	sync150cond
	decfsz	WREG,f,access
	bra		wait.disp.v1		; wait for period
;	
	lfsr	FSR1,Buffer+.0
	lfsr	FSR2,Buffer+.1
	movlw	.15
movedn16.v11
	movff	POSTINC2,POSTINC1	; shift up buffer
	decfsz	WREG,f,access
	bra		movedn16.v11

	movff	Buffer2,Buffer+.15	; transfer from Buffer2 (top) to Buffer (bottom)

	lfsr	FSR1,Buffer2+.0
	lfsr	FSR2,Buffer2+.1
	movlw	.15
movedn16.v12
	movff	POSTINC2,POSTINC1	; shift up buffer2
	decfsz	WREG,f,access
	bra		movedn16.v12
;	
	btfsc	Flag,6,access		; bit 6 set = Display message received
	goto	start.message.h1

	decfsz	ColCount,f,access
	bra		next.row.v1	; ////////
							; one ASCII char passed, go next char
	call	comm.i			; exec short or long pause if it was coomand <i> or <I>
	bra		next.char.v1	; ////////////////


;					VERTICAL 2 MODE														

start.message.v2
	lfsr	FSR0,Disp.text	; initialize read ptr

next.char.v2				; \\\\\\\\\\\\\\\\
	bcf		FlagDemo,1		; bit 1 set = next char immediate
	movlw	.14
	movwf	ColCount,access

	call	clrbuf2
gnc.v2
	btfsc	FlagDemo,1,access	; bit 1 set = next char immediate
	goto	gnc.h2.spec

	movf	POSTINC0,w		; get next char
	bz		start.message.v2	; if end of message detected

	call	spec.case		; special case: possible <x>
	bnc		nospec.v2
	dcfsnz	WREG,f,access
	goto	next.char.h1
	dcfsnz	WREG,f,access
	goto	next.char.h2
	dcfsnz	WREG,f,access
	goto	next.char.v1
	dcfsnz	WREG,f,access
	goto	next.char.v2
	dcfsnz	WREG,f,access
	goto	next.char.l
nospec.v2
	addlw	0
	bz		gnc.v2			; this 0 is not terminator, but pause or immediate was executed
	addlw	-0x20			; ASCII " " offset
	mullw	.14				; bytes/char
	movf	PRODL,w,access
	addlw	low	(charg3)
	movwf	TBLPTRL			; low charg ptr
	movf	PRODH,w,access
	ifc
	addlw	1				; addc
	addlw	high (charg3)
	movwf	TBLPTRH			; hi charg ptr

	lfsr	FSR1,Buffer2+.1	; start of next char in Buffer2
	movlw	.14
go14txt.v2
	TBLRD*+
	movff	TABLAT,POSTINC1	; transfer char bitmap from char gen to buffer2
	decfsz	WREG,f,access
	bra		go14txt.v2
;	
	movf	Buffer2+.12,w,banked
	ifz
	decf	ColCount,f,access	; if lower row blank, decrement shift count
	movf	Buffer2+.12,w,banked
	ifz
	decf	ColCount,f,access	; if lower row blank, decrement shift count
	movf	Buffer2+.12,w,banked
	ifz
	decf	ColCount,f,access	; if lower row blank, decrement shift count
	movf	Buffer2+.12,w,banked
	ifz
	decf	ColCount,f,access	; if lower row blank, decrement shift count
	movf	Buffer2+.1,w,banked
	ifz
	call	move2up			; if upper row blank, shift buffer2 up
	movf	Buffer2+.1,w,banked
	ifz
	call	move2up			; if upper row blank, shift buffer2 up
	movf	Buffer2+.1,w,banked
	ifz
	call	move2up			; if upper row blank, shift buffer2 up
	movf	Buffer2+.1,w,banked
	ifz
	call	move2up			; if upper row blank, shift buffer2 up

;	
next.row.v2				; \\\\\\\\
	movf	DispSpeed,w,access	; this determines moving message speed
	addlw	-.2
	ifnc
	movlw	1
	ifz
	movlw	1
	
wait.disp.v2
	call	sync150cond
	decfsz	WREG,f,access
	bra		wait.disp.v2		; wait for period
;	
	lfsr	FSR1,Buffer+.0
	lfsr	FSR2,Buffer+.1
	movlw	.15
movedn16.v21
	movff	POSTINC2,POSTINC1	; shift up buffer
	decfsz	WREG,f,access
	bra		movedn16.v21

	movff	Buffer2,Buffer+.15	; transfer from Buffer2 (top) to Buffer (bottom)

	lfsr	FSR1,Buffer2+.0
	lfsr	FSR2,Buffer2+.1
	movlw	.15
movedn16.v22
	movff	POSTINC2,POSTINC1	; shift up buffer2
	decfsz	WREG,f,access
	bra		movedn16.v22

	btfsc	Flag,6,access		; bit 6 set = Display message received
	goto	start.message.h1
;	
	decfsz	ColCount,f,access
	bra		next.row.v2	; ////////
							; one ASCII char passed, go next char
	call	comm.i			; exec short or long pause if it was coomand <i> or <I>
	bra		next.char.v2	; ////////////////

;					LATERAL MODE														

start.message.l
	lfsr	FSR0,Disp.text	; initialize read ptr

next.char.l				; \\\\\\\\\\\\\\\\
	bcf		FlagDemo,1		; bit 1 set = next char immediate
	movlw	.6
	movwf	ColCount,access

	call	clrbuf2
gnc.l
	btfsc	FlagDemo,1,access	; bit 1 set = next char immediate
	goto	gnc.h2.spec

	movf	POSTINC0,w		; get next char
	bz		start.message.l	; if end of message detected

	call	spec.case		; special case: possible <x>
	bnc		nospec.l
	dcfsnz	WREG,f,access
	goto	next.char.h1
	dcfsnz	WREG,f,access
	goto	next.char.h2
	dcfsnz	WREG,f,access
	goto	next.char.v1
	dcfsnz	WREG,f,access
	goto	next.char.v2
	dcfsnz	WREG,f,access
	goto	next.char.l
nospec.l
	addlw	0
	bz		gnc.l			; this 0 is not terminator, but pause or immediate was executed
	addlw	-0x20			; ASCII " " offset
	mullw	.6				; bytes/char
	movf	PRODL,w,access
	addlw	low	(charg2)
	movwf	TBLPTRL			; low charg ptr
	movf	PRODH,w,access
	ifc
	addlw	1				; addc
	addlw	high (charg2)
	movwf	TBLPTRH			; hi charg ptr

	lfsr	FSR1,Buffer2+.11	; start of next char in Buffer2
	movlw	.5
go5txt.l
	TBLRD*+
	movff	TABLAT,POSTINC1	; transfer char bitmap from char gen to buffer2
	decfsz	WREG,f,access
	bra		go5txt.l

next.row.l				; \\\\\\\\
	movf	DispSpeed,w,access	; this determines moving message speed
wait.disp.l
	call	sync150cond
	decfsz	WREG,f,access
	bra		wait.disp.l		; wait for period
;	
	lfsr	FSR2,Buffer+.14
	lfsr	FSR1,Buffer+.15
	movlw	.15
movedn16.l1
	movff	POSTDEC2,POSTDEC1	; shift down buffer
	decfsz	WREG,f,access
	bra		movedn16.l1

	movff	Buffer2+.15,Buffer	; transfer from Buffer2 (bottom) to Buffer (top)

	lfsr	FSR2,Buffer2+.14
	lfsr	FSR1,Buffer2+.15
	movlw	.6
movedn16.l2
	movff	POSTDEC2,POSTDEC1	; shift down buffer2
	decfsz	WREG,f,access
	bra		movedn16.l2

	btfsc	Flag,6,access		; bit 6 set = Display message received
	goto	start.message.h1
;	
	decfsz	ColCount,f,access
	bra		next.row.l	; ////////
							; one ASCII char passed, go next char
	call	comm.i			; exec short or long pause if it was coomand <i> or <I>
	bra		next.char.l	; ////////////////

;																				
;																				

spec.case	; if w='<', test if it is a spec message <h>,<H>,<v>,<V>,<l>,<i>,<I>,<1...9>
			; if it is, return with w=1,2,3,4,5 and update pointer FSR0 and set C
			; if <i> then place next char immediatelly and pause 500ms
			; if <I> then place next char immediatelly and pause 2s
			; if <1>...<9> then pause
	xorlw	'<'
	bnz		no.spec.sign
	movf	POSTINC0,f	; +1 dummy
	movf	POSTINC0,w	; +2 get ">"
	xorlw	'>'
	bnz		spec.not.found	
	movf	POSTDEC0,f	; +1 dummy
	movf	POSTDEC0,f	;  0 dummy
	movf	POSTINC0,w	; +1 get "h"|"H"|"v"|"V"|"l"|"i"|"I"|"1"..."9"
	movf	POSTINC0,f	; +2 dummy

	scf					; set C, this may be special case "h"|"H"|"v"|"V"|"l"|"i"|"I"|"1"..."9"
	xorlw	'h'
	bz		case.h1		; horizontal
	xorlw	'h'			; restore
	xorlw	'H'
	bz		case.h2		; horizontal large
	xorlw	'H'			; restore
	xorlw	'v'
	bz		case.v1		; vertical
	xorlw	'v'			; restore
	xorlw	'V'
	bz		case.v2		; vertical large
	xorlw	'V'			; restore
	xorlw	'l'
	bz		case.l		; lateral
	xorlw	'l'			; restore
	xorlw	'i'
	bz		case.i1		; immediate short
	xorlw	'i'			; restore
	xorlw	'I'
	bz		case.i2		; immediate long
	xorlw	'I'			; restore

	addlw	-'1'
	bnc		nonum.case	; not pause (if w was <'1')
	addlw	-.9	
	bc		nonum2.case	; not pause (if w was >'9')
	addlw	.10			; restore w=1...9
						; here the command was <1>...<9>, w=1...9
	movwf	Outer,access
go.pause
	movlw	.37
go250ms
	sync150
	decfsz	WREG,f,access
	bra		go250ms
	decfsz	Outer,f,access
	bra		go.pause

	movlw	0			; W=0 is flag that this was pause, pointer is on next byte, and...
						; ...it has to be tested again, as it may be the new command
	rcf					; reset C
	return
;	
case.i2					; just set flags, main routine will do the job
	bsf		FlagDemo,0	; bit 0 set = long pause (667 ms)
	bra		case.ix
case.i1					; just set flags, main routine will do the job
	bcf		FlagDemo,0	; bit 0 reset = short pause (333 ms)
case.ix
	bsf		FlagDemo,1	; bit 1 set = next char immediate
	movlw	0			; W=0 is flag that this was immediate cmd, pointer is on next byte...
						; ...and it has to be tested again, as it may be the new command
	rcf					; reset C
	return
;	
nonum2.case
	addlw	.10			; restore
nonum.case
	addlw	'1'			; restore
	bra		spec.not.found	; if char was not "h"|"H"|"v"|"V"|"l"|"i"|"I"|"1"..."9"
;	
case.h1
	movlw	.1
	return
case.h2
	movlw	.2
	return
case.v1
	movlw	.3
	return
case.v2
	movlw	.4
	return
case.l
	movlw	.5
	return
;	
spec.not.found
	movf	POSTDEC0,f	; +1 dummy
	movf	POSTDEC0,f	;  0 dummy
	movlw	'<'			; get ">" again
	rcf					; reset C
	return
;	
no.spec.sign
	xorlw	'<'			; restore w
	rcf					; reset C
	return

;																				
comm.i			; exec short or long pause if it was coomand <i> or <I>
	btfss	FlagDemo,1		; bit 1 set = cmd I
	return
	movlw	.30				; short pause
	btfsc	FlagDemo,0		; bit 0 set = long pause
	movlw	.60				; long pause
go.i.pause
	sync150
	decfsz	WREG,f,access
	bra		go.i.pause
	return

;																				

sync150cond
	btfsc	FlagDemo,1		; bit 1 set = cmd I
	return
	sync150
	return
;																				

shlbuf						; wait DispSpeed+3 cycles and shift left buffer
	movf	DispSpeed,w,access	; this determines moving message speed
	addlw	3				; additional slow down in horizontal mode
wait.disp.shlb
	call	sync150cond
	decfsz	WREG,f,access
	bra		wait.disp.shlb		; wait for period

	lfsr	FSR1,Buffer
	movlw	.16
shl16
	rcf
	rlcf	POSTINC1,f
	decfsz	WREG,f,access
	bra		shl16
	return
