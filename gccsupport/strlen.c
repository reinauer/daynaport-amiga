/*
  strlen.c - part of libgccsupport.a, the freestanding helpers for the
  GCC build (see Common.mk).

  size_t strlen(const char *s)
*/
#ifdef __GNUC__

__asm__(
"	.text\n"
"	.even\n"
"	.globl	_strlen\n"
"_strlen:\n"
"	move.l	4(sp),a0\n"
"	move.l	a0,d0\n"
"1:	tst.b	(a0)+\n"
"	bne.s	1b\n"
"	suba.l	d0,a0\n"		/* length + 1 */
"	move.l	a0,d0\n"
"	subq.l	#1,d0\n"
"	rts\n"
);

#endif /* __GNUC__ */
