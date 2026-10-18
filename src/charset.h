/*
 * charset.h
 *
 * Copyright (C) 2026 M. Glargaard, aka graybox
 * zlib licence, see charset.c
 */

#ifndef CHARSET_H
#define CHARSET_H

int LooksLikeUTF8 (const char *text, int len);
char *Latin1Text (const char *text, int len, int utf8, int is_html, int *outlen);

#endif
