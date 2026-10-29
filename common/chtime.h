/*
 * chtime.h - timing output for finding out where page loading spends
 * its time. Enabled by setting CHIMERA_TIMING in the environment:
 *
 *   CHIMERA_TIMING=1 chimera http://example.com/ 2>timing.txt
 *
 * Copyright (C) 2026 M. Glargaard, aka graybox
 * zlib licence, see src/charset.c
 */

#ifndef CHTIME_H
#define CHTIME_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/* True when CHIMERA_TIMING is set; looked up once per file. */
static inline int
ChTimingOn(void)
{
  static int on = -1;

  if (on < 0) on = (getenv("CHIMERA_TIMING") != NULL);
  return on;
}

/* Wall clock time in milliseconds */
static inline double
ChTimeMs(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

#endif
