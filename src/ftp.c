/*
 * ftp.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 * See RFC959 for what we _really_ should be doing (and expecting).
 * (The real world is even weirder, though.) --GN
 */
#include <X11/Intrinsic.h>		//for XTFree

#include "copyright.h"
#include "options.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include <string.h>

#include <stdlib.h>

#include "common.h"
#include "url.h"
#include "util.h"
#include "mime.h"
#include "document.h"
#include "ftp.h"
#include "net.h"
#include "input.h"
#include "stringdb.h"

#define DEFAULT_FTP_PORT 21

static char *ftp_dir _ArgProto((int, char *, int, char *));
static int soak _ArgProto((int, char *, char **));

/*
 * soak
 * 
 * This function is used to "soak up" the multiline BS that some
 * ftp servers spew (not that it is bad to have nice long
 * informational messages its just that I hate them from my
 * programmer's point of view).
 * Slightly modified and debugging hooks added --GN 1997May13
 */
/* #define DEBUG_FTP */
static int
soak(int s, char *msg, char **emsg)
{
  char buffer[BUFSIZ];
  int code;
  int rval;
  char *t;
  int tlen, blen, btlen;

  if (msg != NULL)
  {
    if (WriteBuffer(s, msg, strlen(msg)) == -1) return(999);
#ifdef DEBUG_FTP
    fprintf(stderr, "Send %s", msg);
#endif
  }

  t = NULL;
  tlen = 0;
  btlen = 0;
  code = -1;
  buffer[0] = '\0';
  while ((rval = ReadLine(s, buffer, sizeof(buffer))) == 0)
  {
    blen = strlen(buffer);
    tlen += blen;
    if (t != NULL) t = (char *)realloc_mem(t, tlen + 1);
    else t = (char *)alloc_mem(tlen + 1);
    memcpy(t + btlen, buffer, blen);
    t[tlen] = '\0';
    btlen = tlen;
#ifdef DEBUG_FTP
    fprintf(stderr, "Read %s", t);
#endif
    if (code == -1) code = atoi(buffer);
    if (*buffer && buffer[0] > ' ' && isspace(buffer[3]))
      break;
    buffer[0] = '\0';
  }

  if (emsg != NULL)
  {
    if (t == NULL) *emsg = alloc_string("");
    else *emsg = t;
  }

  if (rval < 0 && code == -1) return(999);

  return(code);
}

/*
 * fixup_ftp_message
 *
 */
static Document *
fixup_ftp_message(char *ftpmsg)
{
  char *msg = GetFromStringDB("ftperror");
  char *r;
  Document *d;

  char *e = EscapeHTML(ftpmsg);	/* the server's text, into a page */

  r = alloc_mem(strlen(msg) + strlen(e) + 1);
  sprintf (r, msg, e);
  free_mem(e);
  free_mem(ftpmsg);
  d = BuildDocument(r, strlen(r), "text/html", 0, 0);
  d->status = DS_ERROR;
  return(d);
}

/*
 * ftp
 *
 * Anonymous FTP interface
 *
 * This is getting to be quite large and that is without all of the
 * necessary error checking being done.
 */
Document *
ftp(URLParts *up, MIMEType *mtlist)
{
  char *emsg;
  char *query;
  char *t;
  char *domain;
  char *uname;
  char *filename;
  int s, d;
  int h0, h1, h2, h3, p0, p1, reply, n;
  int data_port;
  int tlen = 0;
  int flen;
  int size = 0;
  int status;
  char *hostname;
  char *content;
  static char *format = "PASS -%s@%s\r\n"; 
  static char *format2 = "RETR %s\r\n";
  static char *format3 = "CWD %s\r\n";
  static char *format4 = "SIZE %s\r\n";

  filename = up->filename;
  flen = strlen(filename);
  if (flen == 0) return(NULL);	/* we want at least the leading slash */

  /*
   * Contact the ftp server on the usualy port.  Hardcoding ports is bad.
   */
  hostname = up->hostname != NULL ? up->hostname:"ftp";
  s = net_open(hostname, (up->port == 0 ? DEFAULT_FTP_PORT:up->port));
  if (s < 0) return(NULL);

  /*
   * wait for the connect() to succeed
   */
#ifdef NONBLOCKING_CONNECT
  if (WaitForConnect(s) < 0) return(NULL);
#endif

  /*
   * Take care of the greeting.
   */
  if (soak(s, NULL, &emsg) >= 400) 
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }
  free_mem(emsg);

  /*
   * Send the user name
   */
  if ((status = soak(s, "USER anonymous\r\n", &emsg)) >= 400)
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }

  if (status == 331) {		/* remote: user ok, asking for password */
    free_mem(emsg);
    domain = net_gethostname();
    if (domain == NULL)
    {
      net_close(s);
      return(NULL);
    }

    /*
     * Send the password
     */
    if ((uname = NGetFromStringDB("email")) != NULL
	|| (uname = getenv("EMAIL")) != NULL)
    {
      query = alloc_mem(strlen(uname) + 9);
      strcpy(query, "PASS -");
      strcat(query, uname);
      strcat(query, "\r\n");
    }
    else
    {
      if ((uname = getenv("USER")) == NULL) uname = "nobody";
      query = alloc_mem(strlen(uname) + 
			strlen(domain) + strlen(format) + 1);
      sprintf(query, format, uname, domain);
    }

    if ((status = soak(s, query, &emsg)) >= 400)
    {
      net_close(s);
      return(fixup_ftp_message(emsg));
    }
    free_mem(query);
  }
  if (status != 230)		/* are we logged on properly? */
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }
  free_mem(emsg);

  /*
   * Set binary transfers
   */
  if (soak(s, "TYPE I\r\n", &emsg) >= 400)
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }
  else 
  {
    /* The command succeeded, so the error message is not needed */
    if (emsg != NULL) 
    {
      free_mem(emsg);
      emsg = NULL;
    }
  }

  /*
   * Set passive mode and grab the port information
   */
  if (soak(s, "PASV\r\n", &emsg) >= 400)
  {
    net_close(s);
    return(fixup_ftp_message(emsg));
  }

  n = sscanf(emsg, "%d %*[^(] (%d,%d,%d,%d,%d,%d)", &reply,
	     &h0, &h1, &h2, &h3, &p0, &p1);
  if (n >= 1 && n < 7 && reply == 227) /* try shorthand format instead */
  {
    n = sscanf(emsg, "%d %*[^0-9]%d,%d,%d,%d,%d,%d", &reply,
	       &h0, &h1, &h2, &h3, &p0, &p1);
  } /* noticed by Hallvard B Furuseth, fix by GN 1997May13 */
  if (n != 7 || reply != 227)
  {
    /* a copy: fixup_ftp_message() frees its argument */
    char *msg = alloc_string(GetFromStringDB("ftpweirdness"));

    net_close(s);
    free_mem(emsg);
    return(fixup_ftp_message(msg));
  }
  free_mem(emsg);

  /*
   * Open a data connection: to the host we are talking to, not to the
   * address in the PASV reply. A server could name any machine there,
   * on the user's own network too, and have Chimera connect to it; other
   * clients (curl) ignore that address as well. It also helps with
   * servers behind NAT that give their private address.
   */
  data_port = (p0 << 8) + p1;
  if (p0 < 0 || p0 > 255 || p1 < 0 || p1 > 255 || data_port == 0)
  {
    net_close(s);
    return(fixup_ftp_message(alloc_string(GetFromStringDB("ftpweirdness"))));
  }
  d = net_open(hostname, data_port);
  if (d < 0)
  {
    net_close(s);
    return(NULL);
  }

  /*
   * Try to retrieve the file
   * (But not if we know in advance it's a directory.  GN 1997May09)
   */
  if (filename[flen - 1] != '/')
  {
    /*
     * ask for SIZE first
     */
    query = alloc_mem(flen + strlen(format4) + 1);
    sprintf (query, format4, filename);
    if ((status = soak(s, query, &emsg)) == 999)
    {
      net_close(s);
      net_close(d);
      free_mem(query);
      return(fixup_ftp_message(emsg));
    }
    sscanf(emsg, "%*d %d", &size);
    if (status != 213) size = 0;
    free_mem(query);
    free_mem(emsg);
    /*
     * If it's actually a directory, or if the filename is syntactically wrong,
     * we'd be getting a 550 or 553 here, and don't proceed to RETR.
     */
    if (status < 550)
    {
      query = alloc_mem(flen + strlen(format2) + 1);
      sprintf(query, format2, filename);
      if ((reply = soak(s, query, &emsg)) == 999)
      {
	net_close(s);
	net_close(d);
	free_mem(query);
	return(fixup_ftp_message(emsg));
      }
      free_mem(query);
      free_mem(emsg);
    }
    if (status < 550 && reply < 400)
    {
      /*
       * Read file from the FTP host
       */
      t = ReadBuffer(d, &tlen, size, size);
      if (t == NULL) return(NULL);

      content = Ext2Content(mtlist, filename);
      if (content == NULL) content = "text/plain";

      net_close(d);
      net_close(s);

      return(BuildDocument(t, tlen, content, 0, 1));
    }
    /*
     * If the retrieve fails try to treat the file as a directory.
     */
  }
  /*
   * CWD and ask for a listing.
   */
  query = alloc_mem(flen + strlen(format3) + 1);
  sprintf (query, format3, filename);
  if (soak(s, query, &emsg) >= 400)
  {
    net_close(d);
    net_close(s);
    free_mem(query);
    return(fixup_ftp_message(emsg));
  }
  free_mem(query);
  free_mem(emsg);

  if (soak(s, "NLST\r\n", &emsg) >= 400) /* contemplate doing a LIST as well */
  {
    net_close(s);
    net_close(d);
    return(fixup_ftp_message(emsg));
  }
  free_mem(emsg);

  t = ftp_dir(d, hostname, (up->port == 0 ? DEFAULT_FTP_PORT:up->port),
	      filename);
	      
	      
	      
// if (t == NULL) return(NULL);
	if (t == NULL) {	//the directory is empty
		t = "FTP directory is empty.\n";
		content = "text/plain";
	} else {
		content = "text/html";
	}
	
  tlen = strlen(t);

  net_close(d);
  net_close(s);
    
  return(BuildDocument(t, tlen, content, 0, 1));
}

/*
 * ftp_strcmp
 *
 * Wrapper to match type requirements of qsort
 * (changed upon advice by Nelson H F Beebe <beebe@math.utah.edu> 1997May09)
 */
static int
ftp_strcmp(const void *a, const void *b)
{
  return(strcmp(*(const char **) a, *(const char **) b));
}

/*
 * ftp_dir
 *
 * Read directory from FTP server, sort, and display.
 * (The right thing to do might be to fetch and display 00-index.html
 * instead if it exists... --GN)
 */
static char *
ftp_dir(int d, char *hostname, int portno, char *filename)
{
  int count;
  int size;
  int rval;
  char **sa;
  char buffer[BUFSIZ];
  char *cp;
  char *f;
  char *myfilename;
  int flen;
  int filelen;
  int entrylen;
  int i;
  char *header = GetFromStringDB("ftpheader");
  static char *entry = "<li><a href=ftp://%s:%d%s/%s>%s</a>\n";

  myfilename = alloc_string(filename);
  flen = strlen(filename);	/* f, flen will be reused later */
  for (f = myfilename + flen - 1; f >= myfilename && *f == '/'; f--)
    *f = '\0';			/* kill trailing slashes --GN 1997May09 */

  count = 0;
  size = 15;
  sa = (char **)alloc_mem(size * sizeof(char *));
  while ((rval = ReadLine(d, buffer, sizeof(buffer))) == 0)
  {
    if (count >= size)
    {
      size *= 4;
      sa = (char **)realloc_mem((char *)sa, size * sizeof(char *));
    }
    for (cp = buffer; *cp; cp++)
    {
      if (*cp == '\n' || *cp == '\r') break;
      /*      if (isspace(*cp)) break; */
    }
    *cp = '\0';
#ifdef DEBUG_FTP
    fprintf(stderr, "DirLine %s\n", buffer);
#endif
    sa[count] = alloc_string(buffer);
    count += 1;
  }

  if (count == 0)
  {
    free_mem((char *)sa);
    free_mem(myfilename);
    return(NULL);
  }

  qsort(sa, count, sizeof(char *), &ftp_strcmp);

  filelen = strlen(myfilename) + strlen(hostname) + 10;
  entrylen = strlen(entry);

  flen = strlen(header) + 2 * filelen + 1;
  f = (char *)alloc_mem(flen);
  sprintf (f, header, myfilename, hostname, myfilename);

  /*
  if (myfilename[0] != '\0' && myfilename[1] == '\0') myfilename = "";
  * should no longer be necessary --GN 1997May09
  */
  for (i = 0; i < count; i++)
  {
    flen += 2 * strlen(sa[i]) + filelen + entrylen;
    f = (char *)realloc_mem(f, flen);
    sprintf(f + strlen(f), entry, hostname, portno, myfilename, sa[i], sa[i]);
    free_mem(sa[i]);
  }
  sprintf(f + strlen(f), "\n</ul>\n");
  free_mem((char *)sa);
  free_mem(myfilename);

  return(f);
}
