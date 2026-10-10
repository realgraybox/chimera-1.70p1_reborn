
/***********************************************************************
 *
 * Drawing Area Widget Private Data
 *
 * Author : Roland Krause, germany, summer 1996
 *
 ***********************************************************************/

#ifndef _DrawingAreaP_h
#define _DrawingAreaP_h

#include <X11/XawPlus/Add3dExtP.h>
#include <X11/XawPlus/SimpleP.h>
#include <X11/XawPlus/DrawingArea.h>

/* New fields for the drawing area widget class record */

typedef struct {int foo;} DrawingAreaClassPart;


/* Full class record declaration */

typedef struct _DrawingAreaClassRec {
    CoreClassPart		core_class;
    Add3dExtClassPart		add3d_class;
    SimpleClassPart		simple_class;
    DrawingAreaClassPart	drawingArea_class;
} DrawingAreaClassRec;

extern DrawingAreaClassRec drawingAreaClassRec;


/* New fields for the DrawingArea widget record */

typedef struct {

    /* private data */

    Pixmap	 BackingStore;	/* Backing store for the drawing area */
    GC		 gc;		/* Default graphics context */
    Dimension	 max_width,	/* Width and height for resizing */
		 max_height;
} DrawingAreaPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _DrawingAreaRec {
    CorePart		core;
    Add3dExtPart	add3dExt;
    SimplePart		simple;
    DrawingAreaPart	draw;
} DrawingAreaRec;

#endif /* _DrawingAreaP_h */
