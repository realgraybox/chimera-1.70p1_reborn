/*
 * $XConsortium: SmeBSB.h,v 1.5 89/12/11 15:20:14 kit Exp $
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

/*
 * SmeBSB.h - Public Header file for SmeBSB object.
 *
 * This is the public header file for the Athena BSB Sme object.
 * It is intended to be used with the simple menu widget.  This object
 * provides bitmap - string - bitmap style entries.
 *
 * Date:    April 3, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 *
 * 3D extensions added: Roland Krause 1997
 */

#ifndef _SmeBSB_h
#define _SmeBSB_h

#include <X11/Xmu/Converters.h>
#include <X11/XawPlus/Sme.h>

/****************************************************************
 *
 * SmeBSB object
 *
 ****************************************************************

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 OBJECT:
 destroyCallback     Callback		Pointer		NULL

 RECTANGLE:
 borderWidth	     BorderWidth	Dimension	0
 height		     Height		Dimension	0
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	0
 x		     Position		Position	0
 y		     Position		Position	0

 SME:
 callback            Callback           Callback        NULL
 highlightColor	     Background		Pixel		grey90
 shadowColor	     Background		Pixel		grey40

 SMEBSB:
 foreground          Foreground         Pixel           XtDefaultForeground
 font                Font               XFontStruct *   XtDefaultFont
 label               Label              String          Name of entry
 justify	     Justify		Justify	        XtJustifyLeft
 vertSpace           VertSpace          int             25
 leftMargin          HorizontalMargins  Dimension       4
 rightMargin         HorizontalMargins  Dimension       4
 leftBitmap          LeftBitmap         Pixmap          None
 leftClipMask	     LeftBitmap		Pixmap		None
 rightBitmap         RightBitmap        Pixmap          None
 rightClipMask	     RightBitmap	Pixmap		None
*/

typedef struct _SmeBSBClassRec    *SmeBSBObjectClass;
typedef struct _SmeBSBRec         *SmeBSBObject;

extern WidgetClass smeBSBObjectClass;

#define XtNleftBitmap	"leftBitmap"
#define XtNleftClipMask "leftClipMask"
#define XtNrightBitmap	"rightBitmap"
#define XtNrightClipMask "rightClipMask"
#define XtNleftMargin	"leftMargin"
#define XtNrightMargin	"rightMargin"
#define XtNvertSpace	"vertSpace"

#define XtCLeftBitmap	"LeftBitmap"
#define XtCHorizontalMargins "HorizontalMargins"
#define XtCRightBitmap	"RightBitmap"
#define XtCVertSpace	"VertSpace"

#endif /* _SmeBSB_h */
