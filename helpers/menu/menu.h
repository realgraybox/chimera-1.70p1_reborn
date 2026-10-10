#define DEFAULT_FONT	"lucidasans-bold-14"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#define NullEvent -1

typedef struct MenuItem MenuItem;
struct MenuItem {
	MenuItem * next;
	
	char * name;
	char * command;
};

/*	menu.c	*/
extern Display * dpy;
extern int display_width;
extern int display_height;
extern int WinHeight;
extern int not_done;
extern char * display_string;
extern Window xmenu_root;
extern Window xmenu_window;
extern GC xmenu_gc;
extern unsigned long xmenu_black;
extern unsigned long xmenu_white;
extern XFontStruct * xmenu_font;
extern MenuItem * menu;
extern MenuItem * selected;
extern char * argv0;
extern char * argv1;
extern int WinWidth;

/*	disp.c	*/
extern void dispatch(XEvent *);

/*	error.c	*/
extern int ErrorHandler(Display *, XErrorEvent *);
extern void Panic(char *);
