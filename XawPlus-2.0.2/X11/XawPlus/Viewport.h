/* $XConsortium: Viewport.h,v 1.21 91/07/22 19:05:23 converse Exp $ */

/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

Cleaned up the resource description: Roland Krause 1997

********************************************************/

#ifndef _XawViewport_h
#define _XawViewport_h

#include <X11/XawPlus/Form.h>
#include <X11/XawPlus/Reports.h>
#include <X11/Xfuncproto.h>

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 OBJECT:
 destroyCallback     Callback		Pointer	  	NULL

 RECTANGLE:
 borderWidth	     BorderWidth	Dimension	1
 sensitive	     Sensitive		Boolean		True
 height		     Height		Dimension	0
 width		     Width		Dimension	0
 x		     Position		Position	0
 y		     Position		Position	0

 CORE:
 border		     BorderColor	Pixel		XtDefaultForeground
 background	     Background		Pixel		grey75
 mappedWhenManaged   MappedWhenManaged	Boolean		True

 FORM:
 defaultDistance     Thickness		int		4

 VIEWPORT:
 allowHoriz	     Boolean		Boolean		False
 allowVert	     Boolean		Boolean		False
 foreceBars	     Boolean		Boolean		False
 reportCallback	     ReportCallback	Pointer		NULL
 useBottom	     Boolean		Boolean		False
 useRight	     Boolean		Boolean		False

*/

/* fields added to Form */
#ifndef _XtStringDefs_h_
#define XtNforceBars "forceBars"
#define XtNallowHoriz "allowHoriz"
#define XtNallowVert "allowVert"
#define XtNuseBottom "useBottom"
#define XtNuseRight "useRight"
#endif

extern WidgetClass viewportWidgetClass;

typedef struct _ViewportClassRec *ViewportWidgetClass;
typedef struct _ViewportRec  *ViewportWidget;

_XFUNCPROTOBEGIN

extern void XawViewportSetLocation (
#if NeedFunctionPrototypes
    Widget		/* gw */,
#if NeedWidePrototypes
    /* float */ double	/* xoff */,
    /* float */ double	/* yoff */
#else
    float		/* xoff */,
    float		/* yoff */
#endif
#endif
);

extern void XawViewportSetCoordinates (
#if NeedFunctionPrototypes
    Widget		/* gw */,
#if NeedWidePrototypes
    /* Position */ int	/* x */,
    /* Position */ int	/* y */
#else
    Position		/* x */,
    Position		/* y */
#endif
#endif
);

_XFUNCPROTOEND

#endif /* _XawViewport_h */
