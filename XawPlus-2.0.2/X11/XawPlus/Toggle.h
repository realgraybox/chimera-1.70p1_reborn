/*
 * $XConsortium: Toggle.h,v 1.13 91/05/04 18:59:01 rws Exp $
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
 * ToggleP.h - Private definitions for Toggle widget
 *
 * Author: Chris D. Peterson
 *         MIT X Consortium
 *         kit@expo.lcs.mit.edu
 *  
 * Date:   January 12, 1989
 */

#ifndef _XawToggle_h
#define _XawToggle_h

/***********************************************************************
 *
 * Toggle Widget
 *
 ***********************************************************************/

#include <X11/XawPlus/Command.h>
#include <X11/Xfuncproto.h>

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
 
 TOGGLE:
 radioGroup          RadioGroup         Widget          NULL              +
 radioData           RadioData          Pointer         (caddr_t) Widget  ++
 state               State              Boolean         Off


+ To use the toggle as a radio toggle button, set this resource to point to
  any other widget in the radio group.

++ This is the data returned from a call to XtToggleGetCurrent, by default
   this is set to the name of toggle widget.

*/

/*
 * These should be in StringDefs.h but aren't so we will define
 * them here if they are needed.
 */


#define XtCWidget "Widget"
#define XtCState "State"
#define XtCRadioGroup "RadioGroup"
#define XtCRadioData "RadioData"

#ifndef _XtStringDefs_h_
#define XtRWidget "Widget"
#endif

#define XtNstate "state"
#define XtNradioGroup "radioGroup"
#define XtNradioData "radioData"

extern WidgetClass               toggleWidgetClass;

typedef struct _ToggleClassRec   *ToggleWidgetClass;
typedef struct _ToggleRec        *ToggleWidget;


/************************************************************
 * 
 * Public Functions
 *
 ************************************************************/

_XFUNCPROTOBEGIN
   
/*	Function Name: XawToggleChangeRadioGroup
 *	Description: Allows a toggle widget to change radio lists.
 *	Arguments: w - The toggle widget to change lists.
 *                 radio_group - any widget in the new list.
 *	Returns: none.
 */

extern void XawToggleChangeRadioGroup(
#if NeedFunctionPrototypes
    Widget		/* w */,
    Widget		/* radio_group */
#endif
);

/*	Function Name: XawToggleGetCurrent
 *	Description: Returns the RadioData associated with the toggle
 *                   widget that is currently active in a toggle list.
 *	Arguments: radio_group - any toggle widget in the toggle list.
 *	Returns: The XtNradioData associated with the toggle widget.
 */

extern XtPointer XawToggleGetCurrent(
#if NeedFunctionPrototypes
    Widget		/* radio_group */
#endif
);

/*	Function Name: XawToggleSetCurrent
 *	Description: Sets the Toggle widget associated with the
 *                   radio_data specified.
 *	Arguments: radio_group - any toggle widget in the toggle list.
 *                 radio_data - radio data of the toggle widget to set.
 *	Returns: none.
 */

extern void XawToggleSetCurrent(
#if NeedFunctionPrototypes
    Widget		/* radio_group */,
    XtPointer		/* radio_data */
#endif
);
 
/*	Function Name: XawToggleUnsetCurrent
 *	Description: Unsets all Toggles in the radio_group specified.
 *	Arguments: radio_group - any toggle widget in the toggle list.
 *	Returns: none.
 */

extern void XawToggleUnsetCurrent(
#if NeedFunctionPrototypes
    Widget		/* radio_group */
#endif
);

_XFUNCPROTOEND

#endif /* _XawToggle_h */
/* DON'T ADD STUFF AFTER THIS */
