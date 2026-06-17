#ifndef SCSIDAYNA_TIMEVAL_COMPAT_H
#define SCSIDAYNA_TIMEVAL_COMPAT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#if defined(__VBCC__) && !defined(_TIMEVAL_DEFINED)
#define _TIMEVAL_DEFINED
struct timeval {
    ULONG tv_secs;
    ULONG tv_micro;
};
#endif

#endif
