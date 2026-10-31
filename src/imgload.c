/*
 * imgload.c
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
 * Background image loading
 *
 * Originally every image was fetched while the page was laid out, one
 * after the other, with the window frozen. On a page like cnn.com that
 * meant 80 fetches of about 200 ms each: most of it the start of a
 * helper process and a TLS handshake, not the download.
 *
 * Now the page is shown at once with placeholders. Its remote images are
 * queued here (those in view first) and fetched while the event loop
 * runs:
 *
 *   - Images for a protocol with a helper program (https through
 *     https-helper.sh) run up to max_running helpers at the same time.
 *     Their output is read through XtAppAddInput as it arrives, so the
 *     window stays usable.
 *   - Other images (Chimera's own http, ftp, ...) can only be fetched in
 *     the foreground. They are fetched one at a time from a timeout, so
 *     the window gets its events in between.
 *
 * A finished image is kept here until the page is laid out again: the
 * image resolver in main.c takes it with ImgLoadTake() instead of
 * fetching it. Arrivals are collected and the page is laid out at most
 * every APPLY_DELAY ms, since one layout of a big page costs as much as
 * several images.
 *
 * ImgLoadCancel() stops everything; main.c calls it when another page is
 * displayed and from the Stop button.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>

#include "common.h"
#include "url.h"
#include "mime.h"
#include "document.h"
#include "http.h"
#include "imgload.h"
#include "chtime.h"

#define APPLY_DELAY	250		/* ms between layouts while loading */
#define MAX_IMAGE_SIZE	(32 * 1024 * 1024) /* larger responses are dropped */

enum { E_QUEUED, E_RUNNING, E_READY, E_TAKEN, E_FAILED };

typedef struct img_entry
{
  char *src;			/* as written in the page */
  URLParts *up;			/* absolute */
  int external;			/* fetched by a helper program */
  int state;
  pid_t pid;			/* helper process group */
  int fd;			/* helper's stdout */
  XtInputId id;
  char *buf;
  int len, size;
  Document *doc;
  double started;
  struct img_entry *next;
} ImgEntry;

static XtAppContext app;
static Protocol *protocols;
static ImgFetchProc fetch_proc;
static ImgApplyProc apply_proc;
static int max_running = 6;

static ImgEntry *entries = NULL, *last_entry = NULL;
static int total = 0;		/* entries queued for this page */
static int finished = 0;	/* arrived or failed */
static int fresh = 0;		/* arrived since the last layout */
static int running = 0;		/* helpers running */
static int internal_busy = 0;	/* an internal fetch is scheduled */
static XtIntervalId apply_timer = 0;
static XtIntervalId internal_timer = 0;

static void StartMore _ArgProto((void));

void
ImgLoadInit(XtAppContext a, Protocol *plist, ImgFetchProc fetch, ImgApplyProc apply, int max)
{
  app = a;
  protocols = plist;
  fetch_proc = fetch;
  apply_proc = apply;
  max_running = max > 0 ? max : 1;
}

static char *
HelperCommand(URLParts *up)
{
  Protocol *p;

  if (up->protocol == NULL) return(NULL);
  for (p = protocols; p != NULL; p = p->next)
  {
    if (strcasecmp(up->protocol, p->proto) == 0) return(p->command);
  }
  return(NULL);
}

void
ImgLoadAdd(const char *src, URLParts *up)
{
  ImgEntry *e;

  for (e = entries; e != NULL; e = e->next)
  {
    if (strcmp(e->src, src) == 0) return;	/* already queued */
  }

  e = (ImgEntry *)alloc_mem(sizeof(ImgEntry));
  memset(e, 0, sizeof(ImgEntry));
  e->src = alloc_string((char *)src);
  e->up = DupURLParts(up);
  e->external = (HelperCommand(up) != NULL);
  e->state = E_QUEUED;
  e->fd = -1;

  if (last_entry != NULL) last_entry->next = e;
  else entries = e;
  last_entry = e;
  total++;
}

static void
ApplyCB(XtPointer cldata, XtIntervalId *id)
{
  int relayout = fresh;

  apply_timer = 0;
  fresh = 0;
  if (apply_proc != NULL) (*apply_proc)(relayout, finished, total);
}

static void
ScheduleApply()
{
  if (apply_timer == 0)
  {
    apply_timer = XtAppAddTimeOut(app, finished == total ? 0 : APPLY_DELAY,
				  ApplyCB, NULL);
  }
}

/*
 * An image has arrived (doc != NULL) or failed (doc == NULL)
 */
static void
Finished(ImgEntry *e, Document *doc)
{
  if (doc != NULL && doc->status == DS_OK && doc->text != NULL && doc->len > 0)
  {
    if (doc->up == NULL) doc->up = DupURLParts(e->up);
    e->doc = doc;
    e->state = E_READY;
    fresh++;
  }
  else
  {
    if (doc != NULL) DestroyDocument(doc);
    e->state = E_FAILED;
  }

  if (ChTimingOn())
  {
    fprintf(stderr, "bg    %7.1f ms  %s  %s\n", ChTimeMs() - e->started,
	    e->state == E_READY ? "ok    " : "FAILED", e->src);
  }

  finished++;
  ScheduleApply();
}

static void
StopHelper(ImgEntry *e)
{
  if (e->id != 0) XtRemoveInput(e->id);
  e->id = 0;
  if (e->fd >= 0) close(e->fd);
  e->fd = -1;
  if (e->buf != NULL) free_mem(e->buf);
  e->buf = NULL;
  e->len = e->size = 0;
}

static void
ReadCB(XtPointer cldata, int *fd, XtInputId *id)
{
  ImgEntry *e = (ImgEntry *)cldata;
  int n;

  while (1)
  {
    if (e->len == e->size)
    {
      if (e->size >= MAX_IMAGE_SIZE)
      {
	StopHelper(e);
	kill(-e->pid, SIGTERM);
	running--;
	Finished(e, NULL);
	StartMore();
	return;
      }
      e->size = e->size ? e->size * 2 : 65536;
      e->buf = e->buf ? realloc_mem(e->buf, e->size) : alloc_mem(e->size);
    }

    n = read(e->fd, e->buf + e->len, e->size - e->len);
    if (n > 0)
    {
      e->len += n;
      continue;
    }
    if (n < 0 && errno == EINTR) continue;
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
    break;			/* end of output, or an error */
  }

  {
    Document *doc = NULL;

    if (n == 0 && e->len > 0) doc = ParseHTTPResponseBuffer(e->buf, e->len);
    StopHelper(e);		/* the helper exits; SIGCHLD reaps it */
    running--;
    Finished(e, doc);
  }
  StartMore();
}

/*
 * Start the helper for e. Like PipeCommand(), but the helper gets a
 * process group of its own, so that ImgLoadCancel() can stop it with the
 * curl it runs, and Chimera's ends of the pipes are not passed on to
 * the helpers started later.
 */
static int
StartHelper(ImgEntry *e)
{
  char *command = HelperCommand(e->up);
  int pin[2], pout[2];
  pid_t pid;

  if (command == NULL) return(-1);
  if (pipe(pin) == -1) return(-1);
  if (pipe(pout) == -1)
  {
    close(pin[0]);
    close(pin[1]);
    return(-1);
  }

  fflush(NULL);			/* see PipeCommand() */
  pid = fork();
  if (pid == -1)
  {
    close(pin[0]); close(pin[1]);
    close(pout[0]); close(pout[1]);
    return(-1);
  }
  if (pid == 0)
  {
    setpgid(0, 0);
    dup2(pin[0], 0);
    dup2(pout[1], 1);
    close(pin[0]); close(pin[1]);
    close(pout[0]); close(pout[1]);
    signal(SIGPIPE, SIG_DFL);	/* Chimera ignores these two */
    signal(SIGALRM, SIG_DFL);
    execl("/bin/sh", "sh", "-c", command, (char *)NULL);
    _exit(127);
  }

  setpgid(pid, pid);		/* also here, whichever runs first */
  close(pin[0]);
  close(pout[1]);
  fcntl(pin[1], F_SETFD, FD_CLOEXEC);
  fcntl(pout[0], F_SETFD, FD_CLOEXEC);

  /* the request is a few hundred bytes: the pipe takes it at once */
  if (http_request(pin[1], e->up, 3, 0) == -1)
  {
    close(pin[1]);
    close(pout[0]);
    kill(-pid, SIGTERM);
    return(-1);
  }
  close(pin[1]);

  fcntl(pout[0], F_SETFL, fcntl(pout[0], F_GETFL) | O_NONBLOCK);
  e->pid = pid;
  e->fd = pout[0];
  e->id = XtAppAddInput(app, e->fd, (XtPointer)XtInputReadMask, ReadCB,
			(XtPointer)e);
  return(0);
}

/*
 * One image for which Chimera has no helper, fetched in the foreground
 */
static void
InternalCB(XtPointer cldata, XtIntervalId *id)
{
  ImgEntry *e;
  Document *doc;

  internal_timer = 0;
  for (e = entries; e != NULL; e = e->next)
  {
    if (e->state == E_QUEUED && !e->external) break;
  }
  if (e == NULL)
  {
    internal_busy = 0;
    return;
  }

  e->state = E_RUNNING;
  e->started = ChTimeMs();
  doc = (fetch_proc != NULL) ? (*fetch_proc)(e->up) : NULL;
  internal_busy = 0;
  Finished(e, doc);
  StartMore();
}

static void
StartMore()
{
  ImgEntry *e;

  for (e = entries; e != NULL; e = e->next)
  {
    if (e->state != E_QUEUED) continue;

    if (e->external)
    {
      if (running >= max_running) continue;
      e->started = ChTimeMs();
      if (StartHelper(e) == 0)
      {
	e->state = E_RUNNING;
	running++;
      }
      else Finished(e, NULL);
    }
    else if (!internal_busy)
    {
      internal_busy = 1;
      internal_timer = XtAppAddTimeOut(app, 0, InternalCB, NULL);
    }
  }
}

void
ImgLoadRun()
{
  if (total > 0) StartMore();
}

void
ImgLoadCancel()
{
  ImgEntry *e, *next;

  for (e = entries; e != NULL; e = next)
  {
    next = e->next;
    if (e->state == E_RUNNING && e->external)
    {
      StopHelper(e);
      kill(-e->pid, SIGTERM);
    }
    if (e->buf != NULL) free_mem(e->buf);
    if (e->doc != NULL) DestroyDocument(e->doc);
    if (e->up != NULL) DestroyURLParts(e->up);
    free_mem(e->src);
    free_mem((char *)e);
  }
  entries = last_entry = NULL;
  total = finished = fresh = running = 0;

  if (apply_timer != 0) XtRemoveTimeOut(apply_timer);
  apply_timer = 0;
  if (internal_timer != 0) XtRemoveTimeOut(internal_timer);
  internal_timer = 0;
  internal_busy = 0;
}

int
ImgLoadTake(const char *src, Document **dp)
{
  ImgEntry *e;

  *dp = NULL;
  for (e = entries; e != NULL; e = e->next)
  {
    if (strcmp(e->src, src) != 0) continue;

    switch (e->state)
    {
      case E_QUEUED:
      case E_RUNNING:
	return(IMG_PENDING);
      case E_READY:
	*dp = e->doc;
	e->doc = NULL;
	e->state = E_TAKEN;
	return(IMG_READY);
      default:
	return(IMG_FAILED);
    }
  }
  return(IMG_UNKNOWN);
}

int
ImgLoadBusy()
{
  return(finished < total);
}

/*
 * Move the queued images among srcs (as written in the page) to the
 * front of the queue, in the order given: main.c passes the images in
 * view first when the page has been scrolled. Images already being
 * fetched or finished are not affected.
 */
void
ImgLoadPrioritize(char **srcs, int n)
{
  ImgEntry *front = NULL, *front_last = NULL;
  int i;

  for (i = 0; i < n; i++)
  {
    ImgEntry **pp, *e;

    for (pp = &entries; (e = *pp) != NULL; pp = &e->next)
    {
      if (e->state == E_QUEUED && strcmp(e->src, srcs[i]) == 0) break;
    }
    if (e == NULL) continue;

    *pp = e->next;			/* unlink */
    if (last_entry == e)
    {
      ImgEntry *t;

      last_entry = NULL;
      for (t = entries; t != NULL; t = t->next) last_entry = t;
    }
    e->next = NULL;
    if (front_last != NULL) front_last->next = e;
    else front = e;
    front_last = e;
  }

  if (front != NULL)
  {
    front_last->next = entries;
    entries = front;
    if (last_entry == NULL) last_entry = front_last;
  }
}
