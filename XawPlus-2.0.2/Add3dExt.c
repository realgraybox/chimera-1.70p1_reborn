
/******************************************************************
 *
 * 3D extension widget
 * This widget adds colors and some methods for drawing 3 D styled
 * rectangles to the core widget.
 *
 * Roland Krause 1996, 1997
 * Calling concept of drawing methods changed: Roland Krause 1999
 *
 ******************************************************************/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/XawPlus/XawInit.h>
#include <X11/XawPlus/Add3dExtP.h>

/* Additional resources */

#define offset(field) XtOffset(Add3dExtWidget, field)

static XtResource resources[] = {
  {XtNhighlightColor, XtCBackground, XtRPixel, sizeof(Pixel),
     offset(add3dExt.highlightColor), XtRString, "grey90"},
  {XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
     offset(core.background_pixel), XtRString, "grey75"},
  {XtNshadowColor, XtCBackground, XtRPixel, sizeof(Pixel),
     offset(add3dExt.shadowColor), XtRString, "grey40"},
  {XtNbuttonBorderWidth, XtCWidth, XtRDimension,  sizeof(Dimension),
     offset(add3dExt.borderWidth), XtRImmediate, (XtPointer)2}
#undef offset
};

/* Prototypes */

static void    ClassInit();
static void    Initialize();
static void    Destroy();
static Boolean SetValues();

/* Class record of Add3dExt */

Add3dExtClassRec add3dExtClassRec = {
  { /* core fields */
    /* superclass		*/	(WidgetClass) &widgetClassRec,
    /* class_name		*/	"Add3dExt",
    /* widget_size		*/	sizeof(Add3dExtRec),
    /* class_initialize		*/	ClassInit,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	XtExposeNoCompress,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	Destroy,
    /* resize			*/	NULL,
    /* expose			*/	NULL,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry		*/	NULL,
    /* display_accelerator	*/	NULL,
    /* extension		*/	NULL
  },
  { /* add3dExt fields */
    /* unused			*/	0
  }
};

WidgetClass add3dExtWidgetClass = (WidgetClass)&add3dExtClassRec;

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

static void ClassInit()
{
   /* Arguments for the color converter */

   static XtConvertArgRec colConvertArg[] = {
     {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.screen), sizeof(Screen *)},
     {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.colormap), sizeof(Colormap)}
   };

   XawInitializeWidgetSet();
   XtSetTypeConverter(XtRString, XtRPixel, XtCvtStringToPixel,
	colConvertArg, XtNumber(colConvertArg), XtCacheByDisplay, NULL);
}

/***********************************************************************
 *
 * Initialize method:	Initialize widget instance:
 * 			Get GCs for the drawing functions.
 *
 ***********************************************************************/

static void Initialize(old, new, args, num_args)
Widget old, new;
ArgList args;
Cardinal *num_args;
{
    XGCValues	   values;
    Add3dExtWidget w = (Add3dExtWidget)new;

    /* The border width has to be greater than 0 */

    if (w->add3dExt.borderWidth < 1) w->add3dExt.borderWidth = 1;

    /* Get the required GCs */

    values.foreground	 = w->core.background_pixel;
    w->add3dExt.backgrGC = XtGetGC(new, GCForeground, &values);

    values.foreground	 = w->add3dExt.highlightColor;
    w->add3dExt.highlightGC = XtGetGC(new, GCForeground, &values);

    values.foreground	 = w->add3dExt.shadowColor;
    w->add3dExt.shadowGC = XtGetGC(new, GCForeground, &values);
}

/***********************************************************************
 *
 * SetValues method:	Handle changes of the colors or of the
 *			button border width.
 *
 ***********************************************************************/

static Boolean SetValues(current, request, new, args, num_args)
Widget current, request, new;
ArgList args;
Cardinal *num_args;
{
    Add3dExtWidget old_w = (Add3dExtWidget) current;
    Add3dExtWidget new_w = (Add3dExtWidget) new;
    Display *Dpy	 = XtDisplay(current);
    Boolean redraw	 = FALSE;

    /* Check, if any color has changed. If so, free the old color cell and 
     * store the new color in the GC.
     */
    if (old_w->add3dExt.highlightColor != new_w->add3dExt.highlightColor)
    {
	XSetForeground(Dpy, new_w->add3dExt.highlightGC, new_w->add3dExt.highlightColor);
	redraw = TRUE;
    }
    if (old_w->core.background_pixel != new_w->core.background_pixel)
    {
	XSetForeground(Dpy, new_w->add3dExt.backgrGC, new_w->core.background_pixel);
	redraw = TRUE;
    }
    if (old_w->add3dExt.shadowColor != new_w->add3dExt.shadowColor)
    {
	XSetForeground(Dpy, new_w->add3dExt.shadowGC, new_w->add3dExt.shadowColor);
	redraw = TRUE;
    }
    /* Check, if the border width has changed */

    if (old_w->add3dExt.borderWidth != new_w->add3dExt.borderWidth) redraw = TRUE;

    return(redraw);   
}

/***********************************************************************
 *
 * Destroy method:	Free all GCs
 *
 ***********************************************************************/

static void Destroy(w)
Widget w;
{
    Add3dExtWidget add3d = (Add3dExtWidget)w;

    XtReleaseGC(w, add3d->add3dExt.shadowGC);
    XtReleaseGC(w, add3d->add3dExt.backgrGC);
    XtReleaseGC(w, add3d->add3dExt.highlightGC);
}

/***********************************************************************
 *
 * Class methods for drawing buttons, toggles ...
 *
 ***********************************************************************/

/* Internal function: Draw the border for the rectangle functions
 */
static void DrawRectBorder(Dpy, Win, upperGC, lowerGC, borderWidth, x, y, width, height)
Display	*Dpy;		/* Display to paint to */
Drawable Win;		/* Window to paint in */
GC	 upperGC,	/* GC for upper and left border */
	 lowerGC;	/* GC for lower an right border */
int	 borderWidth,	/* Width of the border */
	 x,y,width,height;
{
   int	 xMin, xMax, yMin, yMax, i;

   /* Calculate points for the upper horizontal border an draw it */

   xMin = x;
   xMax = x + width - 2;
   yMin = y;
   for (i = 0; i < borderWidth; i++)
   {
      XDrawLine(Dpy, Win, upperGC, xMin, yMin, xMax, yMin);
      xMin++; xMax--; yMin++;
   }
   /* Calculate points for the left vertical border an draw it */

   xMin = x;
   yMin = y + 1;
   yMax = y + height - 2;
   for (i = 0; i < borderWidth; i++)
   {
      XDrawLine(Dpy, Win, upperGC, xMin, yMin, xMin, yMax);
      xMin++; yMin++; yMax--;
   }
   /* Calculate points for the horizontal lower border an draw it */

   xMin = x + borderWidth;
   xMax = x + width - borderWidth;
   yMax = y + height - borderWidth;
   for (i = 0; i < borderWidth; i++)
   {
      XDrawLine(Dpy, Win, lowerGC, xMin, yMax, xMax, yMax);
      xMin--; xMax++; yMax++;
   }
   /* Calculate points for the right vertical border an draw it */

   xMax = x + width - borderWidth;
   yMin = y + borderWidth;
   yMax = y + height - borderWidth;
   for (i = 0; i < borderWidth; i++)
   {
      XDrawLine(Dpy,  Win, lowerGC, xMax, yMin, xMax, yMax);
      xMax++; yMin--; yMax++;
   }
}

/******************************************************************
 *
 * Exported class methods
 *
 *****************************************************************/

/* Draw a flat rectangle for buttons -----------------------------------
 */
void XawFlatRectangle(w, x, y, width, height)
Widget w;
int		x, y;
unsigned int	width, height;
{
   Add3dExtWidget add3dExt = (Add3dExtWidget)w;
   unsigned int	 xMin, xWidth, yMin, yHeight, i;

   xMin = x; yMin = y;
   xWidth = width - 1;
   yHeight = height - 1;
   for (i = 0; i < (int)add3dExt->add3dExt.borderWidth; i++)
   {
      XDrawRectangle(XtDisplay(w), add3dExt->core.window, add3dExt->add3dExt.backgrGC,
		     xMin, yMin, xWidth, yHeight);
      xMin++; yMin++;
      xWidth -= 2; yHeight -= 2;
   }
}

/* Draw a 3D styled rectangle for released buttons --------------------
 */
void XawRaisedRectangle(w, x, y, width, height)
Widget w;
int		x, y;
unsigned int	width, height;
{
   Add3dExtWidget add3dExt = (Add3dExtWidget)w;

   
   DrawRectBorder(XtDisplay(w), add3dExt->core.window,
	 add3dExt->add3dExt.highlightGC, add3dExt->add3dExt.shadowGC,
	 (int)add3dExt->add3dExt.borderWidth, x, y, (int)width, (int)height);
}

/* Draw a 3D styled rectangle for pressed buttons ----------------------
 */
void XawSunkenRectangle(w, x, y, width, height)
Widget w;
int		x, y;
unsigned int	width, height;
{
   Add3dExtWidget add3dExt = (Add3dExtWidget)w;

   DrawRectBorder(XtDisplay(w), add3dExt->core.window,
	 add3dExt->add3dExt.shadowGC, add3dExt->add3dExt.highlightGC,
	 (int)add3dExt->add3dExt.borderWidth, x, y, (int)width, (int)height);
}

/* Draw a 3D styled raised button --------------------------------------
 */
void XawRaisedButton(w, x, y, width, height)
Widget		w;
int		x, y;
unsigned int	width, height;
{
   Add3dExtWidget add3dExt = (Add3dExtWidget)w;

   XFillRectangle(XtDisplay(w), add3dExt->core.window, add3dExt->add3dExt.backgrGC,
		x, y, width, height);
   DrawRectBorder(XtDisplay(w), add3dExt->core.window,
	 add3dExt->add3dExt.highlightGC, add3dExt->add3dExt.shadowGC,
	 (int)add3dExt->add3dExt.borderWidth, x, y, (int)width, (int)height);
}

/* Draw a 3D styled sunken button --------------------------------------
 */
void XawSunkenButton(w, x, y, width, height)
Widget w;
int		x, y;
unsigned int	width, height;
{
   Add3dExtWidget add3dExt = (Add3dExtWidget)w;

   XFillRectangle(XtDisplay(w), add3dExt->core.window, add3dExt->add3dExt.backgrGC,
		x, y, width, height);
   DrawRectBorder(XtDisplay(w), add3dExt->core.window,
	 add3dExt->add3dExt.shadowGC, add3dExt->add3dExt.highlightGC,
	 (int)add3dExt->add3dExt.borderWidth, x, y, (int)width, (int)height);
}

/* Draw a flat button --------------------------------------------------
 */
void XawFlatButton(w, x, y, width, height)
Widget w;
int		x, y;
unsigned int	width, height;
{
   Add3dExtWidget add3dExt = (Add3dExtWidget)w;

   XFillRectangle(XtDisplay(w), add3dExt->core.window, add3dExt->add3dExt.backgrGC,
		x, y, width, height);
}


