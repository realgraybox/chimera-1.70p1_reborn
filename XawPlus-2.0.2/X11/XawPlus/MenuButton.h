/*
 * $XConsortium: MenuButton.h,v 1.7 89/12/11 14:57:44 kit Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/***********************************************************************
 *
 * MenuButton Widget
 *
 ***********************************************************************/

/*
 * MenuButton.h - Public Header file for MenuButton widget.
 *
 * This is the public header file for the Athena MenuButton widget.
 * It is intended to provide an easy method of activating pulldown menus.
 *
 * Date:    May 2, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _XawMenuButton_h
#define _XawMenuButton_h

#include <X11/XawPlus/Command.h>

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 OBJECT:
 destroyCallback     Callback		Pointer		NULL

 RECTANGLE:
 borderWidth	     BorderWidth	Dimension	1
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
 highlightThickness  Thickness		Dimension	0 if shaped, else 2
 callback	     Callback		XtCallbackList	NULL
 translations	     Translations	TranslationTable see doc or source
 accelerators	     Accelerators	AcceleratorTable NULL
 ancestorSensitive   AncestorSensitive	Boolean		True

 MENUBUTTON:
 menuName            MenuName           String          "menu"

*/

#define XtNmenuName "menuName"
#define XtCMenuName "MenuName"

extern WidgetClass     menuButtonWidgetClass;

typedef struct _MenuButtonClassRec   *MenuButtonWidgetClass;
typedef struct _MenuButtonRec        *MenuButtonWidget;

#endif /* _XawMenuButton_h --  DON'T ADD STUFF AFTER THIS */
