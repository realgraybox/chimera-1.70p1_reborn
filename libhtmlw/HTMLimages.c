/*
 * MODIFIED VERSION. This file is part of the HTML widget of NCSA Mosaic
 * for the X Window System 2.x, as modified for Chimera 1.70 (1997) and
 * for Chimera 1.70p1 reborn (2026,
 * https://github.com/realgraybox/chimera-1.70p1_reborn). It is not the
 * original NCSA Mosaic distributed by the University of Illinois.
 */
/* substantially modified by WBE, Spring 1997, and by others before me */

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

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "HTMLP.h"
#include "NoImage.xbm"
#include "DelayedImage.xbm"
#include "AnchoredImage.xbm"

#define IMAGE_BORDER	2

ImageInfo no_image;
ImageInfo delayed_image;
ImageInfo anchored_image;

static int allocation_index[256];

/* A very simple error handler that does absolutely nothing */
static int SilenceXErrors(Display *dsp, XErrorEvent *err) {
    return 0;
}

void
FreeColors(Display *dsp, Colormap colormap)
{
    int i, j;
    unsigned long pix;
    int (*old_handler)(Display *, XErrorEvent *);

    if (!dsp || colormap == None)
        return;

    /* 1. Force out all pending commands and install "noise suppressor" */
    XSync(dsp, False);
    old_handler = XSetErrorHandler(SilenceXErrors);

    for (i = 0; i < 256; i++)
    {
        if (allocation_index[i] > 0)
        {
            pix = (unsigned long) i;
            /* We run the j-loop, but the X server will now just ignore 
			if we count too high */
            for (j = 0; j < allocation_index[i]; j++)
            {
                XFreeColors(dsp, colormap, &pix, 1, 0L);
            }
        }
        allocation_index[i] = 0;
    }

    /* 2. VERY IMPORTANT: Force the errors through NOW while SilenceXErrors is still active */
    XSync(dsp, False);

    /* 3. Restore the original handler */
    XSetErrorHandler(old_handler);
}


/*
 * Find the best matching color in the default colormap.
 * Allocates the color if possible; otherwise finds the closest
 * already-allocated color.
 */
void
FindColor(Display *dsp, Colormap colormap, XColor *colr)
{
    int i, match;
    int rd, gd, bd, dist, mindist;
    int cindx;
    static XColor def_colrs[256];
    static int have_colors = 0;
    int NumCells;

    /* Guard: null pointers */
    if (!dsp || !colr)
        return;

    /* Guard: invalid colormap */
    if (colormap == None)
        return;

    NumCells = XDisplayCells(dsp, DefaultScreen(dsp));
    if (NumCells <= 0)      /* Guard: non-positive cell count */
        return;

    if (NumCells <= 2)
    {
        colr->pixel = (colr->red + colr->green + colr->blue > 98304) ?
                      WhitePixel(dsp, DefaultScreen(dsp)) :
                      BlackPixel(dsp, DefaultScreen(dsp));
        XQueryColor(dsp, colormap, colr);
        return;
    }

    match = XAllocColor(dsp, colormap, colr);
    if (match == 0)
    {
        if (!have_colors)
        {
            /* Guard: NumCells may exceed def_colrs size */
            int n = (NumCells > 256) ? 256 : NumCells;
            for (i = 0; i < n; i++)
                def_colrs[i].pixel = i;
            XQueryColors(dsp, colormap, def_colrs, n);
            have_colors = 1;
        }

        mindist = 196608;       /* 256 * 256 * 3 */
        cindx = colr->pixel;
        int n = (NumCells > 256) ? 256 : NumCells;   /* bounded scope */
        for (i = 0; i < n; i++)
        {
            rd = ((int)(def_colrs[i].red >> 8) -
                  (int)(colr->red >> 8));
            gd = ((int)(def_colrs[i].green >> 8) -
                  (int)(colr->green >> 8));
            bd = ((int)(def_colrs[i].blue >> 8) -
                  (int)(colr->blue >> 8));
            dist = rd * rd + gd * gd + bd * bd;
            if (dist < mindist)
            {
                mindist = dist;
                cindx = def_colrs[i].pixel;
                if (dist == 0)
                    break;
            }
        }
        colr->pixel = cindx;
        colr->red   = def_colrs[cindx].red;
        colr->green = def_colrs[cindx].green;
        colr->blue  = def_colrs[cindx].blue;
    }
    else
    {
        /*
         * Keep a count of how many times we have allocated the
         * same color, so we can properly free them later.
         * Guard: ensure pixel index stays within allocation_index bounds.
         */
        int idx = (colr->pixel < 256) ? colr->pixel : 255;
        allocation_index[idx]++;

        /*
         * If this is a new color, we've changed the default colormap,
         * so we must re-query it later.
         */
        if (allocation_index[idx] == 1)
            have_colors = 0;
    }
}


static int highbit(unsigned long ul) {
    if (ul == 0) return -1;
    int pos = 0;
    while (ul >>= 1) pos++;
    return pos;
}


/*
 * Make an image of appropriate depth for display from image data.
 */
XImage *
MakeImage(Display *dsp, unsigned char *data, int width, int height,
          int depth, const ImageInfo *img_info)
{
    int shiftnum, shiftstart, shiftstop, shiftinc;
    int bpp;
    int temp;
    int w, h;
    XImage *newimage;
    unsigned char *bit_data, *bitp, *datap, *endofline;
    unsigned long c;
    int rshift, gshift, bshift;
    int rmask, gmask, bmask;
    Visual *theVisual;
    size_t alloc_size;

    /* Guard: null pointers and sensible dimensions */
    if (!dsp || !data || width <= 0 || height <= 0 || depth <= 0)
        return NULL;

    theVisual = DefaultVisual(dsp, DefaultScreen(dsp));

    /* Create XImage with worst‑case bitmap_pad = 32 */
    newimage = XCreateImage(dsp, theVisual, depth, ZPixmap, 0,
                            (char *) NULL, width, height, 32, 0);
    if (!newimage)
        return NULL;

    /* Compute allocation size with overflow check */
    alloc_size = (size_t)newimage->bytes_per_line * (size_t)height;
    if (alloc_size / height != (size_t)newimage->bytes_per_line) {
        XDestroyImage(newimage);
        return NULL;
    }

    bit_data = (unsigned char *) XtMalloc(alloc_size);
    if (!bit_data) {
        XDestroyImage(newimage);
        return NULL;
    }
    newimage->data = (char *) bit_data;

    bpp = newimage->bits_per_pixel;   /* may differ from depth */

    switch (bpp) {
    case 1:
    case 2:
    case 4:
        /* Guard: bpp <= 8 assumed; avoid division by zero */
        if (bpp <= 0 || bpp > 8) {
            XtFree((char *)bit_data);
            newimage->data = NULL;
            XDestroyImage(newimage);
            return NULL;
        }
        if (newimage->bitmap_bit_order == LSBFirst) {
            shiftstart = 0;
            shiftstop = 8;
            shiftinc = bpp;
        } else {
            shiftstart = 8 - bpp;
            shiftstop = -bpp;
            shiftinc = -bpp;
        }
        datap = data;
        bitp = bit_data;
        for (h = 0; h < height; h++) {
            endofline = bitp + newimage->bytes_per_line;
            temp = 0;
            shiftnum = shiftstart;
            for (w = 0; w < width; w++) {
                temp |= (*datap++) << shiftnum;
                shiftnum += shiftinc;
                if (shiftnum == shiftstop) {
                    *bitp++ = (unsigned char) temp;
                    temp = 0;
                    shiftnum = shiftstart;
                }
            }
            if (bitp != endofline) {
                *bitp++ = (unsigned char) temp;
                while (bitp != endofline)
                    *bitp++ = 0;
            }
        }
        break;

    case 8:
        if (newimage->bytes_per_line == width) {
            memcpy(bit_data, data, (size_t)width * (size_t)height);
        } else {
            datap = data;
            bitp = bit_data;
            for (h = 0; h < height; h++) {
                memcpy(bitp, datap, width);
                datap += width;
                bitp += newimage->bytes_per_line;
            }
        }
        break;

    case 16:
        /* Guard: img_info and visual masks needed */
        if (!img_info) {
            XtFree((char *)bit_data);
            newimage->data = NULL;
            XDestroyImage(newimage);
            return NULL;
        }
        rmask = theVisual->red_mask;
        gmask = theVisual->green_mask;
        bmask = theVisual->blue_mask;
        if (rmask == 0 || gmask == 0 || bmask == 0) {
            XtFree((char *)bit_data);
            newimage->data = NULL;
            XDestroyImage(newimage);
            return NULL;
        }
        rshift = 15 - highbit((unsigned long)rmask);
        gshift = 15 - highbit((unsigned long)gmask);
        bshift = 15 - highbit((unsigned long)bmask);

        datap = data;
        bitp = bit_data;
        if (newimage->byte_order == MSBFirst) {
            for (h = 0; h < height; h++) {
                endofline = bitp + newimage->bytes_per_line;
                for (w = width; w > 0; w--) {
                    temp = (int)*datap++;
                    temp = ((img_info->reds[temp] >> rshift) & rmask) |
                           ((img_info->greens[temp] >> gshift) & gmask) |
                           ((img_info->blues[temp] >> bshift) & bmask);
                    *bitp++ = (temp >> 8) & 0xff;
                    *bitp++ = temp & 0xff;
                }
                bitp = endofline;
            }
        } else {
            for (h = 0; h < height; h++) {
                endofline = bitp + newimage->bytes_per_line;
                for (w = width; w > 0; w--) {
                    temp = (int)*datap++;
                    temp = ((img_info->reds[temp] >> rshift) & rmask) |
                           ((img_info->greens[temp] >> gshift) & gmask) |
                           ((img_info->blues[temp] >> bshift) & bmask);
                    *bitp++ = temp & 0xff;
                    *bitp++ = (temp >> 8) & 0xff;
                }
                bitp = endofline;
            }
        }
        break;

    case 32:
        if (!img_info) {
            XtFree((char *)bit_data);
            newimage->data = NULL;
            XDestroyImage(newimage);
            return NULL;
        }
        rshift = highbit((unsigned long)theVisual->red_mask) - 7;
        gshift = highbit((unsigned long)theVisual->green_mask) - 7;
        bshift = highbit((unsigned long)theVisual->blue_mask) - 7;

        datap = data;
        bitp = bit_data;
        for (h = 0; h < height; h++) {
            endofline = bitp + newimage->bytes_per_line;
            for (w = width; w > 0; w--) {
                temp = (int)*datap++;
                c = (((img_info->reds[temp] >> 8) & 0xff) << rshift) |
                    (((img_info->greens[temp] >> 8) & 0xff) << gshift) |
                    (((img_info->blues[temp] >> 8) & 0xff) << bshift);
                if (newimage->byte_order == MSBFirst) {
                    *bitp++ = (unsigned char)((c >> 24) & 0xff);
                    *bitp++ = (unsigned char)((c >> 16) & 0xff);
                    *bitp++ = (unsigned char)((c >> 8) & 0xff);
                    *bitp++ = (unsigned char)(c & 0xff);
                } else {
                    *bitp++ = (unsigned char)(c & 0xff);
                    *bitp++ = (unsigned char)((c >> 8) & 0xff);
                    *bitp++ = (unsigned char)((c >> 16) & 0xff);
                    *bitp++ = (unsigned char)((c >> 24) & 0xff);
                }
            }
            bitp = endofline;
        }
        break;

    default:
        /* unsupported bits_per_pixel */
        XtFree((char *)bit_data);
        newimage->data = NULL;
        XDestroyImage(newimage);
        return NULL;
    }

    return newimage;
}

int
AnchoredHeight (hw)
HTMLWidget hw;
{
  return ((int) (AnchoredImage_height + IMAGE_BORDER));
}


static const char *const ismapformstring = "ISMAP Form";

char *
IsMapForm (hw)
HTMLWidget hw;
{
  return (XtNewString (ismapformstring));
}


int
IsIsMapForm (hw, href)
HTMLWidget hw;
char *href;
{
  return (href != NULL  &&
	  strcmp (href, ismapformstring) == 0);
}


/*
 * Delayed images are anchored if they aren't already.
 * This accomplishes several things:
 * 1) _HTMLInput (which handles mouse button up events) is only called by
 *     ExtendEnd for button up events in an anchor, and _HTMLInput only
 *     processes elements that look like anchors;
 * 2) a standard anchor-style bounding box will be drawn around the delayed
 *     image icon, so that its borders are obvious; and
 * 3) the Anchor Display feature will consider the presence of the mouse
 *     cursor in the delayed image area to be worth highlighting.
 *
 * Clicking on a delayed image will cause the image to be loaded (see
 * _HTMLInput() ).
 *
 * Changed that pseudo-href string to look like a scheme (`protocol') name
 * which is less likely to appear in real HRefs. --GN 1997May14
 */

static const char *const delayedhrefstring = "DelayedImage:";

char *
DelayedHRef (hw)
HTMLWidget hw;
{
  return (XtNewString (delayedhrefstring));
}


int
IsDelayedHRef (hw, href)
HTMLWidget hw;
char *href;
{
  return (href != NULL  &&
	  strcmp (href, delayedhrefstring) == 0);
}

/*
 * two-style function definition avoids conflict in meaning of Boolean on
 * some Unices -- Nelson H F Beebe 1997May09 (not really needed here)
 */
#ifdef __STDC__
ImageInfo *
DelayedImageData (Boolean anchored)
#else
ImageInfo *
DelayedImageData (anchored)
Boolean anchored;
#endif
{
  static Boolean di_inited = False;

  if (di_inited == False)
  {
    anchored_image.delayed = True;
    anchored_image.internal = True;
    anchored_image.fetched = 0;
    anchored_image.width = DelayedImage_width;
    anchored_image.height =
	DelayedImage_height + AnchoredImage_height + IMAGE_BORDER;
    anchored_image.num_colors = 0;
    anchored_image.reds = NULL;
    anchored_image.greens = NULL;
    anchored_image.blues = NULL;
    anchored_image.image_data = NULL;
    anchored_image.image = (Pixmap) NULL;
    anchored_image.name = NULL;
    anchored_image.next = NULL;

    delayed_image.delayed = True;
    delayed_image.internal = True;
    delayed_image.fetched = 0;
    delayed_image.width = DelayedImage_width;
    delayed_image.height = DelayedImage_height;
    delayed_image.num_colors = 0;
    delayed_image.reds = NULL;
    delayed_image.greens = NULL;
    delayed_image.blues = NULL;
    delayed_image.image_data = NULL;
    delayed_image.image = (Pixmap) NULL;
    delayed_image.name = NULL;
    delayed_image.next = NULL;

    di_inited = True;
  }

  return (anchored == True) ?  &anchored_image : &delayed_image;
}


ImageInfo *
NoImageData ()
{
  static Boolean ni_inited = False;

  if (ni_inited == False)
  {
    no_image.delayed = 0;
    no_image.internal = True;
    no_image.fetched = 0;
    no_image.width = NoImage_width;
    no_image.height = NoImage_height;
    no_image.num_colors = 0;
    no_image.reds = NULL;
    no_image.greens = NULL;
    no_image.blues = NULL;
    no_image.image_data = NULL;
    no_image.image = (Pixmap) NULL;
    no_image.name = NULL;
    no_image.next = NULL;

    ni_inited = True;
  }

  return (&no_image);
}


/*
 * Build pixmaps for the internal images
 */
void
BuildInternalImagePixmaps (hw)
HTMLWidget hw;
{
  if (no_image.image == (Pixmap) NULL)
  {
    (void) NoImageData ();	/* make sure noimage has been init'd */
    no_image.image =
	XCreatePixmapFromBitmapData (XtDisplay (hw->html.view),
				     XtWindow (hw->html.view),
				     NoImage_bits,
				     NoImage_width, NoImage_height,
				     hw->html.foreground,
				     hw->core.background_pixel,
				     DefaultDepthOfScreen (XtScreen (hw)));
  }

  if (delayed_image.image == (Pixmap) NULL)
  {
    (void) DelayedImageData (False);  /* make sure delayed_image init'd */
    delayed_image.image =
	XCreatePixmapFromBitmapData (XtDisplay (hw->html.view),
				     XtWindow (hw->html.view),
				     DelayedImage_bits,
				     DelayedImage_width, DelayedImage_height,
				     hw->html.foreground,
				     hw->core.background_pixel,
				     DefaultDepthOfScreen (XtScreen (hw)));
  }

  if (anchored_image.image == (Pixmap) NULL)
  {
    Pixmap pix;

    (void) DelayedImageData (True);  /* make sure anchored_image init'd */
    anchored_image.image =
	XCreatePixmapFromBitmapData (XtDisplay (hw->html.view),
				     XtWindow (hw->html.view),
				     AnchoredImage_bits,
				     AnchoredImage_width, AnchoredImage_height,
				     hw->html.foreground,
				     hw->core.background_pixel,
				     DefaultDepthOfScreen (XtScreen (hw)));
    pix = XCreatePixmap (XtDisplay (hw->html.view),
			 XtWindow (hw->html.view),
			 DelayedImage_width,
			 (DelayedImage_height + AnchoredImage_height +
			    IMAGE_BORDER),
			 DefaultDepthOfScreen (XtScreen (hw)));
    XSetForeground (XtDisplay (hw), hw->html.drawGC,
		    hw->core.background_pixel);
    XFillRectangle (XtDisplay (hw->html.view), pix,
		    hw->html.drawGC, 0, 0,
		    DelayedImage_width,
		    (DelayedImage_height + AnchoredImage_height +
		     IMAGE_BORDER));
    XCopyArea (XtDisplay (hw->html.view),
	       anchored_image.image, pix, hw->html.drawGC,
	       0, 0, AnchoredImage_width, AnchoredImage_height,
	       0, 0);
    XCopyArea (XtDisplay (hw->html.view),
	       delayed_image.image, pix, hw->html.drawGC,
	       0, 0, DelayedImage_width, DelayedImage_height,
	       0, (AnchoredImage_height + IMAGE_BORDER));
    XFreePixmap (XtDisplay (hw->html.view), anchored_image.image);

    anchored_image.image = pix;
  }
}


Pixmap
InfoToImage(HTMLWidget hw, const ImageInfo *img_info)
{
    int i, size;
    int delta, not_right_col, not_last_row;
    Pixmap Img;
    XImage *tmpimage;
    XColor tmpcolr;
    int *Mapping = NULL;
    unsigned char *tmpdata = NULL;
    unsigned char *ptr;
    unsigned char *ptr2;
    int Vclass;
    XVisualInfo vinfo, *vptr = NULL;
    Boolean need_to_dither;
    unsigned long black_pixel =0;
    unsigned long white_pixel =0;

    /* Guard: null pointers */
    if (!hw || !img_info)
        return (Pixmap) NULL;

    /* Guard: sensible dimensions */
    if (img_info->width <= 0 || img_info->height <= 0 || img_info->num_colors <= 0)
        return (Pixmap) NULL;

    /* Guard: null image_data */
    if (!img_info->image_data)
        return (Pixmap) NULL;

    /* Guard: validate display */
    if (!XtDisplay(hw))
        return (Pixmap) NULL;
 /*   static int image_count = 0;    
    fprintf(stderr,
    "InfoToImage %d img=%p image=%lu\n",
    ++image_count, img_info,
    (unsigned long)img_info->image);*/

    /* find the visual class */
    vinfo.visualid = XVisualIDFromVisual(DefaultVisual(XtDisplay(hw),
                                         DefaultScreen(XtDisplay(hw))));
    vptr = XGetVisualInfo(XtDisplay(hw), VisualIDMask, &vinfo, &i);
    if (!vptr)  /* Guard: XGetVisualInfo failed */
        return (Pixmap) NULL;
    
    Vclass = vptr->class;
    if (vptr->depth == 1)
    {
        need_to_dither = True;
        black_pixel = BlackPixel(XtDisplay(hw),
                                 DefaultScreen(XtDisplay(hw)));
        white_pixel = WhitePixel(XtDisplay(hw),
                                 DefaultScreen(XtDisplay(hw)));
    }
    else
    {
        need_to_dither = False;
    }
    XFree((char *) vptr);

    /* Guard: allocation with overflow check */
    if ((size_t)img_info->num_colors > (size_t)-1 / sizeof(int))
{
    if (Mapping) XtFree((char *) Mapping);
    return (Pixmap) NULL;
}
    Mapping = (int *) XtMalloc((size_t)img_info->num_colors * sizeof(int));
    if (!Mapping)
        return (Pixmap) NULL;

    for (i = 0; i < img_info->num_colors; i++)
    {
        tmpcolr.red = img_info->reds[i];
        tmpcolr.green = img_info->greens[i];
        tmpcolr.blue = img_info->blues[i];
        tmpcolr.flags = DoRed | DoGreen | DoBlue;
        if ((Vclass == TrueColor) || (Vclass == DirectColor))
        {
            Mapping[i] = i;
        }
        else if (need_to_dither == True)
        {
            /* Guard: avoid division by zero */
            int divisor = 65504 / 64;
            if (divisor == 0) divisor = 1;
            Mapping[i] = ((tmpcolr.red >> 5) * 11 +
                          (tmpcolr.green >> 5) * 16 +
                          (tmpcolr.blue >> 5) * 5) / divisor;
        }
        else
        {
            FindColor(XtDisplay(hw),
                      DefaultColormapOfScreen(XtScreen(hw)),
                      &tmpcolr);
            Mapping[i] = tmpcolr.pixel;
        }
    }

    /* Special case: For 2 color non-black&white images */
    if ((need_to_dither == True) && (img_info->num_colors == 2))
    {
        if (Mapping[0] < Mapping[1])
        {
            Mapping[0] = 0;
            Mapping[1] = 64;
        }
        else
        {
            Mapping[0] = 64;
            Mapping[1] = 0;
        }
    }

    /* Guard: overflow check for size */
    size = img_info->width * img_info->height;
    if (img_info->width != 0 && (size_t)img_info->width * (size_t)img_info->height / (size_t)img_info->width != (size_t)img_info->height)
{
    XtFree((char *) Mapping);
    return (Pixmap) NULL;
}

    if (size == 0)
    {
        tmpdata = NULL;
    }
    else
    {
        tmpdata = (unsigned char *) XtMalloc((size_t)size);
        if (!tmpdata)
        {
            XtFree((char *) Mapping);
            return (Pixmap) NULL;
        }
    }

    tmpimage = NULL;
    Img = (Pixmap) NULL;

    if (tmpdata != NULL)
    {
        ptr = img_info->image_data;
        ptr2 = tmpdata;

        if (need_to_dither == True)
        {
            int cx, cy;

            /* First pass: simple mapping */
            for (i = 0; i < size; i++)
            {
                ptr2[i] = (unsigned char) Mapping[(int) ptr[i]];
            }

            ptr2 = tmpdata;
            for (cy = 0; cy < img_info->height; cy++)
            {
                for (cx = 0; cx < img_info->width; cx++)
                {
                    /* Guard: ensure ptr2 is within bounds */
                    if ((ptr2 - tmpdata) >= size)
                        break;

                    /*
                     * Assume high numbers are
                     * really negative.
                     */
                    if (*ptr2 > 128)
                        *ptr2 = 0;
                    if (*ptr2 > 64)
                        *ptr2 = 64;

                    /*
                     * Traditional Floyd-Steinberg
                     */
                    if (*ptr2 < 32)
                    {
                        delta = *ptr2;
                        *ptr2 = (unsigned char) black_pixel;
                    }
                    else
                    {
                        delta = *ptr2 - 64;
                        *ptr2 = (unsigned char) white_pixel;
                    }

                    not_right_col = (cx < (img_info->width - 1));
                    not_last_row = (cy < (img_info->height - 1));

                    if (not_right_col)
                    {
                        /* Guard: bounds check for ptr2 + 1 */
                        if ((ptr2 + 1 - tmpdata) < size)
                            *(ptr2 + 1) += delta * 7 >> 4;
                    }

                    if (not_last_row)
                    {
                        /* Guard: bounds check for ptr2 + width */
                        if ((ptr2 + img_info->width - tmpdata) < size)
                            *(ptr2 + img_info->width) += delta * 5 >> 4;
                    }

                    if (not_right_col && not_last_row)
                    {
                        /* Guard: bounds check */
                        if ((ptr2 + img_info->width + 1 - tmpdata) < size)
                            *(ptr2 + img_info->width + 1) += delta >> 4;
                    }

                    if (cx && not_last_row)
                    {
                        /* Guard: bounds check */
                        if ((ptr2 + img_info->width - 1 - tmpdata) < size)
                            *(ptr2 + img_info->width - 1) += delta * 3 >> 4;
                    }
                    ptr2++;
                }
            }
        }
        else
        {
            for (i = 0; i < size; i++)
            {
                *ptr2++ = (unsigned char) Mapping[(int) *ptr];
                ptr++;
            }
        }

        tmpimage = MakeImage(XtDisplay(hw), tmpdata,
                             img_info->width, img_info->height,
                             DefaultDepthOfScreen(XtScreen(hw)), img_info);

        XtFree((char *) tmpdata);
        tmpdata = NULL;

        /* Guard: validate display widget for pixmap creation */
        if (hw->html.view && XtWindow(hw->html.view))
        {
            Img = XCreatePixmap(XtDisplay(hw->html.view),
                                XtWindow(hw->html.view),
                                img_info->width, img_info->height,
                                DefaultDepthOfScreen(XtScreen(hw)));
        }
    }

    /* Cleanup and error handling */
    if (!tmpimage || !Img)
    {
        if (tmpimage)
        {
            if (tmpimage->data)
            {
                XtFree((char *)(tmpimage->data));
                tmpimage->data = NULL;
            }
            XDestroyImage(tmpimage);
            tmpimage = NULL;
        }
        if (Img)
        {
            XFreePixmap(XtDisplay(hw), Img);
            Img = (Pixmap) NULL;
        }
    }
    else
    {
        /* Guard: validate GC exists */
        if (hw->html.drawGC)
        {
            XPutImage(XtDisplay(hw), Img, hw->html.drawGC, tmpimage, 0, 0,
                      0, 0, img_info->width, img_info->height);
        }
        if (tmpimage->data)
        {
            XtFree((char *)(tmpimage->data));
            tmpimage->data = NULL;
        }
        XDestroyImage(tmpimage);
    }

    if (Mapping)
    {
        XtFree((char *) Mapping);
        Mapping = NULL;
    }

    return Img;
}

/*
 * Free the ImageInfo block and the all the resources it owns.
 * Returns the value of pic_data->next as a convenience to the caller.
 */
ImageInfo *
FreeOneImage (hw, pic_data)
HTMLWidget hw;
ImageInfo *pic_data;		/* NULL allowed */
{
  ImageInfo *next;

  /*
   * Don't actually free special built-in images
   * pic_data == NULL is also accepted.
   */
  if (pic_data != NULL  &&
      pic_data->internal == False  &&	 /* the test that ought to work */
      pic_data != &delayed_image  &&	/* paranoia */
      pic_data != &anchored_image &&	/* paranoia */
      pic_data != &no_image)		/* paranoia */
  {
    if (pic_data->image_data != NULL) free_mem ((char *)pic_data->image_data);
    if (pic_data->reds != NULL)       free_mem ((char *)pic_data->reds);
    if (pic_data->greens != NULL)     free_mem ((char *)pic_data->greens);
    if (pic_data->blues != NULL)      free_mem ((char *)pic_data->blues);
    if (pic_data->name != NULL)       free_mem ((char *)pic_data->name);
    if (pic_data->image != (Pixmap) NULL)
	XFreePixmap (XtDisplay (hw), pic_data->image);

    next = pic_data->next;
    free_mem ((char *)pic_data);
  }
  else next = NULL;

  return next;
}


/*
 * Free up all the image data used by the current document.
 */
void
FreeImages (hw)
HTMLWidget hw;
{
  ImageInfo *pic_data = hw->html.image_list;

  while (pic_data != NULL)   pic_data = FreeOneImage (hw, pic_data);

  hw->html.image_list = NULL;
}


/*
 * Search the list of images for one with NAME.
 * Return a pointer to an ImageInfo *, or NULL if not found.
 */
ImageInfo *
IsCurrentImage (list, name)
ImageInfo *list;
const char *const name;
{
  if (list == NULL)
  {
    return (NULL);
  }
  if (name == NULL)
  {
  	return (NULL);
  }

  for ( ;  list != NULL;  list = list->next)
  {
    if (strcmp (list->name, name) == 0)
    {
      return list;
    }
  }

  return NULL;
}


/*
 * Do appropriate things to get the specified image or a suitable substitute.
 * If a new real image is obtained, it is added to the image_list.
 * eptr->img_data is created and initialized, and img_data->pic_data is set
 * to whatever image we're going to display.
 * This code does not generate pic_data->image; ImageRefresh() does that.
 */

/*
 * two-style function definition avoids conflict in meaning of Boolean on
 * some Unices -- Nelson H F Beebe 1997May09
 */
#ifdef __STDC__
void
HTMLGetImage (HTMLWidget hw, struct ele_rec *eptr, Boolean now)
#else
void
HTMLGetImage (hw, eptr, now)
HTMLWidget hw;
struct ele_rec *eptr;
Boolean now;			/* True: force loading now */
#endif
{
  const char *const edata = eptr->edata;
  ImageInfo *pic_data;

  /*
   * Is this image on image_list?
   */
  pic_data = IsCurrentImage (hw->html.image_list, edata);

  if (pic_data == NULL)		/* not on image_list; a new image */
  {
    if (hw->html.resolveImage != NULL)
    {
      /*
       * arg 3:  True = don't load if downloading is required
       *
       * controls arg4 for the resolveImageProc, and arg3
       * passes the appropriate bg color - djhjr - May 12 97
       */
      pic_data = (* (resolveImageProc) (hw->html.resolveImage) )
	         (hw, edata, hw->core.background_pixel,
		  (hw->html.delay_images  &&  !now));

      if (pic_data != NULL)	/* got a real image */
      {
	if (pic_data->image_data != NULL)
	{
	  pic_data->delayed = 0;
	  pic_data->name    = XtNewString (edata);
	  pic_data->fetched = True;

	  /*
	   * Add pic_data to head of image_list.  Adding to the head helps
	   * in the common case where ImagePlace() asks for the image twice
	   * in a row because it didn't fit in the first place tried.
	   */
	  pic_data->next = hw->html.image_list;
	  hw->html.image_list = pic_data;
	}
	else			/* well, *that* was useless */
	{
	  (void) FreeOneImage (hw, pic_data); /* --Frang G Liu 1997May09 */
	  pic_data = NULL;
	}
      }
      /*
       * The inclusion of && !now is optional.  It's arguable whether to
       * give the user another try when delayed image loading fails or
       * leave the user thinking Chimera is broken because it didn't load
       * the image and redisplays the delayed image icon.  I opted for
       * making it clear the load failed.  The user still has other ways
       * to prod Chimera to try again to load the image.
       */
      if (pic_data == NULL  &&	/* don't have an image */
	  hw->html.delay_images  &&  !now)
      {
	/*
	 * Use "delayed image" or "delayed image that's an anchor" box.
	 */
	if (eptr->anchorHRef == NULL)  /* image isn't part of an anchor */
	{
	  pic_data = DelayedImageData (False);
	  /*
	   * Make delayed images look like anchors.
	   * Why?  See DelayedHRef()  (above).
	   */
	  eptr->anchorHRef = DelayedHRef (hw);  /* pseudo-anchor */
	  eptr->fg = hw->html.anchor_fg;  /* just like a real anchor */
	}
	else			/* delayed image that's an anchor */
	{
	  pic_data = DelayedImageData (True);
	}
      }
    }

    if (pic_data == NULL)	/* still nothing?  Give up */
    {
      pic_data = NoImageData ();  /* never returns NULL */
    }
  }

  /*
   * Create the img_data structure to hold any HTML tag qualifiers.
   */
  FreeIMGInfo (eptr->img_data);	 /* free any old img_data; NULL okay */

  eptr->img_data = (IMGInfo *) XtMalloc (sizeof (IMGInfo));
  eptr->img_data->alt = (const char *) NULL;
  eptr->img_data->pic_data = pic_data;
  
  /*
   * Note: never exits with eptr->img_data->pic_data non-NULL
   */
}
