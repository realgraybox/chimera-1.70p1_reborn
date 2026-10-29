/*
 * main.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "charset.h"
#include "options.h"

#include <stdio.h>
#include <locale.h>

#include <ctype.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>

#include <stdlib.h>

#include <unistd.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <xaw_wrapper.h>
//#include <X11/Xaw/Toggle.h>	/* for DeferPix (WBE) */

#include "StrReq.h"
#include "AuthReq.h"
#include "OutputSel.h"
#include "Bookmark.h"

#ifndef __STDC__
#define _NO_PROTO 1
#endif

#include <HTMLP.h>

#include "HTML.h"

#include "common.h"
#include "util.h"
#include "url.h"
#include "mime.h"
#include "document.h"
#include "cache.h"
#include "convert.h"
#include "inline.h"
#include "http.h"
#include "input.h"
#include "net.h"
#include "lang.h"
#include "stringdb.h"
#include "widget.h"
#include "local.h"

/* Flag which tells if program fhould close */
static volatile sig_atomic_t g_should_quit = 0;

#ifdef IMAGENEW
extern char *ParseMarkTag ();
extern int global_html_width;
extern int global_html_height;
#endif

/*
 * convenience routines.
 */
static Document *LoadDoc _ArgProto((URLParts *, int, int));
static void HandleDoc _ArgProto((Document *, int));
static void AddMessage _ArgProto((char *));
static void DisplayCurrent _ArgProto((void));
static char *GetTitle _ArgProto((Widget));
/*static*/ void SetTitle _ArgProto((char *));
static void SetURL _ArgProto((URLParts *));
static void sigigh_handler _ArgProto(());
static void SaveDocument _ArgProto((Document *, char *, int));
static void MailDocument _ArgProto((Document *, char *, int));
static void PrintDocument _ArgProto((Document *, char *, int));
static void SaveOCallback _ArgProto((Widget, XtPointer, XtPointer));
static void SaveDCallback _ArgProto((Widget, XtPointer, XtPointer));
static void AuthOKCallback _ArgProto((Widget, XtPointer, XtPointer));
static void AuthDismissCallback _ArgProto((Widget, XtPointer, XtPointer));
static void SearchCallback _ArgProto((Widget, XtPointer, XtPointer));
static void InputHandler _ArgProto((XtPointer, int *, XtInputId *));
static void CreateStrReq _ArgProto((char *, char *, int, XtCallbackProc, XtCallbackProc, XtPointer));
//static void WarningHandler _ArgProto((String));
static void ErrorHandler _ArgProto((String));
void Siaghelp(char *p);

int DisplayTransferStatus _ArgProto((int, int, int));
int DisplayConnectStatus _ArgProto((int, char *));

static void           HomeAction();
static void           BackAction();
static void           FwrdAction();
static void           ReloadAction();
static void           ColorToggle();
static void           HelpAction();
static void           QuitAction();
static void           SourceAction();
void                  OpenAction();
void                  FileAction();
void                  SearchAction();
void                  BookmarkAction();
static void	      DeferPixAction();
static void           OpenURL();

static AppResources root;

#define BUTTON_LIST "quit, open, home, back, source, reload, file, help, bookmark, search, cancel, deferpix"
int mycolors = 1;
static char defaultTranslations[] =
"\
<Key>h: home()\n\
<Key>u: back()\n\
<Key>l: reload()\n\
<Key>?: help()\n\
<Key>q: quit()\n\
<Key>d: source()\n\
<Key>o: open()\n\
<Key>f: file()\n\
<Key>s: search()\n\
<Key>m: bookmark()\n\
<Key>i: deferpix()\n\
";

#define offset(field) XtOffset(AppResources *, field)
static XtResource       resource_list[] =
{
  { "convertFiles", "Files", XtRString, sizeof(char *),
	offset(convertFiles), XtRString, (XtPointer)CONVERT_FILES },
  { "homeURL", "URL", XtRString, sizeof(char *),
	offset(homeURL), XtRString, (XtPointer)HOME_URL },
  { "helpURL", "URL", XtRString, sizeof(char *),
	offset(helpURL), XtRString, (XtPointer)HELP_URL },
  { "showURL", XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(showURL), XtRImmediate, (XtPointer)True },
  { "showTitle", XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(showTitle), XtRImmediate, (XtPointer)True },
  { "showLiveAnchors", XtCBoolean, XtRBoolean, sizeof(Boolean),
       offset(showLiveAnchors), XtRImmediate, (XtPointer)True },
  { "button1Box", "BoxList", XtRString, sizeof(char *),
	offset(button1Box), XtRString, (XtPointer)BUTTON_LIST },
  { "button2Box", "BoxList", XtRString, sizeof(char *),
	offset(button2Box), XtRString, (XtPointer)NULL },
  { "printerName", "PrinterName", XtRString, sizeof(char *),
	offset(printerName), XtRString, (XtPointer)"lp" },
  { "keyTrans", XtCString, XtRString, sizeof(char *),
        offset(keyTrans), XtRString, (XtPointer)defaultTranslations },
  { "cacheOff", XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(cacheOff), XtRImmediate, (XtPointer)False },
  { "cacheDir", "CacheDir", XtRString, sizeof(char *),
	offset(cacheDir), XtRString, (XtPointer)"/tmp" },
  { "cacheTTL", "CacheTTL", XtRInt, sizeof(int),
	offset(cacheTTL), XtRImmediate, (XtPointer)14400 },
  { "cacheSize", "CacheSize", XtRInt, sizeof(int),
	offset(cacheSize), XtRImmediate, (XtPointer)4000000 },
  { "cacheClean", "CacheClean", XtRBoolean, sizeof(Boolean),
	offset(cacheClean), XtRImmediate, (XtPointer)True },
  { "mimeTypeFiles", "Files", XtRString, sizeof(char *),
	offset(mimeTypeFiles), XtRString, (XtPointer)MIME_TYPE_FILES },
  { "mailCapFiles", "Files", XtRString, sizeof(char *),
	offset(mailCapFiles), XtRString, (XtPointer)MAIL_CAPS },
  { "bookmarkFile", XtCFile, XtRString, sizeof(char *),
	offset(bookmarkFile), XtRString, (XtPointer)"~/.chimera/bookmark.htm" },
  { "protocolFiles", "Files", XtRString, sizeof(char *),
	offset(protocolFiles), XtRString, (XtPointer)PROTOCOL_FILES },
  { "statusUpdate", "StatusUpdate", XtRInt, sizeof(int),
	offset(statusUpdate), XtRImmediate, (XtPointer)10 },
  { "path", "Path", XtRString, sizeof(char *),
	offset(path), XtRString, (XtPointer)PATH },
  { "inPort", "InPort", XtRInt, sizeof(int),
	offset(inPort), XtRImmediate, (XtPointer)0 },
  { "languageDB", "LanguageDB", XtRString, sizeof(char *),
	offset(languageDB), XtRString, (XtPointer)NULL },
  { "httpProxy", "URL", XtRString, sizeof(char *),
	offset(httpProxy), XtRString, (XtPointer)NULL },
  { "gopherProxy", "URL", XtRString, sizeof(char *),
	offset(gopherProxy), XtRString, (XtPointer)NULL },
  { "ftpProxy", "URL", XtRString, sizeof(char *),
	offset(ftpProxy), XtRString, (XtPointer)NULL },
  { "waisProxy", "URL", XtRString, sizeof(char *),
	offset(waisProxy), XtRString, (XtPointer)NULL },
  { "newsProxy", "URL", XtRString, sizeof(char *),
	offset(newsProxy), XtRString, (XtPointer)NULL },
  { "nntpProxy", "URL", XtRString, sizeof(char *),
	offset(nntpProxy), XtRString, (XtPointer)NULL },
  { "email", "Email", XtRString, sizeof(char *),
	offset(email), XtRString, (XtPointer)NULL },
  { "urnProxy", "URL", XtRString, sizeof(char *),
	offset(urnProxy), XtRString, (XtPointer)NULL },
  { "noProxy", "URLs", XtRString, sizeof(char *),
	offset(noProxy), XtRString, (XtPointer)NULL },
  { "allProxy", "Proxy", XtRString, sizeof(char *),
	offset(allProxy), XtRString, (XtPointer)NULL },
  { "maxColors", "MaxColors", XtRInt, sizeof(int),
	offset(maxColors), XtRImmediate, (XtPointer)256 },
  { "cacheInfoFiles", "Files", XtRString, sizeof(char *),
	offset(cacheInfoFiles), XtRString, (XtPointer)CACHE_INFO_FILES },
  { "cacheIgnoreExpires", XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(cacheIgnoreExpires), XtRImmediate, (XtPointer)False },
  { "gammaCorrect", "GammaCorrect", XtRFloat, sizeof(float),
	offset(gamma), XtRImmediate, (XtPointer)0 },
  { "openButtonShortcut", XtCBoolean, XtRBoolean, sizeof(Boolean),
	offset(openButtonShortcut), XtRImmediate, (XtPointer)True },
  { "localIndexFiles", "LocalIndexFiles", XtRString, sizeof(char *),
	offset(localIndexFiles), XtRString, (XtPointer)"index.html" },
};

static XtActionsRec actionsList[] =
{
  { "home",     (XtActionProc) HomeAction },
  { "back",     (XtActionProc) BackAction },
  { "forward",  (XtActionProc) FwrdAction },
  { "reload",   (XtActionProc) ReloadAction },
  { "help",     (XtActionProc) HelpAction },
  { "colr-tgl", (XtActionProc) ColorToggle },
  { "quit",     (XtActionProc) QuitAction },
  { "source",   (XtActionProc) SourceAction },
  { "open",     (XtActionProc) OpenAction },
  { "file",     (XtActionProc) FileAction },
  { "search",   (XtActionProc) SearchAction },
  { "bookmark", (XtActionProc) BookmarkAction },
  { "openurl",  (XtActionProc) OpenURL },
  { "deferpix",	(XtActionProc) DeferPixAction },
};

static char *fallback_resources[] =
{
  NULL /* I'm bitter about this because I have to include stdio.h which
          seems like worthlessness */
};

//new code
static void
CheckQuitTimer(XtPointer client_data, XtIntervalId *id)
{
  XtAppContext app = (XtAppContext)client_data;

  /* If our signal handler has set the flag, we close nicely now */
  if (g_should_quit) {
    //printf("DBG: in g_should_quit (Timer detected the signal)\n");
    if (root.toplevel != NULL) {
       XtDestroyWidget(root.toplevel);
    }
   
    Quit(root.quit, &root, NULL);
  }

  /* Check again in 100 milliseconds (10 times per second) */
  XtAppAddTimeOut(app, 100, CheckQuitTimer, client_data);
}
//new end

/*
 * main
 *
 * this is where the whole thing kicks off.
 */
#if MCB
int chimera_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  char *first;
  Document *d;
  URLParts *up, *tup;
  URLParts *default_url;
  char *msg;
  XtPointer inputmask;
  int netfd;
  /*  Colormap cmap; // removed this - djhjr - May 12 97 */
  char *base_url;
  char path[MAXPATHLEN + 1];
  char *cwd;
  Atom delete;
  
  // Sets the program to use the operating system's language/character set (e.g. da_DK.UTF-8)
  setlocale(LC_ALL, ""); 

  /*
   * ...except for characters. Chimera's text fields and fonts are Latin-1
   * (ISO 8859-1), and pages are converted to it (charset.c). Xlib turns
   * key presses into text in the encoding of LC_CTYPE, so in a UTF-8
   * locale a typed "ø" arrived as the two bytes of its UTF-8 form and
   * showed as "Ã¸". In the C locale Xlib uses ISO 8859-1.
   */
  setlocale(LC_CTYPE, "C");
    
  // Then the X system must be told to accept this locale
  if (!XSupportsLocale()) {
	fprintf(stderr, "The X server does not support the selected locale.\n");
  }

#ifdef SOCKS
  SOCKSinit(argv[0]);
#endif

  /*
   * Huh?  This is goofy.  We are trying to trap a bunch of signals
   * in case the user has set cleanUp to True so that cache files can
   * be cleaned.--- SIGQUIT should never be caught. [Jim Rees & GN]
   */
 /*signal(SIGINT, sigigh_handler);
  signal(SIGHUP, sigigh_handler);
  signal(SIGTERM, sigigh_handler);
  signal(SIGALRM, SIG_IGN);	// changed when necessary in src/net.c
  signal(SIGPIPE, SIG_IGN);*/
  //new code:
    struct sigaction sa_catch, sa_ignore;

  /* Configure setup for the signals we want to capture */
  sa_catch.sa_handler = sigigh_handler;
  sigemptyset(&sa_catch.sa_mask);
  sa_catch.sa_flags = 0; // IMPORTANT: No SA_RESTART. Makes XtAppMainLoop terminate gracefully.

  sigaction(SIGINT,  &sa_catch, NULL);
  sigaction(SIGHUP,  &sa_catch, NULL);
  sigaction(SIGTERM, &sa_catch, NULL);

  /* Configure settings for the signals we want to ignore */
  sa_ignore.sa_handler = SIG_IGN;
  sigemptyset(&sa_ignore.sa_mask);
  sa_ignore.sa_flags = 0;

  sigaction(SIGALRM, &sa_ignore, NULL); /* changed when necessary in src/net.c */
  sigaction(SIGPIPE, &sa_ignore, NULL);
//new end

  StartReaper();

  root.toplevel = XtAppInitialize(&(root.appcon), "Chimera", NULL, 0,
				  &argc, argv, fallback_resources, NULL, 0);
  if (root.toplevel == 0) exit(1);

  /*
   * Grab up the resources.
   */
  XtGetApplicationResources(root.toplevel, &root, resource_list,
			    XtNumber(resource_list), NULL, 0);

  /*
   * Setup the translations stuff for the keyboard shortcuts.
   */
  XtAppAddActions(root.appcon,
		  actionsList, XtNumber(actionsList));
  root.trans = XtParseTranslationTable(root.keyTrans);

  /*
   * this is hiding in widget.c.
   */
  CreateWidgets(&root);

  if (root.urldisplay != 0)
  {
    XtOverrideTranslations(root.urldisplay,
			   XtParseTranslationTable("<Key>Return: openurl()"));
  }

  XtRealizeWidget(root.toplevel);

  /*
   * Setup handler for incoming connections.
   */
  if (root.inPort > 0)
  {
    netfd = net_bind(root.inPort);
    if (netfd > 0)
    {
      inputmask = (XtPointer)XtInputReadMask;
      XtAppAddInput(root.appcon, netfd, inputmask, InputHandler, 0);
    }
  }

  XtAppSetErrorHandler(root.appcon, ErrorHandler);
//  XtAppSetWarningHandler(root.appcon, WarningHandler);

  delete = XInternAtom(XtDisplay(root.toplevel), "WM_DELETE_WINDOW", False);
  XtOverrideTranslations (root.toplevel, 
			  XtParseTranslationTable
			  ("<Message>WM_PROTOCOLS: quit()"));
  XSetWMProtocols (XtDisplay(root.toplevel), XtWindow(root.toplevel),
		   &delete, 1);

  /* bgcolor now set by libhtmlw's initialization - djhjr - May 12 97 */

  /*
   * Initialize AppResources
   */
  root.group = NULL;
  root.dlist = NULL;
  root.watch = XCreateFontCursor(XtDisplay(root.toplevel), XC_watch);
  root.left_ptr = XCreateFontCursor(XtDisplay(root.toplevel), XC_left_ptr);
  root.searchstr = NULL;
  root.loadstr = NULL;
  root.savestr = NULL;
  root.printstr = NULL;
  root.mailstr = NULL;
  root.rflag = False;
  root.rlist = NULL;

  root.clist = ReadConvertFiles(root.convertFiles);
  root.mclist = ReadMailCapFiles(root.mailCapFiles);
  root.mtlist = ReadMIMETypeFiles(root.mimeTypeFiles);
  root.plist = ReadProtocolFiles(root.protocolFiles);
  ReadIndexFilenames(root.localIndexFiles);

  AddLanguage(root.languageDB);

  if (root.httpProxy != NULL) AddToStringDB("http_proxy", root.httpProxy);
  if (root.gopherProxy != NULL) AddToStringDB("gopher_proxy",root.gopherProxy);
  if (root.ftpProxy != NULL) AddToStringDB("ftp_proxy", root.ftpProxy);
  if (root.waisProxy != NULL) AddToStringDB("wais_proxy", root.waisProxy);
  if (root.newsProxy != NULL) AddToStringDB("news_proxy", root.newsProxy);
  if (root.nntpProxy != NULL) AddToStringDB("nntp_proxy", root.nntpProxy);
  if (root.urnProxy != NULL) AddToStringDB("urn_proxy", root.urnProxy);
  if (root.noProxy != NULL) AddToStringDB("no_proxy", root.noProxy);
  if (root.allProxy != NULL) AddToStringDB("all_proxy", root.allProxy);
  if (root.email != NULL) AddToStringDB("email", root.email);

  if (!root.cacheOff)
  {
    InitCache(root.cacheDir, root.cacheSize, root.cacheTTL,
	      root.cacheClean ? 1:0, root.cacheIgnoreExpires ? 1:0,
	      root.cacheInfoFiles);
  }

  /*
   * Grab the first document.  First, build a fake context URL in case the
   * user provides a relative URL.  Create a new URL based on the user
   * supplied URL and then fake context and try to load it.  If that
   * fails then supply a document that is just an error message.
   */
  if (argc > 1) first = alloc_string(argv[1]);
  else if ((first = getenv("WWW_HOME")) != NULL)
  {
    first = alloc_string(first);
  }
  else first = alloc_string(root.homeURL);

  if ((cwd = getcwd(path, sizeof(path) - 1)) != NULL)
  {
    base_url = alloc_mem(16 + strlen(cwd) + 1 + 1);
    strcpy(base_url, "file://localhost");
    strcat(base_url, cwd);
    strcat(base_url, "/");
  }
  else base_url = alloc_string("file://localhost/");

  default_url = ParseURL(base_url);
  free_mem(base_url);

  if (default_url == NULL)
  {
    /* panicking */
    fprintf (stderr, "%s",GetFromStringDB("crash"));
    exit(1);
  }

  up = ParseURL(first);
  free_mem(first);

  if (up == NULL) tup = NULL;
  else
  {
    tup = MakeURLParts(up, default_url);
    DestroyURLParts(up);
    DestroyURLParts(default_url);
  }

  if (tup == NULL)
  {
    msg = GetFromStringDB("nofirst");
    d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
    d->status = DS_ERROR;
  }
  else
  {
    d = LoadDoc(tup, 0, 0);
    if (d == NULL)
    {
      msg = GetFromStringDB("nofirst");
      d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
      d->status = DS_ERROR;
    }

    DestroyURLParts(tup);
  }

  HandleDoc(d, 0);
  
  /* Set the timer to watch for signals */
  XtAppAddTimeOut(root.appcon, 100, CheckQuitTimer, (XtPointer)root.appcon);
  
  XtAppMainLoop(root.appcon);

  if (root.toplevel != NULL) {
      XtDestroyWidget(root.toplevel); 
  }
    
  return (0);
}

/*
 * InputHandler
 *
 * Handle incoming network connections
 */
static void
InputHandler(cldata, netfd, xid)
XtPointer cldata;
int *netfd;
XtInputId *xid;
{
  int fd;
  Document *d;

  fd = net_accept(*netfd);
  if (fd < 0) return;

  d = (Document *)ParseHTTPRequest(fd);

  close(fd);

  if (d != NULL)
  {
    if (d->status == DS_REDIRECT)
    {
      URLParts *up;
      char *value;

      if ((value = GetMIMEValue(d->mflist, "location", 0)) == NULL)
      {
	value = GetMIMEValue(d->mflist, "uri", 0);
      }
      if (value != NULL)
      {
	up = ParseURL(value);
      
	DestroyDocument(d);
      
	if (up != NULL && IsAbsoluteURL(up))
	{
	  d = LoadDoc(up, 0, 0);
	  DestroyURLParts(up);
	}
      }
    }

    if (d != NULL)
    {
      if (d->status != DS_OK) DestroyDocument(d);
      else HandleDoc(d, 0);
    }
  }

  return;
}

/*
 * GetTitle
 *
 * Extracts the title from the current HTML widget if it has one.
 *
 * ... and while we're at it, get rid of some annoying extra spaces.--GN
 */
static char *
GetTitle(hw)
Widget hw;
{
  char *t;
  char *s;

  XtVaGetValues(hw, WbNtitleText, &t, NULL);
  if (t == NULL) s = alloc_string("");
  else
  {
    while (*t && isspace(*t)) t++; /* skip leading whitespace */
    s = alloc_string(t);
    for (t = s; *t; t++)
    {
      if (isspace(*t)) *t = ' '; /* normalize any further whitespace */
    }
  }
  return(s);
}

/*
 * SetTitle
 *
 * Sets the string in the title widget.  Passing a NULL as a string
 * makes the title of the current document the title.  If it doesn't
 * have a title, keep the last message around [GN].
 */
/*static*/ void
SetTitle(title)
char *title;
{
  char *t = NULL;

  if (root.showTitle)
  {
    if (title == NULL) title = t = GetTitle(root.w);

    if (title[0])
      XtVaSetValues(root.titledisplay, XtNstring, title, NULL);

    if (t != NULL) free_mem(t);

    XFlush(XtDisplay(root.w));
  }

  return;
}

/*
 * SetURL
 *
 * Change the URL display
 */
static void
SetURL(up)
URLParts *up;
{
  if (root.showURL)
  {
    char *url, *t = NULL;

    if (up == NULL) url = "";
    else t = url = MakeURL(up, 1);
    /*
     * Wanna hear something weird?  I'm getting segfaults without the following
     * line.  Possibly this exercises some optimization bug in gcc. The SIGSEGV
     * arrives in the call chain underneath XtVaSetValues.  --GN 1997May21
     */
    if (0) fprintf(stderr, "%s\n", url);
    XtVaSetValues(root.urldisplay, XtNstring, url, NULL);
    if (t != NULL) free_mem(t);

    XFlush(XtDisplay(root.w));
  }

  return;
}

/*
 * LiveAnchorCallback
 *
 * Called (from TrackMotion in libhtmlw/HTML.c)
 * when the mouse moves over an anchor
 * rwmcm / GN 1997May12, May15
 */
void
LiveAnchorCallback(w, c)
Widget w;			/* not used */
char *c;
{
  char *href, *t;
  int len;

  if (c != (char *) NULL && (len = strlen(c)) > 0)
  {
    t = href = alloc_mem(len + 1);
    for(; *c; c++)
    {
      if ((*c != '\r') && (*c != '\f'))
      {
	if ((*c == '\t') || (*c == '\n'))
	  *t++ = ' ';
	else
	  *t++ = *c;
      }
    }
    *t = '\0';
    XtVaSetValues(root.anchordisplay, XtNstring, href, NULL);
    if (href != NULL) free_mem(href);
  }
  else
    XtVaSetValues(root.anchordisplay, XtNstring, "", NULL);

  XFlush(XtDisplay(root.w));
}

/*
 * DisplayCurrent
 *
 * Displays the current HTML screen.  It also changes the title display
 * and the URL display.-- However, in certain error situations we prefer
 * to leave the last status message in the title display untouched.
 */
static void
DisplayCurrent()
{
  Document *d;
  DocNode *t, *tp;
  Arg args[2];
  Widget view;
  char *displaying = NULL;	/* as in DisplayTransferStatus --GN */

  if (root.dlist == NULL)
  {
    AddMessage("nofirst");
    return;
  }

  XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel), root.watch);
  if (displaying == NULL) displaying = GetFromStringDB("display");

  /*
   * Give the new scrollbar position.
   */
  XtSetArg(args[0], WbNverticalScrollBarPos, root.dlist->vpos);
  XtSetArg(args[1], WbNhorizontalScrollBarPos, root.dlist->hpos);
  XtSetValues(root.w, args, 2);

  SetURL(root.dlist->up);

  /*
   * Try to display the current document.  If there isn't one, see if the
   * URL is still around.  If not, try to load the previous document.
   */
  d = root.dlist->doc;
  if (d == NULL)
  {
    if (root.dlist->up != NULL)
    {
      d = LoadDoc(root.dlist->up, 0, 0);
      DestroyURLParts(root.dlist->up);
      if (d->up) root.dlist->up = DupURLParts(d->up);
      root.dlist->doc = d;
    }
    else
    {
      t = root.dlist;
      root.dlist = root.dlist->next;
      while(t->prev){
        tp = t->prev;
        if (t->base != NULL) free_mem(t->base);
        free_mem((char *)t);
        t = tp;
      }
      t = root.dlist;
      DisplayCurrent();		/* recursive call */
      return;
    }
  }
  if (d == NULL || d->status != DS_ERROR)
    SetTitle(displaying);

  /*DiscardCurPageObjects (root.w, root.w);*/ /* done by HTMLSetText */

  root.cancelop = False;

  if (d->pcontent != NULL  &&  strcasecmp(d->pcontent, "text/html") == 0)
  {
    HTMLSetText(root.w, d->ptext, NULL, NULL);
  }
  else
  {
    HTMLSetText(root.w, d->text, NULL, NULL);
  }

  if (d->view_frag && d->up != NULL && d->up->anchor != NULL)
  {
    int id;

    id = HTMLAnchorToId(root.w, d->up->anchor);
    if (id > 0) HTMLGotoId(root.w, id);
    d->view_frag = 0;		/* use remembered scrollbar pos'n henceforth */
  }

  root.cancelop = False;

  /*
   * Screw around with the translations, cursor, URL display and the
   * title display.
   */
  XtVaGetValues(root.w, WbNview, &view, NULL);
  XtOverrideTranslations(view, root.trans);

  XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		root.left_ptr);
  
  SetTitle(NULL);
  SetURL(root.dlist->up);

  if (root.dlist->next == NULL)
  {
    if (root.home != 0) XtSetSensitive(root.home, False);
    if (root.back != 0) XtSetSensitive(root.back, False);
  }
  else
  {
    if (root.home != 0) XtSetSensitive(root.home, True);
    if (root.back != 0) XtSetSensitive(root.back, True);
  }
  return;
}

/*
 * LoadDoc
 *
 * Always pass up through MakeURLParts to ensure some degree of sanity
 * --GN 1997May18
 */
static Document *
LoadDoc(up, reload, localonly)
URLParts *up;
int reload;
int localonly;			/* boolean;  in cache or on localhost */
{
  Document *d;
  URLParts *nup = NULL;
  static char *download = NULL;	/* as in DisplayTransferStatus --GN */

  if (root.dlist != NULL && reload == 0)
  {
    if (root.dlist->base != NULL)
    {
      URLParts *kup;

      kup = ParseURL(root.dlist->base);
      if (kup != NULL)
      {
	nup = MakeURLParts(up, kup);
	DestroyURLParts(kup);
      }
      else nup = MakeURLParts(up, (URLParts *) NULL);
    }
    else if (root.dlist->up != NULL)
    {
      nup = MakeURLParts(up, root.dlist->up);
    }
    else nup = MakeURLParts(up, (URLParts *) NULL);
  }
  else nup = MakeURLParts(up, (URLParts *) NULL);

  d = (root.cacheOff  ||  reload) ?  NULL :  ReadCache (nup);

  /*
   * It might be nice to have a cheap, full-blown "Is this URL on the local
   * host" test, but it's too expensive to strcmp the hostname of every image
   * (when localonly == True) against the full set of gethostbyname() h_name
   * and h_aliases[].  Checking for "localhost" at least handles the most
   * common case.  Furthermore, there's not that much to be gained, since
   * clicking on delayed local-but-not-"localhost" images now works.  The
   * ability to test delayed and non-delayed image loading using just files
   * on the local host might also be considered a feature.
   */
  if (d == NULL  &&
       (!localonly  ||
	((nup->hostname != NULL)  &&  (strcmp (nup->hostname, "localhost") == 0))))
  {
    if (!localonly)
    {
      /*
       * Prepare for a potentially lengthy document load.  While we're busy
       * with this, the event loop is (not yet running or) stalled waiting
       * for us to return from a callback.  So we'll be polling the Cancel
       * button  (this is actually done from within the DisplayConnectStatus
       * and DisplayTransferStatus routines below, and these will be exercised
       * regularly at short intervals _except_ during a blocking connect.
       * Sorry, nothing we can do about that latter case). -- Modified by
       * GN 1997May11
       */

      SetURL(nup);
      if (download == NULL)  download = GetFromStringDB("download");
      SetTitle(download);

      XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		    root.watch);
      if (root.cancel != 0)   XtSetSensitive(root.cancel,   True);
      /* This doesn't redraw the button label, but we'll handle this later */
      if (root.home != 0)     XtSetSensitive(root.home,     False);
      if (root.back != 0)     XtSetSensitive(root.back,     False);
      if (root.load != 0)     XtSetSensitive(root.load,     False);
      if (root.source != 0)   XtSetSensitive(root.source,   False);
      if (root.reload != 0)   XtSetSensitive(root.reload,   False);
      if (root.file != 0)     XtSetSensitive(root.file,     False);
      if (root.help != 0)     XtSetSensitive(root.help,     False);
      if (root.bookmark != 0) XtSetSensitive(root.bookmark, False);
      if (root.search != 0)   XtSetSensitive(root.search,   False);
      if (root.quit != 0)     XtSetSensitive(root.quit,     False);
      if (root.deferpix != 0) XtSetSensitive(root.deferpix, False);
    }

    d = LoadDocument(nup, root.plist, root.mtlist, reload);

    if (!localonly)
    {
      /*
       * Thank goodness it's over, and we can return control to the event
       * loop.  Restore general conditions accordingly.  The Home and Back
       * buttons will be re-activated (or not) by DisplayCurrent.
       */
      if (root.cancel != 0)   XtSetSensitive(root.cancel,   True);
      if (root.load != 0)     XtSetSensitive(root.load,     True);
      if (root.source != 0)   XtSetSensitive(root.source,   True);
      if (root.reload != 0)   XtSetSensitive(root.reload,   True);
      if (root.file != 0)     XtSetSensitive(root.file,     True);
      if (root.help != 0)     XtSetSensitive(root.help,     True);
      if (root.bookmark != 0) XtSetSensitive(root.bookmark, True);
      if (root.search != 0)   XtSetSensitive(root.search,   True);
      if (root.quit != 0)     XtSetSensitive(root.quit,     True);
      if (root.deferpix != 0) XtSetSensitive(root.deferpix, True);
      XDefineCursor(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		    root.left_ptr);
    }
  }
  if (d == NULL  ||  d->status != DS_ERROR)  /* take care --GN */
    SetTitle("");

  DestroyURLParts (nup);

  return (d);
}

/*
 * AuthDismissCallback
 */
static void
AuthDismissCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  /*
   * added the following - djhjr - May 13 97, modified by GN 1997May14
   * When this is called, cldata points at a document with
   * d->status == DS_NEEDS>AUTH which HandleDoc knew nothing better to
   * do with.  It might contain some information useful to the user.
   * We get here when the user has given up trying to remember the
   * proper password.
   */
  Document *d = (Document *)cldata;
  d->status = DS_OK;
  HandleDoc(d, 0);		/* this will destroy d */

  XtDestroyWidget(w);
  /*  SetTitle(NULL); */		/* restore title */
  /*  SetURL(root.dlist ? root.dlist->up : NULL); */  /* restore URL */
  /* no longer needed -- HandleDoc will look after these */
  return;
}

/*
 * AuthOKCallback
 *
 * Called when the user enters username/password in the auth widget
 * Marginally streamlined -- GN 1997May11
 */
static void
AuthOKCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  AuthReqReturnStruct *ar = (AuthReqReturnStruct *)cbdata;
  Document *d = (Document *)cldata;
  RealmInfo *r;

  root.rflag = True;

  d->up->username = alloc_string(ar->username);
  d->up->password = alloc_string(ar->password);
  d->up->auth_type = alloc_string(d->auth_type);

  for (r = root.rlist; r; r = r->next)
  {
    if (strcasecmp(d->auth_type, r->up->auth_type) == 0 &&
	strcasecmp(d->auth_realm, r->name) == 0 &&
	strcasecmp(d->up->protocol, r->up->protocol) == 0 &&
	strcasecmp(d->up->hostname, r->up->hostname) == 0 &&
	d->up->port == r->up->port)
    {
      if (strcmp(d->up->username, r->up->username) != 0 ||
	  strcmp(d->up->password, r->up->password) != 0)
      {
	DestroyURLParts(r->up);
	r->up = DupURLParts(d->up);
      }
      break;
    }
  }

  if (r == NULL)
  {
    r = (RealmInfo *)alloc_mem(sizeof(RealmInfo));
    r->name = alloc_string(d->auth_realm);
    r->up = DupURLParts(d->up);
    r->next = root.rlist;
    root.rlist = r;
  }

  XtDestroyWidget(w);
  HandleDoc(d, 0);		/* will destroy d when we're done */

  return;
}

/*
 * CreateStrReq
 */
static void
CreateStrReq(s, defstr, hidden, ocallback, dcallback, cldata)
char *s;
char *defstr;
int hidden;
XtCallbackProc ocallback;
XtCallbackProc dcallback;
XtPointer cldata;
{
  Widget nw;
  Arg args[10];
  int argcnt;
  Window rw, cw;
  int rx, ry, wx, wy;
  unsigned int mask;

  XQueryPointer(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		&rw, &cw,
		&rx, &ry,
		&wx, &wy,
		&mask);

  argcnt = 0;
  XtSetArg(args[argcnt], XtNdefaultString, defstr); argcnt++;
  XtSetArg(args[argcnt], XtNhiddenText, hidden); argcnt++;
  XtSetArg(args[argcnt], XtNx, rx - 2); argcnt++;
  XtSetArg(args[argcnt], XtNy, ry - 2); argcnt++;
  nw = XtCreatePopupShell(s,
			  strReqWidgetClass, root.toplevel,
			  args, argcnt);
  XtAddCallback(nw, XtNokCallback, ocallback, cldata);
  if (dcallback != NULL)
  {
    XtAddCallback(nw, XtNdismissCallback, dcallback, cldata);
  }

  XtRealizeWidget(nw);
  XtPopup(nw, XtGrabNone);

  return;
}

/*
 * CheckRealm
 */
static RealmInfo *
CheckRealm(d)
Document *d;
{
  RealmInfo *r;

  for (r = root.rlist; r; r = r->next)
  {
    if (strcasecmp(d->auth_type, r->up->auth_type) == 0 &&
	strcasecmp(d->auth_realm, r->name) == 0 &&
	strcasecmp(d->up->protocol, r->up->protocol) == 0 &&
	strcasecmp(d->up->hostname, r->up->hostname) == 0 &&
	d->up->port == r->up->port)
    {
      if (d->up->username != NULL) free_mem(d->up->username);
      d->up->username = alloc_string(r->up->username);
      if (d->up->password != NULL) free_mem(d->up->password);
      d->up->password = alloc_string(r->up->password);
      if (d->up->auth_type != NULL) free_mem(d->up->auth_type);
      d->up->auth_type = alloc_string(r->up->auth_type);

      return(r);
    }
  }    

  return(NULL);
}

/*
 * HandleDoc
 */
static void
HandleDoc(d, download)
Document *d;
int download;
{
  Arg args[10];
  int argcnt;
  DocNode *dn = NULL;
  RealmInfo *r;
  Document *x;

  if (d->status == DS_NOTHING)
  {
    DestroyDocument(d);
    /* stay on the current page: put its URL back in the URL field */
    SetURL(root.dlist ? root.dlist->up : NULL);
    return;
  }

  /*
   * This is where chimera figures out whether or not authentication
   * information is needed.
   */
  if (d->up != NULL && d->status == DS_NEEDS_AUTH)
  {
    if ((r = CheckRealm(d)) != NULL)
    {
      x = LoadDoc(d->up, 1, 0);
      if (x->status == DS_NEEDS_AUTH)
      {
	r = NULL;
	DestroyDocument(x);
      }
      else HandleDoc(x, download);
    }

    if (r == NULL)
    {
      Widget nw;
      Window rw, cw;
      int rx, ry, wx, wy;
      unsigned int mask;
      
      XQueryPointer(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		    &rw, &cw,
		    &rx, &ry,
		    &wx, &wy,
		    &mask);
      
      argcnt = 0;
      XtSetArg(args[argcnt], XtNdefaultUsername, ""); argcnt++;
      XtSetArg(args[argcnt], XtNx, rx - 2); argcnt++;
      XtSetArg(args[argcnt], XtNy, ry - 2); argcnt++;
      nw = XtCreatePopupShell("authreq",
			      authReqWidgetClass, root.toplevel,
			      args, argcnt);
      XtAddCallback(nw, XtNokCallback, AuthOKCallback, (XtPointer)d);
      XtAddCallback(nw, XtNdismissCallback, AuthDismissCallback, (XtPointer)d);
      
      XtRealizeWidget(nw);
      XtPopup(nw, XtGrabNone);
    }
    else DestroyDocument(d);
    return;
  }

  /*
   * Convert needs to be called in case there is a content transfer or content
   * encoding.  But not for downloads, says Roman.
   */
  if (d->ptext == NULL && ! download)
  {
    /*
     * This frightens me.  I know it is bad but I can't think of anything
     * I'd rather do right now.
     */
    x = ConvertDocument(d, root.clist, NULL, root.path);
    if (x != d) HandleDoc(x, 0);
  }
  
  if (!root.cacheOff) WriteCache(d);

  /*
   * Save the old scrollbar position.
   */
  if (root.dlist)
  {
    argcnt = 0;
    XtSetArg(args[argcnt], WbNhorizontalScrollBarPos, &(root.dlist->hpos));
    argcnt++;
    XtSetArg(args[argcnt], WbNverticalScrollBarPos, &(root.dlist->vpos));
    argcnt++;
    XtGetValues(root.w, args, argcnt);
  }

  if (d->status == DS_OK &&
      (download || (d->content == NULL && d->pcontent == NULL)))
  {

	/* Let's offer to save the downloaded file under the same name.
        Roman Czyborra, 1995-04-13 */
      
      char *filename=NULL, *basename; int length;

      if (d->up) if ((filename = d->up->filename)) if ((length = strlen(filename)))
      {
         if (filename[length-1]=='/')
         {
             filename[length-1]='\0';
         }
         if ((basename = strrchr (filename, '/')))
         {
             filename = basename + 1;
         }
      }
      if (! filename) filename="";

printf("Here 1.\n");
      CreateStrReq("filename", filename, False, SaveOCallback,
		   SaveDCallback, (XtPointer)d);
  }
  else if ((d->content != NULL && strncasecmp(d->content, "text/html",9) == 0) ||
	   (d->pcontent != NULL && strncasecmp(d->pcontent, "text/html",9) == 0))
  {
    dn = (DocNode *)alloc_mem(sizeof(DocNode));
    dn->vpos = 0;
    dn->hpos = 0;
    dn->doc = d;
    dn->base = NULL;
    if (d->up != NULL) dn->up = DupURLParts(d->up);
    else dn->up = NULL;
    dn->next = root.dlist;
    dn->prev = NULL;
    root.dlist = dn;
  }
  else
  {
    char filename[L_tmpnam + 1];

    if (SaveData(d->text, d->len, tmpnam(filename)) != -1)
    {
      if (DisplayExternal(filename, d->content, root.path, root.mclist))
      {
printf("Here 2 d='%s' dp='%s'.\n",d->content,d->pcontent);
	CreateStrReq("filename", root.savestr, False, SaveOCallback,
		     SaveDCallback, (XtPointer)d);
      }
      else
      {
	DestroyDocument(d);
	SetTitle(NULL);		/* one of those cases where the Title
				   used to be lost... --GN */
	SetURL(root.dlist ? root.dlist->up : NULL);  /* restore URL, too */
	return;
      }
    }
    else
    {
      char *msg = GetFromStringDB("notsaveext");

      DestroyDocument(d);

      d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
      d->status = DS_ERROR;
      HandleDoc(d, 0);
      return;
    }
  }

  DisplayCurrent();
  return;
}

/*
 * AddMessage
 */
static void
AddMessage(text)
char *text;
{
  Document *d;
  char *value;

  value = GetFromStringDB(text);
  d = BuildDocument(value, strlen(value), "text/html", 1, 0);
  d->status = DS_ERROR;
  HandleDoc(d, 0);

  return;
}

/*
 * Anchor
 *
 * Called when the user clicks on an anchor
 */
void
Anchor(w, cldata, c)
Widget w;
XtPointer cldata;
WbAnchorCallbackData *c;
{
  URLParts *tup;
  Document *d;

  if (c == NULL || c->href == NULL) return;

  if (c->event)
  {
    tup = ParseURL(c->href);
    if (tup == NULL) return;

    switch (c->event->xbutton.button)
    {
      case 2:
        d = LoadDoc(tup, 0, 0);
	HandleDoc(d, 1);
        break;

      default:
	if (c->href[0] == '#')
	{
	  int id;
	  
	  id = HTMLAnchorToId(root.w, c->href + 1);
	  if (id > 0) HTMLGotoId(root.w, id);
	}
	else
	{
	  d = LoadDoc(tup, 0, 0);
	  HandleDoc(d, 0);
	}
    }

    DestroyURLParts(tup);
  }

  return;
}

/*
 * OpenDocument
 *
 * Called when the user clicks on the open button.
 *
 * This function grabs the text in the text widget which it assumes to
 * be a URL and tries to access the document.  It will not handle local
 * files.
 */
void
OpenDocument(url)
char *url;
{
  URLParts *up;
  Document *d;

  if (NullString(url))
  {
    AddMessage("emptyurl");
  }
  else
  {
    if (root.loadstr) free_mem(root.loadstr);
    /*
     * short cut -- if user types in "foo.com" turn that into "http://foo.com/"
     * this means you must type "./foo.com" if opening a relative URL by that
     * name.  Colons and slashes disable this, so you can open mailto:foo@bar
     * or news:alt.gourmand if you have mailto or news helper scripts.
     * You can disable this by setting X resource openButtonShortcut False.
     */
    if (root.openButtonShortcut && !strchr(url, ':') && !strchr(url, '/')) {
      root.loadstr = alloc_mem(strlen(url) + 9);
      sprintf(root.loadstr, "http://%s/", url);
    } else
      root.loadstr = alloc_string(url);

    up = ParseURL(root.loadstr);
    if (up != NULL)
    {
      d = LoadDoc(up, 0, 0);
      HandleDoc(d, 0);
      DestroyURLParts(up);
    }
    else AddMessage("invalidurl");
  }

  return;
}

/*
 * OpenURL
 *
 * Called when the user presses RETURN in the urldisplay
 */
static void
OpenURL()
{
  String s;

  XtVaGetValues(root.urldisplay, XtNstring, &s, NULL);

  OpenDocument(s);

  return;
}

/*
 * OOCallback
 */
static void
OOCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  char *t;

  if (cbdata != NULL)
  {
    t = alloc_string((char *)cbdata);

    XtPopdown(w);
    XtDestroyWidget(w);

    OpenDocument(t);

    free_mem(t);
  }

  return;
}

/*
 * OpenAction
 */
void
OpenAction()
{
  CreateStrReq("url", root.loadstr, False, OOCallback, NULL, 0);

  return;
}

/*
 * Home
 *
 * Called when the user clicks on the home button.
 *
 * The equivalent of clicking on "Back" until the first document is
 * visible.
 */
void
Home(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DocNode *c, *t;

  c = root.dlist;
  if (c)
  {
    if (c->next != NULL) DestroyDocument(c->doc);

    while (c->next)
    {
      t = c;
      c = c->next;
      if (t->base != NULL) free_mem(t->base);
      if (t->up != NULL) DestroyURLParts(t->up);
      free_mem((char *)t);
    }
    root.dlist = c;
    DisplayCurrent();
  }

  return;
}

/*
 * HomeAction
 */
static void
HomeAction()
{
  /*Boolean sensitive;

  XtVaGetValues(root.home, XtNsensitive, &sensitive, NULL);
  if (sensitive)*/ Home(root.home, &root, NULL);

  return;
}

/*
 * SaveDocument
 *
 * Saves the current savedoc.
 */
static void
SaveDocument(d, filename, otype)
Document *d;
char *filename;
int otype;
{
  char *text = NULL, *data;
  int len;

  if (root.savestr) free_mem(root.savestr);
  root.savestr = alloc_string(filename);
 
  if (otype == 3)
  {
    data = d->text;
    len = d->len;
  }
  else
  {
    data = text = HTMLGetText(root.w, otype);
    if (text == NULL) return;
    len = strlen(data);
  }  

  filename = FixFilename(filename); 
  if (filename == NULL)
  {
    AddMessage("invalidfilename");
    if (text != NULL) free_mem(text);
    return;
  }

  if (SaveData(data, len, filename) == -1)
  {
    AddMessage("notsaved");
    if (text != NULL) free_mem(text);
    return;
  }

  if (text != NULL) free_mem(text);

  return;
}

/*
 * SaveOCallback
 */
static void
SaveOCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  SaveDocument((Document *)cldata, (char *)cbdata, 3);
  XtDestroyWidget(w);

  return;
}

/*
 * SaveDCallback
 */
static void
SaveDCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DestroyDocument((Document *)cldata);
  XtDestroyWidget(w);

  return;
}

/*
 * Back
 *
 * Called when the user clicks on the back button.  It moves back one
 * HTML frame.
 */
void
Back(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DocNode *t;

  if (root.dlist->next == NULL) return;

  t = root.dlist;
  root.dlist = root.dlist->next;
  root.dlist->prev = t;
/*
  if (t->doc != NULL) DestroyDocument(t->doc);
  if (t->base != NULL) free_mem(t->base);
  if (t->up != NULL) DestroyURLParts(t->up);
  free_mem((char *)t);
*/
  DisplayCurrent();

  return;
}

/*
 * BackAction
 */
static void
BackAction()
{
//  Boolean sensitive;

//  XtVaGetValues(root.back, XtNsensitive, &sensitive, NULL);
//  if (sensitive)
    Back(root.back, &root, NULL);

  return;
}

/*
 * Back
 *
 * Called when the user clicks on the back button.  It moves back one
 * HTML frame.
 */
void
Forward(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DocNode *t;

  if (root.dlist->prev == NULL) return;

  t = root.dlist;
  root.dlist = root.dlist->prev;
  root.dlist->next = t;
/*
  if (t->doc != NULL) DestroyDocument(t->doc);
  if (t->base != NULL) free_mem(t->base);
  if (t->up != NULL) DestroyURLParts(t->up);
  free_mem((char *)t);
*/
  DisplayCurrent();

  return;
}

/*
 * FwrdAction
 */
static void
FwrdAction()
{
//  Boolean sensitive;

//  XtVaGetValues(root.back, XtNsensitive, &sensitive, NULL);
//  if (sensitive)
    Forward(root.back, &root, NULL);

  return;
}

/*
 * Reload
 */
void
Reload(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  DocNode *t;
  Document *d;
  URLParts *copy_up; // We need a copy of the URL

  root.rflag = True;

  t = root.dlist;
  if (t->up == NULL) return;

  // 1. Make a copy of the URL so we can delete the 't' immediately
  copy_up = MakeURLParts(t->up, (URLParts *) NULL);

  // 2. Move the list forward
  root.dlist = root.dlist->next;

  // 3. CLEAN UP NOW - before we load anything new
  HTMLSetText(root.w, "", NULL, NULL);
  if (t->doc != NULL) DestroyDocument(t->doc);
  if (t->up != NULL) DestroyURLParts(t->up);
  if (t->base != NULL) free_mem(t->base);
  free_mem((char *)t);

  // 4. Now load the new document with our copied URL
  d = LoadDoc(copy_up, 1, 0);

  // 5. Show it and clean up the copy
  HandleDoc(d, 0);
  DestroyURLParts(copy_up);

  root.rflag = False;
  return;
}


/*
 * ReloadAction
 */
static void
ReloadAction()
{
  Reload(root.reload, &root, NULL);
  return;
}
/*
 * ColorToggle
 */
static void
ColorToggle()
{
  mycolors ^= 1;
 }
/*
 * Cancel
 *
 * Neither does this do anything, nor does it need to.  We'll be polling
 * Cancel at times when the event loop isn't running coz we haven't yet
 * started it or we're in the middle of a lengthy callback.-- In fact, this
 * thing should never get called at all. [GN]
 */
void
Cancel(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  return;
}

/*
 * Help
 *
 * Shows the help file.
 */
void
Help(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  URLParts *up;
  Document *d;

  up = ParseURL(root.helpURL);
  if (up != NULL)
  {
    d = LoadDoc(up, 0, 0);
    HandleDoc(d, 0);
    DestroyURLParts(up);
  }
  else AddMessage("invalidurl");

  return;
}

/*
 * HelpAction
 */
static void
HelpAction()
{
  Help(root.help, &root, NULL);
  return;
}

/*
 * SearchCallback
 *
 * Called when the user enters a search string.
 */
void
SearchCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  char *ss = (char *)cbdata;
  static ElementRef start, end;
  Boolean wrap;

  if (NullString(ss))
  {
    AddMessage("emptystring");
  }
  else
  {
    if (root.searchstr != NULL)
    {
      if (strcasecmp(root.searchstr, ss) != 0)
      {
	start.id = 0;
	free_mem(root.searchstr);
	root.searchstr = alloc_string(ss);
      }
      else
      {
	start.pos = end.pos;
      }
    }
    else
    {
      root.searchstr = alloc_string(ss);
      start.id = 0;
      start.pos = 0;
    }

    do
    {
      wrap = False;
      if (HTMLSearchText(root.w, ss, &start, &end, 0, 1) == 1)
      {
	HTMLSetSelection(root.w, &start, &end);
	HTMLGotoId(root.w, start.id);
      }
      else
      {
	if (start.pos == 0)
	{
	  AddMessage("searchfailed");
	}
	else
	{
	  start.id = 0;
	  start.pos = 0;
	  wrap = True;
	}
      }
    } while (wrap);
  }

  return;
}

void
SearchAction()
{
  CreateStrReq("search", root.searchstr, False, SearchCallback, NULL, 0);

  return;
}

/*
 * PrintDocument
 *
 * Called when the user clicks on the print button.
 */
static void
PrintDocument(d, printer, otype)
Document *d;
char *printer;
int otype;
{
  char *data;
  char *text = NULL;
  int len;
  FILE *pp;
  char *prncmd;

  if (NullString(printer))
  {
    AddMessage("emptyprinter");
    return;
  }

  if (root.printstr) free_mem(root.printstr);
  root.printstr = alloc_string(printer);
  
  if (otype == 3)
  {
    data = d->text;
    len = d->len;
  }
  else
  {
    data = text = HTMLGetText(root.w, otype);
    if (text == NULL) return;
    len = strlen(text);
  }

  prncmd = alloc_mem(strlen(PRINT_COMMAND) + strlen(printer) + 1);
  sprintf (prncmd, PRINT_COMMAND, printer);

  pp = popen(prncmd, "w");
  if (pp == NULL)
  {
    AddMessage("notppipe");
    if (text != NULL) free_mem(text);
    free_mem(prncmd);
    return;
  }
  free_mem(prncmd);

  if (fwrite(data, 1, len, pp) < len)
  {
    AddMessage("notpdata");
    pclose(pp);
    if (text != NULL) free_mem(text);
    return;
  }

  pclose(pp);
  if (text != NULL) free_mem(text);

  return;
}

/*
 * MailDocument
 */
static void
MailDocument(d, email, otype)
Document *d;
char *email;
int otype;
{
  char *text = NULL;
  char *data;
  int len;
  FILE *pp;
  char *emailcmd;

  if (NullString(email))
  {
    AddMessage("emptyemail");
    return;
  }

  if (root.mailstr != NULL) free_mem(root.mailstr);
  root.mailstr = alloc_string(email);
  
  if (otype == 3)
  {
    data = d->text;
    len = d->len;
  }
  else
  {
    data = text = HTMLGetText(root.w, otype);
    if (text == NULL) return;
    len = strlen(text);
  }

  emailcmd = alloc_mem(strlen(EMAIL_COMMAND) + strlen(email) + 1);
  sprintf (emailcmd, EMAIL_COMMAND, email);

  pp = popen(emailcmd, "w");
  if (pp == NULL)
  {
    AddMessage("notepipe");
    if (text != NULL) free_mem(text);
    free_mem(emailcmd);
    return;
  }
  free_mem(emailcmd);

  if (fwrite(data, 1, len, pp) < len)
  {
    AddMessage("notedata");
    if (text != NULL) free_mem(text);
    pclose(pp);
    return;
  }

  pclose(pp);
  if (text != NULL) free_mem(text);

  return;
}

/*
 * FileCallback
 */
static void
FileCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  OutputSelCallbackInfo *osci = (OutputSelCallbackInfo *)cbdata;
  Document *d = (Document *)cldata;

  if (osci->device == 0)
  {
    if (NullString(osci->str))
    {
      AddMessage("emptyprinter");
      return;
    }
    PrintDocument(d, osci->str, osci->type);
  }
  else if (osci->device == 1)
  {
    if (NullString(osci->str))
    {
      AddMessage("emptyfilename");
      return;
    }
    SaveDocument(d, osci->str, osci->type);
  }
  else
  {
    if (NullString(osci->str))
    {
      AddMessage("emptyemail");
      return;
    }
    MailDocument(d, osci->str, osci->type);
  }

  XtDestroyWidget(w);

  return;
}

/*
 * FileAction
 *
 * Popup the output selector
 */
void
FileAction()
{
  Widget nw;
  Arg args[10];
  int argcnt;
  Window rw, cw;
  int rx, ry, wx, wy;
  unsigned int mask;

  XQueryPointer(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		&rw, &cw,
		&rx, &ry,
		&wx, &wy,
		&mask);

  argcnt = 0;
  XtSetArg(args[argcnt], XtNdefaultPrinter, ""); argcnt++;
  XtSetArg(args[argcnt], XtNdefaultEmail, ""); argcnt++;
  XtSetArg(args[argcnt], XtNdefaultFilename, ""); argcnt++;
  XtSetArg(args[argcnt], XtNx, rx); argcnt++;
  XtSetArg(args[argcnt], XtNy, ry); argcnt++;
  nw = XtCreatePopupShell("outputsel",
			  outputSelWidgetClass, root.toplevel,
			  args, argcnt);
  XtAddCallback(nw, XtNokCallback, FileCallback, root.dlist->doc);

  XtRealizeWidget(nw);
  XtPopup(nw, XtGrabNone);

  return;
}

/*
 * sigigh_handler
 */
/*static void
sigigh_handler()
{
  Quit(root.quit, &root, NULL);

  return;
}*/
//new code:
static void
sigigh_handler(int sig)
{
  (void)sig; // Prevents compiler warning
  g_should_quit = 1;
}
//end new

/*
 * Quit
 *
 * Called when the user clicks on the quit button, or when handling a signal
 * asking us to terminate cleanly.  Prevent re-entrant use of the signal
 * handler, but allow SIGTERM to cause termination in case something is going
 * seriously wrong during the cleanup phase.
 */
/*void
Quit(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  signal(SIGINT, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGALRM, SIG_IGN);
  signal(SIGTERM, SIG_DFL);

  if (root.dlist && root.dlist->doc) DestroyDocument(root.dlist->doc);

  if (!root.cacheOff) CleanCache();

  exit(0);
}*/
//new code
void Quit(Widget w, XtPointer cldata, XtPointer cbdata)
{
  struct sigaction sa_ignore, sa_default;

  // Ignore signals during cleanup
  sa_ignore.sa_handler = SIG_IGN;
  sigemptyset(&sa_ignore.sa_mask);
  sa_ignore.sa_flags = 0;
  sigaction(SIGINT,  &sa_ignore, NULL);
  sigaction(SIGHUP,  &sa_ignore, NULL);
  sigaction(SIGALRM, &sa_ignore, NULL);

  // Set SIGTERM to default behavior
  sa_default.sa_handler = SIG_DFL;
  sigemptyset(&sa_default.sa_mask);
  sa_default.sa_flags = 0;
  sigaction(SIGTERM, &sa_default, NULL);

  if (root.dlist && root.dlist->doc) 
  {
    DestroyDocument(root.dlist->doc);
  }

  if (!root.cacheOff) 
  {
    CleanCache();
  }

  exit(0);
}
//new end

/*
 * Quit action.
 */
static void
QuitAction()
{
  Quit(root.quit, &root, NULL);
  return;
}

/*
 * Source
 *
 * Called when the user clicks on the source button.  Allocates yet more
 * memory to put the current HTML in a document with <plaintext> at the
 * top.
 */
void
Source(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  Document *d;

  if (root.dlist != NULL && root.dlist->doc != NULL &&
      root.dlist->doc->text != NULL)
  {
    char *data;

    if ((data = BuildDocumentInfo(root.dlist->doc)) != NULL)
    {
      d = BuildDocument(data, strlen(data), "text/plain", 0, 0);
      if (root.dlist->doc->up != (URLParts *)NULL)
      {
	d->up = DupURLParts(root.dlist->doc->up);
      }

      HandleDoc(d, 0);
    }
  }

  return;
}

/*
 * SourceAction
 */
static void
SourceAction()
{
  Source(root.source, &root, NULL);
  return;
}

/*
 * Link
 *
 * The link callback
 */
void
LinkCB(w, cldata, cbdata)
Widget w;
XtPointer cldata;
XtPointer cbdata;
{
  LinkInfo *li = (LinkInfo *)cbdata;

  if (li != NULL && li->href != NULL)
      root.dlist->base = alloc_string(li->href);
  else root.dlist->base = NULL;

  return;
}

/*
 * ImageResolve
 *
 * Called by the HTML widget to turn an image into something the widget
 * understands.  This treats images like documents.
 * Background arg added - djhjr - May 12 97
 */

ImageInfo *
ImageResolve(w, url, bg, delay)
Widget w;
char *url;
Pixel bg;
int delay;	/* boolean; if true, only if in cache or on localhost */
{
  Document *d, *x;
  URLParts *tup;
  ImageInfo *i = NULL;
  Display *dpy;
  int screen;
  Visual *v;
  int depth;
  int stype;
  RealmInfo *r;

  if (root.cancelop || url == NULL) /* honor click-ahead cancel */
  {				/* (doesn't actually do that though ;) [GN] */
    root.cancelop = False;
    return(NULL);
  }

  tup = ParseURL(url);
  if (tup == NULL) return(NULL);

  //d = LoadDoc (tup, 0, delay);		//FIX for valgrind
				/* following needed since LoadDoc may have
				 * set these insensitive -- GN1997May19 */
  d = LoadDoc (tup, 0, delay);
    
 //debug offset width/height with one image
 #ifdef IMAGENEW
HTMLWidget hw = (HTMLWidget) w;
  struct mark_up *mptr;
  char *tptr;
  char *srcptr;
  int found_width = -1;
  int found_height = -1;

  /* 
   * 1. HUNT: We loop through all HTML objects to find the tag 
   * that matches the URL we are currently retrieving.
   */
  mptr = hw->html.html_objects;
  while (mptr != NULL)
  {
    if (mptr->type == M_IMAGE)
    {
      srcptr = ParseMarkTag (mptr->start, MT_IMAGE, "SRC");
      if (srcptr != NULL) 
      {
        /* Does this HTML tag match the URL we are trying to resolve? */
        if (strcmp(srcptr, url) == 0) 
        {
          /* Bingo! We've found the correct tag. Get the dimensions directly from here */
          tptr = ParseMarkTag (mptr->start, MT_IMAGE, "WIDTH");
          if (tptr != NULL) {
              found_width = atoi(tptr);
              XtFree(tptr);
          }
          
          tptr = ParseMarkTag (mptr->start, MT_IMAGE, "HEIGHT");
          if (tptr != NULL) {
              found_height = atoi(tptr);
              XtFree(tptr);
          }
          
          XtFree(srcptr);
          break; /* Stop searching, we have found the target */
        }
        XtFree(srcptr);
      }
    }
    mptr = mptr->next;
  }

  /* 2. Pass the WIDTH and HEIGHT found in the IMG tag on to the document */
  if (d != NULL) {
      d->req_width = found_width;
      d->req_height = found_height;
      
    //  printf("SUCCESS: Synced dimensions for %s -> W: %d, H: %d\n", url, found_width, found_height);
  }

#endif
 
  if (root.dlist->next == NULL)
  {
    if (root.home != 0) XtSetSensitive(root.home, False);
    if (root.back != 0) XtSetSensitive(root.back, False);
  }
  else
  {
    if (root.home != 0) XtSetSensitive(root.home, True);
    if (root.back != 0) XtSetSensitive(root.back, True);
  }

  DestroyURLParts(tup);

  if (d == NULL) return(NULL);

  if (d->status == DS_NEEDS_AUTH)
  {
    if ((r = CheckRealm(d)) != NULL) /* anything useful in the Realm cache? */
    {
      x = LoadDoc(d->up, 1, 0);

      if (root.dlist->next == NULL) /* again */
      {
	if (root.home != 0) XtSetSensitive(root.home, False);
	if (root.back != 0) XtSetSensitive(root.back, False);
      }
      else
      {
	if (root.home != 0) XtSetSensitive(root.home, True);
	if (root.back != 0) XtSetSensitive(root.back, True);
      }

      if (x->status == DS_NEEDS_AUTH)
      {				/* too bad, didn't work. */
	DestroyDocument(x);
	DestroyDocument(d);
	return (NULL);		/* this isn't the time to pop up an auth box */
      }
      else
      {
	DestroyDocument(d);
	d = x;
      }
    }
    else			/* nothing found, give up on this one */
    {
      DestroyDocument(d);
      return (NULL);
    }
  }

  if (!root.cacheOff) WriteCache(d);

  /*
   * Figure out if the screen is color, grayscale, or monochrome.
   */
  XtVaGetValues(root.toplevel, XtNdepth, &depth, NULL);
    
  if (depth > 1)
  {
    dpy = XtDisplay(root.toplevel);
    screen = DefaultScreen(dpy);
    v = DefaultVisual(dpy, screen);
    switch(v->class)
    {
      case StaticColor:
      case DirectColor:
      case TrueColor:
      case PseudoColor:
        stype = COLOR_DISPLAY;
	break;
      case StaticGray:
      case GrayScale:
	stype = GRAY_DISPLAY;
	break;
      default:
	stype = MONO_DISPLAY;
	break;
    }
  }
  else
  {
    stype = MONO_DISPLAY;
  }

  /* added this - djhjr - May 12 97 */
  {
    Arg arg[1];
    XColor xcolor;
    Colormap cmap;

    xcolor.pixel = bg;
    XtSetArg(arg[0], XtNcolormap, &cmap);
    XtGetValues(root.w, arg, 1);

#ifdef DEBUG_BGCOLOR
    fprintf (stderr, "Image resolver calling XQueryColor\n");
#endif
    XQueryColor(XtDisplay(root.w), cmap, &xcolor);
#ifdef DEBUG_BGCOLOR
    fprintf (stderr, "Returned ok\n");
#endif
    /* changed this - djhjr - May 12 97 */
    i = CreateImageInfo(root.toplevel, d, root.clist, root.path,
			stype, root.maxColors, &xcolor,
			(double)root.gamma);
  }

  DestroyDocument(d);

  return(i);
}


/*
 * DeferPix
 */
void
DeferPix(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  extern void SetDelayedImageLoading();	 /* in libhtmlw/HTML.c */

  SetDelayedImageLoading (root.w, cbdata ? True : False);

  return;
}

/*
 * DeferPixAction
 */
static void
DeferPixAction()
{
  Boolean state;
  Widget w = root.deferpix;

  if (w != NULL)
  {
    XtVaGetValues (w, XtNstate, &state, NULL);
    state =  (state == False) ? True : False;
    XtVaSetValues (w, XtNstate, state, NULL);

    DeferPix(w, &root, state);
  }
  else				/* user chose not to have this button */
  {
    extern void ToggleDelayedImageLoading();  /* in libhtmlw/HTML.c */

    /*
     * Without the DeferPix button, there's no way to indicate explicitly
     * which mode is current, but the user can tell by what happens.
     * Toggle the delay_images state.
     */
    ToggleDelayedImageLoading (root.w);
  }

  return;
}

/*
 * SetDeferPixButtonState
 *
 * Allows initialization of the button state from the
 * X resource *delayImageLoads.
 */
void
SetDeferPixButtonState (newstate)
int newstate;
{
  Widget w = root.deferpix;

  if (w != NULL)
  {
    newstate = newstate ?  True : False;  /* canonicalize state */
    XtVaSetValues (w, XtNstate, newstate, NULL);
  }
}


/*
 * BookmarkCallback
 */
static void
BookmarkCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  OpenDocument((char *)cbdata);

  return;
}

/*
 * MyBookmarkProc
 */
static void
MyBookmarkProc(cldata, display, url)
XtPointer cldata;
char **display;
char **url;
{
  *display = NULL;
  *url = NULL;

  if (root.dlist != NULL && root.dlist->up != NULL)
  {
    char *t;

    if ((t = MakeURL(root.dlist->up, 1)) != NULL)
    {
      *display = alloc_string (GetTitle(root.w));
      *url = alloc_string (t); 
      free_mem(t);
    }
  }

  return;
}

/*
 * BookmarkDestroyCallback
 */
static void
BookmarkDestroyCallback(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  String g;

  XtVaGetValues(w, XtNcurrentGroup, &g, NULL);
  if (g != NULL)
  {
    if (root.group != NULL) free_mem (root.group);
    root.group = alloc_string(g);
  }

  return;
}

/*
 * BookmarkAction
 */
void
BookmarkAction()
{
  Widget nw;
  Arg args[10];
  int argcnt;
  Window rw, cw;
  int rx, ry, wx, wy;
  unsigned int mask;
  char *filename;

  filename = alloc_string(FixFilename(root.bookmarkFile));

  XQueryPointer(XtDisplay(root.toplevel), XtWindow(root.toplevel),
		&rw, &cw,
		&rx, &ry,
		&wx, &wy,
		&mask);

  argcnt = 0;
  XtSetArg(args[argcnt], XtNbookmarkProc, MyBookmarkProc); argcnt++;
  XtSetArg(args[argcnt], XtNbookmarkProcData, 0); argcnt++;
  XtSetArg(args[argcnt], XtNfilename, filename); argcnt++;
  XtSetArg(args[argcnt], XtNx, rx); argcnt++;
  XtSetArg(args[argcnt], XtNy, ry); argcnt++;
  if (root.group != NULL)
  {
    XtSetArg(args[argcnt], XtNcurrentGroup, root.group); argcnt++;
  }
  nw = XtCreatePopupShell("bookmark",
			  bookmarkWidgetClass, root.toplevel,
			  args, argcnt);
  XtAddCallback(nw, XtNcallback, BookmarkCallback, 0);
  XtAddCallback(nw, XtNdestroyCallback, BookmarkDestroyCallback,
		(XtPointer)&root);

  XtRealizeWidget(nw);
  XtPopup(nw, XtGrabNone);

  return;
}

/*
 * VisitTest
 *
 * Called by the HTML widget functions to determine if a link has been
 * visited.
 */
int
VisitTest(w, url)
Widget w;
char *url;
{
  URLParts *up;
  URLParts *r;

  if (root.dlist == NULL) return(0);

  up = ParseURL(url);
  if (up == NULL) return(0);

  if (root.dlist->base != NULL)
  {
    URLParts *bup;

    bup = ParseURL(root.dlist->base);
    if (bup == NULL)
    {
      DestroyURLParts(up);
      return(0);
    }
    r = MakeURLParts(up, bup);
    DestroyURLParts(bup);
  }
  else
  {
    r = MakeURLParts(up, root.dlist->up);
  }

  DestroyURLParts(up);

  if (r == NULL) return(0);

  if (IsCached(r))
  {
    DestroyURLParts(r);
    return(1);
  }

  DestroyURLParts(r);

  return(0);
}

/*
 * DisplayConnectStatus
 *
 * This is called by the networking and data transfer functions to display
 * the status of a DNS lookup, a socket being connected, or an outbound
 * request, and to poll the Cancel button.
 *
 * Also poll the XEvent queue for Expose events.  Not for anything else --
 * we sure don't want no re-entrant calls to LoadDoc... --GN 1997May11
 *
 * First arg tells us which message to show, second is a hostname and/or IP
 * address  (as a dotted quad string)  or NULL.
 * --GN 1997Apr30
 */
int
DisplayConnectStatus(msg, host)
int msg;
char *host;
{
  static char *DNSquery = NULL;	/* msgs 0...3 use the host arg */
  static char *TCPconnecting = NULL;
  static char *TCPconnected = NULL;
  static char *TCPsendreq = NULL;
  static char *TCPsentawait = NULL; /* msg 4 ignores it */
  static char *download = NULL;	/* should never happen */
  static char ourhost[204];

  if (host != NULL)
  {
    if (strlen(host) < 204)
      strcpy(ourhost, host);
    else
    {
      strncpy(ourhost, host, 200);
      strcpy(ourhost+200, "...");
    }
  } /* else re-use previous hostname, which we got from net_open */

  if (root.titledisplay != 0)
  {
    char buffer[256];

    switch (msg)
    {
    case 0:
      if (DNSquery == NULL) DNSquery = GetFromStringDB("DNSquery");
      sprintf(buffer, DNSquery, ourhost);
      break;
    case 1:
      if (TCPconnecting == NULL)
	TCPconnecting = GetFromStringDB("TCPconnecting");
      sprintf(buffer, TCPconnecting, ourhost);
      break;
    case 2:
      if (TCPconnected == NULL)
	TCPconnected = GetFromStringDB("TCPconnected");
      sprintf(buffer, TCPconnected, ourhost);
      break;
    case 3:
      if (TCPsendreq == NULL) TCPsendreq = GetFromStringDB("TCPsendreq");
      sprintf(buffer, TCPsendreq, ourhost);
      break;
    case 4:
      if (TCPsentawait == NULL) TCPsentawait = GetFromStringDB("TCPsentawait");
      strcpy(buffer, TCPsentawait);
      break;
    default:
      if (download == NULL) download = GetFromStringDB("download");
      strcpy(buffer, download);
      break;
    }

    SetTitle(buffer);
  }

  if (root.cancel)
  {
    XEvent xe;

    if (XCheckWindowEvent(XtDisplay(root.cancel), XtWindow(root.cancel),
			  ButtonReleaseMask, &xe))
    {
      root.cancelop = True;
      return(1);
    }
    else
    {				/* redraw exposures --GN 1997May11 */
      while (XCheckMaskEvent(XtDisplay(root.toplevel), ExposureMask, &xe))
      {
	XtDispatchEvent(&xe);
      }
    }
  }

  return(0);
}

/*
 * DisplayTransferStatus
 *
 * This is called by the data transfer functions to display the status of
 * an inbound transfer, and to poll the Cancel button.
 *
 * Also poll the XEvent queue for Expose events.  Not for anything else --
 * we sure don't want no re-entrant calls to LoadDoc... --GN 1997May11
 *
 * When max < 0, we're reading HTTP response headers or Gopher menus or
 * FTP control channel chats, so we say "Reading response: mumble bytes".
 * When max == 0, we're reading data but don't know how much to expect,
 * so we say "Downloading document..." and then "mumble bytes loaded".
 * When max > 0, we expect max bytes, and say "mumble bytes of out max loaded".
 * --GN 1997Apr30
 */
int
DisplayTransferStatus(n, max, showit)
int n, max, showit;
{
  static int count = 0;
  static char *byte2 = NULL;
  static char *byte1 = NULL;
  static char *byte0 = NULL;
  static char *byte21 = NULL;
  static char *byte11 = NULL;
  static char *byte01 = NULL;

  if (showit) count = 0;	/* force an up-to-date display */

  if (root.titledisplay != 0 &&
      count++ % (root.statusUpdate * (n > 0 && max < 0 ? 5 : 1)) == 0)
  {
    char buffer[256];		/* Sheesh.  Probably safe though unless s.o.
				   introduces 240-digit integers. --GN */
    if (max < 0)
    {
      if (n == 0 && max == -8)        /* still waiting for start of response */
      {
      return(DisplayConnectStatus(4, NULL));
      }
      if (byte0 == NULL) byte0 = GetFromStringDB("byte0");
      if (byte01 == NULL) byte01 = GetFromStringDB("byte01");
      if (n == 1) strcpy (buffer, byte01);
      else sprintf (buffer, byte0, n);
    }
    else if (max > 0)
    {
      if (byte2 == NULL) byte2 = GetFromStringDB("byte2");
      if (byte21 == NULL) byte21 = GetFromStringDB("byte21");
      if (n == 1) sprintf (buffer, byte21, max);
      else sprintf (buffer, byte2, n, max);
    }
    else
    {
      if (byte1 == NULL) byte1 = GetFromStringDB("byte1");
      if (byte11 == NULL) byte1 = GetFromStringDB("byte11");
      if (n == 1) strcpy (buffer, byte11);
      else sprintf (buffer, byte1, n);
    }

    SetTitle(buffer);
  }

  if (root.cancel)
  {
    XEvent xe;

    if (XCheckWindowEvent(XtDisplay(root.cancel), XtWindow(root.cancel),
			  ButtonReleaseMask, &xe))
    {
      root.cancelop = True;
      return(1);
    }
    else
    {				/* redraw exposures --GN 1997May11 */
      while (XCheckMaskEvent(XtDisplay(root.toplevel), ExposureMask, &xe))
      {
	XtDispatchEvent(&xe);
      }
    }
  }

  return(0);
}

/*
 * SubmitForm
 *
 * Form callback for the HTML widget.
 */
void
SubmitForm(w, cldata, cbdata)
Widget w;
XtPointer cldata, cbdata;
{
  char *url;
  Document *d;
  URLParts *up;
  int i, utf8;
  WbFormCallbackData *formdata;

  formdata = (WbFormCallbackData *)cbdata;
  if (formdata == NULL) return;

  if (NullString(formdata->href)) url = MakeURL(root.dlist->up, 1);
  else url = alloc_string(formdata->href);

  up = ParseURL(url);
  free_mem(url);
  if (up == NULL)
  {
    AddMessage("invalidaction");
    return;
  }

  up->method = formdata->method != NULL ?
      alloc_string(formdata->method):alloc_string("GET");

  /*
   * Pages that were UTF-8 are shown converted to Latin-1 (charset.c);
   * their servers expect form data in UTF-8, so convert it back.
   */
  utf8 = (root.dlist != NULL  &&  DocumentWasUTF8(root.dlist->doc));

  up->attribute_count = formdata->attribute_count;
  if (up->attribute_count > 0)
  {
    up->attribute_names = (char **)alloc_mem(sizeof(char **) *
					     formdata->attribute_count);
    up->attribute_values = (char **)alloc_mem(sizeof(char **) *
					      formdata->attribute_count);

    for (i = 0; i < up->attribute_count; i++)
    {
      char *n = formdata->attribute_names[i];
      char *v = formdata->attribute_values[i];

      up->attribute_names[i] = n == NULL ? NULL :
	  utf8 ? Latin1ToUTF8(n) : alloc_string(n);
      up->attribute_values[i] = v == NULL ? NULL :
	  utf8 ? Latin1ToUTF8(v) : alloc_string(v);
    }
  }
// HTMLSetText is called inside HandleDoc, which cleans up form data automatically
  d = LoadDoc(up, 0, 0);
  HandleDoc(d, 0);
  DestroyURLParts(up);

// NO MANUAL RELEASE OF FORM DATA HERE!

  return;
}


/*
 * ErrorHandler
 */
static void
ErrorHandler(msg)
String msg;
{
  /* panicking */
  fprintf (stderr, "Xt Error: %s\n", msg);
  abort();
  Quit(root.quit, &root, NULL);
  return;
}

/*
 * WarningHandler
 */
/*static void
WarningHandler(msg)
String msg;
{
  char *xmsg = GetFromStringDB("xwarning");
  Document *d;
  char *r;

  r = alloc_mem(strlen(msg) + strlen(xmsg) + 1);
  sprintf (r, xmsg, msg);
  d = BuildDocument(r, strlen(r), "text/html", 0, 0);
  d->status = DS_ERROR;
  HandleDoc(d, 0);

  return;
}*/
