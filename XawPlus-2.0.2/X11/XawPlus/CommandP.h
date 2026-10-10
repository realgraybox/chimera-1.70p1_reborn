/*
* $XConsortium: CommandP.h,v 1.30 90/12/01 13:00:10 rws Exp $
*/


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

3D extensions added : Roland Krause 1996
Helper function added: Roland Krause 1998
Redesign for KDE styled buttons: Roland Krause 1999

******************************************************************/

/* 
 * CommandP.h - Private definitions for Command widget
 * 
 */

#ifndef _XawCommandP_h
#define _XawCommandP_h

#include <X11/XawPlus/Command.h>
#include <X11/XawPlus/LabelP.h>

/***********************************************************************
 *
 * Command Widget Private Data
 *
 ***********************************************************************/

#define HELP_TIMER		1200	/* Time until the help window pops up */
#define DEFAULT_THICKNESS 	2	/* Default for 'highlight_thickness' */

/***********************************************************************
 *
 *  Class structure
 *
 ***********************************************************************/


/* New fields for the Command widget class record */

typedef struct _CommandClass 
{
  int makes_compiler_happy;  /* not used */
} CommandClassPart;

/* Full class record declaration */

typedef struct _CommandClassRec
{
  CoreClassPart	core_class;
  Add3dExtClassPart	add3d_class;
  SimpleClassPart	simple_class;
  LabelClassPart	label_class;
  CommandClassPart    command_class;
} CommandClassRec;

extern CommandClassRec commandClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

/* New fields for the Command widget record */

typedef struct {
    String		helpText;		/* resources used for the helper */
    unsigned char	helpEncoding;
    Boolean		useHelp;
    Pixel		helpBackground;

    Dimension		highlight_thickness;	/* border around the button */
    Boolean		highlightMode;		/* New highlight mode */
    XtCallbackList	callbacks;

/* private state */

    Pixmap      	gray_pixmap; 
    Boolean     	set;

    XtIntervalId	helpTimer;   		/* used for the helper */
    Widget		helpWidget,
			helpLabel;
} CommandPart;

   /* Full widget declaration */
typedef struct _CommandRec {
    CorePart         core;
    Add3dExtPart     add3dExt;
    SimplePart	     simple;
    LabelPart	     label;
    CommandPart      command;
} CommandRec;

#endif /* _XawCommandP_h */


