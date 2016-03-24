;*******************************************************************************
;																				
;				T  E  T  R  I  S												
;																				
;*******************************************************************************

tetris							; new game starts
	clrf	EECON1,access		; Disable write to EEPROM
	bcf		RXFlag,0,access	; disable RX
	bcf		Flag,5,access	; bit 5 set = Only two steps for key 0, without pause
	clrf	KeyEdge,access
	clrf	Score,access
	clrf	Score+1,access

	call	print.score		; Bcd+0,Bcd+1 ---> BufferPause
	banksel	Buffer				; all BANKED modes will address bank 7
	clrf	Speed.index,access
	movlw	.82
	movwf	Speed,access

	call	clrbuf				; clear buffer
	call	move.bufer.buffer2	; clear aux buffer
	clrf	Block.count,access

; new block appears																		
tetris.cont								; new block has to appear
	bcf		KeyEdge,3,access
	call	full.line.blink
	call	move.bufer.buffer2		; clear aux buffer

	incf	Block.count,f,access
	bcf		Block.count,3,access	; range 0...7
	movf	Block.count,f,access
	bnz		no.speed.inc
									; time to advance speed
	incf	Speed.index,f,access
	btfsc	Speed.index,5,access
	decf	Speed.index,f,access	; keep it in range 0...31

	movlw	high (speedtab)
	movwf	TBLPTRH
	movlw	low (speedtab)
	addwf	Speed.index,w,access
	movwf	TBLPTRL
	ifc
	incf	TBLPTRH,f				; readjust for page boundary
	tblrd*							; read speed period
	movff	TABLAT,Speed
no.speed.inc
	movlw	.3				; initial X pos of appearing block
	movwf	BlockX,access
	movlw	.0				; initial Y pos of appearing block
	movwf	BlockY,access
	movlw	.0				; initial angle of appearing block
	movwf	BlockA,access
newrnd
	call	rnd
	andlw	.7				; range 0...7
	addlw	-.1
	bnc		newrnd			; if it was 7, then repeat
							; w in range 0...6
	movwf	Block,access		; initial block type

; initial drawing																
	call	clrbuf				; clear buffer
	call	get.block			; get block <Block>, X pos <BlockX>, angle <Blocka> in <CurrentBlock:4>
	call	draw.block			; draws <CurrentBlock:4> in <BlockX>, <BlockY> Buffer bitmap
	call	test.overlap		; test if first appearing of new block overlapped
	bnc		not.end
							; game end mask
		call	ior.bufer2.buffer	; moves Buffer2 to Buffer

								; this is game end!
		movlw	.60
wait.60
		sync150					; first wait a little
		decfsz	WREG,f,access
		bra		wait.60
	
		lfsr	FSR0,Buffer
		movlw	.16
		movwf	Inner
mask.16
		clrf	POSTINC0		; mask down
		movlw	.7
inmask
		sync150
		decfsz	WREG,f,access
		bra		inmask

		decfsz	Inner,f,access
		bra		mask.16

		movlw	.22
wait.22
		sync150					; wait a little more...
		decfsz	WREG,f,access
		bra		wait.22

	call	print.score.buf		; Bcd+0,Bcd+1 ---> conv ---> Buffer

	bcf		KeyEdge,2,access
new.wait.key.2
	bsf		FlagDemo,2			; bit 2 set = showing final result
	clrf	KeyEdge,access
	bsf		Flag,5,access		; bit 5 set = Only two steps for key 0, without pause
	call	moving.message
								; press any key to exit pause or to start new game
	btfsc	KeyEdge,1,access
	bra		pressed.new.game
	btfsc	KeyEdge,2,access
	bra		pressed.new.game
	btfsc	KeyEdge,3,access
	bra		pressed.new.game
	btfss	KeyEdge,4,access
	bra		new.wait.key.2		; wait for key 2 press
pressed.new.game
	clrf	KeyEdge,access
	bra		tetris				; new game

not.end
	call	ior.bufer2.buffer	; both buffers to main buffer

;*******************************************************************************
;							T E T R I S	  (MAIN LOOP)							
;*******************************************************************************
tetris.farm
	sync150						; sync 150 Hz
	btfsc	Flag,0,access		; pause flag
	bra		tetris.farm			; pause loop

	movff	BlockX,Temp.BlockX	; will be needed if action was impossible
	movff	BlockA,Temp.BlockA	; will be needed if action was impossible
	bcf		FlagDemo,3				; Flag3 set = key action, Flag3 clr = no action or vertical action

; Vertical action (automatic)													

	incf	Uniform,f,access
	movf	Speed,w,access

	btfsc	Rotor3,0,access
	bra		notdown
	btfsc	KeyEdge,3,access
	movlw	.8					; fast drop speed (key DOWN pressed)
notdown
	subwf	Uniform,w,access	; Uniform-Speed, C set if Uniform >= Speed
	bnc		tetris.farmw			; loop and wait for scan
	bcf		FlagDemo,3,access		; Flag3 set = key action, Flag3 clr = no action or vertical action
	clrf	Uniform,access

	incf	BlockY,f,access		; ONE PIXEL DOWN ------------------
	movf	BlockY,w,access
	addlw	-.16
	ifc
	decf	BlockY,f,access		; restore Blocky
tetris.farmw
; Horizontal actions (by key command)												

	btfss	KeyEdge,1,access	; test left key
	bra		no.key.1
	bcf		KeyEdge,1,access	; clear key flag
	bsf		FlagDemo,3,access		; Flag3 set = key action, Flag3 clr = no action or vertical action
	tstfsz	BlockX				; avoid zero BlockX (normal range is 1...8)
	decf	BlockX,f,access		; ------------ LEFT action ------------------
	bra		show
no.key.1
	btfss	KeyEdge,2,access	; test middle key
	bra		no.key.2
	bcf		KeyEdge,2,access	; clear key flag
	bsf		FlagDemo,3				; Flag3 set = key action, Flag3 clr = no action or vertical action
							; PATCH: if Block=0 & BlockX=7, then dec blockX
	movf	Block,f,access		;
	bnz		nopatchI			;
	movf	BlockX,w,access		;
	xorlw	6					;
	bnz		nopatchI			;
	decf	BlockX,f,access		;
nopatchI					; END PATCH
	incf	BlockA,f,access		; ------------ MIDDLE action ------------------
	bcf		BlockA,2,access		; limit angle counter to 0...3
	bra		show
no.key.2
	btfss	KeyEdge,4,access	; test right key
	bra		no.key.3
	bcf		KeyEdge,4,access	; clear key flag
	bsf		FlagDemo,3,access		; Flag3 set = key action, Flag3 clr = no action or vertical action
	incf	BlockX,f,access		; ------------ RIGHT action ------------------
	movf	BlockX,w,access
	addlw	-.8
	ifc							; if overflow right, then...
	decf	BlockX,f,access		; ...restore BlockX
	bra		show
no.key.3

; Game logic and buffer handling												
; Takes block bitmap from table and tests for horizontal overflows and			
; restores previously saved values of BlockX and BlockA if action not possible	

show
	call	get.block			; get block <Block>, X pos <BlockX>, angle <Blocka> in <CurrentBlock:4>
	bnc		noovf1
	movff	Temp.BlockX,BlockX	; restore if action was impossible
	movff	Temp.BlockA,BlockA	; restore if action was impossible
	call	get.block			; get block <Block>, X pos <BlockX>, angle <Blocka> in <CurrentBlock:4>
noovf1

; Clears Buffer and Draws block bitmap (game dump bitmap is at Buffer2, without current block)	

	call	clrbuf
	call	draw.block			; draws <CurrentBlock:4> in <BlockX>, <BlockY> Buffer bitmap

	call	test.overlap		; test Buffer and Buffer2 for pixel overlaping
	bnc		no.overlap
								; overlapping detected; let's see is it Y drop or X impossible command
	btfsc	FlagDemo,3,access		; Flag3 set = key action, Flag3 clr = no action or vertical action
	bra		impossible	
								; Y drop caused overlapping, go to new block
	decf	BlockY,f,access
	movff	Temp.BlockX,BlockX	; restore if Y drop caused overlap
	movff	Temp.BlockA,BlockA	; restore if Y drop caused overlap

	call	get.block			; get block <Block>, X pos <BlockX>, angle <Blocka> in <CurrentBlock:4>

	call	clrbuf
	call	draw.block			; draws <CurrentBlock:4> in <BlockX>, <BlockY> Buffer bitmap

	call	ior.bufer2.buffer	
	call	move.bufer.buffer2	; integrate current block in bitmap dump (Buffer2)
	goto 	tetris.cont			; try again

impossible						; impossible key command caused overlapping
	movff	Temp.BlockX,BlockX	; restore if key action caused overlap
	movff	Temp.BlockA,BlockA	; restore if key action caused overlap
	call	move.bufer2.buffer
	call	get.block			; get block <Block>, X pos <BlockX>, angle <Blocka> in <CurrentBlock:4>
	call	draw.block			; draws <CurrentBlock:4> in <BlockX>, <BlockY> Buffer bitmap
	call	ior.bufer2.buffer

	goto	tetris.farm			; try again

no.overlap
	movf	Buffer+.15,w,BANKED	; test if current block occupies lower row
	bz		no.floor
end.of.drop						; end of drop (block reached floor)
	call	ior.bufer2.buffer	
	call	move.bufer.buffer2	; integrate current block in bitmap dump (Buffer2)
	goto 	tetris.cont			; try again

no.floor
	call	ior.bufer2.buffer	; moves Buffer2 to Buffer
	bra		tetris.farm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

full.line.blink		; the lowest FULL line in Buffer blinks, then discard, then go to next line

	clrf	Inner,access	; counter how many lines are cleared at once
	lfsr	FSR0,Buffer+.16

	movlw	.16
	movwf	Outer,access	; line counter (line 0 does not act here)
go.16.blink
	movf	POSTDEC0,w		; dummy, just to pre-decrement INDF0
	incf	INDF0,w			; test if line = 11111111
	ifz
	call	full.line		; yes, process
	decfsz	Outer,f,access
	bra		go.16.blink		; loop next line
							; all lines finished
	movf	Inner,f,access
	ifz
	return

	dcfsnz	Inner,f,access
	bra		conv.sc
	call	advscore		; 2 lines

	dcfsnz	Inner,f,access
	bra		conv.sc	
	call	advscore		; 3 lines

	dcfsnz	Inner,f,access
	bra		conv.sc	
	call	advscore
	call	advscore		; 4 lines
conv.sc
	call	print.score		; Bcd+0,Bcd+1 ---> BufferPause
	return
;	
advscore
	movf	Score+1,w,access
	addlw	-0x27
	bnz		no.lo.test
	movf	Score,w,access
	addlw	-0x0F
no.lo.test
	ifc
	return
	incf	Score,f,access
	ifz
	incf	Score+1,f,access
	return

;	
full.line					; this line is full, it should blink
	incf	Inner,f,access
	call	advscore

	call	clr.blink
	call	clr.blink
	call	clr.blink
	call	clr.blink
	call	clr.blink
	call	clr.blink

	movff	FSR0L,FSR1L
	movff	FSR0H,FSR1H
	movff	FSR0L,FSR2L
	movff	FSR0H,FSR2H

	movf	POSTINC0,w		; dummy, just to increment INDF0 (readjust after shifting buffer down)
	movf	POSTDEC1,w		; dummy, just to decrement INDF1
not.yet
	movff	POSTDEC1,POSTDEC2	; moving...
	movf	FSR2L,w,access
	xorlw	low Buffer			; compare is it end of moving
	bnz		not.yet
								; end of moving
	return

;**********************************************************************

clr.blink
	clrf	INDF0,access		; off
	movlw	.7					; blinking period *10 ms
off.loop
	sync150
	decfsz	WREG,f,access
	bra		off.loop
	setf	INDF0,access		; on
	movlw	.7					; blinking period *10 ms
on.loop
	sync150
	decfsz	WREG,f,access
	bra		on.loop
	return


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

test.overlap		; test Buffer and Buffer2 for pixel overlaping, C set if overlap
	overlap	.0
	overlap	.1
	overlap	.2
	overlap	.3
	overlap	.4
	overlap	.5
	overlap	.6
	overlap	.7
	overlap	.8
	overlap	.9
	overlap	.10
	overlap	.11
	overlap	.12
	overlap	.13
	overlap	.14
	overlap	.15
	rcf					; NC = no overlap
	return
ovl.found
	scf					; C = overlap found
	return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

draw.block				; puts <CurrentBlock:4> in <BlockY> Buffer
	lfsr	FSR0,Buffer
	movf	BlockY,w,access
	addwf	FSR0L,f,access
	movf	POSTDEC0,w				; dummy
	movff	CurrentBlock+.0,POSTINC0
	movff	CurrentBlock+.1,POSTINC0
	movff	CurrentBlock+.2,POSTINC0
	movff	CurrentBlock+.3,POSTINC0
	return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ior.bufer2.buffer		; buffer2 or buffer ---> buffer (optimized for speed)
	iorbuf	.0
	iorbuf	.1
	iorbuf	.2
	iorbuf	.3
	iorbuf	.4
	iorbuf	.5
	iorbuf	.6
	iorbuf	.7
	iorbuf	.8
	iorbuf	.9
	iorbuf	.10
	iorbuf	.11
	iorbuf	.12
	iorbuf	.13
	iorbuf	.14
	iorbuf	.15
	return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

move.bufer.buffer2
	lfsr	FSR1,Buffer
	lfsr	FSR2,Buffer2
	bra		move.fsr1.fsr2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

move.bufer2.buffer
	lfsr	FSR1,Buffer2
	lfsr	FSR2,Buffer
move.fsr1.fsr2
	variable xx
xx=0
	while xx<.16
		movff	POSTINC1,POSTINC2
xx+=1
	endw
	return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

clrbuf.pause
	lfsr	FSR1,BufferPause
	bra		clrx
;	
clrbuf2
	lfsr	FSR1,Buffer2
	bra		clrx
;	
clrbuf
	lfsr	FSR1,Buffer
clrx
	variable xx
xx=0
	while xx<.16
		clrf	POSTINC1
xx+=1
	endw
	return

; puts block <Block>, X pos <BlockX>, angle <Blocka> in <CurrentBlock:4>
; Set C if out of screen
get.block
; calculate table position for <Block>
	movlw	high (blocks)
	movwf	TBLPTRH,access		; table pointer hi byte
	movf	Block,w,access		; 0=I, 1=J, 2=L, 3=O, 4=S, 5=Z, 6=T
	mullw	.16			; Block * 16 ---> PRODL
	movf	PRODL,w,access
	addlw	low (blocks)
	movwf	TBLPTRL,access		; calculated low position of <Block> ---> TBLPTRL
	ifc
	incf	TBLPTRH,f,access	; if page boundary is crossed, incf high pointer
; calculate table position for angle <BlockA>
	movf	BlockA,w,access
	mullw	.4			; BlockA * 4 ---> PRODL
	movf	PRODL,w,access
	addwf	TBLPTRL,f,access	; add table offset for angle <BlockA>
	ifc
	incf	TBLPTRH,f,access	; if page boundary is crossed, add carry to high pointer
; places table members in <CurrentBlock:4>
	tblrd*+
	movff	TABLAT,CurrentBlock+0
	tblrd*+
	movff	TABLAT,CurrentBlock+1
	tblrd*+
	movff	TABLAT,CurrentBlock+2
	tblrd*+
	movff	TABLAT,CurrentBlock+3
; rotate bits right <BlockX> times
	decf	BlockX,w,access
	bz		zero.shift		; if BlockX was =1, then no shift
	tstfsz	BlockX,access
	bra		nzblockx		; if Block was 2 or more
							; if Block was 0 then shift left once
	rcf
	rlcf	CurrentBlock+0,f,access
	bc		overflow
	rlcf	CurrentBlock+1,f,access
	bc		overflow
	rlcf	CurrentBlock+2,f,access
	bc		overflow
	rlcf	CurrentBlock+3,f,access
	bc		overflow
	bra		zero.shift

nzblockx
	movwf	Outer			; initialize shift_right loop
sr.loop
	rcf
	rrcf	CurrentBlock+0,f,access
	bc		overflow
	rrcf	CurrentBlock+1,f,access
	bc		overflow
	rrcf	CurrentBlock+2,f,access
	bc		overflow
	rrcf	CurrentBlock+3,f,access
	bc		overflow
	decfsz	Outer,f,access
	bra		sr.loop
zero.shift
	rcf						; NC = no overflow
	return

overflow
	scf						; C = overflow
	return

;******************************************************************

blocks			; block bitmaps, 4 bytes each:  I  J  L  O  S  Z  T
				; 4 angles each

;				 block.i					
	db	b'00000000',b'11110000',b'00000000',b'00000000'	; I (0 degrees)
	db	b'01000000',b'01000000',b'01000000',b'01000000'	; I (rotated 90 degrees CW)
	db	b'00000000',b'11110000',b'00000000',b'00000000'	; I (rotated 180 degrees CW)
	db	b'01000000',b'01000000',b'01000000',b'01000000'	; I (rotated 270 degrees CW)
;				 block.j					
	db	b'00000000',b'11100000',b'00100000',b'00000000'	; J (0 degrees)
	db	b'00100000',b'00100000',b'01100000',b'00000000'	; J (rotated 90 degrees CW)
	db	b'00000000',b'01000000',b'01110000',b'00000000'	; J (rotated 180 degrees CW)
	db	b'00000000',b'01100000',b'01000000',b'01000000'	; J (rotated 270 degrees CW)
;				 block.l					
	db	b'00000000',b'01110000',b'01000000',b'00000000'	; L (0 degrees)
	db	b'00000000',b'01100000',b'00100000',b'00100000'	; L (rotated 90 degrees CW)
	db	b'00100000',b'11100000',b'00000000',b'00000000'	; L (rotated 180 degrees CW)
	db	b'01000000',b'01000000',b'01100000',b'00000000'	; L (rotated 270 degrees CW)
;				 block.o					
	db	b'00000000',b'01100000',b'01100000',b'00000000'	; O (0 degrees)
	db	b'00000000',b'01100000',b'01100000',b'00000000'	; O (rotated 90 degrees CW)
	db	b'00000000',b'01100000',b'01100000',b'00000000'	; O (rotated 180 degrees CW)
	db	b'00000000',b'01100000',b'01100000',b'00000000'	; O (rotated 270 degrees CW)
;				 block.s					
	db	b'00000000',b'01100000',b'11000000',b'00000000'	; S (0 degrees)
	db	b'01000000',b'01100000',b'00100000',b'00000000'	; S (rotated 90 degrees CW)
	db	b'00000000',b'01100000',b'11000000',b'00000000'	; S (rotated 180 degrees CW)
	db	b'01000000',b'01100000',b'00100000',b'00000000'	; S (rotated 270 degrees CW)
;				 block.z					
	db	b'00000000',b'11000000',b'01100000',b'00000000'	; Z (0 degrees)
	db	b'00100000',b'01100000',b'01000000',b'00000000'	; Z (rotated 90 degrees CW)
	db	b'00000000',b'11000000',b'01100000',b'00000000'	; Z (rotated 180 degrees CW)
	db	b'00100000',b'01100000',b'01000000',b'00000000'	; Z (rotated 270 degrees CW)
;				 block.t					
	db	b'00000000',b'11100000',b'01000000',b'00000000'	; T (0 degrees)
	db	b'01000000',b'11000000',b'01000000',b'00000000'	; T (rotated 90 degrees CW)
	db	b'01000000',b'11100000',b'00000000',b'00000000'	; T (rotated 180 degrees CW)
	db	b'01000000',b'01100000',b'01000000',b'00000000'	; T (rotated 270 degrees CW)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

speedtab
	db	.76,.70,.65,.60,.76,.72,.68,.64,.60,.56,.52,.48,.44,.40,.36,.33
	db	.30,.28,.26,.24,.22,.20,.18,.17,.16,.15,.14,.13,.12,.12,.12,.12

;	db	.56,.50,.48,.44,.41,.38,.35,.32,.29,.26,.24,.22,.20,.19,.18,.17
;	db	.16,.16,.15,.15,.14,.14,.13,.13,.12,.12,.10,.10, .9, .9, .8, .8

;																							

bin2bcd							; convert from bin to bcd
	clrf	Bcd+0,access
	clrf	Bcd+1,access
;	clrf	Bcd2,access
	movlw	.16
	movwf	Inner,access

gobinbcd
	rlcf	Bin+0,f,access
	rlcf	Bin+1,f,access
;   rlcf	Bin2,f,access		; ...
	movf	Bcd+0,w,access
	addwfc	Bcd+0,w,access		; 2* and carry
	daw
	movwf	Bcd+0,access		; Save result back

	movf	Bcd+1,W,access
	addwfc	Bcd+1,W,access
	daw
	movwf	Bcd+1,access

;	movf	Bcd+2,W,access		; repeat for BcdX+1...
;	addwfc	Bcd+2,W,access
;	daw
;	movwf	Bcd+2,access		; ...

	decfsz	Inner,f,access
	bra		gobinbcd
	return

;																							

print.score.buf	; Bcd+0,Bcd+1 ---> Buffer
	call	clrbuf
	lfsr	FSR0,Buffer+4
	bra		p.sc

print.score		; Bcd+0,Bcd+1 ---> BufferPause
	call	clrbuf.pause
	lfsr	FSR0,BufferPause+4
p.sc
	movff	Score,Bin+0
	movff	Score+1,Bin+1
	call	bin2bcd				; conv
							; ripple blanking
	movf	Bcd+1,w,access
	andlw	0xF0				; isolate thousands
	bnz		no.ripple			; if thousands not zero
	movlw	0xA0				; blank char instead of 0
	iorwf	Bcd+1,f,access		; insert

	movf	Bcd+1,w,access
	andlw	0x0F				; isolate hundreds
	bnz		no.ripple			; if hundreds not zero
	movlw	0x0A				; blank char instead of 0
	iorwf	Bcd+1,f,access		; insert

	movf	Bcd+0,w,access
	andlw	0xF0				; isolate tens
	bnz		no.ripple			; if tens not zero
	movlw	0xA0				; blank char instead of 0
	iorwf	Bcd+0,f,access		; insert

;	movf	Bcd+0,w,access
;	andlw	0x0F				; isolate ones
;	bnz		no.ripple			; if hundreds not zero
;	movlw	0x0A				; blank char instead of 0
;	iorwf	Bcd+0,f,access		; insert
no.ripple
	swapf	Bcd+1,w,access
	andlw	0x0F			; isolate thousands
	call	one.byte.map
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTDEC0,f
	movf	POSTDEC0,f		; dummy
	movf	POSTDEC0,f		; dummy
	movf	POSTDEC0,f		; dummy

	movf	Bcd+1,w,access
	andlw	0x0F			; isolate hundreds
	call	one.byte.map
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f

	clrf	POSTINC0		; dummy

	swapf	Bcd+0,w,access
	andlw	0x0F			; isolate tens
	call	one.byte.map
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	swapf	WREG,f,access
	iorwf	POSTDEC0,f
	movf	POSTDEC0,f		; dummy
	movf	POSTDEC0,f		; dummy
	movf	POSTDEC0,f		; dummy

	movf	Bcd+0,w,access
	andlw	0x0F			; isolate ones
	call	one.byte.map
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f
	tblrd*+
	movf	TABLAT,w,access
	iorwf	POSTINC0,f
	return

;							
moving.message
	movf	Score+1,f,access
	ifnz
	return					; if score is >99, then no room for text
	movf	Score,w,access
	addlw	-.100
	ifc
	return			; if score is >99, then no room for text
;	
	lfsr	FSR0,Disp.text	; dst ptr
	movlw	low (txt.p)	; text PAUSE
	movwf	TBLPTRL,access
	movlw	high (txt.p)
	movwf	TBLPTRH,access
	btfss	FlagDemo,2		; bit 2 set = showing final result
	bra		txt.ptr.set
	movlw	low (txt.f)	; final text PLAY TETRIS!
	movwf	TBLPTRL,access
	movlw	high (txt.f)
	movwf	TBLPTRH,access
txt.ptr.set
	tblrd*+
	movf	TABLAT,w,access
	movwf	POSTINC0
	bnz		txt.ptr.set		; copy all txt from rom to Disp.text
start.message.t		;				 moving message start								
	lfsr	FSR0,Disp.text	; initialize read ptr
next.char.t				; \\\\\\\\\\\\\\\\
	movlw	.6
	movwf	ColCount,access
	call	clrbuf2
gnc.t
	movf	POSTINC0,w		; get next char
	bz		start.message.t	; if end of message detected
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

	lfsr	FSR1,Buffer2+1	; start of next char in Buffer2 (defines vert text pos)
	movlw	.9
go9txt.t
	TBLRD*+
	movff	TABLAT,POSTINC1	; transfer char bitmap from char gen to buffer2
	decfsz	WREG,f,access
	bra		go9txt.t
next.column.t			; \\\\\\\\
							; press any key to return from moving message and start new game
	btfsc	KeyEdge,1,access
	return
	btfsc	KeyEdge,2,access
	return
	btfsc	KeyEdge,3,access
	return
	btfsc	KeyEdge,4,access
	return

	movlw	.18				; this determines moving message speed
wait.disp.t
	sync150
	decfsz	WREG,f,access
	bra		wait.disp.t		; wait for period
;	
	lfsr	FSR1,Buffer
	lfsr	FSR2,Buffer2

	movlw	.10
moveleft9.t
	rcf
	rlcf	POSTINC2,f
	rlcf	POSTINC1,f		; shift left buffer2 to buffer
	decfsz	WREG,f,access
	bra		moveleft9.t

	decfsz	ColCount,f,access
	bra		next.column.t; ////////
							; one ASCII char passed, go next char
	bra		next.char.t	; ////////////////

					;				 moving message end								
notxt
	return

one.byte.map
	mullw	.6				; bytes/char

	movlw	low (charg4)
	movwf	TBLPTRL,access
	movlw	high (charg4)
	movwf	TBLPTRH,access

	movf	PRODL,w,access
	addwf	TBLPTRL,f,access
	ifc
	incf	TBLPTRH,f,access

	tblrd*+
	movf	TABLAT,w,access
	return

txt.p
	db		"Pause  ",0
txt.f
	db		"PLAY TETRIS!  ",0


