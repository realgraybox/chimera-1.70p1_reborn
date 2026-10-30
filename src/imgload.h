/*
 * imgload.h - loading a page's images in the background
 *
 * Copyright (C) 2026 M. Glargaard, aka graybox
 * zlib licence, see imgload.c
 */

#ifndef IMGLOAD_H
#define IMGLOAD_H

/* Fetch a document right away, without touching the window (main.c) */
typedef Document *(*ImgFetchProc) _ArgProto((URLParts *));

/*
 * Called when images have arrived: relayout is non-zero when the page
 * should be laid out again to show them; done of total images are
 * finished (arrived or failed).
 */
typedef void (*ImgApplyProc) _ArgProto((int relayout, int done, int total));

/* ImgLoadTake results */
#define IMG_UNKNOWN 0		/* not loaded in the background */
#define IMG_PENDING 1		/* queued or on its way */
#define IMG_READY   2		/* here it is; the caller owns *dp now */
#define IMG_FAILED  3		/* failed, or already handed out */

void ImgLoadInit _ArgProto((XtAppContext, Protocol *, ImgFetchProc,
			    ImgApplyProc, int));
void ImgLoadAdd _ArgProto((const char *, URLParts *));
void ImgLoadRun _ArgProto((void));
void ImgLoadCancel _ArgProto((void));
int  ImgLoadTake _ArgProto((const char *, Document **));
int  ImgLoadBusy _ArgProto((void));
void ImgLoadPrioritize _ArgProto((char **, int));

#endif
