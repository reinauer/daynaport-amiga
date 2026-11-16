/*
  gccsupport.c

  Freestanding helpers for the GCC build. The device has no startup code
  and keeps its library bases in the device base, so it must not pull in
  libnix functions that reach for a global SysBase (libnix memcpy() calls
  exec CopyMem()).
*/
#ifdef __GNUC__

#include <stddef.h>

/* keep GCC from turning the copy loops back into a memcpy() call */
__attribute__((optimize("no-tree-loop-distribute-patterns")))
void *memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	/* the 68000 needs even addresses for word/long accesses */
	if ((((unsigned long)d | (unsigned long)s) & 1) == 0) {
		while (n >= 4) {
			*(unsigned long *)d = *(const unsigned long *)s;
			d += 4;
			s += 4;
			n -= 4;
		}
	}
	while (n--)
		*d++ = *s++;
	return dest;
}

#endif /* __GNUC__ */
