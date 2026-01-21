*-----------------------------------------------------------------------------*
*			     Eagleripper V1.0				      *
*				IFF					      *
*-----------------------------------------------------------------------------*
Test	= 0
		incdir	include:
		include	misc/eagleplayerripper.i
		include	exec/exec_lib.i
			
	if	Test
	lea	data,a0
	*lea	IFF8SVXTags,a0
	move.l	#datasize,d0
	move.l	a0,a5
	move.l	a0,a4
	move.l	d0,d4
	move.l	d0,d6
wait:	move.l	a5,a1
	move.l	a4,a0
	move.l	d4,d0
	move.l	(a1),d1
	*bsr	IFF8SVXRipp1
	bsr	IFFSMUSRipp1
	tst.l	d0
	beq.s	.Ja
	addq.l	#2,a5
	subq.l	#2,d6
	blt.s	.Err
	btst	#6,$bfe001
	bne.s	wait
.err:	moveq	#-1,d0
.ret:	illegal

.ja:	move.l	d1,d0
	bsr	GetModuleName
	bra.s	.Ret
	endc
		RIPPERHEADER	IFF8SVXTags

	dc.b	"Eagleripper-IFF-Sampleripper (8SVX...) V1.0",10
	dc.b	"done by Eagleeye/DFT 28.08.97",0
	even

IFF8SVXTags:	dc.l	RPT_Formatname,.Formatname
		dc.l	RPT_Ripp1,IFF8SVXRipp1
		dc.l	RPT_RequestRipper,1
		dc.l	RPT_Version,1<<16!0
		dc.l	RPT_Creator,.Creator
		dc.l	RPT_GetModuleName,GetModuleName
		dc.l	RPT_Playername,.Playername
		dc.l	RPT_Prefix,.Prefix
		dc.l	RPT_Next,IFFSMUSTags
		dc.l	0

.Creator:	dc.b	"???, adapted by DEFECT",0
.Formatname:	dc.b	"IFF-8SVX",0
.Playername:	dc.b	"IFF 8SVX",0
.Prefix:	dc.b	"IFF.",0
		even

IFFSMUSTags:	dc.l	RPT_Formatname,.Formatname
		dc.l	RPT_Ripp1,IFFSMUSRipp1
		dc.l	RPT_RequestRipper,1
		dc.l	RPT_Version,1<<16!0
		dc.l	RPT_Creator,.Creator
		dc.l	RPT_GetModuleName,GetModuleName
		dc.l	RPT_Playername,.Playername
		dc.l	RPT_Prefix,.Prefix
		dc.l	0

.Creator:	dc.b	"???, adapted by DEFECT",0
.Playername:
.Formatname:	dc.b	"IFF-SMUS",0
.Prefix:	dc.b	"SMUS.",0
		even

*-----------------------------------------------------------------------------*
* Input: a0=Adr (start of memory)
*	 d0=Size (size of memory)
*	 a1=current adr
*	 d1=(a1.l)
* Output:d0=Error oder NULL
*	 d1=Size
*	 a0=Startadr (data)
*-----------------------------------------------------------------------------*
IFF8SVXRipp1:	cmp.l	#"FORM",d1
		beq.s	.Ja
		rts

		*--- Jetzt testen, ob es ein IFF-Sample sein kann ---*
.Ja:		move.l	a1,-(a7)
		add.l	d0,a0
		sub.l	a1,a0
		move.l	a0,d1			->> max. Size
		move.l	(a7)+,a0		;Startadr

		subq.l	#8,d1

		move.l	4(a1),d0
		cmp.l	d0,d1
		blo.s	.Error
		addq.l	#8,a1
		subq.l	#4,d1

		move.l	(a1)+,d0
		cmp.l	#"8SVX",d0
		bne.s	.Error

		*--- Chunks durchtesten ---*
.NextChunk:	move.l	(a1)+,d0
		move.l	(a1)+,d2
		addq.l	#1,d2
		bclr	#0,d2
		cmp.l	#4,d2
		blt.s	.Error
		subq.l	#8,d1
		cmp.l	d2,d1
		blo.s	.Error
		bsr	CheckChunkName
		bne.s	.Error
		move.l	-8(a1),d0
		lea	(a1,d2.l),a1
		sub.l	d2,d1
		cmp.l	#"BODY",d0
		beq.s	.Fertig
		bra.s	.NextChunk

.Fertig:	move.l	a1,d1
		sub.l	a0,d1
		moveq	#0,d0
.Return:	rts
.Error:		moveq	#-1,d0
		rts

*-----------------------------------------------------------------------------*
* Input: a0=Adr (start of memory)
*	 d0=Size (size of memory)
*	 a1=current adr
*	 d1=(a1.l)
* Output:d0=Error oder NULL
*	 d1=Size
*	 a0=Startadr (data)
*-----------------------------------------------------------------------------*
IFFSMUSRipp1:	cmp.l	#"FORM",d1
		beq.s	.Ja
		rts

		*--- Jetzt testen, ob es ein IFF-Sample sein kann ---*
.Ja:		move.l	a1,-(a7)
		add.l	d0,a0
		sub.l	a1,a0
		move.l	a0,d1			->> max. Size
		move.l	(a7)+,a0		;Startadr

		subq.l	#8,d1

		move.l	4(a1),d0
		cmp.l	d0,d1
		blo.s	.Error
		addq.l	#8,a1
		subq.l	#4,d1

		move.l	(a1)+,d0
		cmp.l	#"SMUS",d0
		bne.s	.Error

		*--- Chunks durchtesten ---*
.NextChunk:	move.l	(a1)+,d0
		move.l	(a1)+,d2
		addq.l	#1,d2
		bclr	#0,d2
		cmp.l	#4,d2
		blt.s	.Error
		subq.l	#8,d1
		cmp.l	d2,d1
		blo.s	.Error
		bsr	CheckChunkName
		bne.s	.Error
		move.l	-8(a1),d0
		lea	(a1,d2.l),a1
		sub.l	d2,d1
		cmp.l	#"TRAK",d0
		bne.s	.NextChunk

.AlleTrackssuchen:
		move.l	(a1)+,d0
		move.l	(a1)+,d2
		cmp.l	#"TRAK",d0
		bne.s	.Fertig
		addq.l	#1,d2
		bclr	#0,d2
		cmp.l	#4,d2
		blt.s	.Error
		subq.l	#8,d1
		cmp.l	d2,d1
		blo.s	.Error
		bsr	CheckChunkName
		bne.s	.Error
		move.l	-8(a1),d0
		lea	(a1,d2.l),a1
		sub.l	d2,d1
		bra.s	.AlleTrackssuchen

.Fertig:	subq.l	#8,a1
		move.l	a1,d1
		sub.l	a0,d1
		moveq	#0,d0
.Return:	rts
.Error:		moveq	#-1,d0
		rts

*-----------------------------------------------------------------------------*
CheckChunkName:	movem.l	d1/d2,-(a7)
		move.l	d0,d1
		moveq	#3,d2
.NextByte:	cmp.b	#" ",d1
		blo.s	.Return
		cmp.b	#"z",d1
		bhi.s	.Return
		lsr.l	#8,d1
		dbf	d2,.NextByte
		moveq	#0,d0
.Return:	movem.l	(a7)+,d1/d2
		tst.l	d0
		rts

*-----------------------------------------------------------------------------*
* Input: a0=Start of Modul
*	 d0=Size
* Output:d0=Ptr to name or NULL
*-----------------------------------------------------------------------------*
GetModuleName:	move.l	a2,-(a7)
		move.l	a0,a2
		move.l	a0,a1
		move.l	d0,d3
		move.l	(a1),d1
		bsr	IFF8SVXRipp1
		beq.s	.Ja
		move.l	d3,d0
		move.l	a2,a0
		move.l	a2,a1
		move.l	(a1),d1
		bsr	IFFSMUSRipp1
		bne.s	.Error

.Ja:		*--- Name-Hunk finden ---*
		lea	12(a2),a2
.NextHunk:	move.l	(a2)+,d0
		move.l	(a2)+,d1
		cmp.l	#"BODY",d0
		beq.s	.Error
		cmp.l	#"TRAK",d0
		beq.s	.Error
		cmp.l	#"NAME",d0
		beq.s	.NameFound
		lea	(a2,d1.l),a2
		bra.s	.NextHunk
.Error:		sub.l	a2,a2
.NameFound:	move.l	a2,d0		
		move.l	(a7)+,a2
		rts


	if	test
data:		incdir	"Modules:ADPCM-IFF-MPEG-Wave/"
		*ds.b	200

		*incbin	IFF.Anrufb.Anonym             
		*incbin	IFF.Anrufb.Beatles            
		*incbin	IFF.Anrufb.BeichtStuhl        
		*incbin	IFF.Anrufb.Blühm_Anruf        
		*incbin	IFF.Anrufb.Boney_M            
		*incbin	IFF.Anrufb.Der_Hund           
		*incbin	IFF.Anrufb.Faust              
		*incbin	IFF.Anrufb.Grizmek            
		*incbin	IFF.Anrufb.Hallervorden       
		*incbin	IFF.Anrufb.Kohl&Bush          
		*incbin	IFF.Anrufb.Kohl_English       
		*incbin	IFF.Anrufb.Magnum             
		*incbin	IFF.Anrufb.Mcp                
		*incbin	IFF.Anrufb.Moser              
		*incbin	IFF.Anrufb.Nasa               
		*incbin	IFF.Anrufb.Nutte              
		*incbin	IFF.Anrufb.Papst2             
		*incbin	IFF.Anrufb.Putze              
		*incbin	IFF.Anrufb.Ruehmann           
		*incbin	IFF.Anrufb.Todesstern         
		*incbin	IFF.Anrufb.Werners-Röhrich    
		*incbin	IFF.Anrufb.Willy              
		*incbin	IFF.Anrufp.Die_Prinzen        
		*incbin	IFF.Bundy-Titelsong           
		*incbin	IFF.Hupe                      
		*incbin	IFF.KitomerII                 
		*incbin	IFF.Milka_It's cool Man       
		*incbin	IFF.Pronounce                 
		*incbin	IFF.Roadrunner                
		*incbin	IFF.Sprichniewieder           

		*incdir	"Modules:Verschiedenes/"
		*incbin	"BD.Motorhead 1"
		*incbin	"BD.Motorhead 2"
		*incbin	"BD.Motorhead 3"

		incdir	modules:smus/
		incbin	smus.painterman




datasize	= *-data
	endc
