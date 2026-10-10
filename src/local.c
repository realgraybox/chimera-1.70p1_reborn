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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

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
 * add_list_entry adds one HTML table based long directory to file list.
 */
void add_list_entry (char *current_dir, char *filename, int count, char **sa, char *filetype)
{
	char naam[2048] = "";	// complete
	char rowdata[7][2048];	// for 1024 path+table overhead
	int i, j=0;
	struct stat buf;

	strncpy(naam, current_dir, 2047);
	//if (filetype == "dir") {
	if (strcmp(filetype, "dir") == 0) {
		strncat(naam, filename, 2047 - strlen(naam));
	} else {
		strcat(naam, "/");
		strncat(naam, filename, 2047 - strlen(naam));
	}
	lstat (naam, &buf);
	
	rowdata[0][0] = 0;	/* file name */
	rowdata[1][0] = 0;	/* mode */
	rowdata[2][0] = 0;	/* owner */
	rowdata[3][0] = 0;	/* group ID */
	rowdata[4][0] = 0;	/* file size */
	rowdata[5][0] = 0;	/* type */
	rowdata[6][0] = 0;	/* last modified */

//	filename
	sprintf (rowdata[j],"<DT><tr><td width=\"150\"><a href=\"%s\">%s</a></td> ",naam,filename);
	j++;

//	SUID
	sprintf (rowdata[j],"<td width=\"025\"> %3i</td>",((int)buf.st_mode  & 0xf000) >> 13);
	j++;

//	perms
	sprintf (rowdata[j],"<td width=\"050\"> %5o</td>",buf.st_mode & 0xfff0fff);
	j++;

//	uid
	sprintf (rowdata[j],"<td width=\"050\"> %5i</td>",buf.st_uid);
	j++;

//	gid
	sprintf (rowdata[j],"<td width=\"050\"> %5i</td>",buf.st_gid);
	j++;

//	size
	sprintf (rowdata[j],"<td width=\"100\"> %11i</td>",(int)buf.st_size);
	j++;

//	modified date/time
	{	// decode date/time in sortable format: 'yyyy/mm/dd hh:mm;ss'
		char ret[22];
		struct tm *tmb = (localtime(&buf.st_mtime));
		sprintf (ret," 1%03i/%02i/%02i %02i:%02i:%02i",
					tmb->tm_year, tmb->tm_mon+1, tmb->tm_mday, 
					tmb->tm_hour, tmb->tm_min, tmb->tm_sec);
		// sprintf returns 103 for year 2003, 95 for 1995
		if(ret[2]=='1')
			{ret[1] = '2';ret[2]='0';}
		else
			{ret[2]='9';}
		sprintf (rowdata[j],"<td width=\"150\"> %s</td></tr>\n",ret);
		j++;
	}
	naam[0] = '\0';
	for(i=0; i < j; i++ ) strncat(naam, rowdata[i], 2047 - strlen(naam));
	sa[count] =  alloc_string(naam);
	return;
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
  int flen;
  int filelen;
  int formlen;
  int count, size;
  int i;
  char *f;
  static char *format = "<li><a href=\"file:%s/%s\">%s</a>\n";
  static char *rformat = "<li><a href=\"file:%s%s\">%s</a>\n";
  char *header = GetFromStringDB("localheader");
  char *useformat = (filename[strlen(filename) - 1] == '/')? rformat: format;
  char *useshtfmt = (filename[strlen(filename) - 1] == '/')? "%s%s": "%s/%s";
  char **sa;
  char **lifn;
  struct stat s;
  char *filetype = (filename[strlen(filename) - 1] == '/')? "dir": "file";
   
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

    // construct table based long directory entry for each file
	add_list_entry(filename, (char*) &de->d_name, count, sa, filetype);
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

  filelen = strlen(filename);
  formlen = strlen(useformat);

  // hack string length for loooong table field header, e.g.:
  // Filename SUID Perms UID GID Size Last-Mod
  flen = strlen(header) - 2 * 2 + 2 * filelen + 1 + 236;
  f = (char *)alloc_mem(flen);
  sprintf (f, header, filename, filename, "\n<table border=\"0\">\n<DT><tr><td width=\"150\">Filename</td> <td width=\"025\">SUID</td><td width=\"050\">Perms</td><td width=\"050\">UID</td><td width=\"050\">GID</td><td width=\"050\">Size</td><td width=\"150\">Last Mod</td></tr>\n");

  for (i = 0; i < count; i++)
  {
    flen += formlen + 2 * strlen(sa[i]) + filelen - 3 * 2;
    f = (char *)realloc_mem(f, flen);
   // sprintf(f + strlen(f), sa[i]);
   sprintf(f + strlen(f), "%s", sa[i]);
    free_mem(sa[i]);
  }
  sprintf(f + strlen(f), "\n</table>\n");
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

  filename = FixFilename(up->filename);
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
