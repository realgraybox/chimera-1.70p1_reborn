
#ifndef _DrawingArea_h
#define _DrawingArea_h

/****************************************************************
 *
 * Drawing Area Widget
 * 
 * This widget adds a backing store machanism to the simple
 * widget. So it becomes usable as a drawing area for any
 * painting. Note that all Xlib painting functions are replaced
 * by a version, that works with a widget.
 *
 * Author : Roland Krause, germany, summer 1996
 *
 ****************************************************************/

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
 highlightColor	     Background		Pixel		grey90 *)
 shadowColor	     Background		Pixel		grey40 *)
 buttonBorderWidth   Width		Dimension	2      *)

 SIMPLE:
 cursor		     Cursor		Cursor		None
 cursorName	     Cursor		String		NULL
 pointerColor        Foreground         Pixel           XtDefaultForeground
 pointerColorBackground Background      Pixel           XtDefaultBackground
 insensitiveBorder   Insensitive	Pixmap		Gray

 DRAWING AREA: 	     No additional resources

 *) Resource has no meaning for this widget

*/

typedef struct _DrawingAreaClassRec	*DrawingAreaWidgetClass;
typedef struct _DrawingAreaRec		*DrawingAreaWidget;

extern WidgetClass drawingAreaWidgetClass;

/******************************************************************
 *
 * Exported drawing functions
 *
 *****************************************************************/

_XFUNCPROTOBEGIN

/*  Function Name : XawClearWindow
 *  Description   : clear the drawing area
 *  Arguments	  : w - drawing area widget
 *  Returns	  : none
 */
extern void XawClearWindow(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

/*  Function Name : XawDrawPoint
 *  Description   : draw a point using the given gc in the drawing area
 *  Arguments	  : w    - drawing area widget
 *		    gc   - graphics context
 *		    x, y - coordinates of the point
 *  Returns	  : none
 */
extern void XawDrawPoint(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int		/* coordinates */
#endif
);

/*  Function Name : XawDrawPoints
 *  Description   : draw points using the given gc in the drawing area
 *  Arguments	  : w        - drawing area widget
 *		    gc       - graphics context
 *		    points[] - list of point coordinates
 * 		    npoints  - number of points
 *		    mode     - drawing mode: Relative to origin or previous point
 *  Returns	  : none
 */
extern void XawDrawPoints(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XPoint *,		/* list of point coordinates */
    int,		/* number of points */
    int			/* drawing mode */
#endif
);

/*  Function Name : XawDrawLine
 *  Description   : draw a line using the given gc in the drawing area
 *  Arguments	  : w	   - drawing area widget
 * 		    gc 	   - graphics context
 * 		    x1, y1 - starting point
 *		    x1, y2 - end point
 *  Returns	  : none
 */
extern void XawDrawLine(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int,		/* coordinates starting point */
    int, int		/* coordinates end point */
#endif
);

/*  Function Name : XawDrawLines
 *  Description   : draw connected lines using the given gc in the drawing area
 *  Arguments	  : w        - drawing area widget
 *		    gc       - graphics context
 *		    points[] - list of point coordinates
 * 		    npoints  - number of points
 *		    mode     - drawing mode: Relative to origin or previous point
 *  Returns	  : none
 */
extern void XawDrawLines(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XPoint *,		/* list of point coordinates */
    int,		/* number of points */
    int			/* drawing mode */
#endif
);

/*  Function Name : XawDrawSegments
 *  Description   : draw unconnected lines in the drawing area
 *  Arguments	  : w		- drawing area widget
 *		    gc		- graphics context
 *		    segments[]  - list of line coordinates
 *		    n		- number of segments
 *  Returns	  : none
 */
extern void XawDrawSegments(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XSegment *,		/* list of line coordinates */
    int			/* number of segments */
#endif
);

/*  Function Name : XawDrawRectangle
 *  Description   : draw an rectangle
 *  Arguments	  : w		  - drawing area widget
 *		    gc		  - graphics context
 *		    x,y		  - upper left edge of the rectangle
 *		    width, height - widht and height of the rectangle
 *  Returns	  : none
 */
extern void XawDrawRectangle(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int,		/* upper left edge */
    unsigned int,	/* widht and height */
    unsigned int
#endif
);

/*  Function Name : XawDrawRectangles
 *  Description   : draw rectangles
 *  Arguments	  : w		- drawing area widget
 *		    gc		- graphics context
 *		    rectangles  - list of rectangle coordinates
 *		    n		- number of rectangles
 *  Returns	  : none
 */
extern void XawDrawRectangles(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XRectangle *,	/* rectangle coordinates */
    int			/* number of rectangles */
#endif
);

/*  Function Name : XawDrawArc
 *  Description   : draw a circle or an elipsoid or a part of it
 *  Arguments	  : w		   - drawing area widget
 *		    gc		   - graphics context
 *		    x,y		   - center position of the circle/elipsoid
 *		    width, height  - widht and height of the circle/elipsoid
 *		    angle1, angle2 - draw from angle 1 to angle 2
 *  Returns	  : none
 */
extern void XawDrawArc(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int,		/* center */
    unsigned int,	/* width */
    unsigned int,	/* height */
    int, int		/* start angle, end angle */
#endif
);

/*  Function Name : XawDrawArcs
 *  Description   : draw n circles or elipsoids
 *  Arguments	  : w	 - drawing area widget
 *		    gc	 - graphics context
 *		    arcs - parameters of the circles/elipsoids
 *		    n	 - number of circles
 *  Returns	  : none
 */
extern void XawDrawArcs(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XArc *,		/* list of parameters */
    int			/* number of circles */
#endif
);

/*  Function Name : XawFillRectangle
 *  Description   : draw a filled rectangle
 *  Arguments	  : w		  - drawing area widget
 *		    gc		  - graphics context
 *		    x,y		  - upper left edge of the rectangle
 *		    width, height - widht and height of the rectangle
 *  Returns	  : none
 */
extern void XawFillRectangle(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int,		/* upper left edge */
    unsigned int,	/* widht and height */
    unsigned int
#endif
);

/*  Function Name : XawFillRectangles
 *  Description   : draw n filled rectangles
 *  Arguments	  : w		- drawing area widget
 *		    gc		- graphics context
 *		    rectangles  - list of rectangle coordinates
 *		    n		- number of rectangles
 *  Returns	  : none
 */
extern void XawFillRectangles(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XRectangle *,	/* rectangle coordinates */
    int			/* number of rectangles */
#endif
);

/*  Function Name : XawFillArc
 *  Description   : draw a filled circle or elipsoid
 *  Arguments	  : w		   - drawing area widget
 *		    gc		   - graphics context
 *		    x,y		   - center position of the circle/elipsoid
 *		    width, height  - widht and height of the circle/elipsoid
 *		    angle1, angle2 - draw from angle 1 to angle 2
 *  Returns	  : none
 */
extern void XawFillArc(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int,		/* center */
    unsigned int,	/* width */
    unsigned int,	/* height */
    int, int		/* start angle, end angle */
#endif
);

/*  Function Name : XawFillArcs
 *  Description   : draw n filled circles or elipsoids
 *  Arguments	  : w	 - drawing area widget
 *		    gc	 - graphics context
 *		    arcs - parameters of the circles/elipsoids
 *		    n	 - number of circles
 *  Returns	  : none
 */
extern void XawFillArcs(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XArc *,		/* list of parameters */
    int			/* number of circles */
#endif
);

/*  Function Name : XawFillPolygon
 *  Description   : draw a filled polygon
 *  Arguments	  : w	   - drawing area widget
 *		    gc	   - graphics context
 *		    points - point coordinates of the polygon
 *		    n	   - number of points
 *		    shape  - drawing mode complex, convex or nonconvex
 *		    mode   - relative or absolute coordinates
 *  Returns	  : none
 */
extern void XawFillPolygon(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    XPoint *,		/* list of points */
    int,		/* number of points */
    int,		/* drawing mode */
    int			/* relative or absolute coordinates */
#endif
);

/*****************************************************************
 *
 * Exported text drawing functions
 *
 *****************************************************************/

/*  Function Name : XawDrawString
 *  Description   : draw a string without filling the background
 *  Arguments	  : w	 - drawing area widget
 *		    gc	 - graphics context
 *		    x,y  - position of the string in the widget
 *		    str  - the text string
 *		    n	 - number of characters
 *  Returns	  : none
 */
extern void XawDrawString(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int,		/* string position */
    char *,		/* the string */
    int			/* number of chararcters */
#endif
);

/*  Function Name : XawDrawImageString
 *  Description   : draw a string with filling the background
 *  Arguments	  : w	 - drawing area widget
 *		    gc	 - graphics context
 *		    x,y  - position of the string in the widget
 *		    str  - the text string
 *		    n	 - number of characters
 *  Returns	  : none
 */
extern void XawDrawImageString(
#if NeedFunctionPrototypes
    Widget,		/* w */
    GC,			/* graphics context */
    int, int,		/* string position */
    char *,		/* the string */
    int			/* number of chararcters */
#endif
);

_XFUNCPROTOEND

#endif /* _DrawingArea_h */
