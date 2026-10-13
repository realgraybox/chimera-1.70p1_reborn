/*
 * common.h
 */
#include <X11/Intrinsic.h>

/*
 * ANSI-C stuff.  Snarfed from xloadimage.
 */
#ifdef __STDC__

#if !defined(_ArgProto)
#define _ArgProto(ARGS) ARGS
#endif

#else /* !__STDC__ */

#if !defined(const) /* "const" is an ANSI thing */
#define const
#endif
#if !defined(_ArgProto)
#define _ArgProto(ARGS) ()
#endif

#endif /* __STDC__ */

#define free_mem(m)            XtFree((char *)(m))
#define alloc_mem(len)         XtMalloc(len)
#define calloc_mem(n, len)     XtCalloc(n, len)
#define realloc_mem(s, len)    XtRealloc((char *)(s), len)
#define alloc_string(str)      XtNewString(str)
