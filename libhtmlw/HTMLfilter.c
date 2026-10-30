/*
 * HTMLfilter.c
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
 * A pass over the page text before the (1990s) HTML parser sees it, for
 * two things of today's HTML that it does not know:
 *
 *   - Hidden content. An element with style="display:none" or the
 *     hidden attribute is left out, up to its matching end tag. A
 *     browser does not show it either; pages show it with JavaScript,
 *     which Chimera does not run. <html> and <body> are never left out
 *     (some sites hide the page until their scripts have loaded), and
 *     aria-hidden does not count: it hides only from screen readers.
 *
 *   - Inline SVG. An <svg> element is left out (Chimera cannot draw it
 *     in the page; its text would show), and so is an XML processing
 *     instruction such as <?xml version="1.0"?>, which pages get when
 *     an SVG file is pasted into them.
 *
 *   - Links left without text. A link with no visible text and no
 *     image (an icon link whose icon was SVG or a CSS background)
 *     gets its aria-label, or else its title, as text, so that it can
 *     be seen and clicked.
 *
 *   - <button>...</button>. It becomes an <input> of the same type
 *     (submit when not given), so the widget code makes a button of it.
 *     Its text, with tags removed, goes in X-CHIMERA-LABEL for the
 *     label, while VALUE stays the value the form sends.
 *
 * Comments, <script>, <style> and <textarea> contents are copied as they
 * are. HTMLFilterText() returns a new string (free it with free()), or
 * NULL when nothing was changed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "HTMLfilter.h"

typedef struct
{
  char *s;
  size_t len, size;
} Buf;

static void
put(Buf *b, const char *p, size_t n)
{
  if (b->len + n + 1 > b->size)
  {
    size_t want = b->len + n + 1;

    b->size = b->size ? b->size : 4096;
    while (b->size < want) b->size *= 2;
    b->s = realloc(b->s, b->size);
  }
  memcpy(b->s + b->len, p, n);
  b->len += n;
  b->s[b->len] = '\0';
}

static void
puts_(Buf *b, const char *p)
{
  put(b, p, strlen(p));
}

/* The end of the tag starting at p ('<'): just after its '>', with quoted
 * attribute values skipped. NULL if the tag never ends. */
static const char *
TagEnd(const char *p)
{
  const char *q = p + 1;
  char quote = 0;

  for (; *q != '\0'; q++)
  {
    if (quote)
    {
      if (*q == quote) quote = 0;
    }
    else if ((*q == '"' || *q == '\'') && q > p + 1 && q[-1] == '=')
    {
      quote = *q;
    }
    else if (*q == '>')
    {
      return q + 1;
    }
  }
  /* unterminated quote: fall back to the first '>' */
  q = strchr(p, '>');
  return q ? q + 1 : NULL;
}

/* Lower-case tag name of the tag at p into name (size n); 0 if none */
static int
TagName(const char *p, char *name, int n)
{
  int i = 0;

  p++;
  if (*p == '/') p++;
  if (!isalpha((unsigned char) *p)) return 0;
  while (i < n - 1 && (isalnum((unsigned char) *p) || *p == '-'))
    name[i++] = tolower((unsigned char) *p++);
  name[i] = '\0';
  return i;
}

/*
 * Look up attribute attr (lower case) in the tag [p, e). Returns 1 if
 * present; its value, without quotes, goes to *v and *vlen (an
 * attribute without a value gets length 0).
 */
static int
GetAttr(const char *p, const char *e, const char *attr,
	const char **v, size_t *vlen)
{
  const char *q = p + 1;
  size_t alen = strlen(attr);

  while (q < e && *q != ' ' && *q != '\t' && *q != '\n' && *q != '\r' &&
	 *q != '>')
    q++;				/* the tag name */

  while (q < e)
  {
    const char *ns, *ne;

    while (q < e && (isspace((unsigned char) *q) || *q == '/')) q++;
    if (q >= e || *q == '>') break;
    ns = q;
    while (q < e && !isspace((unsigned char) *q) && *q != '=' &&
	   *q != '>')
      q++;
    ne = q;
    while (q < e && isspace((unsigned char) *q)) q++;

    *v = q;
    *vlen = 0;
    if (q < e && *q == '=')
    {
      q++;
      while (q < e && isspace((unsigned char) *q)) q++;
      if (q < e && (*q == '"' || *q == '\''))
      {
	char quote = *q++;

	*v = q;
	while (q < e && *q != quote) q++;
	*vlen = q - *v;
	if (q < e) q++;
      }
      else
      {
	*v = q;
	while (q < e && !isspace((unsigned char) *q) && *q != '>') q++;
	*vlen = q - *v;
      }
    }

    if ((size_t)(ne - ns) == alen && strncasecmp(ns, attr, alen) == 0)
      return 1;
  }
  return 0;
}

/* Is the element of the tag [p, e) hidden by style or hidden attribute? */
static int
IsHidden(const char *p, const char *e)
{
  const char *v;
  size_t n, i, j = 0;
  char style[256];

  if (GetAttr(p, e, "hidden", &v, &n)) return 1;
  if (!GetAttr(p, e, "style", &v, &n)) return 0;

  for (i = 0; i < n && j < sizeof(style) - 1; i++)	/* no blanks */
  {
    if (!isspace((unsigned char) v[i]))
      style[j++] = tolower((unsigned char) v[i]);
  }
  style[j] = '\0';
  return strstr(style, "display:none") != NULL;
}

static int
IsVoid(const char *name)
{
  static const char *voids[] =
  {
    "area", "base", "br", "col", "embed", "hr", "img", "input", "link",
    "meta", "param", "source", "track", "wbr", NULL
  };
  int i;

  for (i = 0; voids[i] != NULL; i++)
    if (strcmp(name, voids[i]) == 0) return 1;
  return 0;
}

/*
 * Skip something that is copied unchanged and cannot contain tags: a
 * comment, or the contents of script, style and textarea. p points at
 * '<'. Returns the end of it, or NULL if p starts none of these.
 */
static const char *
SkipRaw(const char *p)
{
  char name[16];
  const char *e, *q;

  if (strncmp(p, "<!--", 4) == 0)
  {
    q = strstr(p + 4, "-->");
    return q ? q + 3 : p + strlen(p);
  }
  if (p[1] == '/' || !TagName(p, name, sizeof(name))) return NULL;
  if (strcmp(name, "script") != 0 && strcmp(name, "style") != 0 &&
      strcmp(name, "textarea") != 0)
    return NULL;

  e = TagEnd(p);
  if (e == NULL) return p + strlen(p);
  for (q = e; (q = strchr(q, '<')) != NULL; q++)
  {
    char end[16];

    if (q[1] == '/' && TagName(q, end, sizeof(end)) &&
	strcmp(end, name) == 0)
    {
      e = TagEnd(q);
      return e ? e : q + strlen(q);
    }
  }
  return p + strlen(p);
}

/*
 * The end of the element whose start tag [p, e) is named name: just
 * after its matching end tag. NULL if there is none.
 */
static const char *
ElementEnd(const char *e, const char *name)
{
  const char *q = e;
  int depth = 1;

  while ((q = strchr(q, '<')) != NULL)
  {
    char n[32];
    const char *r = SkipRaw(q);

    if (r != NULL)
    {
      q = r;
      continue;
    }
    if (TagName(q, n, sizeof(n)) && strcmp(n, name) == 0)
    {
      const char *te = TagEnd(q);

      if (q[1] == '/') depth--;
      else if (te == NULL || te[-2] != '/') depth++;	/* not <x/> */
      if (depth == 0) return te;
      if (te == NULL) return NULL;
      q = te;
      continue;
    }
    q++;
  }
  return NULL;
}

/* Text of [p, e) without tags, blanks collapsed, '"' made '\'' */
static void
PutText(Buf *b, const char *p, const char *e)
{
  int blank = 0, any = 0;

  while (p < e)
  {
    if (*p == '<')
    {
      const char *te = TagEnd(p);

      if (te == NULL || te > e) break;
      p = te;
      blank = any;
      continue;
    }
    if (isspace((unsigned char) *p))
    {
      blank = any;
    }
    else
    {
      char c = (*p == '"') ? '\'' : *p;

      if (blank) put(b, " ", 1);
      put(b, &c, 1);
      blank = 0;
      any = 1;
    }
    p++;
  }
}

/* Append ' attr="value"' of the tag [p, e), if the tag has attr */
static void
CopyAttr(Buf *b, const char *p, const char *e, const char *attr)
{
  const char *v;
  size_t n, i;

  if (!GetAttr(p, e, attr, &v, &n)) return;
  puts_(b, " ");
  puts_(b, attr);
  puts_(b, "=\"");
  for (i = 0; i < n; i++)
    put(b, v[i] == '"' ? "'" : &v[i], 1);
  puts_(b, "\"");
}

/*
 * <button> at p with start tag ending at e: write an <input> for it and
 * return where to continue (after </button>).
 */
static const char *
Button(Buf *b, const char *p, const char *e)
{
  const char *v, *end, *close;
  size_t n;
  char type[16] = "submit";
  Buf label = { NULL, 0, 0 };

  if (GetAttr(p, e, "type", &v, &n) && n > 0 && n < sizeof(type))
  {
    size_t i;

    for (i = 0; i < n; i++) type[i] = tolower((unsigned char) v[i]);
    type[n] = '\0';
    if (strcmp(type, "reset") != 0 && strcmp(type, "button") != 0)
      strcpy(type, "submit");
  }

  end = ElementEnd(e, "button");
  if (end != NULL)
  {
    close = end - 1;			/* the '>' of </button> */
    while (close > e && *close != '<') close--;
    PutText(&label, e, close);
  }
  else
  {
    end = e;				/* no end tag: just the start tag */
  }
  if (label.len == 0)
  {
    if (GetAttr(p, e, "aria-label", &v, &n) && n > 0) put(&label, v, n);
    else if (GetAttr(p, e, "value", &v, &n) && n > 0) put(&label, v, n);
    else puts_(&label, strcmp(type, "reset") == 0 ? "Reset" :
	       strcmp(type, "button") == 0 ? "Button" : "Submit");
  }

  puts_(b, "<input type=\"");
  puts_(b, type);
  puts_(b, "\"");
  CopyAttr(b, p, e, "name");
  if (GetAttr(p, e, "value", &v, &n))
    CopyAttr(b, p, e, "value");
  else
  {
    /* no value: the label is also what is sent, as with <input> */
    size_t i;

    puts_(b, " value=\"");
    for (i = 0; i < label.len; i++)
      put(b, label.s[i] == '"' ? "'" : &label.s[i], 1);
    puts_(b, "\"");
  }
  puts_(b, " x-chimera-label=\"");
  {
    size_t i;

    for (i = 0; i < label.len; i++)
      put(b, label.s[i] == '"' ? "'" : &label.s[i], 1);
  }
  puts_(b, "\">");
  free(label.s);
  return end;
}

/*
 * Does [p, e) show anything: text outside tags, an image or a form
 * field?
 */
static int
ShowsSomething(const char *p, const char *e)
{
  while (p < e)
  {
    if (*p == '<')
    {
      char name[16];
      const char *te = TagEnd(p);

      if (TagName(p, name, sizeof(name)) && p[1] != '/' &&
	  (strcmp(name, "img") == 0 || strcmp(name, "input") == 0))
	return 1;
      if (te == NULL || te > e) return 0;
      p = te;
      continue;
    }
    if (!isspace((unsigned char) *p)) return 1;
    p++;
  }
  return 0;
}

/*
 * Second pass: links that show nothing get their aria-label or title as
 * text. Returns a new string, or NULL if no link needed it.
 */
static char *
FillEmptyLinks(const char *text)
{
  Buf b = { NULL, 0, 0 };
  const char *p, *done;
  int changed = 0;

  for (p = done = text; (p = strchr(p, '<')) != NULL; )
  {
    char name[8];
    const char *e, *end, *close, *v, *r;
    size_t n;

    if ((r = SkipRaw(p)) != NULL)
    {
      p = r;
      continue;
    }
    if (p[1] == '/' || !TagName(p, name, sizeof(name)) ||
	strcmp(name, "a") != 0 || (e = TagEnd(p)) == NULL)
    {
      p++;
      continue;
    }
    end = ElementEnd(e, "a");
    if (end == NULL)
    {
      p = e;
      continue;
    }
    close = end - 1;			/* the '>' of </a> */
    while (close > e && *close != '<') close--;

    if (!ShowsSomething(e, close) &&
	((GetAttr(p, e, "aria-label", &v, &n) && n > 0) ||
	 (GetAttr(p, e, "title", &v, &n) && n > 0)))
    {
      put(&b, done, close - done);
      put(&b, v, n);
      done = close;
      changed = 1;
    }
    p = e;
  }

  if (!changed)
  {
    free(b.s);
    return NULL;
  }
  puts_(&b, done);
  return b.s;
}

static char *FilterPass(const char *text);

char *
HTMLFilterText(const char *text)
{
  char *first, *second;

  if (text == NULL) return NULL;
  first = FilterPass(text);
  second = FillEmptyLinks(first != NULL ? first : text);
  if (second == NULL) return first;
  free(first);
  return second;
}

/* First pass: hidden elements, SVG, processing instructions, buttons */
static char *
FilterPass(const char *text)
{
  Buf b = { NULL, 0, 0 };
  const char *p, *done;
  int changed = 0;

  for (p = done = text; (p = strchr(p, '<')) != NULL; )
  {
    char name[32];
    const char *e, *r;

    if ((r = SkipRaw(p)) != NULL)
    {
      p = r;
      continue;
    }
    if (p[1] == '?')			/* <?xml ... ?> */
    {
      const char *q = strstr(p + 2, "?>");

      put(&b, done, p - done);
      p = done = q ? q + 2 : p + strlen(p);
      changed = 1;
      continue;
    }
    if (p[1] == '/' || !TagName(p, name, sizeof(name)) ||
	(e = TagEnd(p)) == NULL)
    {
      p++;
      continue;
    }

    if (strcmp(name, "svg") == 0)
    {
      const char *end = (e[-2] == '/') ? e : ElementEnd(e, name);

      put(&b, done, p - done);
      p = done = end ? end : e;
      changed = 1;
      continue;
    }

    if (strcmp(name, "html") != 0 && strcmp(name, "body") != 0 &&
	IsHidden(p, e))
    {
      const char *end = IsVoid(name) ? e : ElementEnd(e, name);

      if (end == NULL) end = e;		/* no end tag: drop the tag only */
      put(&b, done, p - done);
      p = done = end;
      changed = 1;
      continue;
    }

    if (strcmp(name, "button") == 0)
    {
      put(&b, done, p - done);
      p = done = Button(&b, p, e);
      changed = 1;
      continue;
    }

    p = e;
  }

  if (!changed)
  {
    free(b.s);
    return NULL;
  }
  puts_(&b, done);
  return b.s;
}
