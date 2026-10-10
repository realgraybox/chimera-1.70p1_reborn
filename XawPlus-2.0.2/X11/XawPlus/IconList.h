/*
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
 *
 * This software based completely upon the sources of the Athena List Widget,
 * provided by the MIT. It has only some minor changes to support pixmaps as
 * Icons for the list entries. All supported functions of this widget are
 * renamed from XawList... to XawIconList... to avoid conflicts with the original
 * list widget. The class name changes to IconList. The function XawIconListChange()
 * get some additional parameters to supply width, height and depth of the icons.
 *
 * The following ressources are added or changed:
 *
 * list              List		XawIconList*    NULL
 * iconHeight	     Height		Dimension	0
 * iconWidth	     Width		Dimension	0
 * iconDepth	     Depth		Dimension	1
 * 
 * The list is now an array of strings and pixmaps. It is not required to support
 * all pixmap entries. Initialize unused pixmap entries with `XtUnspecifiedPixmap'.
 * If the resources iconWidth and/or iconHeight are set to 0 the widget don't
 * display any icon. 
 * At this time, the icon is used as `left bitmap'. I plan to realize a new
 * ressource to define the position of the icon. Possible values are `left
 * bitmap', `right bitmap', `top bitmap' and `bottom bitmap'.
 * 
 * Roland Krause, germany, in spring 1996
 */

#ifndef _XawIconList_h
#define _XawIconList_h

/***********************************************************************
 *
 * Icon List Widget
 *
 ***********************************************************************/

#include <X11/XawPlus/Simple.h>
#include <X11/Xfuncproto.h>

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 OBJECT:
 destroyCallback     Callback		Pointer		NULL

 RECTANGLE:
 borderWidth	     BorderWidth	Dimension	1
 height		     Height		Dimension	0
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	0
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

 ICONLIST:
 callback            Callback           XtCallbackList  NULL
 pasteBuffer         Boolean            Boolean         False
 foreground	     Foreground		Pixel		XtDefaultForeground
 font		     Font		XFontStruct*	XtDefaultFont
 internalHeight	     Height		Dimension	2
 internalWidth	     Width		Dimension	4
 defaultColumns      Columns            int             2
 columnSpacing       Spacing            Dimension       6
 rowSpacing          Spacing            Dimension       4
 forceColumns        Columns            Boolean         False
 verticalList        Boolean            Boolean         False
 list                List		XawIconList*    NULL
 longest             Longest            int             0
 numberStrings       NumberStrings      int             0
 iconHeight	     Height		Dimension	0
 iconWidth	     Width		Dimension	0
 iconDepth	     Depth		Dimension	1

 */

/*
 * Value returned when there are no highlighted objects. 
 */

#define XAW_LIST_NONE -1	

#define XtCList "List"
#define XtCSpacing "Spacing"
#define XtCColumns "Columns"
#define XtCLongest "Longest"
#define XtCNumberStrings "NumberStrings"

#define XtNcursor		"cursor"
#define XtNcolumnSpacing	"columnSpacing"
#define XtNdefaultColumns	"defaultColumns"
#define XtNforceColumns		"forceColumns"
#define XtNlist			"list"
#define XtNiconWidth		"iconWidth"
#define XtNiconHeight		"iconHeight"
#define XtNiconDepth		"iconDepth"
#define XtNlongest		"longest"
#define XtNnumberStrings	"numberStrings"
#define XtNpasteBuffer		"pasteBuffer"
#define XtNrowSpacing		"rowSpacing"
#define XtNverticalList		"verticalList"
 
/* Class record constants */

extern WidgetClass iconListWidgetClass;

typedef struct _IconListClassRec *IconListWidgetClass;
typedef struct _IconListRec      *IconListWidget;

/* The list return structure. */

typedef struct _XawIconListReturnStruct {
  String string;
  int list_index;
} XawIconListReturnStruct;

/* The IconList structure */

typedef struct _XawIconList {
  Pixmap bitmap;	/* the icon, used as left bitmap */
  Pixmap clipMask;	/* drawing mask for the icon */
  String string;	/* text for the list entry */
} XawIconList;

/******************************************************************
 *
 * Exported Functions
 *
 *****************************************************************/

_XFUNCPROTOBEGIN

/*	Function Name: XawIconListChange.
 *	Description: Changes the list being used and shown.
 *	Arguments: w - the list widget.
 *                 list - the new list.
 *                 nitems - the number of items in the list.
 *                 longest - the length (in Pixels) of the longest element
 *                           in the list.
 *                 resize - if TRUE the the list widget will
 *                          try to resize itself.
 *	Returns: none.
 *      NOTE:      If nitems of longest are <= 0 then they will be calculated.
 *                 If nitems is <= 0 then the list needs to be NULL terminated.
 */

extern void XawIconListChange(
#if NeedFunctionPrototypes
    Widget		/* w */,
    XawIconList*	/* list */,
    int			/* no of items in list */,
    int			/* longest string */,
    int			/* icon width  */,
    int			/* icon height */,
    int			/* icon depth  */,
#if NeedWidePrototypes
    /* Boolean */ int	/* resize */
#else
    Boolean		/* resize */
#endif
#endif
);

/*	Function Name: XawIconListUnhighlight
 *	Description: unlights the current highlighted element.
 *	Arguments: w - the widget.
 *	Returns: none.
 */

extern void XawIconListUnhighlight(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

/*	Function Name: XawIconListHighlight
 *	Description: Highlights the given item.
 *	Arguments: w    - the list widget.
 *                 item - the item to highlight.
 *	Returns: none.
 */

extern void XawIconListHighlight(
#if NeedFunctionPrototypes
    Widget		/* w */,
    int			/* item */
#endif
);


/*	Function Name: XawIconListShowCurrent
 *	Description: returns the currently highlighted object.
 *	Arguments: w - the list widget.
 *	Returns: the info about the currently highlighted object.
 */

extern XawIconListReturnStruct * XawIconListShowCurrent(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

_XFUNCPROTOEND

#endif /* _XawIconList_h */
/* DON'T ADD STUFF AFTER THIS #endif */
