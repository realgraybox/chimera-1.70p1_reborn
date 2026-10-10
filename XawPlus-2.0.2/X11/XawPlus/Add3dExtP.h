
/*****************************************************************
 *
 * 3D extension widget
 * This widget adds colors and drawing methods for 3D effects
 * to the core widget.
 *
 * Author: Roland Krause 1996. <Rokrause@aol.com>
 * Calling concept of drawing methods changed: Roland Krause 1999
 *
 *****************************************************************/

#ifndef _Add3dExtP_h
#define _Add3dExtP_h

#include <X11/Xfuncproto.h>
#include <X11/XawPlus/Add3dExt.h>


typedef struct {
    int Dummy;
} Add3dExtClassPart;

typedef struct _Add3dExtClassRec {
    CoreClassPart	core_class;
    Add3dExtClassPart	add3d_class;
} Add3dExtClassRec;

extern Add3dExtClassRec add3dExtClassRec;


typedef struct {
    /* resources */
    Pixel	highlightColor,	/* Colors for 3D effects */
		shadowColor;
    Dimension   borderWidth;	/* Border width for toggles, buttons ... */

    /* private state */

    GC		highlightGC,	/* GC's for drawing buttons */
		backgrGC,
		shadowGC;
} Add3dExtPart;


typedef struct _Add3dExtRec {
    CorePart		core;
    Add3dExtPart	add3dExt;
} Add3dExtRec;

#endif /* _Add3dExtP_h */
