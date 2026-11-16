/*
  memset.c - part of libgccsupport.a, the freestanding helpers for the
  GCC build (see Common.mk).

  void *memset(void *s, int c, size_t n)

  Uses only the scratch registers d0/d1/a0/a1. For more than 16 bytes the
  fill byte is spread over a long, one leading byte makes the pointer
  even (the 68000 faults on odd word/long accesses), the loop stores 16
  bytes per pass with move.l d0,(a0)+ (about 58 cycles on a 68000) and
  the remainder takes at most a long pair, long, word and byte store.
  The dbra loops carry into the upper word for counts of 64K and more.
*/
#ifdef __GNUC__

__asm__(
"	.text\n"
"	.even\n"
"	.globl	_memset\n"
"_memset:\n"
"	move.l	4(sp),a0\n"		/* s */
"	move.l	8(sp),d0\n"		/* c */
"	move.l	12(sp),d1\n"		/* n */
"	cmpi.l	#16,d1\n"
"	bls.s	5f\n"			/* n <= 16: bytewise */
"	andi.w	#0xff,d0\n"		/* d0 = 000000cc */
"	move.w	d0,a1\n"
"	lsl.w	#8,d0\n"
"	add.w	a1,d0\n"		/* d0.w = cccc */
"	move.w	d0,a1\n"
"	swap	d0\n"
"	move.w	a1,d0\n"		/* d0 = cccccccc */
"	move.l	a0,a1\n"		/* test the pointer in a data register: */
"	exg	d1,a1\n"		/* d1 = s, a1 = n (exg keeps the flags) */
"	btst	#0,d1\n"
"	exg	d1,a1\n"
"	beq.s	1f\n"
"	move.b	d0,(a0)+\n"		/* make it even, n stays >= 16 */
"	subq.l	#1,d1\n"
"1:	move.l	d1,a1\n"		/* keep n for the remainder */
"	lsr.l	#4,d1\n"		/* 16 byte blocks, >= 1 */
"	subq.l	#1,d1\n"
"2:	move.l	d0,(a0)+\n"
"	move.l	d0,(a0)+\n"
"	move.l	d0,(a0)+\n"
"	move.l	d0,(a0)+\n"
"	dbra	d1,2b\n"
"	clr.w	d1\n"
"	subq.l	#1,d1\n"
"	bcc.s	2b\n"
"	move.l	a1,d1\n"
"	btst	#3,d1\n"
"	beq.s	3f\n"
"	move.l	d0,(a0)+\n"
"	move.l	d0,(a0)+\n"
"3:	btst	#2,d1\n"
"	beq.s	4f\n"
"	move.l	d0,(a0)+\n"
"4:	btst	#1,d1\n"
"	beq.s	7f\n"
"	move.w	d0,(a0)+\n"
"7:	btst	#0,d1\n"
"	beq.s	8f\n"
"	move.b	d0,(a0)+\n"
"	bra.s	8f\n"
"5:	tst.l	d1\n"
"	beq.s	8f\n"
"	subq.l	#1,d1\n"
"6:	move.b	d0,(a0)+\n"
"	dbra	d1,6b\n"
"	clr.w	d1\n"
"	subq.l	#1,d1\n"
"	bcc.s	6b\n"
"8:	move.l	4(sp),d0\n"		/* return s */
"	rts\n"
);

#endif /* __GNUC__ */
