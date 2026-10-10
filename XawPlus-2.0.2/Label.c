/* $XConsortium: Label.c,v 1.92 91/07/24 16:56:28 converse Exp $ */

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

3D extensions and XPM support added : Roland Krause 1996, 1997

******************************************************************/

/*
 * Label.c - Label widget
 *
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/XawPlus/XawInit.h>
#include <X11/XawPlus/LabelP.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Drawing.h>
#include <stdio.h>
#include <ctype.h>

#define streq(a,b) (strcmp( (a), (b) ) == 0)
#define MULTI_LINE_LABEL 32767

#ifdef CRAY
#define WORD64
#endif

extern void XmuCvtStringToPixmap();	/* The XPM converter */

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

#define offset(field) XtOffsetOf(LabelRec, field)
static XtResource resources[] = {
    {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	offset(label.foreground), XtRString, XtDefaultForeground},
    {XtNfont,  XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	offset(label.font),XtRString, XtDefaultFont},
    {XtNlabel,  XtCLabel, XtRString, sizeof(String),
	offset(label.label), XtRString, NULL},
    {XtNencoding, XtCEncoding, XtRUnsignedChar, sizeof(unsigned char),
	offset(label.encoding), XtRImmediate, (XtPointer)XawTextEncoding8bit},
    {XtNjustify, XtCJustify, XtRJustify, sizeof(XtJustify),
	offset(label.justify), XtRImmediate, (XtPointer)XtJustifyCenter},
    {XtNinternalWidth, XtCWidth, XtRDimension,  sizeof(Dimension),
	offset(label.internal_width), XtRImmediate, (XtPointer)4},
    {XtNinternalHeight, XtCHeight, XtRDimension, sizeof(Dimension),
	offset(label.internal_height), XtRImmediate, (XtPointer)2},
    {XtNresize, XtCResize, XtRBoolean, sizeof(Boolean),
	offset(label.resize), XtRImmediate, (XtPointer)True},

    {XtNbitmap, XtCPixmap, XtRBitmap, sizeof(Pixmap),
	offset(label.pixmap), XtRImmediate, (XtPointer)None},

    {XtNleftBitmap, XtCLeftBitmap, XtRBitmap, sizeof(Pixmap),
       offset(label.left_bitmap), XtRImmediate, (XtPointer)None}
};
#undef offset

/* Prototypes of label methods */

static void Initialize();
static void Resize();
static void Redisplay();
static Boolean SetValues();
static void ClassInitialize();
static void Destroy();
static XtGeometryResult QueryGeometry();

LabelClassRec labelClassRec = {
  {
/* core_class fields */	
#define superclass		(&simpleClassRec)
    /* superclass	  	*/	(WidgetClass) superclass,
    /* class_name	  	*/	"Label",
    /* widget_size	  	*/	sizeof(LabelRec),
    /* class_initialize   	*/	ClassInitialize,
    /* class_part_initialize	*/	NULL,
    /* class_inited       	*/	FALSE,
    /* initialize	  	*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize		  	*/	XtInheritRealize,
    /* actions		  	*/	NULL,
    /* num_actions	  	*/	0,
    /* resources	  	*/	resources,
    /* num_resources	  	*/	XtNumber(resources),
    /* xrm_class	  	*/	NULLQUARK,
    /* compress_motion	  	*/	TRUE,
    /* compress_exposure  	*/	XtExposeCompressSeries,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest	  	*/	FALSE,
    /* destroy		  	*/	Destroy,
    /* resize		  	*/	Resize,
    /* expose		  	*/	Redisplay,
    /* set_values	  	*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus	 	*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private   	*/	NULL,
    /* tm_table		   	*/	NULL,
    /* query_geometry		*/	QueryGeometry,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
/* add3dExt fields initialization */
  {
    /* unused			*/	0
  },
/* Simple class fields initialization */
  {
    /* change_sensitive		*/	XtInheritChangeSensitive
  }
};
WidgetClass labelWidgetClass = (WidgetClass)&labelClassRec;
/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

#ifndef WORD64
#define TXT16 XChar2b
#else

#define TXT16 char

static XChar2b *buf2b;
static int buf2blen = 0;

_XawLabelWidth16(fs, str, n)
    XFontStruct *fs;
    char *str;
    int	n;
{
    int i;
    XChar2b *ptr;

    if (n > buf2blen) {
	buf2b = (XChar2b *)XtRealloc((char *)buf2b, n * sizeof(XChar2b));
	buf2blen = n;
    }
    for (ptr = buf2b, i = n; --i >= 0; ptr++) {
	ptr->byte1 = *str++;
	ptr->byte2 = *str++;
    }
    return XTextWidth16(fs, buf2b, n);
}

_XawLabelDraw16(dpy, d, gc, x, y, str, n)
    Display *dpy;
    Drawable d;
    GC gc;
    int x, y;
    char *str;
    int n;
{
    int i;
    XChar2b *ptr;

    if (n > buf2blen) {
	buf2b = (XChar2b *)XtRealloc((char *)buf2b, n * sizeof(XChar2b));
	buf2blen = n;
    }
    for (ptr = buf2b, i = n; --i >= 0; ptr++) {
	ptr->byte1 = *str++;
	ptr->byte2 = *str++;
    }
    XDrawString16(dpy, d, gc, x, y, buf2b, n);
}

#define XTextWidth16 _XawLabelWidth16
#define XDrawString16 _XawLabelDraw16

#endif /* WORD64 */

/*
 * Calculate width and height of displayed text in pixels
 */

static void SetTextWidthAndHeight(lw)
    LabelWidget lw;
{
    register XFontStruct *fs = lw->label.font;
    Window root;
    int x, y;
    unsigned int width, height, bw, depth;
    char *nl;

    if (lw->label.pixmap != None) {		/* bitmap defined ? */
	if (XGetGeometry(XtDisplay(lw), lw->label.pixmap, &root, &x, &y,
			 &width, &height, &bw, &depth)) {
	    lw->label.label_height = height;
	    lw->label.label_width  = width;
	    lw->label.label_depth  = depth;
	    return;
	}
    }
    lw->label.label_height = fs->max_bounds.ascent + fs->max_bounds.descent;
    if (lw->label.label == NULL) {
	lw->label.label_len = 0;
	lw->label.label_width = 0;
    }
    else if ((nl = index(lw->label.label, '\n')) != NULL) {
	char *label;
	lw->label.label_len = MULTI_LINE_LABEL;
	lw->label.label_width = 0;
	for (label = lw->label.label; nl != NULL; nl = index(label, '\n')) {
	    int width;

	    if (lw->label.encoding)
		width = XTextWidth16(fs, (TXT16*)label, (int)(nl - label)/2);
	    else
		width = XTextWidth(fs, label, (int)(nl - label));
	    if (width > (int)lw->label.label_width)
		lw->label.label_width = width;
	    label = nl + 1;
	    if (*label)
		lw->label.label_height +=
		    fs->max_bounds.ascent + fs->max_bounds.descent;
	}
	if (*label) {
	    int width = XTextWidth(fs, label, strlen(label));

	    if (lw->label.encoding)
		width = XTextWidth16(fs, (TXT16*)label, (int)strlen(label)/2);
	    else
		width = XTextWidth(fs, label, strlen(label));
	    if (width > (int) lw->label.label_width)
		lw->label.label_width = width;
	}
    } else {
	lw->label.label_len = strlen(lw->label.label);
	if (lw->label.encoding)
	    lw->label.label_width =
		XTextWidth16(fs, (TXT16*)lw->label.label,
			     (int) lw->label.label_len/2);
	else
	    lw->label.label_width =
		XTextWidth(fs, lw->label.label, (int) lw->label.label_len);
    }
}

/* Create required GCs
 */
static void GetnormalGC(lw)
    LabelWidget lw;
{
    XGCValues	values;

    values.foreground		= lw->label.foreground;
    values.background		= lw->core.background_pixel;
    values.font			= lw->label.font->fid;
    values.graphics_exposures 	= False;

    lw->label.normal_GC = XtGetGC((Widget)lw,
		             GCForeground|GCBackground|GCFont|GCGraphicsExposures,
		             &values);
}

static void GetgrayGCs(lw)
    LabelWidget lw;
{
    XGCValues	values;

    values.foreground = lw->add3dExt.shadowColor;
    values.background = lw->core.background_pixel;
    values.font	      = lw->label.font->fid;
    values.graphics_exposures = False;
    lw->label.gray_GC = XtGetGC((Widget)lw,
			  GCForeground|GCBackground|GCFont|GCGraphicsExposures,
			  &values);

    values.foreground  = lw->add3dExt.highlightColor;
    lw->label.lgray_GC = XtGetGC((Widget)lw,
			  GCForeground|GCBackground|GCFont|GCGraphicsExposures,
			  &values);

}

static void compute_bitmap_offsets(clw, lw)
LabelWidget lw, clw;
{
    /*
     * label will be displayed at (internal_width, internal_height + lbm_y)
     */
    if (lw->label.lbm_height != 0) {
	lw->label.lbm_y = (((int)clw->core.height) -
			   ((int) lw->label.internal_height * 2) -
			   ((int) lw->label.lbm_height)) / 2;
        lw->label.lbm_x = (int) lw->label.internal_width;
    }
    else {
	lw->label.lbm_y = 0;
    	lw->label.lbm_x = 0;
    }
}

/* Check if bitmaps or pixmaps in use and determine width, height and depth
 */
static void set_bitmap_info(clw, lw)
LabelWidget clw, lw;
{
    Window root;
    int x, y;
    unsigned int bw;

    if (!(lw->label.left_bitmap &&
	  XGetGeometry(XtDisplay(lw), lw->label.left_bitmap, &root, &x, &y,
	      &lw->label.lbm_width, &lw->label.lbm_height, &bw, &lw->label.label_depth))) {
	lw->label.lbm_width = lw->label.lbm_height = 0;
    }
    compute_bitmap_offsets(clw, lw);
}

/***********************************************************************
 *
 * Methods of this Widget
 *
 ***********************************************************************/

/***********************************************************************
 *
 * Class initialize method:	Initialize widget class:
 * 				Install required converters
 *
 ***********************************************************************/

static void ClassInitialize()
{
  static XtConvertArgRec xpmCvtArg[] = {
    {XtWidgetBaseOffset,(XtPointer)XtOffsetOf(LabelRec,core.screen),sizeof(Screen *)},
    {XtWidgetBaseOffset,(XtPointer)XtOffsetOf(LabelRec,core.colormap),sizeof(Colormap)},
    {XtWidgetBaseOffset,(XtPointer)XtOffsetOf(LabelRec,label.xpm_clipmask),sizeof(Pixmap *)}
  };

  XawInitializeWidgetSet();
  XtAddConverter(XtRString,XtRJustify,XmuCvtStringToJustify, NULL, 0);
  XtAddConverter(XtRString,XtRBitmap,XmuCvtStringToPixmap,xpmCvtArg,XtNumber(xpmCvtArg));
}

/***********************************************************************
 *
 * Initialize method:	Initialize widget instance:
 * 			Get GCs for drawing bitmaps, pixmaps or text.
 *			Determine required size of this label and call
 *			resize method of the underlying core widget.
 *
 ***********************************************************************/

static void Initialize(request, new)
 Widget request, new;
{
    LabelWidget lw = (LabelWidget) new;

    if (lw->label.label == NULL) lw->label.label = XtNewString(lw->core.name);
    else			 lw->label.label = XtNewString(lw->label.label);

    GetnormalGC(lw);
    GetgrayGCs(lw);

    SetTextWidthAndHeight(lw);

    if (lw->core.height == 0)
        lw->core.height = lw->label.label_height + 2*lw->label.internal_height;

    set_bitmap_info(lw, lw);		/* need core.height */

    if (lw->core.width == 0)		/* need label.lbm_width */
        lw->core.width = (lw->label.label_width + 2 * lw->label.internal_width
			  + LEFT_OFFSET(lw));

    lw->label.label_x = lw->label.label_y = 0;
    (*XtClass(new)->core_class.resize)(new);

}

/***********************************************************************
 *
 * Redisplay method:	Redisplay the label
 *
 ***********************************************************************/

static void _DrawLabelString(lw, labelText, y, len)
LabelWidget lw;
char	   *labelText;
int	    y, len;
{
  Display *disp = XtDisplay((Widget)lw);
  Window   win  = XtWindow((Widget)lw);

  if (lw->label.encoding) /* UNICODE string */
  {
    if (XtIsSensitive((Widget)lw))
      XDrawString16(disp, win, lw->label.normal_GC, lw->label.label_x, y,
		   (TXT16*)labelText, len/2);
    else
    {
	XDrawString16(disp, win, lw->label.lgray_GC, lw->label.label_x+1, y+1,
		     (TXT16*)labelText, len/2);
	XDrawString16(disp, win, lw->label.gray_GC, lw->label.label_x, y,
		     (TXT16*)labelText, len/2);
    }
  }
  else /* ASCII character string */
  {
    if (XtIsSensitive((Widget)lw))
      XDrawString(disp, win, lw->label.normal_GC, lw->label.label_x, y, labelText, len);
    else
    {
      XDrawString(disp, win, lw->label.lgray_GC, lw->label.label_x+1, y+1, labelText, len);
      XDrawString(disp, win, lw->label.gray_GC, lw->label.label_x, y, labelText, len);
    }
  }
}
/* ------------------------------------------------------------------ */

static void Redisplay(w, event, region)
Widget w;
XEvent *event;
Region region;
{
   LabelWidget lw = (LabelWidget) w;
   GC	       gc = lw->label.normal_GC;

   if (region != NULL) {
       int x = lw->label.label_x;
       unsigned int width = lw->label.label_width;
       if (lw->label.lbm_width) {
	   if (lw->label.label_x > (x = lw->label.internal_width))
	       width += lw->label.label_x - x;
       }
       if (XRectInRegion(region, x, lw->label.label_y,
			 width, lw->label.label_height) == RectangleOut)
	   return;
   }

   if (lw->label.pixmap == None) {
       int len = lw->label.label_len;
       char *label = lw->label.label;
       Position y = lw->label.label_y + lw->label.font->max_bounds.ascent;

       /* Display left bitmap or pixmap */

       if (lw->label.left_bitmap && lw->label.lbm_width != 0) {
	 if (lw->label.label_depth == 1)
	   XCopyPlane(XtDisplay(w), lw->label.left_bitmap, XtWindow(w), gc,
		       0, 0, lw->label.lbm_width, lw->label.lbm_height,
		       (int) lw->label.lbm_x, 
		       (int) lw->label.internal_height + lw->label.lbm_y,
		       (unsigned long) 1L);
         else {
	   if (lw->label.xpm_clipmask != None) {
	     XSetClipMask(XtDisplay(w), gc, lw->label.xpm_clipmask);
	     XSetClipOrigin(XtDisplay(w), gc, lw->label.lbm_x,
			    lw->label.internal_height + lw->label.lbm_y);
	   }
	   XCopyArea(XtDisplay(w), lw->label.left_bitmap, XtWindow(w), gc,
		       0, 0, lw->label.lbm_width, lw->label.lbm_height,
		       (int) lw->label.lbm_x, 
		       (int) lw->label.internal_height + lw->label.lbm_y);
	   XSetClipMask(XtDisplay(w), gc, None);
         }
       }
       /* Display label text */

       if (len == MULTI_LINE_LABEL) {
	   char *nl;
	   while ((nl = index(label, '\n')) != NULL)
	   {
             len = lw->label.encoding ? (int)(nl - label)/2 : (int)(nl - label);
	     _DrawLabelString(lw, label, (int)y, len);
	     y += lw->label.font->max_bounds.ascent + lw->label.font->max_bounds.descent;
	     label = nl + 1;
	   }
	   len = strlen(label);
       }
       if (len) _DrawLabelString(lw, label, (int)y, len);
   }
   else	/* Label with only a bitmap or pixmap */
   {
      if (lw->label.label_depth == 1)
        XCopyPlane(XtDisplay(w), lw->label.pixmap, XtWindow(w), gc,
		   0, 0, lw->label.label_width, lw->label.label_height,
		   lw->label.label_x, lw->label.label_y, 1L);
      else {
	if (lw->label.xpm_clipmask != None) {
	  XSetClipMask(XtDisplay(w), gc, lw->label.xpm_clipmask);
	  XSetClipOrigin(XtDisplay(w), gc, lw->label.label_x, lw->label.label_y);
			    
	}
	XCopyArea(XtDisplay(w), lw->label.pixmap, XtWindow(w), gc,
		  0, 0, lw->label.label_width, lw->label.label_height,
		  lw->label.label_x, lw->label.label_y);
	XSetClipMask(XtDisplay(w), gc, None);
      }
   }
}

/***********************************************************************
 *
 * Resize method:	Resize or reposition the label
 *
 ***********************************************************************/

static void _Reposition(lw, width, height)
register LabelWidget lw;
Dimension width, height;
{
    Position newPos;
    Position leftedge = lw->label.internal_width + LEFT_OFFSET(lw);

    switch (lw->label.justify) {

	case XtJustifyLeft   :
	    newPos = leftedge;
	    break;

	case XtJustifyRight  :
	    newPos = width - (lw->label.label_width + lw->label.internal_width);
	    break;

	case XtJustifyCenter :
	default:
	    newPos = (int)(width - lw->label.label_width) / 2;
	    break;
    }
    if (newPos < (Position)leftedge) newPos = leftedge;
    lw->label.label_x = newPos;
    lw->label.label_y = (int)(height - lw->label.label_height)/2;
    return;
}

static void Resize(w)
    Widget w;
{
    LabelWidget lw = (LabelWidget)w;

    _Reposition(lw, w->core.width, w->core.height);
    compute_bitmap_offsets(lw, lw);
}

/***********************************************************************
 *
 * Set values method:	Set specified arguments into widget
 *
 ***********************************************************************/

#define PIXMAP 0
#define WIDTH 1
#define HEIGHT 2
#define NUM_CHECKS 3

static Boolean SetValues(current, request, new, args, num_args)
    Widget current, request, new;
    ArgList args;
    Cardinal *num_args;
{
    LabelWidget curlw = (LabelWidget) current;
    LabelWidget reqlw = (LabelWidget) request;
    LabelWidget newlw = (LabelWidget) new;
    int i;
    Boolean was_resized = False, redisplay = False, checks[NUM_CHECKS];

    for (i = 0; i < NUM_CHECKS; i++) checks[i] = FALSE;

    for (i = 0; i < *num_args; i++) {
	if (streq(XtNbitmap, args[i].name) || streq(XtNpixmap, args[i].name))
	    checks[PIXMAP] = TRUE;
	if (streq(XtNwidth, args[i].name))
	    checks[WIDTH] = TRUE;
	if (streq(XtNheight, args[i].name))
	    checks[HEIGHT] = TRUE;
    }
    if (newlw->label.label == NULL) newlw->label.label = newlw->core.name;

    /*
     * resize on bitmap change
     */
    if (curlw->label.left_bitmap != newlw->label.left_bitmap) was_resized = True;
    if (curlw->label.encoding != newlw->label.encoding) was_resized = True;

    if (curlw->label.label != newlw->label.label) {
        if (curlw->label.label != curlw->core.name) XtFree((char *)curlw->label.label);

	if (newlw->label.label != newlw->core.name)
          newlw->label.label = XtNewString(newlw->label.label);
	was_resized = True;
    }

    if (was_resized || (curlw->label.font != newlw->label.font) ||
	(curlw->label.justify != newlw->label.justify) || checks[PIXMAP]) {

	SetTextWidthAndHeight(newlw);
	was_resized = True;
    }
    /* recalculate the window size if something has changed. */

    if (newlw->label.resize && was_resized) {
      if ((curlw->core.height == reqlw->core.height) && !checks[HEIGHT])
	newlw->core.height = newlw->label.label_height + 2 * newlw->label.internal_height;

	set_bitmap_info(curlw, newlw);

	if ((curlw->core.width == reqlw->core.width) && !checks[WIDTH])
	    newlw->core.width = newlw->label.label_width + LEFT_OFFSET(newlw) +
				 2 * newlw->label.internal_width;
    }

    if (curlw->label.foreground != newlw->label.foreground
	|| curlw->core.background_pixel != newlw->core.background_pixel
        || curlw->add3dExt.shadowColor != newlw->add3dExt.shadowColor
        || curlw->add3dExt.highlightColor != newlw->add3dExt.highlightColor
	|| curlw->label.font->fid != newlw->label.font->fid) {

	XtReleaseGC(new, curlw->label.normal_GC);
	XtReleaseGC(new, curlw->label.gray_GC);
	XtReleaseGC(new, curlw->label.lgray_GC);
	GetnormalGC(newlw);
	GetgrayGCs(newlw);
	redisplay = True;
    }

    if ((curlw->label.internal_width != newlw->label.internal_width) ||
        (curlw->label.internal_height != newlw->label.internal_height) || was_resized) {
	/* Resize() will be called if geometry changes succeed */

	_Reposition(newlw, curlw->core.width, curlw->core.height);
    }

    return was_resized || redisplay ||
	   XtIsSensitive(current) != XtIsSensitive(new);
}

/***********************************************************************
 *
 * Destroy method:	Release all used resources
 *
 ***********************************************************************/

static void Destroy(w)
Widget w;
{
    LabelWidget lw = (LabelWidget)w;

    XtFree(lw->label.label);
    XtReleaseGC(w, lw->label.normal_GC);
    XtReleaseGC(w, lw->label.gray_GC);
    XtReleaseGC(w, lw->label.lgray_GC);
}

/***********************************************************************
 *
 * QueryGeometry()
 *
 ***********************************************************************/

static XtGeometryResult QueryGeometry(w, intended, preferred)
    Widget w;
    XtWidgetGeometry *intended, *preferred;
{
    register LabelWidget lw = (LabelWidget)w;

    preferred->request_mode = CWWidth | CWHeight;
    preferred->width = (lw->label.label_width + 2 * lw->label.internal_width +
			LEFT_OFFSET(lw));
    preferred->height = lw->label.label_height + 2*lw->label.internal_height;
    if (  ((intended->request_mode & (CWWidth | CWHeight))
	   	== (CWWidth | CWHeight)) &&
	  intended->width == preferred->width &&
	  intended->height == preferred->height)
	return XtGeometryYes;
    else if (preferred->width == w->core.width &&
	     preferred->height == w->core.height)
	return XtGeometryNo;
    else
	return XtGeometryAlmost;
}
