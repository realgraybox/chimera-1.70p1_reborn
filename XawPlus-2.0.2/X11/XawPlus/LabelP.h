/*
* $XConsortium: LabelP.h,v 1.27 91/06/22 19:34:58 rws Exp $
*/


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

3D extensions added : Roland Krause 1996, 1997

******************************************************************/

/* 
 * LabelP.h - Private definitions for Label widget
 * 
 */

#ifndef _XawLabelP_h
#define _XawLabelP_h

/***********************************************************************
 *
 * Label Widget Private Data
 *
 ***********************************************************************/

#include <X11/XawPlus/Label.h>
#include <X11/XawPlus/Add3dExtP.h>
#include <X11/XawPlus/SimpleP.h>

/* New fields for the Label widget class record */

typedef struct {int foo;} LabelClassPart;

/* Full class record declaration */
typedef struct _LabelClassRec {
    CoreClassPart	core_class;
    Add3dExtClassPart	add3d_class;
    SimpleClassPart	simple_class;
    LabelClassPart	label_class;
} LabelClassRec;

extern LabelClassRec labelClassRec;

/* New fields for the Label widget record */
typedef struct {
    /* resources */
    Boolean	resize;
    Pixel	foreground;
    XFontStruct	*font;
    unsigned char encoding;
    char	*label;
    XtJustify	justify;
    Dimension	internal_width;
    Dimension	internal_height;
    Pixmap	pixmap;
    Pixmap	left_bitmap;

    /* private state */
    Pixmap	xpm_clipmask;		/* for XPM clipmask support */
    GC		normal_GC;
    GC          gray_GC;		/* for displaying insensitive text labels */
    GC		lgray_GC;
    Position	label_x;
    Position	label_y;
    Dimension	label_width;
    Dimension	label_height;
    Dimension	label_len;
    int		lbm_y, lbm_x;		/* Position of the left bitmap in the label */

    unsigned int lbm_width, lbm_height;	/* size of left bitmap */
    unsigned int label_depth;		/* depth of bitmap or left bitmap */
} LabelPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _LabelRec {
    CorePart	 core;
    Add3dExtPart add3dExt;
    SimplePart	 simple;
    LabelPart	 label;
} LabelRec;

#define LEFT_OFFSET(lw) ((lw)->label.left_bitmap \
			 ? (lw)->label.lbm_width + (lw)->label.internal_width \
			 : 0)

#endif /* _XawLabelP_h */
