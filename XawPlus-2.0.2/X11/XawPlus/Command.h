/* $XConsortium: Command.h,v 1.27 91/07/28 18:51:35 converse Exp $ */

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
Helper function added: Roland Krause 1998

Note:	At this time, I only support rectangular buttons. So the
	the resources 'shapeStyle' and 'cornerRoundPercent' are
	not longer supported.

******************************************************************/

#ifndef _XawCommand_h
#define _XawCommand_h

#include <X11/XawPlus/Label.h>

/* Command widget resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 OBJECT:
 destroyCallback     Callback		Pointer		NULL

 RECTANGLE:
 borderWidth	     BorderWidth	Dimension	0
 height		     Height		Dimension	text height
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	text width
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
 cursorName	     Cursor		String		NULL
 pointerColor        Foreground         Pixel           XtDefaultForeground
 pointerColorBackground Background      Pixel           XtDefaultBackground
 insensitiveBorder   Insensitive	Pixmap		Gray

 LABEL:
 font		     Font		XFontStruct*	XtDefaultFont
 encoding	     Encoding		unsigned char	XawTextEncoding8bit
 foreground	     Foreground		Pixel		XtDefaultForeground
 internalHeight	     Height		Dimension	2
 internalWidth	     Width		Dimension	4
 justify	     Justify		XtJustify	XtJustifyCenter
 resize		     Resize		Boolean		True
 label		     Label		String		NULL
 leftBitmap	     LeftBitmap		Pixmap		None
 bitmap		     Pixmap		Pixmap		None

 COMMAND:
 helpText	     Label		String		NULL
 helpEncoding	     Encoding		unsigned char	XawTextEncoding8bit
 helpBackground	     Background		Pixel		LightGoldenrodYellow
 useHelp	     Boolean		Boolean		True
 highlightThickness  Thickness		Dimension	2
 highlightMode       Boolean		Boolean		True
 callback	     Callback		XtCallbackList	NULL
 translations	     Translations	TranslationTable see doc or source
 accelerators	     Accelerators	AcceleratorTable NULL
 ancestorSensitive   AncestorSensitive	Boolean		True

*/

#define XtNhighlightThickness	"highlightThickness"
#define XtNhighlightMode	"highlightMode"
#define XtNuseHelp		"useHelp"
#define XtNhelpText		"helpText"
#define XtNhelpEncoding		"helpEncoding"
#define XtNhelpBackground	"helpBackground"

extern WidgetClass     commandWidgetClass;

typedef struct _CommandClassRec   *CommandWidgetClass;
typedef struct _CommandRec        *CommandWidget;

#endif /* _XawCommand_h */
/* DON'T ADD STUFF AFTER THIS */
