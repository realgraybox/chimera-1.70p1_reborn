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

#ifndef _Add3dExt_h
#define _Add3dExt_h

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

*/

#define XtNhighlightColor		"highlightColor"
#define XtNshadowColor			"shadowColor"
#define XtNbuttonBorderWidth		"buttonBorderWidth"

typedef struct _Add3dExtClassRec	*Add3dExtWidgetClass;
typedef struct _Add3dExtRec		*Add3dExtWidget;

extern WidgetClass add3dExtWidgetClass;

/******************************************************************
 *
 * Exported class methods
 *
 *****************************************************************/

/* Add3dExt has the following drawing methods :
 *
 *  XawFlatRectangle()	 : Draw a flat rectangle to hide the border
 *  XawRaisedRectangle() : Draw a rised 3D rectangle
 *  XawSunkenRectangle() : Draw a sunken 3D rectangle
 *  XawFlatButton()	 : Draw a flat button
 *  XawRaisedButton()    : Draw a raised 3D styled button
 *  XawSunkenButton()    : Draw a sunken 3D styled button
 *
 * All functions has the following arguments:
 *
 *  Arguments	  : w		 - the widget
 *		    x,y		 - origin of the rectangle
 *		    width, height- Dimension of the rectangle
 *  Returns	  : none
 */

_XFUNCPROTOBEGIN

extern void XawFlatRectangle(
#if NeedFunctionPrototypes
	Widget,				/* w    */
	int, int,			/* x, y */
	unsigned int, unsigned int	/* width, height */
#endif
);

extern void XawRaisedRectangle(
#if NeedFunctionPrototypes
	Widget,				/* w    */
	int, int,			/* x, y */
	unsigned int, unsigned int	/* width, height */
#endif
);

extern void XawSunkenRectangle(
#if NeedFunctionPrototypes
	Widget,				/* w    */
	int, int,			/* x, y */
	unsigned int, unsigned int	/* width, height */
#endif
);

extern void XawFlatButton(
#if NeedFunctionPrototypes
	Widget,				/* w    */
	int, int,			/* x, y */
	unsigned int, unsigned int	/* width, height */
#endif
);

extern void XawRaisedButton(
#if NeedFunctionPrototypes
	Widget,				/* w    */
	int, int,			/* x, y */
	unsigned int, unsigned int	/* width, height */
#endif
);

extern void XawSunkenButton(
#if NeedFunctionPrototypes
	Widget,				/* w    */
	int, int,			/* x, y */
	unsigned int, unsigned int	/* width, height */
#endif
);

_XFUNCPROTOEND

#endif /* _Add3dExt_h */
