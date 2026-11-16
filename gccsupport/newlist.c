/*
  newlist.c - part of libgccsupport.a, the freestanding helpers for the
  GCC build (see Common.mk). Replaces amiga.lib NewList().

  void NewList(struct List *list)
*/
#ifdef __GNUC__

__asm__(
"	.text\n"
"	.even\n"
"	.globl	_NewList\n"
"_NewList:\n"
"	move.l	4(sp),a0\n"
"	lea	4(a0),a1\n"
"	move.l	a1,(a0)\n"		/* lh_Head = &lh_Tail */
"	clr.l	(a1)\n"			/* lh_Tail = NULL */
"	move.l	a0,8(a0)\n"		/* lh_TailPred = list */
"	rts\n"
);

#endif /* __GNUC__ */
