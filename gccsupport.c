/*
  gccsupport.c

  Freestanding helpers for the GCC build. The device has no startup code
  and keeps its library bases in the device base, so it must not pull in
  libnix functions that reach for a global SysBase (libnix memcpy() calls
  exec CopyMem()).
*/
#ifdef __GNUC__

#include <stddef.h>

/*
  void *memcpy(void *dest, const void *src, size_t n)

  Hand-written because GCC turns the block loop into indexed moves plus
  lea pointer updates, and saves registers it does not need. This only
  uses the scratch registers d0/d1/a0/a1.

  The 68000 faults on word and long accesses to odd addresses, so longs
  are only used when source and destination have the same alignment.
  One leading byte makes both even, the loop moves 16 bytes per pass
  with move.l (a0)+,(a1)+ (about 90 cycles on a 68000), and the
  remainder takes at most one long pair, long, word and byte move.
  Short or mismatched copies go bytewise.

  The dbra loops count in 16 bits; clr.w/subq.l/bcc carries the upper
  word so counts of 64K blocks or bytes and more still work.
*/
__asm__(
"	.text\n"
"	.even\n"
"	.globl	_memcpy\n"
"_memcpy:\n"
"	move.l	4(sp),a1\n"		/* dest */
"	move.l	8(sp),a0\n"		/* src */
"	move.l	12(sp),d1\n"		/* n */
"	moveq	#16,d0\n"
"	cmp.l	d0,d1\n"
"	bls.s	5f\n"			/* n <= 16: bytewise */
"	move.l	a1,d0\n"
"	sub.l	a0,d0\n"
"	btst	#0,d0\n"
"	bne.s	5f\n"			/* alignments differ: bytewise */
"	move.l	a0,d0\n"
"	btst	#0,d0\n"
"	beq.s	1f\n"
"	move.b	(a0)+,(a1)+\n"		/* make both even, n stays >= 16 */
"	subq.l	#1,d1\n"
"1:	move.l	d1,d0\n"
"	lsr.l	#4,d0\n"		/* 16 byte blocks, >= 1 */
"	subq.l	#1,d0\n"
"2:	move.l	(a0)+,(a1)+\n"
"	move.l	(a0)+,(a1)+\n"
"	move.l	(a0)+,(a1)+\n"
"	move.l	(a0)+,(a1)+\n"
"	dbra	d0,2b\n"
"	clr.w	d0\n"
"	subq.l	#1,d0\n"
"	bcc.s	2b\n"
"	btst	#3,d1\n"
"	beq.s	3f\n"
"	move.l	(a0)+,(a1)+\n"
"	move.l	(a0)+,(a1)+\n"
"3:	btst	#2,d1\n"
"	beq.s	4f\n"
"	move.l	(a0)+,(a1)+\n"
"4:	btst	#1,d1\n"
"	beq.s	7f\n"
"	move.w	(a0)+,(a1)+\n"
"7:	btst	#0,d1\n"
"	beq.s	8f\n"
"	move.b	(a0)+,(a1)+\n"
"	bra.s	8f\n"
"5:	move.l	d1,d0\n"
"	beq.s	8f\n"
"	subq.l	#1,d0\n"
"6:	move.b	(a0)+,(a1)+\n"
"	dbra	d0,6b\n"
"	clr.w	d0\n"
"	subq.l	#1,d0\n"
"	bcc.s	6b\n"
"8:	move.l	4(sp),d0\n"		/* return dest */
"	rts\n"
);

#endif /* __GNUC__ */
