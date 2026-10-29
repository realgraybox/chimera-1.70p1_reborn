/*
 * local.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <ctype.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#if defined(SYSV) || defined(SVR4) || defined(__arm)
#include <dirent.h>
#define DIRSTUFF struct dirent
#else
#include <sys/dir.h>
#define DIRSTUFF struct direct
#endif

/* Jim Rees fix */
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#define DEFAULT_TELNET_PORT 23

#include "common.h"
#include "util.h"
#include "url.h"
#include "mime.h"
#include "document.h"
#include "local.h"
#include "ftp.h"
#include "input.h"
#include "net.h"
#include "stringdb.h"

static char **ilist;

/*
 * ReadIndexFilenames
 * 
 * Convert colon-delimited string into array of strings, delimited by a
 * NULL pointer. Memory allocated here stays allocated until Chimera exits.
 */
void
ReadIndexFilenames(filelist)
char *filelist;
{
  int sl, colons;
  char *filenames, *cp, *cp2;

  if (filelist == NULL)
  {
    ilist = (char **)alloc_mem(sizeof(char *));
    *ilist = (char *)NULL;
    return;
  }
  sl = strlen(filelist);
  if (sl == 0)
  {
    ilist = (char **)alloc_mem(sizeof(char *));
    *ilist = (char *)NULL;
    return;
  }
  filenames = (char *)alloc_mem(sl * sizeof(char)+1);
  colons = 2;			/* count final delimiters */
  for (cp=filelist,cp2=filenames; (*cp2 = *cp) != '\0'; cp++,cp2++)
  {
    if (*cp == ':')
    {
      colons++;
      *cp2 = '\0';
    }
  }
  ilist = (char **)alloc_mem(colons * sizeof(char *));
  ilist[0] = filenames;
  colons = 1;
  for (cp=filelist,cp2=filenames; *cp != '\0'; cp++,cp2++)
  {
    if (*cp == ':')
    {
      ilist[colons] = cp2 + 1;
      colons++;
    }
  }
  ilist[colons] = (char *)NULL;
  return;
}

/*
 * LoadFile
 *
 * Load the contents of a file into memory.
 */
static Document *
LoadFile(filename, size, mtlist)
char *filename;
int size;
MIMEType *mtlist;
{
  FILE *fp;
  char *b;
  char *content;
  Document *d;

  fp = fopen(filename, "r");
  if (fp == NULL)
  {
    char *msg = GetFromStringDB("localaccess");

    d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
    d->status = DS_ERROR;
    return(d);
  }

  b = (char *)alloc_mem(size + 1);
  if (fread(b, 1, size, fp) < size)
  {
    free_mem(b);
    fclose(fp);
    return(NULL);
  }
  b[size] = '\0';

  fclose(fp);

  content = Ext2Content(mtlist, filename);
  if (content == NULL) content = "text/plain";
  d = BuildDocument(b, size, content, 0, 0);

  return(d);
}

/*
 * local_strcmp
 *
 * Wrapper to match type requirements of qsort
 * (changed upon advice by Nelson H F Beebe <beebe@math.utah.edu> 1997May09)
 */
static int
local_strcmp(a, b)
const void *a, *b;
{
  return(strcmp(*(const char **)a, *(const char **)b));
}

/*
 * dir_entry_path
 *
 * The path of entry "name" in directory "dir". "." and ".." are resolved
 * to the directory itself and its parent, so that going up from a listing
 * gives "/a" rather than "/a/b/..", "/a/b/../..", and so on.
 * Returns allocated memory.
 */
static char *
dir_entry_path(const char *dir, const char *name)
{
  char *p;
  size_t len = strlen(dir);

  p = (char *)alloc_mem(len + strlen(name) + 2);
  strcpy(p, dir);
  while (len > 1 && p[len - 1] == '/') p[--len] = '\0';  /* "/a/b/" -> "/a/b" */

  if (strcmp(name, ".") == 0)
    return(p);

  if (strcmp(name, "..") == 0)
  {
    char *slash = strrchr(p, '/');

    if (slash == NULL || slash == p) strcpy(p, "/");
    else *slash = '\0';
    return(p);
  }

  if (len > 1 || p[0] != '/') strcat(p, "/");
  strcat(p, name);
  return(p);
}

/*
 * escape_path
 *
 * %-encodes a local path for use in a file: URL. ParseURL drops spaces
 * from URLs, and '#', '?', '%' and quotes have their own meaning, so
 * everything except letters, digits, '/' and "-_." is encoded.
 * file() decodes it again. Returns allocated memory.
 */
static char *
escape_path(const char *path)
{
  static const char *hex = "0123456789ABCDEF";
  const unsigned char *cp;
  char *s, *n;

  s = n = (char *)alloc_mem(strlen(path) * 3 + 1);
  for (cp = (const unsigned char *)path; *cp; cp++)
  {
    if (isalnum(*cp) || strchr("/-_.", *cp) != NULL) *n++ = *cp;
    else
    {
      *n++ = '%';
      *n++ = hex[*cp / 16];
      *n++ = hex[*cp % 16];
    }
  }
  *n = '\0';
  return(s);
}

/*
 * escape_html
 *
 * Makes a file name safe to show as HTML text. Returns allocated memory.
 */
static char *
escape_html(const char *text)
{
  const char *cp;
  char *s, *n;

  s = n = (char *)alloc_mem(strlen(text) * 6 + 1);
  for (cp = text; *cp; cp++)
  {
    switch (*cp)
    {
      case '&': strcpy(n, "&amp;"); n += 5; break;
      case '<': strcpy(n, "&lt;"); n += 4; break;
      case '>': strcpy(n, "&gt;"); n += 4; break;
      case '"': strcpy(n, "&quot;"); n += 6; break;
      default: *n++ = *cp; break;
    }
  }
  *n = '\0';
  return(s);
}

/*
 * add_list_entry
 *
 * Builds one row of the HTML table for a directory listing and stores
 * it in sa[count].
 */
static void
add_list_entry(char *current_dir, char *filename, int count, char **sa)
{
  struct stat buf;
  char *path, *href, *label, *row;
  char date[32];
  struct tm *tmb;
  size_t rowlen;

  path = dir_entry_path(current_dir, filename);
  if (lstat(path, &buf) != 0) memset(&buf, 0, sizeof(buf));

  href = escape_path(path);
  label = escape_html(filename);

  tmb = localtime(&buf.st_mtime);
  if (tmb != NULL)
    snprintf(date, sizeof(date), "%04d/%02d/%02d %02d:%02d:%02d",
	     tmb->tm_year + 1900, tmb->tm_mon + 1, tmb->tm_mday,
	     tmb->tm_hour, tmb->tm_min, tmb->tm_sec);
  else
    date[0] = '\0';

  rowlen = strlen(href) + strlen(label) + 512;
  row = (char *)alloc_mem(rowlen);
  snprintf(row, rowlen,
	   "<DT><tr><td width=\"150\"><a href=\"file:%s\">%s</a></td> "
	   "<td width=\"025\"> %3i</td>"
	   "<td width=\"050\"> %5o</td>"
	   "<td width=\"050\"> %5i</td>"
	   "<td width=\"050\"> %5i</td>"
	   "<td width=\"100\"> %11ld</td>"
	   "<td width=\"150\"> %s</td></tr>\n",
	   href, label,
	   ((int)buf.st_mode & 0xf000) >> 13,
	   (unsigned int)(buf.st_mode & 0xfff0fff),
	   (int)buf.st_uid,
	   (int)buf.st_gid,
	   (long)buf.st_size,
	   date);

  sa[count] = row;
  free_mem(path);
  free_mem(href);
  free_mem(label);
}

/*
 * LoadDir
 *
 * Reads a local directory and converts it into HTML so the user can
 * navigate the local filesystem.
 */
static Document *
LoadDir(filename, mtlist)
char *filename;
MIMEType *mtlist;
{
  Document *d;
  DIR *dp;
  DIRSTUFF *de;
  size_t flen;
  int count, size;
  int i;
  char *f, *title;
  static char *tablehead = "\n<table border=\"0\">\n<DT><tr><td width=\"150\">Filename</td> <td width=\"025\">SUID</td><td width=\"050\">Perms</td><td width=\"050\">UID</td><td width=\"050\">GID</td><td width=\"050\">Size</td><td width=\"150\">Last Mod</td></tr>\n";
  char *header = GetFromStringDB("localheader");
  char *useshtfmt = (filename[strlen(filename) - 1] == '/')? "%s%s": "%s/%s";
  char **sa;
  char **lifn;
  struct stat s;
   
  /*
   * First see if there is a welcome or index file.  If you don't like this
   * behavior, set X resource localIndexFiles to an empty string.
   */
  for (lifn = ilist; *lifn; lifn++)
  {
    if (strlen(*lifn) == 0)
      continue;
    f = (char *) alloc_mem(strlen(filename) + strlen(*lifn) + 2);
    sprintf(f, useshtfmt, filename, *lifn);
    i = stat(f, &s);
    if (i >= 0 && !S_ISDIR(s.st_mode))
    {
      d = LoadFile(f, (int) s.st_size, mtlist);
      free_mem(f);
      return(d);
    }
    else
    {
      free_mem(f);
      continue;
    }
  }

  dp = opendir(filename);
  if (dp == NULL)
  {
    char *msg = GetFromStringDB("localaccess");

    d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
    d->status = DS_ERROR;
    return(d);
  }
  
  count = 0;
  size = 15;
  sa = (char **)alloc_mem(size * sizeof(char *));
  for (de = readdir(dp); de != NULL; de = readdir(dp))
  {
    if (count >= size)
    {
      size *= 4;
      sa = (char **)realloc_mem((char *)sa, size * sizeof(char *));
    }

    /* one table row for each file */
    add_list_entry(filename, de->d_name, count, sa);
    count += 1;
  }
  closedir(dp);

  if (count == 0)
  {
    char *msg = GetFromStringDB("localaccess");

    free_mem((char *)sa);
    d = BuildDocument(msg, strlen(msg), "text/html", 1, 0);
    d->status = DS_ERROR;
    return(d);
  }

  qsort(sa, count, sizeof(char *), &local_strcmp);

  title = escape_html(filename);
  flen = strlen(header) + 2 * strlen(title) + strlen(tablehead) + 32;
  for (i = 0; i < count; i++) flen += strlen(sa[i]);

  f = (char *)alloc_mem(flen);
  snprintf(f, flen, header, title, title, tablehead);
  free_mem(title);

  for (i = 0; i < count; i++)
  {
    strcat(f, sa[i]);
    free_mem(sa[i]);
  }
  strcat(f, "\n</table>\n");
  free_mem((char *)sa);

  return(BuildDocument(f, strlen(f), "text/html", 0, 0));
}

/*
 * file
 *
 * Grabs a local file or tries to grab the file using FTP (if the hostname
 * is not localhost).
 * [One might try an rcp as an intermediate approach, using the username
 * supplied with the hostname if present... GN 1997May06, just musing]
 * Empty (or NULL) hostnames should be treated as equivalent to localhost.
 * GN 1997May18
 */
Document *
file(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  char *domain;
  struct stat s;
  char *filename;
  Document *d;
  char *hostname;

  hostname = ((up->hostname != NULL && up->hostname[0] != '\0') ?
    up->hostname : "localhost");
  if (strcasecmp(hostname, "localhost") != 0)
  {
    domain = net_gethostname(); /* gets the full domain name */
    if (domain == NULL) return(NULL);
    if (strcasecmp(domain, hostname) != 0)
    {
      char myhostname[256];

      gethostname(myhostname, sizeof(myhostname)); /* gets only hostname ? */
      if (strcasecmp(hostname, myhostname) != 0)
      {
	return(ftp(up, mtlist));
      }
    }
  }

  /* file: URLs are %-encoded, see escape_path() */
  {
    char *plain = UnescapeURL(up->filename);

    filename = (plain != NULL) ? FixFilename(plain) : NULL;
    if (plain != NULL) free_mem(plain);
  }
  if (filename == NULL || stat(filename, &s) < 0)
  {
    char *msg = GetFromStringDB("localaccess");

    return(BuildDocument(msg, strlen(msg), "text/html", 1, 0));
  }

  if (S_ISDIR(s.st_mode)) d = LoadDir(filename, mtlist);
  else d = LoadFile(filename, (int) s.st_size, mtlist);

  d->cache = 0;

  return(d);
}

/*
 * telnet_main
 *
 * Handles telnet and tn3270 URLs
 */
static Document *
telnet_main(up, content)
URLParts *up;
char *content;
{
  char *username;
  char *password;
  char *t;
  int portno;
  static char *format = "%s %d %s %s\n";

  if (up->hostname == NULL) return(NULL);

  if (up->username == NULL) username = alloc_string("");
  else username = up->username;

  if (up->password == NULL) password = alloc_string("");
  else password = up->password;

  if (up->port == 0) portno = DEFAULT_TELNET_PORT;
  else portno = up->port;

  t = alloc_mem(strlen(format) +
		strlen(up->hostname) +
		12 +
		strlen(username) +
		strlen(password));
  sprintf (t, format, up->hostname, portno, username, password);
  //valgrind
  if (username != NULL)
  {
    free_mem (username);
  }
  
  if (password != NULL)
  {
    free_mem (password);
  }
//
  return(BuildDocument(t, strlen(t), content, 0, 0));
}

/*
 * telnet
 *
 * Handles telnet URLs
 */
Document * 
telnet(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(telnet_main(up, "application/x-telnet"));
}

/*
 * tn3270
 *
 * Handles tn3270 URLs
 */
Document *
tn3270(up, mtlist)
URLParts *up;
MIMEType *mtlist;
{
  return(telnet_main(up, "application/x-tn3270"));
}
