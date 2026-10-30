/*
 * MODIFIED VERSION. This file is part of the HTML widget of NCSA Mosaic
 * for the X Window System 2.x, as modified for Chimera 1.70 (1997) and
 * for Chimera 1.70p1 reborn (2026,
 * https://github.com/realgraybox/chimera-1.70p1_reborn). It is not the
 * original NCSA Mosaic distributed by the University of Illinois.
 */
/* substantially modified by WBE, Spring 1997, and by others before me */

/****************************************************************************
 * NCSA Mosaic for the X Window System                                      *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 * mosaic@ncsa.uiuc.edu                                                     *
 *                                                                          *
 * Copyright (C) 1993, Board of Trustees of the University of Illinois      *
 *                                                                          *
 * NCSA Mosaic software, both binary and source (hereafter, Software) is    *
 * copyrighted by The Board of Trustees of the University of Illinois       *
 * (UI), and ownership remains with the UI.                                 *
 *                                                                          *
 * The UI grants you (hereafter, Licensee) a license to use the Software    *
 * for academic, research and internal business purposes only, without a    *
 * fee.  Licensee may distribute the binary and source code (if released)   *
 * to third parties provided that the copyright notice and this statement   *
 * appears on all copies and that no charge is associated with such         *
 * copies.                                                                  *
 *                                                                          *
 * Licensee may make derivative works.  However, if Licensee distributes    *
 * any derivative work based on or derived from the Software, then          *
 * Licensee will (1) notify NCSA regarding its distribution of the          *
 * derivative work, and (2) clearly notify users that such derivative       *
 * work is a modified version and not the original NCSA Mosaic              *
 * distributed by the UI.                                                   *
 *                                                                          *
 * Any Licensee wishing to make commercial use of the Software should       *
 * contact the UI, c/o NCSA, to negotiate an appropriate license for such   *
 * commercial use.  Commercial use includes (1) integration of all or       *
 * part of the source code into a product for sale or license by or on      *
 * behalf of Licensee to third parties, or (2) distribution of the binary   *
 * code or source code to third parties that need it to utilize a           *
 * commercial product sold or licensed by or on behalf of Licensee.         *
 *                                                                          *
 * UI MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR   *
 * ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED          *
 * WARRANTY.  THE UI SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY THE    *
 * USERS OF THIS SOFTWARE.                                                  *
 *                                                                          *
 * By using or copying this Software, Licensee agrees to abide by the       *
 * copyright law and all other applicable laws of the U.S. including, but   *
 * not limited to, export control laws, and the terms of this license.      *
 * UI shall have the right to terminate this license immediately by         *
 * written notice upon Licensee's breach of, or non-compliance with, any    *
 * of its terms.  Licensee may be held legally responsible for any          *
 * copyright infringement that is caused or encouraged by Licensee's        *
 * failure to abide by the terms of this license.                           *
 *                                                                          *
 * Comments and questions are welcome and can be sent to                    *
 * mosaic-x@ncsa.uiuc.edu.                                                  *
 ****************************************************************************/

#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <string.h>

#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>

#include "ScrollText.h"

#include "HTMLP.h"

#define STRING String

#define X_NAME		"x"
#define Y_NAME		"y"

#define	W_TEXTFIELD	0
#define	W_CHECKBOX	1
#define	W_RADIOBOX	2
#define	W_PUSHBUTTON	3
#define	W_PASSWORD	4
#define	W_OPTIONMENU	5
#define	W_TEXTAREA	6
#define	W_LIST		7
#define	W_JOT		8
#define	W_HIDDEN	9

extern char *ParseMarkTag ();

void FreeCommaList _ArgProto((char **, int));

static char **ParseCommaList _ArgProto((char *, int *));
static char *MapOptionReturn _ArgProto((char *, char **));
static void setTextSize _ArgProto((Widget, int, int));
static Widget GetAsciiTextWidget _ArgProto((Widget));
static Widget GetListWidget _ArgProto((Widget));
static void ShowToggleMark _ArgProto((Widget, int));
static void SetupToggle _ArgProto((Widget, int));
static void CBToggleMark _ArgProto((Widget, XtPointer, XtPointer));

/*
 * Marks shown in checkboxes and radio buttons that are on. The fonts are
 * Latin-1, so there is no check mark or filled circle.
 */
/* room for a select list's vertical scrollbar, its edges included */
#define LIST_SCROLLBAR_SPACE	18

#define CHECK_MARK	"X"
#define RADIO_MARK	"O"

#define FONTHEIGHT(font) (font->max_bounds.ascent + font->max_bounds.descent)

static Widget
GetAsciiTextWidget(Widget w)
{
  Widget rw;

  if (XtIsSubclass(w, scrollingTextWidgetClass))
  {
    XtVaGetValues(w, XtNtextWidget, &rw, NULL);
  }
  else
  {
    rw = w;
  }
  return(rw);
}

/*
 * The List widget of a W_LIST field. wptr->w is the Viewport that
 * scrolls it, and its children are not always [clip, list]: a Viewport
 * also has its scrollbars as children, and the widget sets do not
 * create them in the same order (Xaw3d differs from XawPlus), so the
 * list is looked up by class. NULL if there is none.
 */
static Widget
GetListWidget(Widget w)
{
  WidgetList wl = NULL;
  Cardinal nc = 0, i;

  if (w == NULL) return(NULL);
  if (XtIsSubclass(w, listWidgetClass)) return(w);

  XtVaGetValues(w, XtNchildren, &wl, XtNnumChildren, &nc, NULL);
  for (i = 0; i < nc; i++)
  {
    if (XtIsSubclass(wl[i], listWidgetClass)) return(wl[i]);
  }

  /* the list may sit inside the Viewport's clip widget */
  for (i = 0; i < nc; i++)
  {
    if (XtIsComposite(wl[i]))
    {
      WidgetList cl = NULL;
      Cardinal cn = 0, j;

      XtVaGetValues(wl[i], XtNchildren, &cl, XtNnumChildren, &cn, NULL);
      for (j = 0; j < cn; j++)
      {
	if (XtIsSubclass(cl[j], listWidgetClass)) return(cl[j]);
      }
    }
  }
  return(NULL);
}

/*
 * Checkboxes and radio buttons are Toggle widgets. With an empty label
 * they were thin, tall boxes, and only the widget set's 3D edge or
 * reverse video showed whether they were on, which a theme could make
 * invisible. Now they are square and show a mark when on.
 *
 * ShowToggleMark() sets the label from the state. It runs on every
 * click (CBToggleMark), and wherever the code sets the state: radio
 * buttons turning each other off, and Reset.
 */
static void
ShowToggleMark(Widget w, int radio)
{
  Boolean state = False;

  if (w == NULL) return;
  XtVaGetValues(w, XtNstate, &state, NULL);
  XtVaSetValues(w, XtNlabel,
		state ? (radio ? RADIO_MARK : CHECK_MARK) : " ", NULL);
}

static void
CBToggleMark(Widget w, XtPointer client_data, XtPointer call_data)
{
  ShowToggleMark(w, client_data != NULL);
}

/*
 * Give a new toggle a fixed, square size that fits the mark, so the
 * box does not change when the mark comes and goes, and show its mark.
 */
static void
SetupToggle(Widget w, int radio)
{
  Dimension width = 0, height = 0;

  /* the label is the mark here, so the natural size fits it */
  XtVaGetValues(w, XtNwidth, &width, XtNheight, &height, NULL);
  if (width < height) width = height;
  else height = width;
  XtVaSetValues(w, XtNresize, False, XtNwidth, width, XtNheight, height,
		NULL);
  ShowToggleMark(w, radio);
  XtAddCallback(w, XtNcallback, CBToggleMark, radio ? (XtPointer) 1 : NULL);
}

static void
setTextSize (Widget w, int columns, int lines)
{
  XFontStruct *font = NULL;
  Position lm, rm, tm, bm;
  Dimension width, height;
  Widget rw;

  rw = GetAsciiTextWidget(w);

  XtVaGetValues (rw,
		 XtNfont, &font,
		 XtNleftMargin, &lm,
		 XtNrightMargin, &rm,
		 XtNtopMargin, &tm,
		 XtNbottomMargin, &bm,
		 NULL);

  if (font != NULL)
  {
    width = rm + lm + columns * XTextWidth(font, "0", 1);
/*
    width = rm + lm + columns *
	(font->max_bounds.lbearing + font->max_bounds.rbearing);
*/
    height = tm + bm + lines * FONTHEIGHT (font);
  }
  else
  {
    width = 200;
    height = 25;
  }

  XtVaSetValues (w,
		 XtNwidth, width,
		 XtNheight, height,
		 NULL);
}

void
CBListDestroy (Widget w, XtPointer client_data, XtPointer call_data)
{
  char **string_list, **p;
  int item_count;

  XtVaGetValues (w,
		 XtNlist, &string_list,
		 XtNnumberStrings, &item_count,
		 NULL);

  p = string_list;
  while (item_count > 0)
  {
    XtFree(*p++);
    item_count--;
  }
  if (string_list != NULL) XtFree((char *)string_list);
}


void
CBTextDestroy (Widget w, XtPointer client_data, XtPointer call_data)
{
  XtFree((char *)client_data);
}


void
CBoption (Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget menuButton = (Widget) client_data;
  char *label;

  XtVaGetValues (menuButton, XtNlabel, &label, NULL);
  XtVaGetValues (w, XtNlabel, &label, NULL);
  XtVaSetValues (menuButton, XtNlabel, label, NULL);
}


void
AddNewForm (HTMLWidget hw, FormInfo *fptr)
{
  FormInfo *ptr;

  fptr->next = NULL;

  ptr = hw->html.form_list;
  if (ptr == NULL)
  {
    hw->html.form_list = fptr;
  }
  else
  {
    while (ptr->next != NULL)
    {
      ptr = ptr->next;
    }
    ptr->next = fptr;
  }
}


void
FreeOneForm (FormInfo *fptr)
{
  if (fptr->action != NULL)      XtFree (fptr->action);
  if (fptr->method != NULL)      XtFree (fptr->method);
  if (fptr->enctype != NULL)     XtFree (fptr->enctype);
  if (fptr->enc_entity != NULL)  XtFree (fptr->enc_entity);
  XtFree ((char *)fptr);
}


void
FreeForms (HTMLWidget hw)
{
  FormInfo *fptr;

  fptr = hw->html.form_list;
  while (fptr != NULL)
  {
    FormInfo *next = fptr->next;
    FreeOneForm (fptr);
    fptr = next;
  }
  hw->html.form_list = NULL;
}


int
CollectSubmitInfo (Widget w, FormInfo *fptr, char ***name_list, char ***value_list)
{
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WbFormCallbackData cbdata;
  WidgetInfo *wptr;
  int cnt;
  Boolean state;

  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    cbdata.attribute_count = cnt;
  }
  else
  {
    cbdata.attribute_count = fptr->end - fptr->start;
  }
  cbdata.attribute_names = (char **) XtMalloc (cbdata.attribute_count *
					       sizeof (char *));
  cbdata.attribute_values = (char **) XtMalloc (cbdata.attribute_count *
						sizeof (char *));

  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  cnt = 0;

  while ((wptr != NULL) && (cnt < cbdata.attribute_count))
  {
    if ((wptr->name) && (wptr->type != W_PUSHBUTTON || wptr->w == w))
    {
      Widget child;
      STRING *str_list;
      int list_cnt;
      char *val;
      XawListReturnStruct *currentSelection;

      cbdata.attribute_names[cnt] = wptr->name;
      switch (wptr->type)
      {
        case W_TEXTFIELD:
	  XtVaGetValues (GetAsciiTextWidget(wptr->w), XtNstring,
			 &(cbdata.attribute_values[cnt]),
			 NULL);
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	  break;
	case W_TEXTAREA:
	  XtVaGetValues (wptr->w, XtNstring,
			 &(cbdata.attribute_values[cnt]),
			 NULL);
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	  break;
	case W_PASSWORD:
	  /* the field shows '*'s; the real text is kept in wptr->password */
	  cbdata.attribute_values[cnt] = wptr->password;
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	 /* char *s;
XtVaGetValues(GetAsciiTextWidget(wptr->w),
              XtNstring, &s,
              NULL);

fprintf(stderr,
        "PWD raw='%s' len=%lu\n",
        s ? s : "(null)",
        s ? (unsigned long)strlen(s) : 0);
	  
	  
	  */
	  break;
	case W_LIST:
	  /*
	   * First get the Widget ID of the proper
	   * list element
	   */
	  child = GetListWidget (wptr->w);
	  if (child == NULL)
	  {
	    cnt--;
	    cbdata.attribute_count--;
	    break;
	  }

	  /*
	   * Now get the list of selected items.
	   */
	  currentSelection = XawListShowCurrent (child);
	  list_cnt = currentSelection->list_index == XAW_LIST_NONE ? 0 : 1;
	  str_list = &(currentSelection->string);
	  
	  if (list_cnt == 0)
	  {
	    cnt--;
	    cbdata.attribute_count--;
	  }
	  else
	      /* list_cnt >= 1 */
	  {
	    int j, new_cnt;
	    char **names;
	    char **values;
	    
	    if (list_cnt > 1)
	    {
	      new_cnt = cbdata.attribute_count + list_cnt - 1;
	      names = (char **) XtMalloc (new_cnt * sizeof (char *));
	      values = (char **) XtMalloc (new_cnt * sizeof (char *));
	      for (j = 0; j < cnt; j++)
	      {
		names[j] = cbdata.attribute_names[j];
		values[j] = cbdata.attribute_values[j];
	      }
	      XtFree((char *) cbdata.attribute_names);
	      XtFree((char *) cbdata.attribute_values);
	      cbdata.attribute_names = names;
	      cbdata.attribute_values = values;
	      cbdata.attribute_count = new_cnt;
	    }
	    
	    for (j = 0; j < list_cnt; j++)
	    {
	      cbdata.attribute_names[cnt + j] = wptr->name;
	      val = str_list[j];
	      if ((val != NULL) && (val[0] == '\0'))
	      {
		val = NULL;
	      }
	      else if (val != NULL)
	      {
		val = MapOptionReturn (val, wptr->mapping);
	      }
	      cbdata.attribute_values[cnt + j] = val;
	    }
	    cnt = cnt + list_cnt - 1;
	  }
	  break;
        /*
	 * For an option menu, first get the label gadget
	 * which holds the current value.
	 * Now get the text from that label as a character
	 * string.
	 */
        case W_OPTIONMENU:
	  XtVaGetValues (wptr->w, XtNlabel, &val, NULL);
	  if ((val != NULL) && (val[0] == '\0'))
	  {
	    val = NULL;
	  }
	  else if (val != NULL)
	  {
	    val = MapOptionReturn (val, wptr->mapping);
	  }
	  cbdata.attribute_values[cnt] = val;
	  if ((cbdata.attribute_values[cnt] != NULL) &&
	      (cbdata.attribute_values[cnt][0] == '\0'))
	  {
	    cbdata.attribute_values[cnt] = NULL;
	  }
	  break;
	case W_CHECKBOX:
	case W_RADIOBOX:
	  XtVaGetValues (wptr->w, XtNstate, &state, NULL);
	  if (state)
	  {
	    cbdata.attribute_values[cnt] = wptr->value;
	  }
	  else
	  {
	    cnt--;
	    cbdata.attribute_count--;
	  }
	  break;
        case W_HIDDEN:
	  cbdata.attribute_values[cnt] = wptr->value;
	  break;
	case W_PUSHBUTTON:
	  cbdata.attribute_values[cnt] = wptr->value; /* SKLindsay 240796 */
	  break;
        default:
	  cbdata.attribute_values[cnt] = NULL;
	  break;
      }
/*fprintf(stderr,
        "FIELD name='%s' value='%s'\n",
        wptr->name ? wptr->name : "(null)",
        cbdata.attribute_values[cnt]
            ? cbdata.attribute_values[cnt]
            : "(null)");      
      */
      cnt++;
    }
    else
    {
      cbdata.attribute_count--;
    }
    wptr = wptr->next;
  }
  cbdata.attribute_count = cnt;

  *name_list = cbdata.attribute_names;
  *value_list = cbdata.attribute_values;
  return (cbdata.attribute_count);
}


void
ImageSubmitForm (FormInfo *fptr, XEvent *event, char *name, int x, int y)
{
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WbFormCallbackData cbdata;
  int i, cnt;
  char **name_list;
  char **value_list;
  char valstr[100];

  cbdata.event = event;
  cbdata.href = fptr->action;
  cbdata.method = fptr->method;
  cbdata.enctype = fptr->enctype;
  cbdata.enc_entity = fptr->enc_entity;

  name_list = NULL;
  value_list = NULL;
  cnt = CollectSubmitInfo (0, fptr, &name_list, &value_list);

  cbdata.attribute_count = cnt + 2;
  cbdata.attribute_names = (char **)XtMalloc (cbdata.attribute_count *
					      sizeof (char *));
  cbdata.attribute_values = (char **)XtMalloc (cbdata.attribute_count *
					       sizeof (char *));
  for (i = 0; i < cnt; i++)
  {
    cbdata.attribute_names[i] = name_list[i];
    cbdata.attribute_values[i] = value_list[i];
  }
  if (name_list != NULL)
  {
    XtFree((char *)name_list);
    name_list = NULL;
  }
  if (value_list != NULL)
  {
    XtFree((char *)value_list);
    value_list = NULL;
  }

  if ((name != NULL) && (name[0] != '\0'))
  {
    cbdata.attribute_names[cnt] = (char *) XtMalloc (sizeof (char) *
						     (strlen (name) +
						      strlen (X_NAME) + 2));
    strcpy (cbdata.attribute_names[cnt], name);
    strcat (cbdata.attribute_names[cnt], ".");
    strcat (cbdata.attribute_names[cnt], X_NAME);
  }
  else
  {
    cbdata.attribute_names[cnt] = XtNewString (X_NAME);
  }
  sprintf (valstr, "%d", x);
  cbdata.attribute_values[cnt] = XtNewString (valstr);

  cnt++;
  if ((name != NULL) && (name[0] != '\0'))
  {
    cbdata.attribute_names[cnt] = (char *) XtMalloc (sizeof (char) *
						     (strlen (name) +
						      strlen (Y_NAME) + 2));
    strcpy (cbdata.attribute_names[cnt], name);
    strcat (cbdata.attribute_names[cnt], ".");
    strcat (cbdata.attribute_names[cnt], Y_NAME);
  }
  else
  {
    cbdata.attribute_names[cnt] = XtNewString (Y_NAME);
  }
  sprintf (valstr, "%d", y);
  cbdata.attribute_values[cnt] = XtNewString (valstr);

  XtCallCallbackList ((Widget)hw, hw->html.form_callback, (XtPointer)&cbdata);
}


void
CBSubmitForm (Widget w, XtPointer client_data, XtPointer call_data)
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WbFormCallbackData cbdata;

  cbdata.event = NULL;
  cbdata.href = fptr->action;
  cbdata.method = fptr->method;
  cbdata.enctype = fptr->enctype;
  cbdata.enc_entity = fptr->enc_entity;

  cbdata.attribute_count = CollectSubmitInfo (w,
					      fptr,
					      &cbdata.attribute_names,
					      &cbdata.attribute_values);

  XtCallCallbackList ((Widget) hw, hw->html.form_callback,
		      (XtPointer) & cbdata);
}


/*
 * A radio buttom was toggled on in a form.
 * If there are other radios of the same name, turn them off.
 */
void
CBChangeRadio (Widget w, XtPointer client_data, XtPointer call_data)
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WidgetInfo *wptr;
  WidgetInfo *wtmp;
  char *name;
  int cnt, count;
  Boolean state;

  /*
   * Only do stuff when the button is turned on.
   * Don't let the button be turned off, by clicking on
   * it, as that would leave all buttons off.
   */
  XtVaGetValues (w, XtNstate, &state, NULL);
  if (!state)
  {
    XtVaSetValues (w, XtNstate, True, NULL);
    return;
  }

  /*
   * Terminate the form if it was never properly terminated.
   */
  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    count = cnt;
  }
  else
  {
    count = fptr->end - fptr->start;
  }

  /*
   * Locate the start of the form.
   */
  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  /*
   * Find the name of the toggle button just pressed.
   */
  name = NULL;
  wtmp = wptr;
  while (wtmp != NULL)
  {
    if (wtmp->w == w)
    {
      name = wtmp->name;
      break;
    }
    wtmp = wtmp->next;
  }

  /*
   * Check for other checked radioboxes of the same name.
   */
  cnt = 0;
  while ((wptr != NULL) && (cnt < count))
  {
    if ((wptr->type == W_RADIOBOX) &&
	(wptr->w != w) &&
	(wptr->name != NULL) &&
	(name != NULL) &&
	(strcasecmp (wptr->name, name) == 0))
    {
      XtVaGetValues (wptr->w, XtNstate, &state, NULL);
      if (state)
      {
	XtVaSetValues (wptr->w, XtNstate, False, NULL);
	ShowToggleMark (wptr->w, 1);
      }
    }
    cnt++;
    wptr = wptr->next;
  }
}

/*
 * RETURN was hit in a textfield in a form.
 * If this is the only textfield in this form, submit the form.
 */
void
CBActivateField (Widget w, XtPointer client_data, XtPointer call_data)
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WidgetInfo *wptr;
  int cnt, count;

  /*
   * Terminate the form if it was never properly terminated.
   */
  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    count = cnt;
  }
  else
  {
    count = fptr->end - fptr->start;
  }

  /*
   * Locate the start of the form.
   */
  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  /*
   * Count the textfields in this form.
   */
  cnt = 0;
  while ((wptr != NULL) && (cnt < count))
  {
    if ((wptr->type == W_TEXTFIELD) || (wptr->type == W_PASSWORD))
    {
      cnt++;
    }
    wptr = wptr->next;
  }

  /*
   * If this is the only textfield in this form, submit the form.
   */
  if (cnt == 1) CBSubmitForm (w, client_data, call_data);
}

void
CBResetForm (Widget w, XtPointer client_data, XtPointer call_data)
{
  FormInfo *fptr = (FormInfo *) client_data;
  HTMLWidget hw = (HTMLWidget) (fptr->hw);
  WidgetInfo *wptr;
  int widget_count, cnt;

  if (fptr->end == -1)		/* unterminated FORM tag */
  {
    wptr = hw->html.widget_list;
    cnt = 0;
    while (wptr != NULL)
    {
      cnt++;
      wptr = wptr->next;
    }
    widget_count = cnt;
  }
  else
  {
    widget_count = fptr->end - fptr->start;
  }

  if (fptr->start == 0)
  {
    wptr = hw->html.widget_list;
  }
  else
  {
    wptr = hw->html.widget_list;
    while (wptr != NULL)
    {
      if (wptr->id == fptr->start)
      {
	wptr = wptr->next;
	break;
      }
      wptr = wptr->next;
    }
  }

  cnt = 0;
  while ((wptr != NULL) && (cnt < widget_count))
  {
    Widget child, atw;
    char *txt = NULL;
    int length = 0;
    Boolean stringInPlace;

    switch (wptr->type)
    {
      case W_TEXTFIELD:
        atw = GetAsciiTextWidget(wptr->w);
        XtVaGetValues (atw,
		       XtNuseStringInPlace, &stringInPlace,
		       XtNlength, &length,
		       NULL);
	if (stringInPlace)
	{
	  XtVaGetValues (atw, XtNstring, &txt, NULL);
	}
	if (wptr->value == NULL)
	{
	  if (stringInPlace)
	  {
	    if (txt) *txt = '\0';
	    XtVaSetValues (atw, XtNstring, txt, NULL);
	  }
	  else
	  {
	    XtVaSetValues (atw, XtNstring, "", NULL);
	  }
	}
	else
	{
	  if (stringInPlace)
	  {
	    strncpy (txt, wptr->value, length);
	    XtVaSetValues (atw, XtNstring, txt, NULL);
	  }
	  else
	  {
	    XtVaSetValues (atw, XtNstring, wptr->value, NULL);
	  }
	}
	break;
      case W_TEXTAREA:
	XtVaSetValues (wptr->w, XtNstring,
		       wptr->value ? wptr->value : "",
		       NULL);
	break;
      case W_PASSWORD:
        atw = GetAsciiTextWidget(wptr->w);
	if (wptr->value == NULL)
	{
	  XtVaSetValues (atw, XtNstring, "", NULL);
	  if (wptr->password != NULL)
	  {
	    XtFree(wptr->password);
	    wptr->password = NULL;
	  }
	}
	else
	{
	  int i, len;
	  
	  if (wptr->password != NULL)
	  {
	    XtFree(wptr->password);
	    wptr->password = NULL;
	  }
	  len = strlen (wptr->value);
	  wptr->password = (char *) XtMalloc (sizeof (char) * (len + 1));
	  for (i = 0; i < len; i++)
	  {
	    wptr->password[i] = '*';
	  }
	  wptr->password[len] = '\0';
	  XtVaSetValues (atw, XtNstring, wptr->password, NULL);
	  strcpy (wptr->password, wptr->value);
	}
	break;
      case W_LIST:
        {
	  char **vlist;
	  int vlist_cnt;
	  STRING *val_list;
	  int i;
	  char **string_list;
	  int list_cnt;
	  
	  child = GetListWidget (wptr->w);
	  if (child == NULL) break;
	  XtVaGetValues (child,
			 XtNlist, &string_list,
			 XtNnumberStrings, &list_cnt, NULL);
	  
	  if (wptr->value != NULL)
	  {
	    vlist = ParseCommaList (wptr->value, &vlist_cnt);
	    val_list = (STRING *) XtMalloc (vlist_cnt * sizeof (STRING));
	    XawListUnhighlight (child);
	    for (i = 0; i < vlist_cnt; i++)
	    {
	      val_list[i] = XtNewString (vlist[i]);
	    }
	    FreeCommaList (vlist, vlist_cnt);
	    
	    if (vlist_cnt > 0)
	    {
#ifdef VERBOSE
	      if (vlist_cnt > 1)
	      {
		fprintf (stderr, "HTML: only a single selection allowed!\n");
	      }
#endif
	      for (i = 0; i < list_cnt; i++)
	      {
		if (!strcasecmp (string_list[i], val_list[0]))
		{
		  XawListHighlight (child, i);
		  break;
		}
	      }
	    }
	    for (i = 0; i < vlist_cnt; i++)
	    {
	      if (val_list[i] != NULL) XtFree(val_list[i]);
	    }
	    if (val_list != NULL) XtFree((char *) val_list);
	  }
	  else
	  {
	    XawListUnhighlight (child);
	  }
	}
	break;
      /*
       * gack, we saved the widget id of the starting default
       * into the value character pointer, just so we could
       * yank it out here, and restore the default.
       */
      case W_OPTIONMENU:
	if (wptr->value != NULL)
	{
	  Widget hist = (Widget) wptr->value;
	  char *txt;
	  
	  XtVaGetValues (hist, XtNlabel, &txt, NULL);
	  XtVaSetValues (wptr->w, XtNlabel, txt, NULL);
	}
	break;
      case W_CHECKBOX:
      case W_RADIOBOX:
	XtVaSetValues (wptr->w, XtNstate, wptr->checked, NULL);
	ShowToggleMark (wptr->w, wptr->type == W_RADIOBOX);
	break;
      case W_HIDDEN:
	break;
      default:
	break;
    }
    cnt++;
    wptr = wptr->next;
  }
}


void
PrepareFormEnd (HTMLWidget hw, Widget w, FormInfo *fptr)
{
  XtAddCallback (w, XtNcallback, (XtCallbackProc) CBSubmitForm,
		 (XtPointer)fptr);
}


void
PrepareFormReset (HTMLWidget hw, Widget w, FormInfo *fptr)
{
  XtAddCallback (w, XtNcallback, (XtCallbackProc) CBResetForm,
		 (XtPointer)fptr);
}


void
HideWidgets (HTMLWidget hw)
{
  WidgetInfo *wptr;
  XEvent event;

  wptr = hw->html.widget_list;
  while (wptr != NULL)
  {
    if ((wptr->w != NULL) && (wptr->mapped == True))
    {
      XtSetMappedWhenManaged (wptr->w, False);
      wptr->mapped = False;
    }
    wptr = wptr->next;
  }

  /*
   * Force the exposure events into the queue
   */
  XSync (XtDisplay (hw), False);

  /*
   * Remove all Expose events for the view window
   */
  while (XCheckWindowEvent (XtDisplay (hw->html.view),
			    XtWindow (hw->html.view),
			    ExposureMask, &event) == True)
  {
  }
}


void
MapWidgets (HTMLWidget hw)
{
  WidgetInfo *wptr;

  wptr = hw->html.widget_list;
  while (wptr != NULL)
  {
    if ((wptr->w != NULL) && (wptr->mapped == False))
    {
      wptr->mapped = True;
      XtSetMappedWhenManaged (wptr->w, True);
    }
    wptr = wptr->next;
  }
}


Boolean
AlreadyChecked (HTMLWidget hw, FormInfo *fptr, char *name)
{
  WidgetInfo *wptr;
  Boolean radio_checked;

  radio_checked = False;
  wptr = hw->html.widget_list;
  while (wptr != NULL)
  {
    if ((wptr->id >= fptr->start) &&
	(wptr->type == W_RADIOBOX) &&
	(wptr->checked == True) &&
	(wptr->name != NULL) &&
	(name != NULL) &&
	(strcasecmp (wptr->name, name) == 0))
    {
      radio_checked = True;
      break;
    }
    wptr = wptr->next;
  }
  return (radio_checked);
}

WidgetInfo *
AddNewWidget (HTMLWidget hw, Widget w, int type, int id, int x, int y, char *name, char *value, char **mapping, Boolean checked)
{
  WidgetInfo *wptr;
  Dimension width, height;

  /*
   * Don't want to do GetValues if this is HIDDEN input
   * tag with no widget.
   */
  if (w != NULL)
  {
    Cardinal argcnt;
    Arg arg[10];

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNwidth, &width); argcnt++;
    XtSetArg (arg[argcnt], XtNheight, &height); argcnt++;
    XtGetValues (w, arg, argcnt);
  }
  else
  {
    width = 0;
    height = 0;
  }

  wptr = (WidgetInfo *) XtMalloc (sizeof (WidgetInfo));
//  wptr->magic1 = 0x1111CAFE;
//  wptr->magic2 = 0x2222CAFE;
  wptr->w = w;
  wptr->type = type;
  wptr->id = id;
  wptr->x = x;
  wptr->y = y;
  wptr->width = width;
  wptr->height = height;
  wptr->name = name;
  wptr->value = value;
  wptr->password = NULL;
  wptr->maxlength = -1;
  wptr->mapping = mapping;
  wptr->checked = checked;
  wptr->mapped = False;
  wptr->next = NULL;

  if (hw->html.widget_list == NULL)
  {
    hw->html.widget_list = wptr;
  }
  else				/* find tail */
  {
    WidgetInfo *tail = hw->html.widget_list;

    while (tail->next != NULL)   tail = tail->next;

    tail->next = wptr;
  }

  if ((wptr->type == W_PASSWORD) && (wptr->value != NULL))
  {
    wptr->password = XtNewString (wptr->value);
  }

  return (wptr);
}

/*
 * For the various widgets, return their font structures so
 * we can use the font's baseline to place them.
 */
XFontStruct *
GetWidgetFont (HTMLWidget hw, WidgetInfo *wptr)
{
  Widget child;
  XFontStruct *font = NULL;

  /*
   * For option menus we have to first get the child that has the
   * font info.
   */
  if (wptr->type == W_OPTIONMENU)
  {
    XtVaGetValues (wptr->w, XtNfont, &font, NULL);
  }
  else
  {
    if (wptr->type == W_LIST)
    {
      child = GetListWidget (wptr->w);
      if (child != NULL) XtVaGetValues (child, XtNfont, &font, NULL);
    }
    else if (wptr->type == W_TEXTFIELD || wptr->type == W_PASSWORD)
    {
      XtVaGetValues (GetAsciiTextWidget(wptr->w), XtNfont, &font, NULL);
    }
    else
    {
      XtVaGetValues (wptr->w, XtNfont, &font, NULL);
    }
  }

  return (font);
}


/*
 * Get the next value in a comma separated list.
 * Also unescape the '\' escaping done in ComposeCommaList
 * and convert the single ''' characters back to '"'
 * characters
 */
char *
NextComma (char *string)
{
  char *tptr;

  tptr = string;
  while (*tptr != '\0')
  {
    if (*tptr == '\\')
    {
      *tptr = '\0';
      strcat (string, (char *) (tptr + 1));
      tptr++;
    }
    else if (*tptr == '\'')
    {
      *tptr = '\"';
      tptr++;
    }
    else if (*tptr == ',')
    {
      return (tptr);
    }
    else
    {
      tptr++;
    }
  }
  return (tptr);
}

/*
 * ParseCommaList
 */
static char **
ParseCommaList (char *str, int *count)
{
  char *str_copy;
  char **list;
  char **tlist;
  char *tptr;
  char *val;
  int i, cnt;
  int max_cnt;

  *count = 0;
  if ((str == NULL) || (*str == '\0')) return ((char **) NULL);

  str_copy = XtNewString (str);
  list = (char **) XtMalloc (50 * sizeof (char *));
  max_cnt = 50;

  /*
   * This loop counts the number of objects
   * in this list.
   * As a side effect, NextComma() unescapes in place so
   * "\\" becomes '\' and "\," becomes ',' and "\"" becomes '"'
   */
  cnt = 0;
  val = str_copy;
  tptr = NextComma (val);
  while (*tptr != '\0')
  {
    if ((cnt + 1) == max_cnt)
    {
      tlist = (char **) XtMalloc ((max_cnt + 50) * sizeof (char *));
      for (i = 0; i < cnt; i++)
      {
	tlist[i] = list[i];
      }
      XtFree((char *) list);
      list = tlist;
      max_cnt += 50;
    }
    *tptr = '\0';
    list[cnt] = XtNewString (val);
    cnt++;

    val = (char *) (tptr + 1);
    tptr = NextComma (val);
  }
  list[cnt] = XtNewString (val);
  cnt++;

  XtFree(str_copy);
  str_copy = NULL;
  tlist = (char **) XtMalloc ((cnt + 1) * sizeof (char *));
  for (i = 0; i < cnt; i++)
  {
    tlist[i] = list[i];
  }
  XtFree((char *) list);
  list = tlist;
  list[cnt] = NULL;

  *count = cnt;
  return (list);
}


/*
 * Compose a single string comma separated list from
 * an array of strings.  Any '\', or ',' in the
 * list are escaped with a prepending '\'.
 * So they become '\\' and '\,'
 * Also we want to allow '"' characters in the list, but
 * they would get eaten by the later parsing code, so we will
 * turn '"' into ''', and turn ''' into '\''
 */
char *
ComposeCommaList (char **list, int cnt)
{
  int i;
  char *buf;
  char *tbuf;
  int len, max_len;

  if (cnt == 0) return (XtNewString(""));

  buf = (char *) XtMalloc (sizeof (char) * 1024);
  max_len = 1024;
  len = 0;
  buf[0] = '\0';

  for (i = 0; i < cnt; i++)
  {
    char *option;
    char *tptr;
    int olen;

    option = list[i];
    if (option == NULL)
    {
      olen = 0;
    }
    else
    {
      olen = strlen (option);
    }
    if ((len + (olen * 2)) >= max_len)
    {
      tbuf = XtMalloc(max_len + olen + 1024);
      strcpy(tbuf, buf);
      XtFree(buf);
      buf = tbuf;
      max_len = max_len + olen + 1024;
    }
    tptr = (char *) (buf + len);
    while ((option != NULL) && (*option != '\0'))
    {
      if ((*option == '\\') || (*option == ',') || (*option == '\''))
      {
	*tptr++ = '\\';
	*tptr++ = *option++;
	len += 2;
      }
      else if (*option == '\"')
      {
	*tptr++ = '\'';
	option++;
	len++;
      }
      else
      {
	*tptr++ = *option++;
	len++;
      }
    }
    if (i != (cnt - 1))
    {
      *tptr++ = ',';
      len++;
    }
    *tptr = '\0';
  }

  tbuf = XtNewString(buf);
  XtFree(buf);

  return (tbuf);
}

void
FreeCommaList (char **list, int cnt)
{
  int i;

  for (i = 0; i < cnt; i++)
  {
    if (list[i] != NULL) XtFree(list[i]);
  }
  if (list != NULL) XtFree((char *) list);
}


/*
 * Clean up the mucked value field for a TEXTAREA.
 * Unescape the things with '\' in front of them, and transform
 * lone ' back to "
 */
void
UnMuckTextAreaValue (char *value)
{
  char *tptr;

  if ((value == NULL) || (value[0] == '\0'))
  {
    return;
  }

  tptr = value;
  while (*tptr != '\0')
  {
    if (*tptr == '\\')
    {
      *tptr = '\0';
      strcat (value, (char *) (tptr + 1));
      tptr++;
    }
    else if (*tptr == '\'')
    {
      *tptr = '\"';
      tptr++;
    }
    else
    {
      tptr++;
    }
  }
}


static char *
MapOptionReturn (char *val, char **mapping)
{
  int cnt;

  if (mapping == NULL)
  {
    return (val);
  }

  cnt = 0;
  while (mapping[cnt] != NULL)
  {
    if (strcasecmp (mapping[cnt], val) == 0)
    {
      return (mapping[cnt + 1]);
    }
    cnt += 2;
  }
  return (val);
}


char **
MakeOptionMappings (char **list1, char **list2, int list_cnt)
{
  int i, cnt;
  char **list;

  /*
   * pass through to see how many mappings we have.
   */
  cnt = 0;
  for (i = 0; i < list_cnt; i++)
  {
    if ((list2[i] != NULL) && (*list2[i] != '\0'))
    {
      cnt++;
    }
  }

  if (cnt == 0) return (NULL);

  list = (char **) XtMalloc (((2 * cnt) + 1) * sizeof (char *));

  cnt = 0;
  for (i = 0; i < list_cnt; i++)
  {
    if ((list2[i] != NULL) && (*list2[i] != '\0'))
    {
      list[cnt] = XtNewString (list1[i]);
      list[cnt + 1] = XtNewString (list2[i]);
      cnt += 2;
    }
  }
  list[cnt] = NULL;

  return (list);
}


/*
 * MakeTextAreaWidget
 */
static WidgetInfo *
MakeTextAreaWidget (char *name,
                    HTMLWidget hw,
                    char *text,
                    int x,
                    int y,
                    int id,
                    FormInfo *fptr)	/* not used */
{
  Widget w;
  char **list;
  int list_cnt;
  int rows, cols;
  char *tptr;
  char *value;
  Cardinal argcnt;
  Arg arg[30];

  /*
   * If there is no SIZE, look for ROWS and COLS
   * directly.
   * SIZE is COLUMNS,ROWS parse the list
   */
  rows = -1;
  cols = -1;
  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  if (tptr == NULL)
  {
    tptr = ParseMarkTag (text, MT_INPUT, "ROWS");
    if (tptr != NULL)
    {
      rows = atoi (tptr);
      XtFree(tptr);
      tptr = NULL;
    }
    tptr = ParseMarkTag (text, MT_INPUT, "COLS");
    if (tptr != NULL)
    {
      cols = atoi (tptr);
      XtFree(tptr);
      tptr = NULL;
    }
  }
  else
  {
    list = ParseCommaList (tptr, &list_cnt);
    XtFree(tptr);
    tptr = NULL;

    if (list_cnt == 1)
    {
      cols = atoi (list[0]);
    }
    else if (list_cnt > 1)
    {
      cols = atoi (list[0]);
      rows = atoi (list[1]);
    }
    FreeCommaList (list, list_cnt);
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
  if ((value = ParseMarkTag (text, MT_INPUT, "VALUE")) != NULL)
  {
    UnMuckTextAreaValue (value);
    XtSetArg (arg[argcnt], XtNstring, value); argcnt++;
  }
  w = XtCreateWidget (name,
		      asciiTextWidgetClass, hw->html.view,
		      arg, argcnt);
  setTextSize (w, cols > 0 ? cols : 20, rows > 0 ? rows : 1);

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_TEXTAREA, id, x, y, name, value, NULL, 0));
}

/*
 * MakeTextFieldWidget
 */
/*
 * Enter in a text or password field submits its form, as in other
 * browsers. This is also what makes <isindex> searches (and Chimera's
 * gopher search prompt, which uses one) work: the field has no submit
 * button. The original code had a callback for this, CBActivateField,
 * which nothing called; Return was bound to no-op.
 *
 * The form is looked up when Enter is pressed, from the widget's id,
 * since the FormInfo records are rebuilt when the page is laid out again.
 */
static void
FieldActivate (Widget w,	/* the AsciiText widget of the field */
               XEvent *event,
               String *params,
               Cardinal *num_params)
{
  Widget p;
  HTMLWidget hw;
  WidgetInfo *wptr;
  FormInfo *fptr;

  for (p = w;  p != NULL  &&  !XtIsSubclass (p, htmlWidgetClass);  p = XtParent (p))
    ;
  if (p == NULL)  return;
  hw = (HTMLWidget) p;

  for (wptr = hw->html.widget_list;  wptr != NULL;  wptr = wptr->next)
  {
    if ((wptr->type == W_TEXTFIELD  ||  wptr->type == W_PASSWORD)  &&
	wptr->w != NULL  &&  GetAsciiTextWidget (wptr->w) == w)
      break;
  }
  if (wptr == NULL)  return;

  /* a form holds the widgets with start < id <= end (end -1: open) */
  for (fptr = hw->html.form_list;  fptr != NULL;  fptr = fptr->next)
  {
    if (wptr->id > fptr->start  &&  (fptr->end == -1  ||  wptr->id <= fptr->end))
    {
      CBSubmitForm (wptr->w, (XtPointer) fptr, NULL);
      return;
    }
  }
}

static void
RegisterFieldActivate (Widget w)
{
  static Boolean added = False;
  static XtActionsRec action[] = {
    { "htmlwFieldActivate", (XtActionProc) FieldActivate },
  };

  if (!added)
  {
    XtAppAddActions (XtWidgetToApplicationContext (w), action, 1);
    added = True;
  }
}

static char field_translations[] =
  "<Key>Return:     htmlwFieldActivate()\n\
   <Key>KP_Enter:   htmlwFieldActivate()";

/*
 * maxlength for text fields
 *
 * The text source calls CBTextLimit after every change. Text beyond
 * maxlength (typed or pasted) is removed again, but not from inside the
 * callback: the text widget is still in the middle of its own edit then,
 * and changing the text under it confuses its cursor and positions
 * (differently in each Athena variant). A zero timeout does the trimming
 * as soon as the widget has finished. Athena's own XtNlength only works
 * with a string edited in place, which is not safe here.
 */
static Boolean text_limit_busy = False;
static WidgetInfo *text_limit_pending = NULL;

static void
TextLimitTrim (XtPointer client_data, XtIntervalId *id)
{
  WidgetInfo *wptr = (WidgetInfo *) client_data;
  Widget tw;
  String str = NULL;
  int len;
  XawTextPosition ins;
  XawTextBlock blk;

  text_limit_pending = NULL;
  if (wptr == NULL  ||  wptr->w == NULL  ||  wptr->maxlength <= 0)  return;
  tw = GetAsciiTextWidget (wptr->w);
  XtVaGetValues (tw, XtNstring, &str, NULL);
  if (str == NULL  ||  (len = strlen (str)) <= wptr->maxlength)  return;

  ins = XawTextGetInsertionPoint (tw);
  text_limit_busy = True;	/* the Replace below calls CBTextLimit */
  blk.firstPos = 0;
  blk.length = 0;
  blk.ptr = "";
  blk.format = FMT8BIT;
  XawTextReplace (tw, wptr->maxlength, len, &blk);
  XawTextSetInsertionPoint (tw, ins > wptr->maxlength ?  wptr->maxlength : ins);
  text_limit_busy = False;
}

static void
CBTextLimit (Widget src, XtPointer client_data, XtPointer call_data)
{
  WidgetInfo *wptr = (WidgetInfo *) client_data;

  if (text_limit_busy  ||  wptr == NULL  ||  wptr->maxlength <= 0)  return;
  if (text_limit_pending == wptr)  return;	/* already scheduled */
  text_limit_pending = wptr;
  XtAppAddTimeOut (XtWidgetToApplicationContext (wptr->w), 0,
		   TextLimitTrim, (XtPointer) wptr);
}

static WidgetInfo *
MakeTextFieldWidget (char *name,
                     HTMLWidget hw,
                     char *text,
                     int x,
                     int y,
                     int id,
                     FormInfo *fptr)	/* not used */
{
  char **list;
  int list_cnt;
  int rows = 0, cols;
  char *tptr;
  int type;
  char *value;
  int maxlength;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  /*
   * SIZE can be either COLUMNS or COLUMNS,ROWS
   * we assume COLUMNS,ROWS and parse the list
   */
  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  list = ParseCommaList (tptr, &list_cnt);
  if (tptr != NULL)
  {
    XtFree(tptr);
    tptr = NULL;
  }

  /*
   * If only COLUMNS specified, or SIZE not specified
   * assume a TEXTFIELD
   * Otherwise a TEXTAREA.
   */
  if (list_cnt <= 1)
  {
    type = W_TEXTFIELD;
    if (list_cnt == 1) cols = atoi (list[0]);
    else cols = -1;
  }
  else
  {
    type = W_TEXTAREA;
    cols = atoi (list[0]);
    rows = atoi (list[1]);
  }
  /*
   * Now that we have cols, and maybe rows, free the list
   */
  FreeCommaList (list, list_cnt);

  /*
   * Grab the starting value of the text here.
   * NULL if none.
   */
  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString ("");

  /*
   * For textfields parse maxlength and
   * set up the widget.
   */
  if (type == W_TEXTFIELD)
  {
    maxlength = -1;
    tptr = ParseMarkTag (text, MT_INPUT, "MAXLENGTH");
    if (tptr != NULL)
    {
      maxlength = atoi (tptr);
      XtFree(tptr);
      tptr = NULL;
    }

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    w = XtCreateWidget (name,
			scrollingTextWidgetClass, hw->html.view,
			arg, argcnt);

    /*
     * Set args for asciitext
     */
    argcnt = 0;
    if (maxlength > 0)
    {
      XtSetArg (arg[argcnt], XtNlength, maxlength); argcnt++;
      XtAddCallback (w, XtNdestroyCallback, CBTextDestroy, (XtPointer)value);
    }
    XtSetArg (arg[argcnt], XtNstring, value); argcnt++;
    XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
    XtSetValues (GetAsciiTextWidget(w), arg, argcnt);
    {
      static XtTranslations trans = NULL;

      RegisterFieldActivate (w);
      if (trans == NULL)  trans = XtParseTranslationTable (field_translations);
      XtOverrideTranslations (GetAsciiTextWidget (w), trans);
    }
    
   /* {
    Widget txt = GetAsciiTextWidget(w);
    {
    Widget src = XawTextGetSource(txt);

    fprintf(stderr,
            "TEXT txt=%p source=%p\n",
            txt,
            src);
}
    
    XawTextEditType et;
    char *str = NULL;
    Pixel fg, bg;
    Boolean sensitive;

    XtVaGetValues(txt,
                  XtNeditType, &et,
                  XtNstring, &str,
                  XtNforeground, &fg,
                  XtNbackground, &bg,
                  XtNsensitive, &sensitive,
                  NULL);

    fprintf(stderr,
            "TEXT txt=%p edit=%d sensitive=%d fg=%lu bg=%lu str='%s'\n",
            txt,
            (int)et,
            (int)sensitive,
            (unsigned long)fg,
            (unsigned long)bg,
            str ? str : "(null)");
}*/
    cols = cols > 0 ? cols:20;
    rows = 1;
  }
  else
  {
    /*
     * Else this is a TEXTAREA.  Maxlength is ignored,
     * and we set up the scrolled window
     */
    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
    if (value != NULL)
    {
      XtSetArg (arg[argcnt], XtNstring, value); argcnt++;
    }
    w = XtCreateWidget (name,
			asciiTextWidgetClass, hw->html.view,
			arg, argcnt);
    cols = cols > 0 ? cols : 20;
    rows = rows > 0 ? rows : 1;
  }

  setTextSize (w, cols, rows);

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  {
    WidgetInfo *nw = AddNewWidget (hw, w, type, id, x, y, name, value,
				   NULL, False);

    if (type == W_TEXTFIELD  &&  maxlength > 0)
    {
      nw->maxlength = maxlength;
      XtAddCallback (XawTextGetSource (GetAsciiTextWidget (w)), XtNcallback,
		     CBTextLimit, (XtPointer) nw);
    }
    return (nw);
  }
}

/*static void DebugKeyPress(Widget w,
                          XtPointer client_data,
                          XEvent *event,
                          Boolean *cont)
{
    KeySym ks;
    char buf[32];
    int n;

    n = XLookupString(&event->xkey,
                      buf,
                      sizeof(buf)-1,
                      &ks,
                      NULL);

    buf[n] = '\0';

    fprintf(stderr,
            "KEYPRESS widget=%p keysym=%lu text='%s'\n",
            w,
            (unsigned long)ks,
            buf);
}*/

/*
 * Password fields
 *
 * The field shows one '*' per character; the real text is kept in
 * wptr->password. Typed characters and BackSpace/Delete go through the
 * actions below, which always work at the end of the text. Other editing
 * keys and pasting are ignored, so field and real text stay in step.
 */
static WidgetInfo *
PasswordInfo (Widget w)	/* the AsciiText widget of the field */
{
  Widget p;
  WidgetInfo *wptr;

  for (p = w;  p != NULL  &&  !XtIsSubclass (p, htmlWidgetClass);  p = XtParent (p))
    ;
  if (p == NULL)  return (NULL);

  for (wptr = ((HTMLWidget) p)->html.widget_list;  wptr != NULL;  wptr = wptr->next)
  {
    if (wptr->type == W_PASSWORD  &&  wptr->w != NULL  &&
	GetAsciiTextWidget (wptr->w) == w)
      return (wptr);
  }
  return (NULL);
}

/*
 * Add one character to the end of a password field: '*' in the field,
 * the character itself in wptr->password. Returns False when it cannot
 * be added (not printable, or the field is full).
 */
static Boolean
PasswordAppend (Widget w, WidgetInfo *wptr, int c)
{
  int len;
  XawTextBlock blk;

  if (c < 32  ||  c == 127)  return (False);
  len = (wptr->password != NULL) ?  strlen (wptr->password) : 0;
  if (wptr->maxlength > 0  &&  len >= wptr->maxlength)  return (False);

  blk.firstPos = 0;
  blk.length = 1;
  blk.ptr = "*";
  blk.format = FMT8BIT;
  if (XawTextReplace (w, len, len, &blk) != XawEditDone)  return (False);

  wptr->password = XtRealloc (wptr->password, len + 2);
  wptr->password[len] = (char) c;
  wptr->password[len + 1] = '\0';
  XawTextSetInsertionPoint (w, len + 1);
  return (True);
}

static void
PasswordInsert (Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  WidgetInfo *wptr;
  char buf[8];
  KeySym ks;

  if (event == NULL  ||  event->type != KeyPress)  return;
  if (XLookupString (&event->xkey, buf, sizeof (buf), &ks, NULL) != 1)  return;
  if ((wptr = PasswordInfo (w)) == NULL)  return;
  PasswordAppend (w, wptr, (unsigned char) buf[0]);
}

/*
 * Pasting: the selection (PRIMARY for the middle mouse button, CLIPBOARD
 * for Ctrl+V) is requested as text and added character by character,
 * so the field shows '*'s and the text itself never appears. Line ends
 * and other control characters are skipped; pasting stops when the
 * field is full.
 */
static void
PasswordPasteDone (Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, unsigned long *length, int *format)
{
  WidgetInfo *wptr;
  unsigned long i;

  if (value == NULL)  return;
  if (*type == XA_STRING  &&  *format == 8  &&
      (wptr = PasswordInfo (w)) != NULL)
  {
    for (i = 0;  i < *length;  i++)
    {
      int c = ((unsigned char *) value)[i];

      if (!PasswordAppend (w, wptr, c)  &&  c >= 32  &&  c != 127)
	break;			/* field full */
    }
  }
  XtFree ((char *) value);
}

static void
PasswordPaste (Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  Atom selection = XA_PRIMARY;
  Time t = CurrentTime;

  if (*num_params > 0  &&  strcmp (params[0], "CLIPBOARD") == 0)
    selection = XInternAtom (XtDisplay (w), "CLIPBOARD", False);
  if (event != NULL  &&
      (event->type == ButtonPress  ||  event->type == ButtonRelease))
    t = event->xbutton.time;
  else if (event != NULL  &&  event->type == KeyPress)
    t = event->xkey.time;

  XtGetSelectionValue (w, selection, XA_STRING, PasswordPasteDone, NULL, t);
}

static void
PasswordDelete (Widget w, XEvent *event, String *params, Cardinal *num_params)
{
  WidgetInfo *wptr;
  int len;
  XawTextBlock blk;

  if ((wptr = PasswordInfo (w)) == NULL  ||  wptr->password == NULL)  return;
  len = strlen (wptr->password);
  if (len == 0)  return;

  blk.firstPos = 0;
  blk.length = 0;
  blk.ptr = "";
  blk.format = FMT8BIT;
  XawTextReplace (w, len - 1, len, &blk);
  wptr->password[len - 1] = '\0';
  XawTextSetInsertionPoint (w, len - 1);
}

static XtActionsRec password_actions[] = {
  { "htmlwPasswordInsert", (XtActionProc) PasswordInsert },
  { "htmlwPasswordDelete", (XtActionProc) PasswordDelete },
  { "htmlwPasswordPaste",  (XtActionProc) PasswordPaste },
};

static char password_translations[] =
  "<Key>Return:     htmlwFieldActivate()\n\
   <Key>KP_Enter:   htmlwFieldActivate()\n\
   <Key>Linefeed:   no-op()\n\
   <Key>BackSpace:  htmlwPasswordDelete()\n\
   <Key>Delete:     htmlwPasswordDelete()\n\
   Ctrl<Key>v:      htmlwPasswordPaste(CLIPBOARD)\n\
   <Key>:           htmlwPasswordInsert()\n\
   <Btn2Down>:      no-op()\n\
   <Btn2Up>:        htmlwPasswordPaste(PRIMARY)";

static WidgetInfo *
MakePasswordWidget (char *name,
                    HTMLWidget hw,
                    char *text,
                    int x,
                    int y,
                    int id,
                    FormInfo *fptr)	/* not used */
{
  Arg arg[30];
  int size, maxlength;
  Widget w;
  Cardinal argcnt;
  char *tptr;
  char *value;
  char *stars;
  Pixel bg;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString("");

  size = -1;
  maxlength = -1;

  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  if (tptr != NULL)
  {
    size = atoi (tptr);
    XtFree(tptr);
    tptr = NULL;
  }

  tptr = ParseMarkTag (text, MT_INPUT, "MAXLENGTH");
  if (tptr != NULL)
  {
    maxlength = atoi (tptr);
    XtFree(tptr);
    tptr = NULL;
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  w = XtCreateWidget (name,
		      scrollingTextWidgetClass, hw->html.view,
		      arg, argcnt);
		      
/*XtAddEventHandler(GetAsciiTextWidget(w),
                  KeyPressMask,
                  False,
                  DebugKeyPress,
                  NULL);*/

  argcnt = 0;
  if (maxlength > 0)
  {
    XtAddCallback (w, XtNdestroyCallback, CBTextDestroy, (XtPointer) value);
    XtSetArg (arg[argcnt], XtNlength, maxlength); argcnt++;
  }

  XtVaGetValues (w, XtNbackground, &bg, NULL);
  /*{
    Widget txt = GetAsciiTextWidget(w);
    {
    Widget src = XawTextGetSource(txt);

    fprintf(stderr,
            "PASSWORD txt=%p source=%p\n",
            txt,
            src);
}
    XawTextEditType et;
    char *str = NULL;
    Pixel fg, bg;
    Boolean sensitive;

    XtVaGetValues(txt,
                  XtNeditType, &et,
                  XtNstring, &str,
                  XtNforeground, &fg,
                  XtNbackground, &bg,
                  XtNsensitive, &sensitive,
                  NULL);

    fprintf(stderr,
            "PASSWORD txt=%p edit=%d sensitive=%d fg=%lu bg=%lu str='%s'\n",
            txt,
            (int)et,
            (int)sensitive,
            (unsigned long)fg,
            (unsigned long)bg,
            str ? str : "(null)");
}*/
  /* show one '*' per character of a preset value; the real value goes
   * to wptr->password through AddNewWidget */
  {
    int i, len = strlen (value);

    stars = XtMalloc (len + 1);
    for (i = 0;  i < len;  i++)  stars[i] = '*';
    stars[len] = '\0';
  }
  XtSetArg (arg[argcnt], XtNstring, stars); argcnt++;
  XtSetArg (arg[argcnt], XtNeditType, XawtextEdit); argcnt++;
  //XtSetValues (GetAsciiTextWidget(w), arg, argcnt);
 /*   fprintf(stderr,
        "PASSWORD setting editType=%d argcnt=%d\n",
        (int)XawtextEdit,
        (int)argcnt);*/
        
  XtSetValues (GetAsciiTextWidget(w), arg, argcnt);
  XtFree (stars);		/* the text widget keeps its own copy */

  {
    static Boolean actions_added = False;
    static XtTranslations trans = NULL;

    RegisterFieldActivate (w);
    if (!actions_added)
    {
      XtAppAddActions (XtWidgetToApplicationContext (w),
		       password_actions, XtNumber (password_actions));
      trans = XtParseTranslationTable (password_translations);
      actions_added = True;
    }
    XtOverrideTranslations (GetAsciiTextWidget (w), trans);
  }
   
  /*{
    XawTextEditType et;

    XtVaGetValues(GetAsciiTextWidget(w),
                  XtNeditType, &et,
                  NULL);

    fprintf(stderr,
            "PASSWORD after SetValues editType=%d\n",
            (int)et);
}*/

  setTextSize (w, size < 1 ? 20 : size, 1);

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  {
    WidgetInfo *nw = AddNewWidget (hw, w, W_PASSWORD, id, x, y,
				   name, value, NULL, False);

    nw->maxlength = maxlength;
    return (nw);
  }
}

/*
 * MakeCheckWidget
 */
static WidgetInfo *
MakeCheckWidget (char *name,
                 HTMLWidget hw,
                 char *text,
                 int x,
                 int y,
                 int id,
                 FormInfo *fptr)	/* not used */
{
  Boolean checked = False;
  char *value;
  Cardinal argcnt;
  char *tptr;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString ("on");

  tptr = ParseMarkTag (text, MT_INPUT, "CHECKED");

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  if (tptr != NULL)
  {
    XtSetArg (arg[argcnt], XtNstate, True); argcnt++;
    checked = True;
    XtFree(tptr);
    tptr = NULL;
  }
  XtSetArg (arg[argcnt], XtNlabel, CHECK_MARK); argcnt++;  /* for its size */
  w = XtCreateWidget (name,
		      toggleWidgetClass, hw->html.view,
		      arg, argcnt);
  SetupToggle (w, 0);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_CHECKBOX, id, x, y,
			name, value, NULL, checked));
}

/*
 * MakeHiddenWidget
 */
static WidgetInfo *
MakeHiddenWidget (char *name,
                  HTMLWidget hw,
                  char *text,
                  int x,
                  int y,
                  int id,
                  FormInfo *fptr)	/* not used */
{
  char *value;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  //if (value == NULL) XtNewString("");

  return (AddNewWidget (hw, NULL, W_HIDDEN, id, x, y,
			name, value, NULL, False));
}

/*
 * MakeRadioBoxWidget
 */
static WidgetInfo *
MakeRadioBoxWidget (char *name, HTMLWidget hw, char *text, int x, int y, int id, FormInfo *fptr)
{
  char *value;
  Arg arg[30];
  char *tptr;
  Cardinal argcnt;
  Widget w;
  Boolean checked = False;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if (value == NULL) value = XtNewString ("on");

  /*
   * Only one checked radio button with the
   * same name per form
   */
  tptr = ParseMarkTag (text, MT_INPUT, "CHECKED");
  if ((tptr != NULL) && (AlreadyChecked (hw, fptr, name) == True))
  {
    XtFree (tptr);
    tptr = NULL;
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  if (tptr != NULL)
  {
    XtSetArg (arg[argcnt], XtNstate, True); argcnt++;
    checked = True;
    XtFree (tptr);
    tptr = NULL;
  }
  XtSetArg (arg[argcnt], XtNlabel, CHECK_MARK); argcnt++;  /* for its size */
  w = XtCreateWidget (name,
		      toggleWidgetClass, hw->html.view,
		      arg, argcnt);
  XtAddCallback (w, XtNcallback,
		 (XtCallbackProc) CBChangeRadio, (XtPointer)fptr);
  SetupToggle (w, 1);		/* its mark callback runs after CBChangeRadio */
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  return (AddNewWidget (hw, w, W_RADIOBOX, id, x, y,
			name, value, NULL, checked));
}

/*
 * MakeSubmitWidget
 */
static WidgetInfo *
MakeSubmitWidget (char *name, HTMLWidget hw, char *text, int x, int y, int id, FormInfo *fptr)
{
  char *value;
  char *label;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if ((value == NULL) || (*value == '\0'))
  {
    if (value != NULL) XtFree (value);
    value = XtNewString ("Submit Query");
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  /*
   * <button>Text</button> arrives as an <input> with the text in
   * X-CHIMERA-LABEL (see HTMLfilter.c): shown on the button, while
   * VALUE is what the form sends.
   */
  label = ParseMarkTag (text, MT_INPUT, "X-CHIMERA-LABEL");
  if (label != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, label); argcnt++;
  }
  else if (value != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, value); argcnt++;
  }
  w = XtCreateWidget (name != NULL ? name:"submit",
		      commandWidgetClass, hw->html.view,
		      arg, argcnt);
  if (label != NULL) XtFree (label);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);
  PrepareFormEnd (hw, w, fptr);

  /*
   * AddNewWidget() keeps the value pointer, it does not copy it: the
   * value is sent with the form when this button is pressed. So it must
   * live as long as the widget, and is freed when the widget goes.
   * (It was freed right here, and the form sent whatever text later
   * took that memory.)
   */
  XtAddCallback (w, XtNdestroyCallback, CBTextDestroy, (XtPointer) value);

  return (AddNewWidget (hw, w, W_PUSHBUTTON, id, x, y,
			name, value, NULL, False));
}

/*
 * MakeSelectWidget
 */
static WidgetInfo *
MakeSelectWidget (char *name,
                  HTMLWidget hw,
                  char *text,
                  int x,
                  int y,
                  int id,
                  FormInfo *fptr)	/* not used */
{
  int type = 0;
  Widget w;
  //STRING label;
  Widget scroll;
  Widget pulldown, button, hist;
  char *options;
  char *returns;
  char **list;
  int list_cnt;
  char **vlist;
  int vlist_cnt;
  char **ret_list;
  int return_cnt;
  int i, mult, size;
  char *tptr;
  char *value;
  char **mapping;
  Cardinal argcnt;
  Arg arg[30];

  tptr = ParseMarkTag (text, MT_INPUT, "HINT");
  if (tptr != NULL)
  {
    if (strcasecmp(tptr, "list") == 0) type = W_LIST;
    else if (strcasecmp (tptr, "menu") == 0) type =  W_OPTIONMENU;
    XtFree(tptr);
    tptr = NULL;
  }
  else type = -1;

  size = 5;
  tptr = ParseMarkTag (text, MT_INPUT, "SIZE");
  if (tptr != NULL)
  {
    size = atoi (tptr);
    if ((size > 1) && (type == -1)) type = W_LIST;
    XtFree (tptr);
    tptr = NULL;
  }

  mult = 0;
  tptr = ParseMarkTag (text, MT_INPUT, "MULTIPLE");
  if (tptr != NULL)
  {
    if (type == -1) type = W_LIST;
    mult = 1;
    XtFree (tptr);
    tptr = NULL;
  }

  if (type == -1)
  {
    type = W_OPTIONMENU;
  }

  //label = NULL;
  hist = NULL;
  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  options = ParseMarkTag (text, MT_INPUT, "OPTIONS");
  list = ParseCommaList (options, &list_cnt);
  if (options != NULL)
  {
    XtFree (options);
    options = NULL;
  }

  returns = ParseMarkTag (text, MT_INPUT, "RETURNS");
  ret_list = ParseCommaList (returns, &return_cnt);
  if (returns != NULL)
  {
    XtFree (returns);
    returns = NULL;
  }

  /*
   * If return_cnt is less than list_cnt, the user made
   * a serious error.  Try to recover by padding out
   * ret_list with NULLs
   */
  if (list_cnt > return_cnt)
  {
    int rcnt;
    char **rlist;

    rlist = (char **) XtMalloc (list_cnt * sizeof (char *));
    for (rcnt = 0; rcnt < return_cnt; rcnt++)
    {
      rlist[rcnt] = ret_list[rcnt];
    }
    for (rcnt = return_cnt; rcnt < list_cnt; rcnt++)
    {
      rlist[rcnt] = NULL;
    }
    if (ret_list != NULL) XtFree ((char *) ret_list);
    ret_list = rlist;
  }

  vlist = ParseCommaList (value, &vlist_cnt);

  if (size > list_cnt) size = list_cnt;
  if (size < 1) size = 1;

  mapping = MakeOptionMappings (list, ret_list, list_cnt);

  if (type == W_OPTIONMENU)
  {
    XFontStruct *font;
    Dimension maxWidth = 0, width, iW;

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    XtSetArg (arg[argcnt], XtNlabel, list[0]); argcnt++;
    w = XtCreateWidget (name,
			menuButtonWidgetClass, hw->html.view,
			arg, argcnt);
    pulldown = XtCreatePopupShell ("menu",
				   simpleMenuWidgetClass, w,
				   NULL, 0);

    for (i = 0; i < list_cnt; i++)
    {
      char bname[30];

      sprintf (bname, "Button%d", (i + 1));
      argcnt = 0;
      XtSetArg (arg[argcnt], XtNlabel, list[i]); argcnt++;
      button = XtCreateWidget (bname,
			       smeBSBObjectClass, pulldown,
                               arg, argcnt);
      XtManageChild (button);

      XtAddCallback (button, XtNcallback,
		     CBoption, (XtPointer) w);

      if (i == 0)
      {
	XtVaGetValues (w,
		       XtNfont, &font,
		       XtNinternalWidth, &iW,
		       NULL);
      }

      width = XTextWidth (font, list[i], strlen (list[i]));

      if (width > maxWidth) maxWidth = width;

      if ((vlist_cnt > 0) && (vlist[0] != NULL) &&
	  (strcasecmp (vlist[0], list[i]) == 0))
      {
	hist = button;
	XtVaSetValues (w,
		       XtNlabel, list[i],
		       NULL);
      }

      /*
       * Start hist out as the first button
       * so that if the user didn't set a
       * default we always default to the
       * first element.
       */
      if ((i == 0) && (hist == NULL))
      {
	hist = button;
      }
    }

    XtVaSetValues (w, XtNwidth, maxWidth + (4 * iW), NULL);

    FreeCommaList (vlist, vlist_cnt);
    if (value != NULL)
    {
      XtFree (value);
      value = NULL;
    }

    if (hist != NULL)
    {
      /*
       * A gaggage.  Value is used to later
       * restore defaults.  For option menu
       * this means we need to save a child
       * widget id as opposed to the
       * character string everyone else uses.
       */
      value = (char *) hist;
    }
  }
  else /* type == W_LIST */
  {
    STRING *string_list;
    STRING *val_list;

    if (list_cnt < 1)
    {
      if (size < 1) size = 1;

      list_cnt = vlist_cnt = size;

      string_list = (STRING *) XtMalloc (list_cnt * sizeof (STRING));
      for (i = 0; i < list_cnt; i++)
      {
	string_list[i] = XtNewString ("");
      }

      val_list = (STRING *) XtMalloc (vlist_cnt * sizeof (STRING));
      for (i = 0; i < vlist_cnt; i++)
      {
	val_list[i] = XtNewString ("");
      }
    }
    else
    {
      if ((!mult) && (vlist_cnt > 1))
      {
	XtFree (value);
	value = XtNewString (vlist[0]);
      }

      string_list = (STRING *) XtMalloc (list_cnt * sizeof (STRING));
      for (i = 0; i < list_cnt; i++)
      {
	string_list[i] = XtNewString (list[i]);
      }

      val_list = (STRING *) XtMalloc (vlist_cnt * sizeof (STRING));
      for (i = 0; i < vlist_cnt; i++)
      {
	val_list[i] = XtNewString (vlist[i]);
      }

      //FreeCommaList (list, list_cnt);
      FreeCommaList (vlist, vlist_cnt);
    }

    argcnt = 0;
    XtSetArg (arg[argcnt], XtNx, x); argcnt++;
    XtSetArg (arg[argcnt], XtNy, y); argcnt++;
    XtSetArg (arg[argcnt], XtNallowVert, True); argcnt++;
    /* the app-defaults set *Viewport*allowHoriz: True for other windows */
    XtSetArg (arg[argcnt], XtNallowHoriz, False); argcnt++;
    /*
     * I pulled these numbers out of my butt.  john.
     */
    XtSetArg (arg[argcnt], XtNwidth, 200); argcnt++;
    XtSetArg (arg[argcnt], XtNheight, 100); argcnt++;

    scroll = XtCreateWidget ("Scroll",
			     viewportWidgetClass, hw->html.view,
                             arg, argcnt);

    argcnt = 0;
    /* one entry per row: without forceColumns the List fills its
     * width with several columns ("a b / c d") */
    XtSetArg (arg[argcnt], XtNdefaultColumns, 1); argcnt++;
    XtSetArg (arg[argcnt], XtNforceColumns, True); argcnt++;
    //note: multiple list are shown but multi select not possible - only one otion can be selected at a time
    w = XtCreateWidget (name,
			listWidgetClass, scroll,
                        arg, argcnt);
    XtManageChild (w);

    XtAddCallback (w, XtNdestroyCallback, CBListDestroy, NULL);

    XawListChange (w, string_list, list_cnt, 0, True);

    if (vlist_cnt > 0)
    {
#ifdef VERBOSE
      if (vlist_cnt > 1)
      {
	fprintf (stderr, "HTML: only a single selection allowed!\n");
      }
#endif
      for (i = 0; i < list_cnt; i++)
      {
	if (!strcasecmp (string_list[i], val_list[0]))
	{
	  XawListHighlight (w, i);
	  break;
	}
      }
    }

    /*
     * Size the Viewport to the list. XawListChange() above has given the
     * List its natural size: as wide as the longest entry, as tall as
     * all entries. If they all fit in SIZE rows the Viewport is exactly
     * that and has no scrollbar; otherwise it shows SIZE rows and the
     * vertical scrollbar. (The fixed sizes used before left part of the
     * list outside, so even short lists got a squeezed scrollbar.)
     */
    {
      Dimension lw = 0, lh = 0, ih = 0;
      int rows = list_cnt > 0 ? list_cnt : 1;

      if (size < 1) size = 1;
      XtVaGetValues (w, XtNwidth, &lw, XtNheight, &lh,
		     XtNinternalHeight, &ih, NULL);

      if (size >= rows)
      {
	XtVaSetValues (scroll, XtNallowVert, False,
		       XtNwidth, lw, XtNheight, lh, NULL);
      }
      else
      {
	int row = ((int) lh - 2 * (int) ih) / rows;

	XtVaSetValues (scroll,
		       XtNwidth, lw + LIST_SCROLLBAR_SPACE,
		       XtNheight, 2 * ih + size * row,
		       NULL);
      }
    }

    w = scroll;

    for (i = 0; i < vlist_cnt; i++)
    {
      if (val_list[i] != NULL) XtFree(val_list[i]);
    }
    if (val_list != NULL) XtFree((char *) val_list);
  }

  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);

  if (list != NULL) {
    // 1. Free each string inside the list
    for (i = 0; i < list_cnt; i++) {
      if (list[i] != NULL) {
        XtFree(list[i]);
      }
    }
    // 2. Free the list itself
    XtFree((char *) list);
    list = NULL;
  }

  if (ret_list != NULL) {
    // 3. Do the same for ret_list
    for (i = 0; i < return_cnt; i++) {
      if (ret_list[i] != NULL) {
        XtFree(ret_list[i]);
      }
    }
    // 4. Free the ret_list itself
    XtFree((char *) ret_list);
    ret_list = NULL;
  }
  /* ----------------------------------------------- */

  
  
  return (AddNewWidget (hw, w, type, id, x, y,
			name, value, mapping, False));
}

/*
 * MakeResetWidget
 */
static WidgetInfo *
MakeResetWidget (char *name, HTMLWidget hw, char *text, int x, int y, int id, FormInfo *fptr)
{
  char *value;
  char *label;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");
  if ((value == NULL) || (*value == '\0'))
  {
    if (value != NULL) XtFree (value);
    value = XtNewString ("Reset");
  }

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  /*
   * <button>Text</button> arrives as an <input> with the text in
   * X-CHIMERA-LABEL (see HTMLfilter.c): shown on the button, while
   * VALUE is what the form sends.
   */
  label = ParseMarkTag (text, MT_INPUT, "X-CHIMERA-LABEL");
  if (label != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, label); argcnt++;
  }
  else if (value != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, value); argcnt++;
  }
  w = XtCreateWidget (name,
		      commandWidgetClass, hw->html.view,
		      arg, argcnt);
  if (label != NULL) XtFree (label);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);
  PrepareFormReset (hw, w, fptr);
  XtAddCallback (w, XtNdestroyCallback, CBTextDestroy, (XtPointer) value);

  return (AddNewWidget (hw, w, W_PUSHBUTTON, id, x, y,
			name, value, NULL, False));
}

/*
 * MakePushButtonWidget
 */
static WidgetInfo *
MakePushButtonWidget (char *name,
                      HTMLWidget hw,
                      char *text,
                      int x,
                      int y,
                      int id,
                      FormInfo *fptr)	/* not used */
{
  char *value;
  char *label;
  Cardinal argcnt;
  Arg arg[30];
  Widget w;

  value = ParseMarkTag (text, MT_INPUT, "VALUE");

  argcnt = 0;
  XtSetArg (arg[argcnt], XtNx, x); argcnt++;
  XtSetArg (arg[argcnt], XtNy, y); argcnt++;
  /*
   * <button>Text</button> arrives as an <input> with the text in
   * X-CHIMERA-LABEL (see HTMLfilter.c): shown on the button, while
   * VALUE is what the form sends.
   */
  label = ParseMarkTag (text, MT_INPUT, "X-CHIMERA-LABEL");
  if (label != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, label); argcnt++;
  }
  else if (value != NULL)
  {
    XtSetArg (arg[argcnt], XtNlabel, value); argcnt++;
  }
  w = XtCreateWidget (name,
		      commandWidgetClass, hw->html.view,
		      arg, argcnt);
  if (label != NULL) XtFree (label);
  XtSetMappedWhenManaged (w, False);
  XtManageChild (w);
  if (value != NULL)
    XtAddCallback (w, XtNdestroyCallback, CBTextDestroy, (XtPointer) value);

  return (AddNewWidget (hw, w, W_PUSHBUTTON, id, x, y,
			name, value, NULL, False));
}

/*
 * Give the widget a new x,y location in the document (wp->x, wp->y) and
 * put it there on the screen, which is that position minus the scroll
 * offset, as ScrollWidgets() does.
 *
 * This used to set XtNx/XtNy to the document position, between
 * XtUnmanageChild() and XtManageChild(). Every layout while scrolled
 * down (each batch of background images) then unmapped and remapped
 * the form widgets, and showed them for a moment where they would be
 * with no scrolling: borders blinking over other parts of the page.
 * XtMoveWidget() moves the window, without a geometry request to the
 * parent and without unmapping it, and only when the place changes.
 */
void
MoveWidget (HTMLWidget hw, WidgetInfo *wp, int x, int y)
{
    wp->x = x;
    wp->y = y;

    /* type HIDDEN has no widget */
    if (wp->w != NULL)
    {
      Position sx = (Position) (x - hw->html.scroll_x);
      Position sy = (Position) (y - hw->html.scroll_y);

      if (wp->w->core.x != sx || wp->w->core.y != sy)
	XtMoveWidget (wp->w, sx, sy);
    }
}


/*
 * Make the appropriate widget for this tag, and fill in an
 * WidgetInfo structure and return it.
 */
WidgetInfo *
MakeWidget (HTMLWidget hw, char *text, int x, int y, int id, FormInfo *fptr)
{
  WidgetInfo *wlist;
  WidgetInfo *wptr;

  for (wlist = hw->html.widget_list;  wlist != NULL;  wlist = wlist->next)
  {
    if (wlist->id == id)
      break;
  }

  /*
   * If this widget is not on the list, we have never
   * used it before.  Create it now.
   */
  if (wlist == NULL)
  {
    int i;
    char *name;
    char *type_str;
    static struct typefunc
    {
      char *name;
      WidgetInfo *(*func) ();
    }
    typefunclist[] =
    {
      { "checkbox", MakeCheckWidget },
      { "hidden", MakeHiddenWidget },
      { "radio", MakeRadioBoxWidget },
      { "submit", MakeSubmitWidget },
      { "reset", MakeResetWidget },
      { "button", MakePushButtonWidget },
      { "select", MakeSelectWidget },
      { "textarea", MakeTextAreaWidget },
      { "password", MakePasswordWidget },
      { NULL, NULL },
    };

    name = ParseMarkTag (text, MT_INPUT, "NAME");
    type_str = ParseMarkTag (text, MT_INPUT, "TYPE");
    if (type_str == NULL)
    {
      wptr = MakeTextFieldWidget (name, hw, text, x, y, id, fptr);
    }
    else
    {
      wptr = NULL;
      for (i = 0; typefunclist[i].name != NULL; i++)
      {
	if (strcasecmp (type_str, typefunclist[i].name) == 0)
	{
	  wptr = (typefunclist[i].func) (name, hw, text, x, y, id, fptr);
	  break;
	}
      }
      if (wptr == NULL)		/* this could be a disaster */
      {
	wptr = MakeTextFieldWidget (name, hw, text, x, y, id, fptr);
      }
    }
    if (type_str != NULL)
    {
      XtFree(type_str);
      type_str = NULL;
    }
  }
  else
    /*
     * We found this widget on the list of already created widgets.
     * Move it to its new location.
     */
  {
    MoveWidget (hw, wlist, x, y);
    wptr = wlist;
  }

  return (wptr);
}


void
WidgetRefresh (HTMLWidget hw, struct ele_rec *eptr)
{
    if (eptr == NULL)
    {
        fprintf(stderr, "  NULL eptr\n");
        return;
    }

    if (eptr->widget_data == NULL)
        return;

  if ((eptr->widget_data != NULL) && (eptr->widget_data->mapped == False) &&
      (eptr->widget_data->w != NULL))
  {
    eptr->widget_data->mapped = True;
    XtSetMappedWhenManaged (eptr->widget_data->w, True);
  }
}
