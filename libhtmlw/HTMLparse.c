/*
 * MODIFIED VERSION. This file is part of the HTML widget of NCSA Mosaic
 * for the X Window System 2.x, as modified for Chimera 1.70 (1997) and
 * for Chimera 1.70p1 reborn (2026,
 * https://github.com/realgraybox/chimera-1.70p1_reborn). It is not the
 * original NCSA Mosaic distributed by the University of Illinois.
 */
/****************************************************************************
 * NCSA Mosaic for the X Window System                                      *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 * mosaic@ncsa.uiuc.edu                                                     *
 *                                                                          *
 * Copyright (C) 1993, Board of Trustees of the University of Illinois      *
 *                                                                          *
 * NCSA Mosaic software, both binary and source (hereafter, Software) is    *
 * copyrighted by The Board of Trustees of the University of Illinois       *
 * (UI), and ownership remains with the UI.                                 *
 *                                                                          *
 * The UI grants you (hereafter, Licensee) a license to use the Software    *
 * for academic, research and internal business purposes only, without a    *
 * fee.  Licensee may distribute the binary and source code (if released)   *
 * to third parties provided that the copyright notice and this statement   *
 * appears on all copies and that no charge is associated with such         *
 * copies.                                                                  *
 *                                                                          *
 * Licensee may make derivative works.  However, if Licensee distributes    *
 * any derivative work based on or derived from the Software, then          *
 * Licensee will (1) notify NCSA regarding its distribution of the          *
 * derivative work, and (2) clearly notify users that such derivative       *
 * work is a modified version and not the original NCSA Mosaic              *
 * distributed by the UI.                                                   *
 *                                                                          *
 * Any Licensee wishing to make commercial use of the Software should       *
 * contact the UI, c/o NCSA, to negotiate an appropriate license for such   *
 * commercial use.  Commercial use includes (1) integration of all or       *
 * part of the source code into a product for sale or license by or on      *
 * behalf of Licensee to third parties, or (2) distribution of the binary   *
 * code or source code to third parties that need it to utilize a           *
 * commercial product sold or licensed by or on behalf of Licensee.         *
 *                                                                          *
 * UI MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR   *
 * ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED          *
 * WARRANTY.  THE UI SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY THE    *
 * USERS OF THIS SOFTWARE.                                                  *
 *                                                                          *
 * By using or copying this Software, Licensee agrees to abide by the       *
 * copyright law and all other applicable laws of the U.S. including, but   *
 * not limited to, export control laws, and the terms of this license.      *
 * UI shall have the right to terminate this license immediately by         *
 * written notice upon Licensee's breach of, or non-compliance with, any    *
 * of its terms.  Licensee may be held legally responsible for any          *
 * copyright infringement that is caused or encouraged by Licensee's        *
 * failure to abide by the terms of this license.                           *
 *                                                                          *
 * Comments and questions are welcome and can be sent to                    *
 * mosaic-x@ncsa.uiuc.edu.                                                  *
 ****************************************************************************/

#ifdef TIMING
#include <sys/time.h>
struct timeval Tv;
struct timezone Tz;
#endif
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "HTML.h"
#include "HTMLamp.h"

extern void FreeObjList ();
extern struct mark_up *AddObj ();
static int ParseMarkType (char *str);



#ifdef NOT_ASCII
#define TOLOWER(x)	(tolower(x))
#else

/*
 * A hack to speed up caseless_equal.  Thanks to Quincey Koziol for
 * developing it for me
 * Constraint: nothing but '\0' may map to 0
 */
unsigned char map_table[256] =
{
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 97, 98,
  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
  116, 117, 118, 119, 120, 121, 122, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
  103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
  120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
  137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
  154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170,
  171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204,
  205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221,
  222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238,
  239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

#define TOLOWER(x)	(map_table[(int)x])
#endif /* NOT_ASCII */


/*
 * Check if string s1 begins with s2, ignoring case (as defined by TOLOWER).
 * To match, s1 must have either NUL or a !isalnum() char where s2 ends.
 * Return 1 if s1 and s2 are non-NULL and s1 begins with s2; 0 otherwise.
 */
#include <ctype.h>
#include <string.h>
#include <stddef.h>

/*
 * Check if string s1 begins with s2, ignoring case.
 * To match, s1 must have either NUL or a !isalnum() char where s2 ends.
 * Return 1 if s1 and s2 are non-NULL and s1 begins with s2; 0 otherwise.
 */
int caseless_equal(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL) {
        return 0;
    }

    while (*s2 != '\0') {
        // Cast to unsigned char protects against negative values ​​for example æ, ø, å
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return 0;
        }
        s1++;
        s2++;
    }

    // Check if s1 ends here, or if the next character is not alphanumeric
    return (*s1 == '\0' || !isalnum((unsigned char)*s1));
}



/*
 * Clean up the white space in a string.
 * Remove all leading and trailing whitespace, and turn all
 * internal whitespace into single spaces separating words.
 * The cleaning is done by rearranging the chars in the passed
 * txt buffer.  The resultant string will probably be shorter,
 * it can never get longer.
 */

void clean_white_space(char *txt) {
    if (txt == NULL || *txt == '\0') {
        return;
    }

    char *src = txt;
    char *dst = txt;
    int in_whitespace = 0;

    // 1. Skip leading spaces
    while (isspace((unsigned char)*src)) {
        src++;
    }

    // 2. Pass through and compress internal spaces
    while (*src != '\0') {
        if (isspace((unsigned char)*src)) {
            in_whitespace = 1;
        } else {
            // If we just left a series of spaces, we insert a single one here
            if (in_whitespace) {
                *dst++ = ' ';
                in_whitespace = 0;
            }
            *dst++ = *src;
        }
        src++;
    }

    // 3. Close the string (trailing spaces will be automatically trimmed)
    *dst = '\0';
}


/*
 * Parse an ampersand escape, and return the appropriate character, or
 * '\0' on error.
 * Turns out the escapes are case sensitive, use strncmp.
 * Termination states:
 *      0: terminated with a ';'
 *      1: unterminated
 *      2: terminated with whitespace
 */

char ExpandEscapes(const char *esc, const char **endp, int termination) {
    int unterminated = (termination & 0x01);
    char val = '\0';

    if (esc == NULL || *esc == '\0') {
        if (endp) *endp = esc;
        return '\0';
    }

    esc++; // Skip '&'

    // 1. HANDLING NUMERIC ESCAPES (e.g. &#64;)
    if (*esc == '#') {
        esc++; // Skip '#'
        char *next_char;
        
        // strtol parses the number directly and tells us exactly where the number stops via next_char
        long num = strtol(esc, &next_char, 10);
        
        if (next_char == esc) {
            // No number was parsed.
            if (endp) *endp = esc;
            return '\0';
        }

        val = (char)num;

        if (unterminated) {
            if (endp) *endp = next_char;
        } else {
            // If it is not unterminated, we must assume that it ends at the semicolon ';'
            if (endp) {
                *endp = (*next_char == ';') ? next_char + 1 : next_char;
            }
        }
    }
    // 2. HANDLING NAMED ESCAPES (e.g. &amp;, &lt;)
    else {
        if (endp) *endp = NULL; // Default if we don't find a match
		int cnt;
        for (cnt = 0; AmpEscapes[cnt].tag != NULL; cnt++) {
            const char *tag = AmpEscapes[cnt].tag;
            size_t tag_len = strlen(tag);

            // Use strncmp to check if the tag matches the start of our esc string
            if (strncmp(esc, tag, tag_len) == 0) {
                val = AmpEscapes[cnt].value;
                
                if (endp) {
                    const char *match_end = esc + tag_len;
                    // If it is properly terminated, we also skip past the semicolon
                    if (!unterminated && *match_end == ';') {
                        *endp = match_end + 1;
                    } else {
                        *endp = match_end;
                    }
                }
                break; // Match found, break the loop
            }
        }

#ifdef VERBOSE
        if (val == '\0') {
            fprintf(stderr, "Error bad & string\n");
        }
#endif
    }

    return val;
}


/*
 * Clean the special HTML character escapes out of the text and replace
 * them with the appropriate characters "&lt;" = "<", "&gt;" = ">",
 * "&amp;" = "&"
 * GAG:  apperantly &lt etc. can be left unterminated, what a nightmare.
 * Ok, better, they have to be terminated with white-space or ';'.
 * The '&' character must be immediately followed by a letter to be
 * a valid escape sequence.  Other &'s are left alone.
 * The cleaning is done by rearranging chars in the passed txt buffer.
 * If any escapes are replaced, the string becomes shorter.
 */

void clean_text(char *txt) {
    if (txt == NULL || *txt == '\0') {
        return;
    }

    // 1. Quick scan after the first escape sequence
    char *src = txt;
    while (*src != '\0') {
        if (*src == '&' && (isalpha((unsigned char)*(src + 1)) || *(src + 1) == '#')) {
            break;
        }
        src++;
    }
    
    if (*src == '\0') {
        return; // No escapes found, return immediately
    }

    // 2. Processing loop (src reads, dst writes in-place)
    char *dst = src;
    
    while (*src != '\0') {
        if (*src == '&' && (isalpha((unsigned char)*(src + 1)) || *(src + 1) == '#')) {
            const char *tend = NULL;
            
            // We fast forward to see how the sequence is terminated
            // (same rule as the original ExpandEscapes code)
            const char *check = src + 1;
            while (*check != ';' && !isspace((unsigned char)*check) && *check != '\0') {
                check++;
            }
            
            int unterminated = (*check == '\0') ? 1 : 0;
            int space_terminated = (isspace((unsigned char)*check)) ? 1 : 0;
            int termination_flags = (space_terminated << 1) + unterminated;

            // Call our new, safe ExpandEscapes without destructive string manipulation!
            char val = ExpandEscapes(src, &tend, termination_flags);

            if (val != '\0' && tend != NULL) {
                *dst++ = val;
                src = (char *)tend; // Fast forward src directly to after the escape sequence
                continue;
            }
        }

        // If it wasn't a valid escape, we just copy the character raw and move on.
        *dst++ = *src++;
    }
    
    *dst = '\0'; // Close the string properly
}



/*
 * Get a block of text from a HTML document.
 * All text from start to the end, or the first mark
 * (a mark is '<' or '</' followed by any letter or a '!')
 * is returned in a malloced buffer.  Also, endp returns
 * a pointer to the next '<' or '\0'
 * The returned text has already expanded '&' escapes.
 */

// Helper function to check if a position starts an HTML tag/comment
static inline int is_html_mark(const char *p) {
    if (*p != '<') return 0;
    
    char next = *(p + 1);
    if (isalpha((unsigned char)next) || next == '!') return 1;
    
    if (next == '/' && isalpha((unsigned char)*(p + 2))) return 1;
    
    return 0;
}

char *get_text(const char *start, const char **endp) {
    if (start == NULL || endp == NULL) {
        return NULL;
    }

    const char *ptr = start;
    
    // Find the end of the text block
    while (*ptr != '\0' && !is_html_mark(ptr)) {
        ptr++;
    }
    
    *endp = ptr;

    // Return NULL if no text was found
    size_t length = ptr - start;
    if (length == 0) {
        return NULL;
    }

    // Allocate exactly the amount of memory to be used (+1 to '\0')
    char *text = malloc(length + 1);
    if (text == NULL) {
        return NULL; // Handle memory allocation errors
    }

    // Copy text securely without changing the source string
    memcpy(text, start, length);
    text[length] = '\0';

    // Clean the text (e.g. &amp; -> &)
    clean_text(text);

    return text;
}

 

/*
 * TagClose
 *
 * The '>' that ends the tag whose text starts at p (or the end of the
 * string). A '>' inside a quoted attribute value does not end the tag:
 * pages such as CNN's carry whole pieces of HTML in attributes
 * (data-source-html="<span>...</span>"), and stopping at the first '>'
 * spilled the rest of the attribute onto the page as text. A quote only
 * starts a value right after '=' (spaces allowed), so a stray apostrophe
 * elsewhere in a tag does no harm. A quote that is never closed is
 * ignored, and the first '>' ends the tag as before.
 */
static const char *
TagClose (const char *p)
{
  const char *q;
  char prev = '\0';

  for (q = p;  *q != '\0'  &&  *q != '>';  q++)
  {
    if ((*q == '"'  ||  *q == '\'')  &&  prev == '=')
    {
      const char *close = strchr (q + 1, *q);

      if (close == NULL)	/* unterminated: old behaviour */
      {
	while (*p != '\0'  &&  *p != '>')  p++;
	return (p);
      }
      q = close;
      prev = '"';
      continue;
    }
    if (!isspace ((unsigned char) *q))  prev = *q;
  }
  return (q);
}

/*
 * Get the mark text between '<' and '>'.  From the text, determine
 * its type, and fill in a mark_up structure to return.  Also returns
 * endp pointing to the trailing '>' in the original string.
 */
 

struct mark_up *get_mark(const char *start, const char **endp) {
    if (start == NULL || endp == NULL || *start != '<') {
        return NULL;
    }

    const char *ptr = start + 1;

    /* 1. COMMENT HANDLING (<! or <!--) */
    if (*ptr == '!') {
        ptr++;
#ifndef SGML_COMMENTS
        if (*ptr == '-' && ptr[1] == '-') {
            for (ptr += 2; *ptr != '\0'; ++ptr) {
                if (*ptr == '-' && ptr[1] == '-' && ptr[2] == '>') {
                    *endp = ptr + 2;
                    return NULL;
                }
            }
            ptr = start + 2;
        }
        while (*ptr != '>' && *ptr != '\0') ptr++;
#else
        {
            int is_comment = 0;
            while ((is_comment || (*ptr != '>')) && (*ptr != '\0')) {
                if ((*ptr == '-') && (*++ptr == '-')) {
                    is_comment = 1 - is_comment;
                }
                ptr++;
            }
        }
#endif
        *endp = ptr;
        return NULL;
    }

    int is_script_or_style = 0;

    /* 2. SCRIPT AND STYLE SPECIAL HANDLING */
    if ((*ptr == 's' || *ptr == 'S') && caseless_equal(ptr, "script")) {
        int is_comment = 1;
        is_script_or_style = 1;
        ptr += 7;
        while ((is_comment || (*ptr != '>')) && (*ptr != '\0')) {
            if ((*ptr == '<') && (*++ptr == '/') && (caseless_equal(++ptr, "script"))) {
                ptr += 6;
                if (*ptr == '>') break;
                else if (isspace((unsigned char)*ptr)) {
                    is_comment = 0;
                    ptr++;
                }
            } else ptr++;
        }
    }
    else if ((*ptr == 's' || *ptr == 'S') && caseless_equal(ptr, "style")) {
        int is_comment = 1;
        is_script_or_style = 1;
        ptr += 6;
        while ((is_comment || (*ptr != '>')) && (*ptr != '\0')) {
            if ((*ptr == '<') && (*++ptr == '/') && (caseless_equal(++ptr, "style"))) {
                ptr += 5;
                if (*ptr == '>') break;
                else if (isspace((unsigned char)*ptr)) {
                    is_comment = 0;
                    ptr++;
                }
            } else ptr++;
        }
    }
    else {
        ptr = TagClose(ptr);
    }
    
    *endp = ptr;

    if (*ptr != '>') {
        return NULL;
    }

    // Allocate the structure via standard malloc
    struct mark_up *mark = malloc(sizeof(struct mark_up));
    if (mark == NULL) return NULL;

    /* 3. BUILDING A MARK OBJECT */
    if (is_script_or_style) {
        mark->is_end = 0;
        mark->type = ParseMarkType(""); 
        mark->start = strdup("");   // Avoids null-pointer crashes
        mark->text = NULL;
        mark->end = NULL;
        mark->next = NULL;
        return mark;
    }

    // Measure the amount of tag content (skip leading '<')
    const char *tag_start = start + 1;
    size_t length = ptr - tag_start;

    // Make a clean copy without modifying the source string
    char *text = malloc(length + 1);
    if (text == NULL) {
        free(mark);
        return NULL;
    }
    memcpy(text, tag_start, length);
    text[length] = '\0';

    if (*text == '/') {
        // Closing tag (e.g. </p>)
        mark->is_end = 1;
        mark->type = ParseMarkType(text + 1);
        mark->start = NULL;
        mark->text = NULL;
        mark->end = text;
    } else {
        // Opening tag (e.g. <a href="...">)
        clean_text(text);
        mark->is_end = 0;
        mark->type = ParseMarkType(text);
        mark->start = text;
        mark->text = NULL;
        mark->end = NULL;
    }
    mark->text = NULL;
    mark->next = NULL;

    return mark;
}




/*
 * Special version of get_text.  It reads all text up to the
 * end of the plain text mark, or the end of the file.
 */

// We reuse the is_html_mark logic from earlier to make the code readable

char *get_plain_text(const char *start, const char **endp) {
    if (start == NULL || endp == NULL) {
        return NULL;
    }

    const char *ptr = start;

    while (*ptr != '\0') {
       // Quickly check if we are at the start of an HTML tag
        if (is_html_mark(ptr)) {
            const char *ep;
            
            //get_mark needs to be updated to take (const char*, const char**)
            struct mark_up *mp = get_mark(ptr, &ep);
            
            if (mp != NULL) {
                // Check if it is the specific end tag we are looking for (</XMP> or </LISTING>)
                if ((mp->type == M_PLAIN_TEXT || mp->type == M_LISTING_TEXT) && mp->is_end) {
                    // We found our endpoint. Release the field structure and break out.
                    free(mp->end);
                    free(mp);
                    break;
                }
                
                // It was another roof. Release it and continue the search.
                free(mp->start);
                free(mp->end);
                free(mp);
            }
        }
        ptr++;
    }
    
    *endp = ptr;

    // Return NULL if there was no text at all
    size_t length = ptr - start;
    if (length == 0) {
        return NULL;
    }

    // Allocate memory safely without modifying the source string
    char *text = malloc(length + 1);
    if (text == NULL) {
        return NULL;
    }

    memcpy(text, start, length);
    text[length] = '\0';

    // Clean the text of HTML escapes
    clean_text(text);

    return text;
}


/*
 * Main parser of HTML text.  Takes raw text, and produces a linked
 * list of mark objects.  Mark objects are either text strings, or
 * starting and ending mark delimiters.
 * The old list is passed in so it can be freed, and in the future we
 * may want to add code to append to the old list.
 */

struct mark_up *HTMLParse(struct mark_up *old_list, const char *str) {
    int preformat = 0;
    const char *start = str;
    const char *end = str;
    char *text = NULL;
    struct mark_up *mark = NULL;
    struct mark_up *list = NULL;
    struct mark_up *current = NULL;

#ifdef TIMING
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    fprintf(stderr, "HTMLParse enter (%ld.%d)\n", (long)tv.tv_sec, (int)tv.tv_usec);
#endif

    // Free the previous list immediately
    FreeObjList(old_list);

    if (str == NULL) {
        return NULL;
    }

    while (*start != '\0') {
        text = NULL;

        /*
         * 1. HANDLING TEXT
         */
        if (mark != NULL && !mark->is_end && mark->type == M_PLAIN_FILE) {
            // At PLAINTEXT we swallow the rest of the document raw.
            end = start;
            while (*end != '\0') {
                end++;
            }
            
            // Standard C replacement for XtNewString
            text = strdup(start); 
        }
        else if (mark != NULL && !mark->is_end && 
                (mark->type == M_PLAIN_TEXT || mark->type == M_LISTING_TEXT)) {
            // If you update get_plain_text, its signature should also take (const char*, const char**)
            text = get_plain_text(start, &end);
        }
        else {
            text = get_text(start, &end);
        }

        /*
         * 2. VALIDATION AND ADDING TEXT
         */
        if (text == NULL) {
            if (start != end) {
#ifdef VERBOSE
                fprintf(stderr, "error parsing text, bailing out\n");
#endif
                return list;
            }
        } else {
            // Default malloc instead of XtMalloc
            mark = malloc(sizeof(struct mark_up));
            if (mark == NULL) return list; // Memory failure protection

            mark->type = M_NONE;
            mark->is_end = 0;
            mark->start = NULL;
            mark->text = text;
            mark->end = NULL;
            mark->next = NULL;
            current = AddObj(&list, current, mark, preformat);
        }

        start = end;
        if (*start == '\0') {
            break;
        }

        /*
         * 3. HANDLING HTML TAGS
         */
        // get_mark must also be updated to (const char*, const char**)
        mark = get_mark(start, &end);
        
        if (mark == NULL) {
            if (start != end && start[1] != '!') {
#ifdef VERBOSE
                fprintf(stderr, "error parsing mark, bailing out\n");
#endif
                return list;
            }
            start = end + 1;
        } else {
            start = end + 1;
            mark->next = NULL;
            current = AddObj(&list, current, mark, preformat);

            // Ignore line breaks directly after specific tags (<PLAINTEXT>, <XMP>, <LISTING>, <PRE>)
            if ((mark->type == M_PLAIN_FILE || mark->type == M_PLAIN_TEXT || mark->type == M_LISTING_TEXT) && !mark->is_end) {
                if (*start == '\n') {
                    start++;
                }
            } else if (mark->type == M_PREFORMAT) {
                if (mark->is_end) {
                    preformat = 0;
                } else {
                    preformat = 1;
                    if (*start == '\n') {
                        start++;
                    }
                }
            }
        }
    }

#ifdef TIMING
    gettimeofday(&tv, &tz);
    fprintf(stderr, "HTMLParse exit (%ld.%d)\n", (long)tv.tv_sec, (int)tv.tv_usec);
#endif

    return list;
}


/*
 * Determine mark type from the identifying string passed
 */
int
ParseMarkType (char *str)
{
  int type;

  if (str == NULL)
  {
    return (M_NONE);
  }

  /*
   * On the reasonable assumptions that our MT_* strings don't have
   * NUL or space characters in them, and that the caller stripped leading
   * spaces (not strictly necessary), this next chunk of code is useless.
   */
/*#if 0
  tptr = str;
  while (*tptr != '\0')
  {
    if (isspace ((int) *tptr))
    {
      break;
    }
    tptr++;
  }
  tchar = *tptr;
  *tptr = '\0';
#endif*/

  if (caseless_equal (str, MT_ANCHOR))
  {
    type = M_ANCHOR;
  }
  else if (caseless_equal (str, MT_TITLE))
  {
    type = M_TITLE;
  }
  else if (caseless_equal (str, MT_FIXED))
  {
    type = M_FIXED;
  }
  else if (caseless_equal (str, MT_BOLD))
  {
    type = M_BOLD;
  }
  else if (caseless_equal (str, MT_ITALIC))
  {
    type = M_ITALIC;
  }
  else if (caseless_equal (str, MT_EMPHASIZED))
  {
    type = M_EMPHASIZED;
  }
  else if (caseless_equal (str, MT_STRONG))
  {
    type = M_STRONG;
  }
  else if (caseless_equal (str, MT_CODE))
  {
    type = M_CODE;
  }
  else if (caseless_equal (str, MT_SAMPLE))
  {
    type = M_SAMPLE;
  }
  else if (caseless_equal (str, MT_KEYBOARD))
  {
    type = M_KEYBOARD;
  }
  else if (caseless_equal (str, MT_VARIABLE))
  {
    type = M_VARIABLE;
  }
  else if (caseless_equal (str, MT_CITATION))
  {
    type = M_CITATION;
  }
  else if (caseless_equal (str, MT_STRIKEOUT))
  {
    type = M_STRIKEOUT;
  }
  else if (caseless_equal (str, MT_HEADER_1))
  {
    type = M_HEADER_1;
  }
  else if (caseless_equal (str, MT_HEADER_2))
  {
    type = M_HEADER_2;
  }
  else if (caseless_equal (str, MT_HEADER_3))
  {
    type = M_HEADER_3;
  }
  else if (caseless_equal (str, MT_HEADER_4))
  {
    type = M_HEADER_4;
  }
  else if (caseless_equal (str, MT_HEADER_5))
  {
    type = M_HEADER_5;
  }
  else if (caseless_equal (str, MT_HEADER_6))
  {
    type = M_HEADER_6;
  }
  else if (caseless_equal (str, MT_ADDRESS))
  {
    type = M_ADDRESS;
  }
  else if (caseless_equal (str, MT_PLAIN_TEXT))
  {
    type = M_PLAIN_TEXT;
  }
  else if (caseless_equal (str, MT_LISTING_TEXT))
  {
    type = M_LISTING_TEXT;
  }
  else if (caseless_equal (str, MT_PLAIN_FILE))
  {
    type = M_PLAIN_FILE;
  }
  else if (caseless_equal (str, MT_PARAGRAPH))
  {
    type = M_PARAGRAPH;
  }
  else if (caseless_equal (str, MT_UNUM_LIST))
  {
    type = M_UNUM_LIST;
  }
  else if (caseless_equal (str, MT_NUM_LIST))
  {
    type = M_NUM_LIST;
  }
  else if (caseless_equal (str, MT_MENU))
  {
    type = M_MENU;
  }
  else if (caseless_equal (str, MT_DIRECTORY))
  {
    type = M_DIRECTORY;
  }
  else if (caseless_equal (str, MT_LIST_ITEM))
  {
    type = M_LIST_ITEM;
  }
  else if (caseless_equal (str, MT_DESC_LIST))
  {
    type = M_DESC_LIST;
  }
  else if (caseless_equal (str, MT_DESC_TITLE))
  {
    type = M_DESC_TITLE;
  }
  else if (caseless_equal (str, MT_DESC_TEXT))
  {
    type = M_DESC_TEXT;
  }
  else if (caseless_equal (str, MT_PREFORMAT))
  {
    type = M_PREFORMAT;
  }
  else if (caseless_equal (str, MT_BLOCKQUOTE))
  {
    type = M_BLOCKQUOTE;
  }
  else if (caseless_equal (str, MT_INDEX))
  {
    type = M_INDEX;
  }
  else if (caseless_equal (str, MT_HRULE))
  {
    type = M_HRULE;
  }
  else if (caseless_equal (str, MT_BASE))
  {
    type = M_BASE;
  }
  else if (caseless_equal (str, MT_LINEBREAK))
  {
    type = M_LINEBREAK;
  }
  else if (caseless_equal (str, MT_IMAGE))
  {
    type = M_IMAGE;
  }
  else if (caseless_equal (str, MT_SELECT))
  {
    type = M_SELECT;
  }
  else if (caseless_equal (str, MT_OPTION))
  {
    type = M_OPTION;
  }
  else if (caseless_equal (str, MT_INPUT))
  {
    type = M_INPUT;
  }
  else if (caseless_equal (str, MT_TEXTAREA))
  {
    type = M_TEXTAREA;
  }
  else if (caseless_equal (str, MT_FORM))
  {
    type = M_FORM;
  }
  else if (caseless_equal (str, MT_TABLE))
    {
    type = M_TABLE;
    }
  else if (caseless_equal (str, MT_TABLE_ITEM))
    {
    type = M_TABLE_ITEM;
    }
  else if (caseless_equal (str, MT_TABLE_HEADER))
    {
    type = M_TABLE_HEADER;
    }
  else if (caseless_equal (str, MT_TABLE_ROW))
    {
    type = M_TABLE_ROW;
    }
  else if (caseless_equal (str, MT_CAPTION))
  {
    type = M_CAPTION;
  }
  /* added this - djhjr - May 9 97 */
  else if (caseless_equal (str, MT_BODY))
  {
    type = M_BODY;
  }
  else
  {
#ifdef VERBOSE
    fprintf (stderr, "warning: unknown mark (%s)\n", str);
#endif
    type = M_UNKNOWN;
  }

/*  *tptr = tchar;*/

  return (type);
}

/*
 * Parse a single anchor tag.  ptrp is a pointer to a pointer to the
 * string to be parsed.  On return, the ptr should be changed to
 * point to after the text we have parsed.
 * On return start and end should point to the beginning, and just
 * after the end of the tag's name in the original anchor string.
 * Finally the function returns the tag value in a malloced buffer.
 */

char *AnchorTag (const char **ptrp, const char **startp, const char **endp)
{
  char *tag_val;
  const char *ptr;
  const char *start;
  int quoted;
  int has_value;
  int len; // New variable to hold the length

  quoted = 0;

  /*
   * remove leading spaces, and set start
   */
  ptr = *ptrp;
  while (isspace ((int) *ptr))
  {
    ptr++;
  }
  *startp = ptr;

  /*
   * Find and set the end of the tag
   */
  while ((!isspace ((int) *ptr)) && (*ptr != '=') && (*ptr != '\0'))
  {
    ptr++;
  }
  *endp = ptr;

  if (*ptr == '\0')
  {
    *ptrp = ptr;
    return (NULL);
  }

  /*
   * Move to the start of the tag value, if there is one.
   * set the has_value flag.
   */
  has_value = 0;
  while ((isspace ((int) *ptr)) || (*ptr == '='))
  {
    if (*ptr == '=')
    {
      has_value = 1;
    }
    ptr++;
  }

  /*
   * For a tag with no value, this is a boolean flag.
   * Return the string "1" so we know the tag is there.
   */
  if (!has_value)
  {
    *ptrp = *endp;
    return (XtNewString("1")); 
  }

  if (*ptr == '\"' || *ptr == '\'') 
    quoted = *ptr++;

  start = ptr;
  
  if (quoted)
  {
    while ((*ptr != quoted) && (*ptr != '\0'))
    {
      ptr++;
    }
  }
  else
  {
    while ((!isspace ((int) *ptr)) && (*ptr != '\0'))
    {
      ptr++;
    }
  }
  if ((quoted) && (*ptr == '\0'))
  {
    *ptrp = ptr;
    return (NULL);
  }

  /*
   * OPTIMIZATION: Calculate the length in advance and use memcpy instead of strncat
   */
  len = ptr - start;
  tag_val = XtMalloc (len + 1);
  
  if (len > 0)
  {
    memcpy (tag_val, start, len);
  }
  tag_val[len] = '\0'; // Insert the trailing zero character manually and quickly

  if (quoted) ptr++;
  *ptrp = ptr;

  return (tag_val);
}


/*
 * Parse mark text for the value associated with the
 * passed mark tag.
 * If the passed tag is not found, return NULL.
 * If the passed tag is found but has no value, return "".
 */
 
extern char *strcasestr(const char *haystack, const char *needle);

char *
ParseMarkTag (const char *text, const char *mtext, const char *mtag)
{
  const char *ptr;

  if ((text == NULL) || (mtext == NULL) || (mtag == NULL))
  {
    return (NULL);
  }

  /*
   * QUICK AND PRECISE SHORTCUT: 
   * If the entire attribute name (e.g. "href" or "width") is not 
   * found in the tag text at all (regardless of case), we reject it immediately!
   */
  if (*mtag != '\0')
  {
    if (strcasestr(text, mtag) == NULL)
    {
      return (NULL); /* Skip - the attribute definitely does not exist in this tag */
    }
  }

  /* Skip over leading mark tag characters */
  if (*text == '+')  ++text;
  if (*text == '/')  ++text;
  ptr = (char *) (text + strlen (mtext));

  while (*ptr != '\0')
  {
    const char *start, *end;

    char *tag_val = AnchorTag (&ptr, &start, &end);

    if (caseless_equal (start, mtag))  /* found! */
    {
      return (tag_val != NULL) ?  tag_val  :  XtNewString("");
    }
    if (tag_val != NULL)  XtFree (tag_val);
  }

  return (NULL);
}



