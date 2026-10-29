/*
 * colour.c
 *
 * Copyright (C) 2026 M. Glargaard, aka graybox
 *
 * This software is provided "as-is", without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

/*
 * #rrggbb colours in X resources
 *
 * Xt turns colour resources into pixels with XAllocNamedColor(). Xlib
 * reads "#rrggbb" itself only when it is built with Xcms (its colour
 * management part); without it the name goes to the X server, which
 * knows names like "grey18" from rgb.txt but not "#2e2e2e". Static
 * builds are often made without Xcms, and then every #rrggbb colour in
 * the app-defaults fails with 'Color name "#2e2e2e" is not defined'.
 *
 * This converter reads #rgb, #rrggbb, #rrrgggbbb and #rrrrggggbbbb
 * itself and hands everything else (colour names, XtDefaultForeground,
 * ...) to Xt's own converter.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>

#include "colour.h"

/* Xt's converter, for everything that is not #rrggbb */
extern Boolean XtCvtStringToPixel(Display *, XrmValuePtr, Cardinal *,
				  XrmValuePtr, XrmValuePtr, XtPointer *);

/* the same arguments as Xt's converter: the widget's screen and colormap */
static XtConvertArgRec colour_args[] =
{
  { XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.screen),
    sizeof(Screen *) },
  { XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.colormap),
    sizeof(Colormap) }
};

/* "#" and 3, 6, 9 or 12 hex digits, as XParseColor accepts */
static int
ParseHex(const char *s, XColor *c)
{
  int n, len, i;
  unsigned int v[3];

  if (s[0] != '#') return 0;
  s++;
  len = strlen(s);
  if (len != 3 && len != 6 && len != 9 && len != 12) return 0;
  for (i = 0; i < len; i++)
  {
    if (!isxdigit((unsigned char) s[i])) return 0;
  }

  n = len / 3;			/* digits per channel */
  for (i = 0; i < 3; i++)
  {
    char part[5];
    unsigned int x;

    memcpy(part, s + i * n, n);
    part[n] = '\0';
    sscanf(part, "%x", &x);
    /*
     * to 16 bits as XParseColor does it: the digits are the high bits
     * ("2e" -> 2e00, and "#fff" is f000 f000 f000, a light grey), so a
     * static build shows exactly the colours the other builds show
     */
    v[i] = x << (16 - 4 * n);
  }

  c->red = v[0];
  c->green = v[1];
  c->blue = v[2];
  c->flags = DoRed | DoGreen | DoBlue;
  return 1;
}

static Boolean
CvtStringToPixel(Display *dpy, XrmValuePtr args, Cardinal *num_args,
		 XrmValuePtr from, XrmValuePtr to, XtPointer *data)
{
  static Pixel result;
  const char *name = (const char *) from->addr;
  Colormap cmap;
  XColor c;

  if (name == NULL || name[0] != '#' || *num_args != 2 ||
      !ParseHex(name, &c))
    return XtCvtStringToPixel(dpy, args, num_args, from, to, data);

  cmap = *((Colormap *) args[1].addr);
  if (!XAllocColor(dpy, cmap, &c))	/* colormap full: let Xt complain */
    return XtCvtStringToPixel(dpy, args, num_args, from, to, data);

  if (to->addr != NULL)
  {
    if (to->size < sizeof(Pixel))
    {
      to->size = sizeof(Pixel);
      return False;
    }
    *(Pixel *) to->addr = c.pixel;
  }
  else
  {
    result = c.pixel;
    to->addr = (XPointer) &result;
  }
  to->size = sizeof(Pixel);
  return True;
}

/*
 * Must be called before the first widget (the toplevel shell) is
 * created, so that every colour resource goes through it, and after
 * the widget classes are initialised: XawPlus's Add3dExt, Form and
 * TextSink install Xt's converter in their ClassInitialize, which
 * would replace this one (XtSetTypeConverter acts on every
 * application context). main() initialises them all first.
 */
void
RegisterColourConverter(XtAppContext app)
{
  XtAppSetTypeConverter(app, XtRString, XtRPixel, CvtStringToPixel,
			colour_args, XtNumber(colour_args),
			XtCacheByDisplay, NULL);
}
