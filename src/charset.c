/*
 * charset.c
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
 * Character sets
 *
 * Chimera draws text with X core fonts in ISO 8859-1 (Latin-1), one byte
 * per character. Most pages today are UTF-8, where every character
 * outside ASCII takes two or more bytes, which would show as several
 * Latin-1 characters ("Ã¦" for "æ").
 *
 * DocumentToLatin1() therefore converts text/html and text/plain
 * documents to Latin-1 right after loading:
 *
 *   - UTF-8 documents are decoded. The charset comes from the HTTP
 *     Content-Type header, else from <meta charset> in the page, else
 *     the text is taken as UTF-8 if it contains UTF-8 sequences and
 *     nothing that is not valid UTF-8 (local files have no header).
 *   - Other documents are read as windows-1252: its bytes 0x80..0x9F
 *     (curly quotes, dashes, euro) get the same substitutes.
 *   - In HTML, character references beyond Latin-1 (&#8217;, &#x2019;,
 *     &rsquo;, ...) are replaced too; Chimera's own parser only knows
 *     decimal references up to 255.
 *
 * Characters outside Latin-1 become a readable substitute where there is
 * one (quotes, dashes, "...", "EUR", accented letters of Central European
 * languages without their accent), otherwise '?'.
 *
 * A converted UTF-8 document gets "x-chimera-from=utf-8" in its
 * Content-Type, so that forms on it can send their text back as UTF-8,
 * which is what the server expects (DocumentWasUTF8, Latin1ToUTF8).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "charset.h"

#ifndef CHARSET_TEST
#include "common.h"
#include "url.h"
#include "mime.h"
#include "document.h"
#endif

#ifdef CHARSET_TEST
#define alloc_mem(n) malloc(n)
#define free_mem(p)  free(p)
#endif

/* Latin Extended-A, U+0100..U+017F, without accents */
static const char latin_ext_a[] =
  "AaAaAaCcCcCcCcDd" "DdEeEeEeEeEeGgGg" "GgGgHhHhIiIiIiIi" "IiIiJjKkkLlLlLlL"
  "lLlNnNnNnnNnOoOo" "OoOoRrRrRrSsSsSs" "SsTtTtTtUuUuUuUu" "UuUuWwYyYZzZzZzs";

static const struct { unsigned long cp; const char *s; } subst[] = {
  { 0x0152, "OE" }, { 0x0153, "oe" }, { 0x0192, "f" },
  { 0x02C6, "^" }, { 0x02DC, "~" },
  { 0x2002, " " }, { 0x2003, " " }, { 0x2009, " " }, { 0x200A, " " },
  { 0x202F, " " }, { 0x205F, " " },
  { 0x200B, "" }, { 0x200C, "" }, { 0x200D, "" }, { 0x200E, "" },
  { 0x200F, "" }, { 0x2060, "" }, { 0xFEFF, "" },
  { 0x2010, "-" }, { 0x2011, "-" }, { 0x2012, "-" }, { 0x2013, "-" },
  { 0x2014, "-" }, { 0x2015, "-" }, { 0x2212, "-" },
  { 0x2018, "'" }, { 0x2019, "'" }, { 0x201A, "," }, { 0x201B, "'" },
  { 0x201C, "\"" }, { 0x201D, "\"" }, { 0x201E, "\"" }, { 0x201F, "\"" },
  { 0x2032, "'" }, { 0x2033, "\"" },
  { 0x2039, "<" }, { 0x203A, ">" },
  { 0x2020, "+" }, { 0x2021, "+" }, { 0x2022, "\267" }, { 0x2027, "\267" },
  { 0x2026, "..." }, { 0x2030, "o/oo" },
  { 0x20AC, "EUR" }, { 0x2122, "(TM)" },
  { 0x2190, "<-" }, { 0x2192, "->" }, { 0x2194, "<->" }, { 0x21D2, "=>" },
  { 0x2248, "~" }, { 0x2260, "!=" }, { 0x2264, "<=" }, { 0x2265, ">=" },
  { 0, NULL }
};

/*
 * Windows-1252 bytes 0x80..0x9F (0: undefined). Pages in windows-1252,
 * and pages labelled ISO 8859-1 (which browsers read as windows-1252),
 * use them for curly quotes, dashes and the euro sign; the Latin-1 fonts
 * have nothing there.
 */
static const unsigned short cp1252[32] = {
  0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
  0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178
};

/* HTML named references for characters beyond Latin-1 */
static const struct { const char *name; unsigned long cp; } named[] = {
  { "OElig", 0x0152 }, { "oelig", 0x0153 }, { "Scaron", 0x0160 },
  { "scaron", 0x0161 }, { "Yuml", 0x0178 }, { "fnof", 0x0192 },
  { "circ", 0x02C6 }, { "tilde", 0x02DC },
  { "ensp", 0x2002 }, { "emsp", 0x2003 }, { "thinsp", 0x2009 },
  { "zwnj", 0x200C }, { "zwj", 0x200D }, { "lrm", 0x200E }, { "rlm", 0x200F },
  { "ndash", 0x2013 }, { "mdash", 0x2014 }, { "minus", 0x2212 },
  { "lsquo", 0x2018 }, { "rsquo", 0x2019 }, { "sbquo", 0x201A },
  { "ldquo", 0x201C }, { "rdquo", 0x201D }, { "bdquo", 0x201E },
  { "prime", 0x2032 }, { "Prime", 0x2033 },
  { "lsaquo", 0x2039 }, { "rsaquo", 0x203A },
  { "dagger", 0x2020 }, { "Dagger", 0x2021 }, { "bull", 0x2022 },
  { "hellip", 0x2026 }, { "permil", 0x2030 },
  { "euro", 0x20AC }, { "trade", 0x2122 },
  { "larr", 0x2190 }, { "rarr", 0x2192 }, { "harr", 0x2194 },
  { "rArr", 0x21D2 }, { "asymp", 0x2248 }, { "ne", 0x2260 },
  { "le", 0x2264 }, { "ge", 0x2265 },
  { NULL, 0 }
};

/*
 * Append the Latin-1 form of code point cp to o. In HTML, characters
 * that would start markup are written as references.
 */
static int
emit (char *o, unsigned long cp, int is_html)
{
  const char *s = NULL;
  char one[2];
  int i, n = 0;

  if (cp < 0x80  ||  (cp >= 0xA0  &&  cp <= 0xFF))
  {
    one[0] = (char) cp;
    one[1] = '\0';
    s = one;
  }
  else if (cp >= 0x100  &&  cp <= 0x17F  &&  cp != 0x152  &&  cp != 0x153)
  {
    one[0] = latin_ext_a[cp - 0x100];
    one[1] = '\0';
    s = one;
  }
  else
  {
    for (i = 0;  subst[i].s != NULL;  i++)
      if (subst[i].cp == cp)  { s = subst[i].s;  break; }
    if (s == NULL)  s = "?";
  }

  for (;  *s;  s++)
  {
    if (is_html  &&  cp >= 0x80  &&  (*s == '<' || *s == '>' || *s == '&'))
    {
      const char *e = (*s == '<') ? "&lt;" : (*s == '>') ? "&gt;" : "&amp;";
      strcpy (o + n, e);
      n += strlen (e);
    }
    else
      o[n++] = *s;
  }
  return (n);
}

/*
 * Decode one UTF-8 sequence at p (at most len bytes). Returns its length
 * and sets *cp, or returns 0 if the bytes are not valid UTF-8.
 */
static int
utf8_decode (const unsigned char *p, int len, unsigned long *cp)
{
  int n, i;
  unsigned long c;

  if (p[0] < 0x80)        { *cp = p[0];  return (1); }
  else if (p[0] < 0xC2)   return (0);
  else if (p[0] < 0xE0)   { n = 2;  c = p[0] & 0x1F; }
  else if (p[0] < 0xF0)   { n = 3;  c = p[0] & 0x0F; }
  else if (p[0] < 0xF5)   { n = 4;  c = p[0] & 0x07; }
  else return (0);

  if (len < n)  return (0);
  for (i = 1;  i < n;  i++)
  {
    if ((p[i] & 0xC0) != 0x80)  return (0);
    c = (c << 6) | (p[i] & 0x3F);
  }
  if ((n == 3 && c < 0x800)  ||  (n == 4 && (c < 0x10000 || c > 0x10FFFF))
      ||  (c >= 0xD800 && c <= 0xDFFF))
    return (0);
  *cp = c;
  return (n);
}

/*
 * True if text has UTF-8 sequences and nothing that is not UTF-8.
 */
int
LooksLikeUTF8 (const char *text, int len)
{
  const unsigned char *p = (const unsigned char *) text;
  int i = 0, multi = 0, n;
  unsigned long cp;

  while (i < len)
  {
    if (p[i] < 0x80)  { i++;  continue; }
    if ((n = utf8_decode (p + i, len - i, &cp)) == 0)  return (0);
    multi = 1;
    i += n;
  }
  return (multi);
}

/*
 * Convert text to Latin-1. utf8: decode UTF-8, else read windows-1252;
 * is_html: also replace
 * character references beyond Latin-1. Returns new memory (alloc_mem)
 * and its length in *outlen.
 */
char *
Latin1Text (const char *text, int len, int utf8, int is_html, int *outlen)
{
  const unsigned char *p = (const unsigned char *) text;
  char *o;
  int i = 0, n = 0, k;
  int size = len * 2 + 64;
  unsigned long cp;

  /*
   * The output grows as needed: one input byte or character reference
   * can become up to 9 characters ("<->" as "&lt;-&gt;" in HTML), and
   * "&#xff" becomes "&#255;". Each step below writes at most
   * LATIN1_STEP bytes, so that much room is made before it. (A fixed
   * len * 3 was too small; found by fuzzing.)
   */
#define LATIN1_STEP 32
  o = alloc_mem (size);

  while (i < len)
  {
    if (n + LATIN1_STEP >= size)
    {
      size = (n + LATIN1_STEP) * 2;
      o = realloc_mem (o, size);
    }
    if (!utf8  &&  p[i] >= 0x80  &&  p[i] <= 0x9F)
    {
      cp = cp1252[p[i] - 0x80];
      n += emit (o + n, cp ? cp : '?', is_html);
      i++;
      continue;
    }

    if (utf8  &&  p[i] >= 0x80)
    {
      if ((k = utf8_decode (p + i, len - i, &cp)) > 0)
      {
	n += emit (o + n, cp, is_html);
	i += k;
      }
      else
	o[n++] = text[i++];	/* not UTF-8: keep the byte */
      continue;
    }

    if (is_html  &&  p[i] == '&'  &&  i + 2 < len)
    {
      int j = i + 1;

      if (p[j] == '#')		/* &#NNNN; or &#xHHHH; */
      {
	int hex = (p[j + 1] == 'x'  ||  p[j + 1] == 'X');
	char *end;

	j += hex ?  2 : 1;
	cp = strtoul (text + j, &end, hex ?  16 : 10);
	if (end > text + j  &&  end - text <= len)
	{
	  k = end - text;
	  if (k < len  &&  text[k] == ';')  k++;
	  if (cp > 0xFF)
	  {
	    n += emit (o + n, cp, is_html);
	    i = k;
	    continue;
	  }
	  if (hex)		/* Chimera only reads decimal */
	  {
	    n += sprintf (o + n, "&#%lu;", cp);
	    i = k;
	    continue;
	  }
	}
      }
      else if (isalpha (p[j]))	/* named references beyond Latin-1 */
      {
	int e;

	for (e = 0;  named[e].name != NULL;  e++)
	{
	  int nl = strlen (named[e].name);

	  if (j + nl < len  &&  strncmp (text + j, named[e].name, nl) == 0
	      &&  !isalnum ((unsigned char) text[j + nl]))
	  {
	    n += emit (o + n, named[e].cp, is_html);
	    i = j + nl + (text[j + nl] == ';' ?  1 : 0);
	    break;
	  }
	}
	if (named[e].name != NULL)  continue;
      }
    }

    o[n++] = text[i++];
  }
  o[n] = '\0';
  *outlen = n;
  return (o);
}

/*
 * The value of "charset" in s (a Content-Type value or the text near a
 * <meta> tag), copied into buf.
 */
static int
find_charset (const char *s, int len, char *buf, int bufsize)
{
  int i, n;

  for (i = 0;  i + 7 < len;  i++)
  {
    if (strncasecmp (s + i, "charset", 7) != 0)  continue;
    i += 7;
    while (i < len  &&  (s[i] == ' ' || s[i] == '='))  i++;
    if (i < len  &&  (s[i] == '"' || s[i] == '\''))  i++;
    for (n = 0;  i < len  &&  n < bufsize - 1  &&
	 (isalnum ((unsigned char) s[i]) || s[i] == '-' || s[i] == '_');  i++)
      buf[n++] = s[i];
    buf[n] = '\0';
    return (n > 0);
  }
  return (0);
}

static int
is_utf8_name (const char *cs)
{
  return (strcasecmp (cs, "utf-8") == 0  ||  strcasecmp (cs, "utf8") == 0);
}

#ifndef CHARSET_TEST
/*
 * Convert a text/html or text/plain document to Latin-1, see above.
 * After converting UTF-8 the Content-Type says iso-8859-1, so a document
 * read back from the cache is not converted twice.
 */
void
DocumentToLatin1 (Document *d)
{
  char cs[32];
  int is_html, utf8, newlen;
  char *t;

  if (d == NULL  ||  d->text == NULL  ||  d->len <= 0  ||  d->content == NULL)
    return;
  is_html = (strncasecmp (d->content, "text/html", 9) == 0);
  if (!is_html  &&  strncasecmp (d->content, "text/plain", 10) != 0)  return;

  if (find_charset (d->content, strlen (d->content), cs, sizeof (cs)))
    utf8 = is_utf8_name (cs);
  else
  {
    /* look for <meta ... charset=...> near the top of the page */
    int scan = d->len < 4096 ?  d->len : 4096, i, found = 0;

    if (is_html)
    {
      for (i = 0;  i + 5 < scan  &&  !found;  i++)
      {
	if (d->text[i] == '<'  &&  strncasecmp (d->text + i + 1, "meta", 4) == 0)
	{
	  char *end = memchr (d->text + i, '>', scan - i);
	  int tl = end ?  (int) (end - (d->text + i)) : scan - i;

	  found = find_charset (d->text + i, tl, cs, sizeof (cs));
	}
      }
    }
    utf8 = found ?  is_utf8_name (cs) : LooksLikeUTF8 (d->text, d->len);
  }

  t = Latin1Text (d->text, d->len, utf8, is_html, &newlen);
  free_mem (d->text);
  d->text = t;
  d->len = newlen;

  if (utf8)
  {
    static const char *html = "text/html; charset=iso-8859-1; x-chimera-from=utf-8";
    static const char *plain = "text/plain; charset=iso-8859-1; x-chimera-from=utf-8";
    char *c = alloc_mem (strlen (plain) + 1);

    strcpy (c, is_html ?  html : plain);
    free_mem (d->content);
    d->content = c;
  }
}

/*
 * True if d was converted from UTF-8 by DocumentToLatin1.
 */
int
DocumentWasUTF8 (Document *d)
{
  const char *c;

  if (d == NULL  ||  d->content == NULL)  return (0);
  for (c = d->content;  *c;  c++)
    if (strncasecmp (c, "x-chimera-from=utf-8", 20) == 0)  return (1);
  return (0);
}
#endif

/*
 * UTF-8 copy of a Latin-1 string (alloc_mem). Used for form data sent
 * from pages that were UTF-8.
 */
char *
Latin1ToUTF8 (const char *s)
{
  const unsigned char *p;
  char *o, *r;

  r = o = alloc_mem (strlen (s) * 2 + 1);
  for (p = (const unsigned char *) s;  *p;  p++)
  {
    if (*p < 0x80)
      *o++ = (char) *p;
    else
    {
      *o++ = (char) (0xC0 | (*p >> 6));
      *o++ = (char) (0x80 | (*p & 0x3F));
    }
  }
  *o = '\0';
  return (r);
}
