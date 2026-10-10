/* $XConsortium: Dialog.h,v 1.26 91/07/22 19:05:10 converse Exp $ */

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

Cleaned up the resource description: Roland Krause 1997

******************************************************************/

#ifndef _XawDialog_h
#define _XawDialog_h

#include <X11/XawPlus/Form.h>
#include <X11/Xfuncproto.h>

/***********************************************************************
 *
 * Dialog Widget
 *
 ***********************************************************************/

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 OBJECT:
 destroyCallback     Callback		Pointer	  	NULL

 RECTANGLE:
 borderWidth	     BorderWidth	Dimension	1
 sensitive	     Sensitive		Boolean		True
 height		     Height		Dimension	computed at create
 width		     Width		Dimension	computed at create
 x		     Position		Position	0
 y		     Position		Position	0

 CORE:
 border		     BorderColor	Pixel		XtDefaultForeground
 background	     Background		Pixel		grey75
 mappedWhenManaged   MappedWhenManaged	Boolean		True

 FORM:
 defaultDistance     Thickness		int		4

 DIALOG:
 icon		     Icon		Pixmap		None
 label		     Label		String		NULL
 value		     Value		String		NULL

*/

#define XtCIcon "Icon"
#define XtNicon "icon"

typedef struct _DialogClassRec	*DialogWidgetClass;
typedef struct _DialogRec	*DialogWidget;

extern WidgetClass dialogWidgetClass;

_XFUNCPROTOBEGIN

extern void XawDialogAddButton(
#if NeedFunctionPrototypes
    Widget		/* dialog */,
    _Xconst char*	/* name */,
    XtCallbackProc	/* function */,
    XtPointer		/* client_data */
#endif
);

extern char *XawDialogGetValueString(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

_XFUNCPROTOEND

#endif /* _XawDialog_h */
/* DON'T ADD STUFF AFTER THIS #endif */
