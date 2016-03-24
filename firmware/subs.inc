
; puts block <Block>, X pos <BlockX>, angle <Blocka> in <CurrentBlock:4>
; Set C if out of screen
get.block
; calculate table position for <Block>
	movlw	high blocks
	movwf	TBLPTRH,ACCESS		; table pointer hi byte
	movf	Block,w,ACCESS		; 0=I, 1=J, 2=L, 3=O, 4=S, 5=Z, 6=T
	mullw	.16			; Block * 16 ---> PRODL
	movf	PRODL,w,ACCESS
	addlw	low blocks
	movwf	TBLPTRL,ACCESS		; calculated low position of <Block> ---> TBLPTRL
	ifc
	incf	TBLPTRH,f,ACCESS	; if page boundary is crossed, incf high pointer
; calculate table position for angle <BlockA>
	movf	BlockA,w,ACCESS
	mullw	.4			; BlockA * 4 ---> PRODL
	movf	PRODL,w,ACCESS
	addwf	TBLPTRL,f,ACCESS	; add table offset for angle <BlockA>
	ifc
	incf	TBLPTRH,f,ACCESS	; if page boundary is crossed, add carry to high pointer
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
	decf	BlockX,w,ACCESS
	bz		zero.shift		; if BlockX was =1, then no shift
	tstfsz	BlockX,ACCESS
	bra		nzblockx		; if Block was 2 or more
							; if Block was 0 then shift left once
	rcf
	rlcf	CurrentBlock+0,f,ACCESS
	bc		overflow
	rlcf	CurrentBlock+1,f,ACCESS
	bc		overflow
	rlcf	CurrentBlock+2,f,ACCESS
	bc		overflow
	rlcf	CurrentBlock+3,f,ACCESS
	bc		overflow
	bra		zero.shift

nzblockx
	movwf	Outer			; initialize shift_right loop
sr.loop
	rcf
	rrcf	CurrentBlock+0,f,ACCESS
	bc		overflow
	rrcf	CurrentBlock+1,f,ACCESS
	bc		overflow
	rrcf	CurrentBlock+2,f,ACCESS
	bc		overflow
	rrcf	CurrentBlock+3,f,ACCESS
	bc		overflow
	decfsz	Outer,f,ACCESS
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
;	db	.56,.48,.41,.35,.30,.26,.22,.19,.17,.15,.14,.13,.12,.11,.10,.9

	db	.56,.50,.48,.44,.41,.38,.35,.32,.29,.26,.24,.22,.20,.19,.18,.17
	db	.16,.16,.15,.15,.14,.14,.13,.13,.12,.12,.10,.10, .9, .9, .8, .8

