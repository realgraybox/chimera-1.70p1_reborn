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
 * Author:  Chris D. Peterson, MIT X Consortium
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
 *
 * 3D extensions added
 */

#ifndef _XawIconListP_h
#define _XawIconListP_h

/***********************************************************************
 *
 * List Widget Private Data
 *
 ***********************************************************************/

#include <X11/XawPlus/Add3dExtP.h>
#include <X11/XawPlus/SimpleP.h>
#include <X11/XawPlus/IconList.h>

#define NO_HIGHLIGHT            XAW_LIST_NONE
#define OUT_OF_RANGE            -1
#define OKAY                     0

/* New fields for the IconList widget class record */

typedef struct {int foo;} IconListClassPart;

/* Full class record declaration */
typedef struct _IconListClassRec {
    CoreClassPart	core_class;
    Add3dExtClassPart	add3d_class;
    SimpleClassPart	simple_class;
    IconListClassPart	list_class;
} IconListClassRec;

extern IconListClassRec iconListClassRec;

/* New fields for the IconList widget record */

typedef struct {
    /* resources */
    Pixel	foreground;
    Dimension	internal_width,
        	internal_height,
		icon_width,
		icon_height,
		icon_depth,
                column_space,
                row_space;
    int         default_cols;
    Boolean     force_cols,
                paste,
                vertical_cols;
    int         longest;
    int         nitems;		/* number of items in the list. */
    XFontStruct	*font;
    XawIconList *list;
    XtCallbackList  callback;

    /* private state */

    int         is_highlighted,	/* set to the item currently highlighted. */
                highlight,	/*set to the item that should be highlighted.*/
                col_width,	/* width of each column. */
                row_height,	/* height of each row. */
                nrows,		/* number of rows in the list. */
                ncols;		/* number of columns in the list. */
    GC		normgc,		/* a couple o' GC's. */
		iconGc,		/* for displaying icons */
                revgc,		/* for highlighting */
                graygc;		/* used when inactive. */
    XawIconList DummyList;	/* use this list if no resource defined */

} IconListPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _IconListRec {
    CorePart		core;
    Add3dExtPart	add3dExt;
    SimplePart		simple;
    IconListPart	list;
} IconListRec;

#endif /* _XawIconListP_h */
