/*
 * Copyright (c) 2026 M. Glargaard
 * License: MIT (Expat)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * String to Bitmap converter for the XawPlus Label widget.
 *
 * This replaces StrToPmap.c and LocPixmap.c, which carried GPL-licensed
 * code and loaded XPM files.  Chimera never sets a bitmap resource, so
 * the standard MIT-licensed Xmu converter is used instead: it loads XBM
 * files from the bitmap search path and returns a 1-bit bitmap.  Label.c
 * draws a 1-bit bitmap with XCopyPlane in the widget's own foreground and
 * background colours, as the original Athena Label does.
 *
 * Label.c registers this converter with three arguments (screen,
 * colormap and clip mask), the set the old XPM converter needed.
 * XmuCvtStringToBitmap() accepts exactly one, the screen, and stops the
 * program with a fatal Xt error if given more, so only the first
 * argument is passed on.
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Converters.h>

void XawPlusCvtStringToBitmap(
    XrmValuePtr args,
    Cardinal *num_args,
    XrmValuePtr fromVal,
    XrmValuePtr toVal
) {
    Cardinal one = 1;

    if (*num_args < 1) {
        XtStringConversionWarning((char *)fromVal->addr, XtRBitmap);
        return;
    }

    XmuCvtStringToBitmap(args, &one, fromVal, toVal);
}
