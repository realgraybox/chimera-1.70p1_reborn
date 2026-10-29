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
 * #rrggbb colours with an Xlib built without Xcms
 *
 * Xt turns colour resources into pixels with XAllocNamedColor(). Xlib
 * reads "#rrggbb" there only when it is built with Xcms (its colour
 * management part); without it the name goes to the X server, which
 * knows names like "grey18" from rgb.txt but not "#2e2e2e". Size-reduced
 * static builds leave Xcms out (it adds about 24 KB to every program),
 * and then every #rrggbb colour in the app-defaults fails with
 * 'Color name "#2e2e2e" is not defined'.
 *
 * In such builds (mk.config: NO_XCMS=1, the default with uClibc) Chimera
 * is linked with -Wl,--wrap=XAllocNamedColor. The linker then sends every
 * call to XAllocNamedColor() in Chimera and in the static libraries,
 * libXt.a included, to __wrap_XAllocNamedColor() below. That reads #rgb,
 * #rrggbb, #rrrgggbbb and #rrrrggggbbbb itself and passes everything
 * else to the real function (__real_XAllocNamedColor), so colour names
 * and the warnings for unknown ones are exactly as before.
 *
 * Other builds use Xlib's own function, and this file compiles to
 * nothing.
 */

#ifdef CHIMERA_NO_XCMS

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>

Status __real_XAllocNamedColor(Display *, Colormap, const char *,
			       XColor *, XColor *);
Status __wrap_XAllocNamedColor(Display *, Colormap, const char *,
			       XColor *, XColor *);

/*
 * "#" and 3, 6, 9 or 12 hex digits, read as XParseColor does: the digits
 * are the high bits of each 16-bit value ("2e" -> 2e00, so "#fff" is
 * f000 f000 f000), so the colours are exactly those of the other builds.
 */
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
    v[i] = x << (16 - 4 * n);
  }

  c->red = v[0];
  c->green = v[1];
  c->blue = v[2];
  c->flags = DoRed | DoGreen | DoBlue;
  return 1;
}

Status
__wrap_XAllocNamedColor(Display *dpy, Colormap cmap, const char *name,
			XColor *screen_def, XColor *exact_def)
{
  XColor c;

  if (name == NULL || name[0] != '#' || !ParseHex(name, &c))
    return __real_XAllocNamedColor(dpy, cmap, name, screen_def, exact_def);

  c.pixel = 0;
  *exact_def = c;		/* the colour asked for */
  *screen_def = c;		/* the colour the screen gives */
  return XAllocColor(dpy, cmap, screen_def);
}

#endif /* CHIMERA_NO_XCMS */
