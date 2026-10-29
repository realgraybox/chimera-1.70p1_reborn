/*
 * inline.h
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */

#define COLOR_DISPLAY 0
#define GRAY_DISPLAY 1
#define MONO_DISPLAY 2

/*
 * Filled in by CreateImageInfo when CHIMERA_TIMING is set, read by
 * ImageResolve in main.c, which prints one line per image.
 */
typedef struct
{
  double decode, scale, colour, total;	/* milliseconds */
  int src_w, src_h;			/* size of the image as served */
  int w, h;				/* size after scaling */
  int ncolors;				/* palette entries used */
  int reduced;				/* 1 if median cut was needed */
  const char *how;			/* decoder used */
} ImgTiming;

extern ImgTiming img_timing;

ImageInfo *CreateImageInfo _ArgProto((Widget, Document *, Convert *, char *, int, int, XColor *, double));
