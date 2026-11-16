/*
  strcpy.c - part of libgccsupport.a, the freestanding helpers for the
  GCC build (see Common.mk).

  char *strcpy(char *dest, const char *src)
*/
#ifdef __GNUC__

__asm__(
"	.text\n"
"	.even\n"
"	.globl	_strcpy\n"
"_strcpy:\n"
"	move.l	4(sp),a0\n"		/* dest */
"	move.l	8(sp),a1\n"		/* src */
"	move.l	a0,d0\n"		/* return dest */
"1:	move.b	(a1)+,(a0)+\n"
"	bne.s	1b\n"
"	rts\n"
);

#endif /* __GNUC__ */
