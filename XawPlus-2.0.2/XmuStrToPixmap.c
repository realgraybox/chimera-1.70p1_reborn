/* 
 * Copyright (c) 2026 M.Glargaard
 * License: MIT (Expat)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Drawing.h> /* Nødvendig for XmuCreatePixmapFromBitmap */

void XmuCvtStringToPixmap(
    XrmValuePtr args,
    Cardinal *num_args,
    XrmValuePtr fromVal,
    XrmValuePtr toVal
) {
    static Pixmap pixmap;
    XrmValue bitmapVal;
    Pixmap bitmap;

    /* 1. Sørg for at initialisere output-strukturen */
    bitmapVal.size = sizeof(Pixmap);
    bitmapVal.addr = (XPointer)&bitmap;

    /* 2. Kald det officielle MIT-licenserede Xmu-kald for at lave strengen til en bitmap */
    XmuCvtStringToBitmap(args, num_args, fromVal, &bitmapVal);

    /* 3. Tjek om konverteringen lykkedes */
    if (bitmapVal.addr == NULL || *(Pixmap *)(bitmapVal.addr) == None) {
        XtStringConversionWarning(fromVal->addr, XtRPixmap);
        toVal->addr = NULL;
        toVal->size = 0;
        return;
    }

    /* 4. Hent skærm- og display-kontekst */
    Screen *screen = *((Screen **)args[0].addr);
    Display *display = DisplayOfScreen(screen);
    Window root = RootWindowOfScreen(screen);

    /* Variables til XGetGeometry */
    Window root_return;
    int x_return, y_return;
    unsigned int width, height, border_width_return, depth_return;

    /* 5. Hent bredde og højde på bitmappen, som XmuCreatePixmapFromBitmap skal bruge */
    if (!XGetGeometry(display, bitmap, &root_return, &x_return, &y_return,
                      &width, &height, &border_width_return, &depth_return)) {
        XtStringConversionWarning(fromVal->addr, XtRPixmap);
        toVal->addr = NULL;
        toVal->size = 0;
        return;
    }

    /* 6. Konverter den 1-bit dybe bitmap til en Pixmap med skærmens korrekte dimensioner og dybde */
    pixmap = XmuCreatePixmapFromBitmap(
        display, 
        root, 
        bitmap, 
        width, 
        height, 
        DefaultDepthOfScreen(screen),
        BlackPixelOfScreen(screen), 
        WhitePixelOfScreen(screen)
    );
    
    /* Hvis det mod forventning fejler under kørsel, ændres DefaultDepthOfScreen til 1 */
   /* pixmap = XmuCreatePixmapFromBitmap(
        display, root, bitmap, width, height, 
        1, // Tving dybde 1 så det matcher XtRBitmap 
        BlackPixelOfScreen(screen), WhitePixelOfScreen(screen)
    );*/
    
    
    /* 7. Returner resultatet */
    toVal->size = sizeof(Pixmap);
    toVal->addr = (XPointer)&pixmap;
}
