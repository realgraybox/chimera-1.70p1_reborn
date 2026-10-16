
/*
  A small vertical, xlib only, single parameter, general purpose pop-up menu.
  version 3.00, Copyright (C) 2005 Terry Loveall, <loveall@iinet.com>
  THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY OR BINARIES. COMPILE AND USE 
  AT YOUR OWN RISK.
*/

#include	"menu.h"

char xmenu_version[] = "Menu 3.00\n";
Display * xmenu_dpy;			/* The connection to the X server. */
int display_width;		/* The width of the screen we're managing. */
int display_height;		/* The height of the screen we're managing. */

Window xmenu_window;			/* Main window. */
Window xmenu_root;			/* Root window. */
GC xmenu_gc;					/* The default GC. */

unsigned long xmenu_black;	/* Black pixel. */
unsigned long xmenu_white;	/* White pixel. */

XFontStruct * xmenu_font;		/* Font. */
Cursor xmenu_mouse_cursor;	/* Mouse cursor. */

MenuItem * menu = 0;
MenuItem * selected = 0;

FILE * shell;
char * param = NULL;
char * argv0;
int numlines = 0;
int cury = -1;
int max_chars = 0;
int WinHeight;
int WinWidth;
int not_done = -1;
int itemHeight;

static void readMenu(int argc, char *argv[]);
static MenuItem *whichItem(int mouseY);
static void addMenuItem(char *, char *);
static void getEvent(XEvent *);
static void initCursor(void);

static void execute(MenuItem *);
static void nullEvent(XEvent *);
static void buttonPress(XEvent *);
static void buttonRelease(XEvent *);
static void mouseMoved(XEvent *);
static void expose(XEvent *);
static void mappingnotify(XEvent *);

/* Dispatcher for main event loop. */
typedef struct Disp Disp;
struct Disp {
	int	type;
	void	(*handler)(XEvent *);
};

static Disp disps[] = {
	{NullEvent, nullEvent},
	{ButtonPress, buttonPress},
	{ButtonRelease, buttonRelease},
	{MotionNotify, mouseMoved},
	{Expose, expose},
	{EnterNotify, nullEvent},
	{LeaveNotify, nullEvent},
	{MappingNotify, mappingnotify},
	{VisibilityNotify, nullEvent},
};

/*ARGSUSED*/
#if MCB
int xmenu_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	XEvent ev;
	XGCValues gv;
	XSetWindowAttributes attr;
	int root_x, root_y, dudi;
	Window dud;
	
	argv0 = argv[0];
	
	/* Open a connection to the X server. */
	xmenu_dpy = XOpenDisplay("");
	if (xmenu_dpy == 0)
		Panic("can't open display.");
	
	/* Find the screen's dimensions. */
	display_width = DisplayWidth(xmenu_dpy, DefaultScreen(xmenu_dpy));
	display_height = DisplayHeight(xmenu_dpy, DefaultScreen(xmenu_dpy));
	
	/* Get the pixel values of the only two colours we use. */
	xmenu_black = BlackPixel(xmenu_dpy, DefaultScreen(xmenu_dpy));
	xmenu_white = WhitePixel(xmenu_dpy, DefaultScreen(xmenu_dpy));
	
	/* Get font. */
	xmenu_font = XLoadQueryFont(xmenu_dpy, "7x14");
	if (xmenu_font == 0)
		Panic("can't find a font.");
	
	/* Get a cursor. */
	initCursor();
	
	shell=popen("/bin/sh","w");

	/* Create the menu items. */
	readMenu(argc, argv);
	
	/* Create the window. */
	xmenu_root = DefaultRootWindow(xmenu_dpy);
	attr.override_redirect = True;
	attr.background_pixel = xmenu_white;
	attr.border_pixel = xmenu_black;
	attr.cursor = xmenu_mouse_cursor;
	attr.event_mask = ExposureMask | VisibilityChangeMask |
		ButtonMotionMask | PointerMotionMask |
		ButtonPressMask | ButtonReleaseMask | StructureNotifyMask |
		EnterWindowMask | LeaveWindowMask | KeyPressMask;
	WinHeight = numlines * (1.2 * (xmenu_font->ascent + xmenu_font->descent));
	WinWidth = max_chars + 20;

	XQueryPointer(xmenu_dpy, xmenu_root, &dud, &dud, &root_x, &root_y, &dudi, &dudi, (unsigned int *)&dudi);

	xmenu_window = XCreateWindow(xmenu_dpy, xmenu_root,
		root_x - (WinWidth / 2) >= 0 ? root_x - (WinWidth / 2) : 0,
		(root_y - WinHeight - 5) >= 0 ? root_y - WinHeight - 5 : 0,
		WinWidth, WinHeight,
		1, CopyFromParent, InputOutput, CopyFromParent,
		CWOverrideRedirect | CWBackPixel | CWBorderPixel |
		CWCursor | CWEventMask,
		&attr);

	/* Create GC. */
	gv.foreground = xmenu_black;
	gv.background = xmenu_white;
	gv.font = xmenu_font->fid;
	xmenu_gc = XCreateGC(xmenu_dpy, xmenu_window, GCForeground | GCBackground | GCFont,
		&gv);
	
	// set itemHeight to font full height
	itemHeight = (1.2 * (xmenu_font->ascent + xmenu_font->descent));

	/* Bring up the window. */
	XMapRaised(xmenu_dpy, xmenu_window);
	
	XGrabKeyboard(xmenu_dpy, xmenu_window, True, GrabModeAsync, GrabModeAsync, CurrentTime);

	/* Make sure all our communication to the server got through. */
	XSync(xmenu_dpy, False);
	
	/* The main event loop. */
	while (not_done) {
		getEvent(&ev);
		dispatch(&ev);
	}
	XUngrabKeyboard(xmenu_dpy, CurrentTime);
	pclose(shell);
	exit(0);
}

static void keyItem(int inc)
{
	if((inc < 0) && (cury <= itemHeight)) cury = (numlines +1) * itemHeight;
	if((inc > 0) && (cury >= (numlines * itemHeight)-1)) cury = -1;
	cury += inc;
	selected = whichItem(cury);
	expose(0);
}

static void getEvent(XEvent * ev) {
	int fd;
	fd_set readfds;
	struct timeval tv;
	
	/* Is there a message waiting? */
	if (QLength(xmenu_dpy) > 0) {
		XNextEvent(xmenu_dpy, ev);
		return;
	}
	
	/* Beg... */
	XFlush(xmenu_dpy);
	
	/* Wait 1 second to see if a message arrives. */
	fd = ConnectionNumber(xmenu_dpy);
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (select(fd + 1, &readfds, 0, 0, &tv) == 1) {
		XKeyEvent *keve = (XKeyEvent *)ev;
		XNextEvent(xmenu_dpy, ev);
		if(keve->type == KeyRelease)
		{
			int count;
			KeySym skey;
			char astr[10] = "";
			// convert key event to X-ascii in skey
			count = XLookupString(keve, astr, sizeof (astr), &skey, NULL);
			if(skey == 0xff1b) not_done = 0;			// <Esc> just exits
			if(skey == 0xff52) keyItem(-itemHeight);	// Up - prev
			if(skey == 0xff54) keyItem(itemHeight);		// Down - next
			if(skey == 0xff0d) buttonRelease(NULL);		// <Enter> execute selected
		}
		return;
	}
	
	/* No message, so we have a null event. */
	ev->type = NullEvent;
}

static void
initCursor() {
	XColor red, xmenu_white, exact;
	Colormap cmp = DefaultColormap(xmenu_dpy, DefaultScreen(xmenu_dpy));
	XAllocNamedColor(xmenu_dpy, cmp, "red", &red, &exact);
	XAllocNamedColor(xmenu_dpy, cmp, "white", &xmenu_white, &exact);
	xmenu_mouse_cursor = XCreateFontCursor(xmenu_dpy, XC_left_ptr);
}

static void
addMenuItem(char * name, char * command) {
    int tmp = 0;
	MenuItem * newItem = (MenuItem *) malloc(sizeof(MenuItem));
	if (newItem == 0)
		return;
	
	newItem->next = 0;
	newItem->name = strdup(name);
	newItem->command = strdup(command);
	tmp = XTextWidth(xmenu_font, newItem->name, strlen(newItem->name));
	
	if (menu == 0) {
		menu = newItem;
	} else {
		MenuItem * last = menu;
		while (last->next != 0)
			last = last->next;
		last->next = newItem;
	}
	numlines++; // count lines in menu
	if(tmp > max_chars) max_chars = tmp;
}

static void
readMenu(int argc, char *argv[]) {
	FILE * fp;
	char* f;
	char line[BUFSIZ];

	switch (argc)
	{
		case 1:
			f=".menu";		/* default menu file to open in current dir */
			break;
		case 2:
			f=argv[1];		/* named menu file to open */
			break;
		case 3:
			f=argv[1];		/* named  menu file to open */
			param = argv[2];	/* passed param always requires named menu file */
			break;
		default:
			fprintf(stderr,"usage: menu [menu-file] [parameter]\n");
			exit(1);
	}

	if (param) {  /* 'param' set to argv[2] */
		/*
		 * Pass argv[2] as shell variable $F, in single quotes so spaces,
		 * '&' and ';' stay part of it. A ' inside is written as '\''.
		 */
		const char *p;
		fputs("F='", shell);
		for (p = param; *p; p++) {
			if (*p == '\'') fputs("'\\''", shell);
			else fputc(*p, shell);
		}
		fputs("'\n", shell);
		fflush(shell);
	}

	fp = fopen(f, "r");
	if (fp == 0) {
		addMenuItem("localhost", "exec chimera-term");
		return;
	}
	
	while (fgets(line, BUFSIZ, fp) != 0) {
		char * tab = strchr(line, '\t');
		if (*line == '#' || tab == 0)
			continue;
		if(tab == line) {
			fputs(++tab,shell);	/* execute inline tab prefixed shell code */
			fflush(shell);
			continue;
		}
		*tab = 0;
		addMenuItem(line, tab+1);
	}
	
	fclose(fp);
}

static MenuItem *
whichItem(int mouseY)
{
	int y = 0;
	MenuItem * item;
	
	for (item = menu; item != 0; item = item->next) {
		if (mouseY >= y && mouseY <= (y + itemHeight)) {
			cury = mouseY;
			return item;
		}
		y += itemHeight;
	}
	
	return 0;
}

extern void
dispatch(XEvent * ev) {
	Disp * p;
	
	for (p = disps; p < disps + sizeof disps / sizeof disps[0]; p++) {
		if (p->type == ev->type) {
			if (p->handler != 0)
				p->handler(ev);
			return;
		}
	}
}

static void
expose(XEvent * ev) {
	int x, y;
	MenuItem * item;
	
	/* Only handle the last in a group of Expose events. */
	if (ev && ev->xexpose.count != 0)
		return;
	
	/* Clear the window. */
	XClearWindow(xmenu_dpy, xmenu_window);
	XSetForeground(xmenu_dpy, xmenu_gc, xmenu_black);
	
	/* Draw the menu. */
	x = 10;
	y = 0 + ( 1.1 * (xmenu_font->ascent));

	for (item = menu; item != 0; item = item->next) {
		if (item == selected) {
			XSetFunction(xmenu_dpy, xmenu_gc, GXinvert);
			XFillRectangle(xmenu_dpy, xmenu_window, xmenu_gc, 0, y - (1.2 * (xmenu_font->ascent + xmenu_font->descent)) + 3, WinWidth, 19);
			XSetFunction(xmenu_dpy, xmenu_gc, GXcopy);
			XSetForeground(xmenu_dpy, xmenu_gc, xmenu_white);
		} else {
			XSetForeground(xmenu_dpy, xmenu_gc, xmenu_black);
		}
		XDrawString(xmenu_dpy, xmenu_window, xmenu_gc, x, y,item->name, strlen(item->name));
		y += (1.2 * (xmenu_font->ascent + xmenu_font->descent));
	}
}

static void
nullEvent(XEvent * ev) {
	expose(0);
}

static void
mouseMoved(XEvent * ev) {
	while (XCheckTypedEvent(xmenu_dpy, MotionNotify, ev)) ;
	
	selected = whichItem(ev->xmotion.y); expose(0);
}

static void
buttonPress(XEvent * ev) {
	selected = whichItem(ev->xbutton.y);
	expose(0);
}

static void
buttonRelease(XEvent * ev) {
	if (selected != 0) {
		execute(selected);
		selected = 0;
	}
	expose(0);
}

static void
mappingnotify(XEvent * ev) {
	XRefreshKeyboardMapping((XMappingEvent *) ev);
}

static void
execute(MenuItem * item)
{
	not_done = 0;	// exit after one execution

	if (item->command != 0) fputs(item->command,shell);
	fflush(shell);
}

void
Panic(char *s) {
	fprintf(stderr, "%s: %s\n", argv0, s);
	exit(EXIT_FAILURE);
}
