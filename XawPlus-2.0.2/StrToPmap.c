
/***********************************************************************
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

I hacked this up from StrToBmap.c.  Since they (MIT X Consortium) did
most of the work and I only hacked it a little bit, I'm including
their copyright, HOWEVER, the hacks (specifically the diffs) are
Copyright 1990 Robert H. Forsman Jr., distributable under the GNU
General Public License.

 <thoth@cis.ufl.edu>

This is an extensively changed version for XawPlus.
All changes are made by Roland Krause 1996.

<Rokrause@aol.com>

***************************************************************************/

#include	<X11/Intrinsic.h>
#include	<X11/StringDefs.h>
#include	<X11/Xmu/Converters.h>
#include	<X11/Xmu/Drawing.h>

extern Pixmap XawLocatePixmapFile();


/*
 * XmuConvertStringToPixmap:
 *
 * Creates a depth-n Pixmap suitable for window manager icons.
 * "string" represents a pixmap filename which may be absolute,
 * or relative to the global resource pixmapFilePath, class
 * PixmapFilePath.  If the resource is not defined, the default
 * value is the build symbol PIXMAPDIR.
 *
 * shares lots of code with XmuConvertStringToCursor.  
 *
 * To use, include the following in your ClassInitialize procedure:
 *
 * static XtConvertArgRec screenConvertArg[] = {
 *  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(Widget, core.screen), sizeof(Screen *)},
 *  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(Widget, core.colormap), sizeof(Colormap)},
 *  {XtWidgetBaseOffset, (XtPointer)XtOffsetOf(Widget, <clipmask>), sizeof(Pixmap *)}
 * };
 *
 * XtAddConverter(XtRString, XtRPixmap, XmuCvtStringToPixmap,
 *		   screenConvertArg, XtNumber(screenConvertArg));
 *
 */

/**************************************************************************
 *
 * XmuCvtStringToPixmap(): Converter for XPM and XBM styled pixmaps
 * 			   If the filname ends with `.xpm', the converter
 * 			   try to convert it as XPM pixmap file, else
 *			   as XBM bitmap.
 *
 **************************************************************************/

#define	done(address, type) \
  (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t)address

void XmuCvtStringToPixmap(args, num_args, fromVal, toVal)
XrmValuePtr args;
Cardinal    *num_args;
XrmValuePtr fromVal;
XrmValuePtr toVal;
{
    static Pixmap pixmap;
    char *name = (char *)fromVal->addr;

    if (*num_args != 3)	/* Number of arguments correct ? */
    {
       XtErrorMsg("wrongParameters","cvtStringToPixmap","XtToolkitError",
                  "String to pixmap conversion needs screen, colormap and pixmap arguments",
                  (String *)NULL, (Cardinal *)NULL);
       return;
    }
    /* Check, if filename ends with `.xpm'. If so, try to convert the file
     * as XPM file format, else try bitmap file format.
     */
    if (strstr(name, ".xpm") == NULL)
         pixmap = XmuLocateBitmapFile(*((Screen **)args[0].addr),
				      name, NULL, 0, NULL, NULL, NULL, NULL);

    else pixmap = XawLocatePixmapFile(*((Screen **)args[0].addr),
				      *((Colormap *)args[1].addr),
				      name, NULL, 0, NULL, NULL,
				      (Pixmap *)args[2].addr);
    /* Conversion failed ? */

    if (pixmap == None) XtStringConversionWarning (name, "Pixmap");
    done(&pixmap, Pixmap);
}

