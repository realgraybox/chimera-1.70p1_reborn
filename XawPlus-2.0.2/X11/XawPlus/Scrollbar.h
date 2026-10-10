/* $XConsortium: Scrollbar.h,v 1.7 91/07/26 21:59:31 converse Exp $ */


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
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

3D extensions added : Roland Krause 1996
Changes for Scrollbar of Xaw3d: Roland Krause 1999

******************************************************************/

#ifndef _Scrollbar_h
#define _Scrollbar_h

/****************************************************************
 *
 * Scrollbar Widget
 *
 ****************************************************************/

#include <X11/Xmu/Converters.h>
#include <X11/Xfuncproto.h>

/* Scrollbar resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 OBJECT:
 destroyCallback     Callback		Pointer		NULL

 RECTANGLE:
 borderWidth	     BorderWidth	Dimension	1
 height		     Height		Dimension	length or thickness
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	thickness or length
 x		     Position		Position	0
 y		     Position		Position	0

 CORE:
 border		     BorderColor	Pixel		XtDefaultForeground
 background	     Background		Pixel		grey75
 mappedWhenManaged   MappedWhenManaged	Boolean		True

 ADD3DEXT:
 highlightColor	     Background		Pixel		grey90
 shadowColor	     Background		Pixel		grey40
 buttonBorderWidth   Width		Dimension	2

 SIMPLE:
 cursor		     Cursor		Cursor		None
 cursorName	     Cursor		String		left_ptr
 pointerColor        Foreground         Pixel           XtDefaultForeground
 pointerColorBackground Background      Pixel           XtDefaultBackground
 insensitiveBorder   Insensitive	Pixmap		Gray

 SCROLLBAR:
 length		     Length		Dimension	1
 minimumThumb	     MinimumThumb	Dimension	12
 thickness	     Thickness		Dimension	16
 orientation	     Orientation	XtOrientation	XtorientVertical
 shown		     Shown		Float		0.0
 topOfThumb	     TopOfThumb		Float		0.0
 pickTop	     PickTop		Boolean		False
 
 accelerators	     Accelerators	AcceleratorTable NULL
 ancestorSensitive   AncestorSensitive	Boolean		True
 jumpProc	     Callback		XtCallbackList	NULL
 scrollProc	     Callback		XtCallbackList	NULL
 thumbProc	     Callback		XtCallbackList	NULL
 translations	     Translations	TranslationTable see source or doc

*/

/* 
 * Most things we need are in StringDefs.h 
 */

#define XtCMinimumThumb "MinimumThumb"
#define XtCShown	"Shown"
#define XtCTopOfThumb	"TopOfThumb"
#define XtCPickTop	"PickTop"

#define XtNminimumThumb "minimumThumb"
#define XtNtopOfThumb	"topOfThumb"
#define XtNpickTop	"pickTop"

typedef struct _ScrollbarRec	  *ScrollbarWidget;
typedef struct _ScrollbarClassRec *ScrollbarWidgetClass;

extern WidgetClass scrollbarWidgetClass;

_XFUNCPROTOBEGIN

extern void XawScrollbarSetThumb(
#if NeedFunctionPrototypes
    Widget		/* scrollbar */,
#if NeedWidePrototypes
    /* float */ double	/* top */,
    /* float */	double	/* shown */
#else
    float		/* top */,
    float		/* shown */
#endif
#endif		 
);

_XFUNCPROTOEND

#endif /* _Scrollbar_h */
