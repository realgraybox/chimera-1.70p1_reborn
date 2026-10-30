/*
 * util.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"

#include "options.h"

#include <stdio.h>
#include <pwd.h>
#include <errno.h>

#include <string.h>

#include <stdlib.h>

#include <unistd.h>

#include <sys/types.h>
/* QNX support added by D J Hawkey Jr <hawkeyd@visi.com> 1997May16 */
#ifdef __QNX__
#include <unix.h>
#else
#include <sys/signal.h>
#endif
#include <sys/wait.h>
#include <sys/stat.h>

#include <sys/param.h>

#include <ctype.h>

#include "common.h"
#include "util.h"

/*
 * FixFilename
 *
 * The only thing this does right now is to handle the '~' stuff.
 */
char *
FixFilename(char *filename)
{
  struct passwd *p;
  char *cp, *cp2;
  char username[BUFSIZ];
  char *fname;
  char *home;
  static char newfilename[MAXPATHLEN + 1];

  if (filename[0] == '~') fname = filename;
  else if (filename[0] == '/' && filename[1] == '~') fname = filename + 1;
  else fname = NULL;

  if (fname != NULL)
  {
    if (fname[1] == '/')
    {
      if ((home = getenv("HOME")) == NULL) return(NULL);
      cp = fname + 1;
    }
    else
    {
      for (cp = fname + 1, cp2 = username; *cp && *cp != '/'; cp++, cp2++)
      {
	*cp2 = *cp;
      }
      *cp2 = '\0';

      p = getpwnam(username);
      if (p == NULL) return(NULL);
      home = p->pw_dir;
    }

    if (strlen(home) + strlen(cp) > MAXPATHLEN) return(NULL);
    strcpy(newfilename, home);
    strcat(newfilename, cp);
  }
  else
  {
    if (strlen(filename) > MAXPATHLEN) return(NULL);
    strcpy(newfilename, filename);
  }

  return(newfilename);
}

/*
 * SaveData
 *
 * Save some memory to a file.  Returns 0 if successful, -1 otherwise.
 */
int
SaveData(char *data, int datalen, char *filename)
{
  FILE *fp;

  if (filename[0] == '|') fp = popen(filename + 1, "w");
  else fp = fopen(filename, "w");
  if (fp == NULL) return(-1);

  if (fwrite(data, 1, datalen, fp) < datalen)
  {
    if (filename[0] == '|') pclose(fp);
    else
    {
      fclose(fp);
      unlink(filename);
    }
  }

  if (filename[0] == '|') pclose(fp);
  else fclose(fp);

  return(0);
}

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

/*
 * ReapChild
 *
 * Automatically and efficiently cleans up all terminated subprocesses,
 * so that we completely avoid zombie processes (<defunct>).
 */
static void
ReapChild(int sig)
{
  (void)sig; // Prevents compiler warning about unused parameter
  int old_errno = errno;
  int pid;
  int status;

  /* Loop ensures that ALL zombies are cleared, even if they die at the same time */
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    /* Subprocess cleaned up. We don't need to do anything else here. */
  }

  errno = old_errno;
  return;
}

/*
 * StartReaper
 *
 * Registers our cleanup function to listen for dead processes.
 */
void
StartReaper()
{
  struct sigaction sa;

  sa.sa_handler = ReapChild;
  sigemptyset(&sa.sa_mask);
  /* SA_RESTART automatically restarts system calls if they are interrupted by the signal*/
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

  sigaction(SIGCHLD, &sa, NULL);
  return;
}

/*
 * mystrtok
 *
 */
char *
mystrtok(char *s, int c, char **cdr)
{
  char *cp, *cp2;
  static char str[BUFSIZ];

  if (s == NULL) return(NULL);

  for (cp = s, cp2 = str; ; cp++)
  {
    if (*cp == '\0') break;
    else if (*cp == c)
    {
      for (cp++; *cp == c; )
      {
	cp++;
      }
      break;
    }
    else *cp2++ = *cp;
  }

  *cp2 = '\0';

  if (*cp == '\0') *cdr = NULL;
  else *cdr = cp;

  return(str);
}

/*
 * CreateCommandLine
 */
char *
CreateCommandLine(char *command, char *path, char *filename)
{
  char *cmdline;

  cmdline = alloc_mem(strlen(command) + strlen(filename) + 1);
  sprintf (cmdline, command, filename);

  if (path != NULL && path[0] != '\0')
  {
#ifdef NOSEMICOLON
    char *format = "PATH=$PATH:%s %s";
#else
    char *format = "PATH=$PATH:%s; export PATH; %s";
#endif
    char *t = cmdline;

    cmdline = alloc_mem(strlen(format) + strlen(path) + strlen(t) + 1);
    sprintf (cmdline, format, path, t);
    free_mem(t);
  }

  return(cmdline);
}

/*
 * EscapeHTML
 *
 * Allocates a new string, and copies text into it, escaping all HTML
 * active characters. The string should be freed with free()
 *
 * This uses a two-pass algorithm.
 * Contributed by David.Robinson@ast.cam.ac.uk
 */
char *
EscapeHTML(char *text)
{
  int j, i;
  char *ret;
    
  /* count length and number of active characters in string */
  j = 0;
  for (i = 0; text[i] != '\0'; i++)
  {
    if (text[i] == '<' || text[i] == '&' || text[i] == '>') j++;
  }

  /* conservative; allow 4 _extra_ characters for each escape sequence */
  ret = (char *)alloc_mem(i + 1 + j*4);

  j = 0;
  for (i=0; text[i] != '\0'; i++)
  {
    if (text[i] == '<')
    {   
      memcpy(&ret[j], "&lt;", 4);
      j += 4;
    }
    else if (text[i] == '>')
    {
      memcpy(&ret[j], "&gt;", 4);
      j += 4;
    }
    else if (text[i] == '&')
    {
      memcpy(&ret[j], "&amp;", 5);
      j += 5;
    }
    else ret[j++] = text[i];
  }
  ret[j] = '\0';
  return ret;
}
