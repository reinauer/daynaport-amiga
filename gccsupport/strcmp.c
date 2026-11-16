/*
  strcmp.c - part of libgccsupport.a, the freestanding helpers for the
  GCC build (see Common.mk).

  int strcmp(const char *a, const char *b)

  Compares as unsigned char, as C requires.
*/
#ifdef __GNUC__

__asm__(
"	.text\n"
"	.even\n"
"	.globl	_strcmp\n"
"_strcmp:\n"
"	move.l	4(sp),a0\n"
"	move.l	8(sp),a1\n"
"	moveq	#0,d0\n"
"	moveq	#0,d1\n"
"1:	move.b	(a0)+,d0\n"
"	move.b	(a1)+,d1\n"
"	cmp.b	d1,d0\n"
"	bne.s	2f\n"
"	tst.b	d0\n"
"	bne.s	1b\n"
"	rts\n"				/* equal: d0 = 0 */
"2:	sub.l	d1,d0\n"
"	rts\n"
);

#endif /* __GNUC__ */
