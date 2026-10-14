/*
 * inline.c
 *
 * Copyright (C) 1993, 1994, 1995, John D. Kilburg (john@cs.unlv.edu)
 *
 * See copyright.h for details.
 */
#include "copyright.h"
#include "options.h"

#include <stdio.h>
#include <sys/types.h>

#include <X11/Intrinsic.h>

#include "HTML.h"

#include "common.h"
#include "image.h"

#include "url.h"
#include "mime.h"
#include "document.h"
#include "convert.h"
#include "util.h"
#include "inline.h"

#ifdef IMAGENEW
#include <math.h>
#ifdef __UCLIBC__
float sinf (float x) {return (float) sin((double)x); }
float cosf(float x) { return (float) cos((double)x); }
float tanf (float x){return (float) tan ( (double)x);}
float atan2f (float x, float y){return (float) atan2((double)x, (double)y);}
float fmodf (float x, float y){return (float) fmod ((double)x, (double)y);}
float acosf (float x){return (float) acos((double)x );}
#endif

int global_html_width = -1;
int global_html_height = -1;

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_ONLY_PSD
#define STBI_ONLY_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef HAVE_POWF
float powf (float x, float y) { return (float) pow( (double)x, (double)y ); }
#endif

/*
 * rgbaToImage
 *
 * Turns a width x height RGBA buffer into a colormapped image with at
 * most 256 colours.  The buffer is not modified.
 *
 * Translucent pixels are blended against bg.  Fully transparent pixels
 * get a palette entry of their own that holds bg exactly, so they match
 * the page background.
 *
 * An image with 256 colours or fewer keeps its colours unchanged.
 * Anything else goes through reduce() from xloadimage (median cut).
 * reduce() works on 5 bits per channel, so afterwards every palette
 * entry is recomputed as the mean of the 24-bit pixels mapped to it.
 */
#define RGBA_MAXCOLORS 256
#define RGBA_HASHSIZE 1024		/* power of 2, > 2 * RGBA_MAXCOLORS */
#define RGBA_TRANSPARENT 0x1000000UL	/* key no 24-bit colour can have */

static Image *rgbaToImage(unsigned char *rgba, int width, int height, RGBColor *bg) {
	Image *timg, *img;
	unsigned long keys[RGBA_HASHSIZE];	/* colour key + 1, 0 = empty */
	unsigned short slots[RGBA_HASHSIZE];	/* palette index of that key */
	unsigned long sum[RGBA_MAXCOLORS][3];
	unsigned long cnt[RGBA_MAXCOLORS];
	unsigned int bg_r = (bg->red >> 8) & 0xFF;
	unsigned int bg_g = (bg->green >> 8) & 0xFF;
	unsigned int bg_b = (bg->blue >> 8) & 0xFF;
	int total_pixels = width * height;
	int have_transparent = 0;
	int ncolors = 0;
	int exact = 1;
	unsigned char *src, *dst;
	int i, c;

	/* 1. Blend against the background into a true colour image */
	timg = newTrueImage(width, height);
	timg->title = alloc_string("inline");

	src = rgba;
	dst = timg->data;
	for (i = 0; i < total_pixels; i++, src += 4, dst += 3) {
		unsigned int a = src[3];

		if (a == 0) {
			dst[0] = bg_r;
			dst[1] = bg_g;
			dst[2] = bg_b;
			have_transparent = 1;
		} else if (a < 255) {
			dst[0] = (src[0] * a + bg_r * (255 - a)) / 255;
			dst[1] = (src[1] * a + bg_g * (255 - a)) / 255;
			dst[2] = (src[2] * a + bg_b * (255 - a)) / 255;
		} else {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
		}
	}

	/* 2. Try to keep the colours as they are */
	img = newRGBImage(width, height, 8);
	img->title = alloc_string("inline");
	memset(keys, 0, sizeof(keys));

	src = rgba;
	dst = timg->data;
	for (i = 0; i < total_pixels; i++, src += 4, dst += 3) {
		unsigned long key;
		unsigned int h;

		if (src[3] == 0) key = RGBA_TRANSPARENT;
		else key = ((unsigned long)dst[0] << 16) | ((unsigned long)dst[1] << 8) | dst[2];

		h = (unsigned int)((key * 2654435761UL) >> 12) & (RGBA_HASHSIZE - 1);
		while (keys[h] != 0 && keys[h] != key + 1) h = (h + 1) & (RGBA_HASHSIZE - 1);

		if (keys[h] == 0) {
			if (ncolors == RGBA_MAXCOLORS) {
				exact = 0;
				break;
			}
			keys[h] = key + 1;
			slots[h] = ncolors;
			if (key == RGBA_TRANSPARENT) {
				img->rgb.red[ncolors] = bg->red;
				img->rgb.green[ncolors] = bg->green;
				img->rgb.blue[ncolors] = bg->blue;
			} else {
				img->rgb.red[ncolors] = dst[0] * 257;
				img->rgb.green[ncolors] = dst[1] * 257;
				img->rgb.blue[ncolors] = dst[2] * 257;
			}
			ncolors++;
		}
		img->data[i] = (unsigned char)slots[h];
	}

	if (exact) {
		img->rgb.used = ncolors;
		freeImage(timg);
		return(img);
	}
	freeImage(img);

	/* 3. Too many colours: median cut, leaving one entry free for bg */
	img = reduce(timg, have_transparent ? RGBA_MAXCOLORS - 1 : RGBA_MAXCOLORS, 0);
	if (img == timg) return(img);	/* reduce() declined, caller copes with true colour */

	memset(sum, 0, sizeof(sum));
	memset(cnt, 0, sizeof(cnt));
	src = rgba;
	dst = timg->data;
	for (i = 0; i < total_pixels; i++, src += 4, dst += 3) {
		if (src[3] == 0) continue;
		c = img->data[i];
		sum[c][0] += dst[0];
		sum[c][1] += dst[1];
		sum[c][2] += dst[2];
		cnt[c]++;
	}
	for (c = 0; c < (int)img->rgb.used && c < RGBA_MAXCOLORS; c++) {
		if (cnt[c] == 0) continue;
		img->rgb.red[c] = (unsigned short)((sum[c][0] * 257 + cnt[c] / 2) / cnt[c]);
		img->rgb.green[c] = (unsigned short)((sum[c][1] * 257 + cnt[c] / 2) / cnt[c]);
		img->rgb.blue[c] = (unsigned short)((sum[c][2] * 257 + cnt[c] / 2) / cnt[c]);
	}

	if (have_transparent && img->rgb.used < img->rgb.size && img->rgb.used < RGBA_MAXCOLORS) {
		c = img->rgb.used++;
		img->rgb.red[c] = bg->red;
		img->rgb.green[c] = bg->green;
		img->rgb.blue[c] = bg->blue;
		for (i = 0; i < total_pixels; i++) {
			if (rgba[i * 4 + 3] == 0) img->data[i] = (unsigned char)c;
		}
	}

	freeImage(timg);
	return(img);
}

Image *imgLoad(char *data, int datalen, RGBColor *bg, int target_width, int target_height) {
	int width, height, n;
	unsigned char *orig_rgba;
    unsigned char *raw_rgba;
	Image *image;
	int is_scaled = 0;
    
	orig_rgba = stbi_load_from_memory((unsigned char*)data, datalen, &width, &height, &n, 4);
	if(orig_rgba == NULL) {
		printf("ERROR: Couldn't load image: %s!\n", stbi_failure_reason());
		return(NULL);
	}
  
    raw_rgba = orig_rgba;

	// --- scaling logic ---
	const int MAX_WIDTH = 600;

	int new_w = width;
	int new_h = height;

	// Apply HTML requested dimensions while preserving the original image aspect ratio.
	if (target_width > 0 || target_height > 0) {
		if (target_width > 0 && target_height > 0) {
			// Fit image inside requested box
			int w_from_h = (width * target_height) / height;
			int h_from_w = (height * target_width) / width;

			if (w_from_h <= target_width) {
				new_w = w_from_h;
				new_h = target_height;
			} else {
				new_w = target_width;
				new_h = h_from_w;
			}
		} else if (target_width > 0) {
			// Width only
			new_w = target_width;
			new_h = (height * target_width) / width;
		} else { // target_height > 0
			// Height only
			new_h = target_height;
			new_w = (width * target_height) / height;
		}
	}

	// Never upscale.
	if (new_w > width) {
		new_h = (new_h * width) / new_w;
		new_w = width;
	}
	if (new_h > height) {
		new_w = (new_w * height) / new_h;
		new_h = height;
	}

	// MAX_WIDTH always wins.
	if (new_w > MAX_WIDTH) {
		new_h = (new_h * MAX_WIDTH) / new_w;
		new_w = MAX_WIDTH;
	}

	// Safety
	if (new_w < 1) new_w = 1;
	if (new_h < 1) new_h = 1;

	//printf("Final size: %d x %d\n", new_w, new_h);

    //  Nearest Neighbor algorithm
	if (new_w != width || new_h != height) {
		raw_rgba = (unsigned char *)alloc_mem(new_w * new_h * 4);
		int x, y;
		for (y = 0; y < new_h; y++) {
            // Pre-calculate original Y-coordinate with fixed-point math
			int src_y = (y * height) / new_h;
			int dest_row_idx = y * new_w * 4;
			int src_row_idx = src_y * width * 4;

			for (x = 0; x < new_w; x++) {
				int src_x = (x * width) / new_w;
				int dest_pixel = dest_row_idx + (x * 4);
				int src_pixel = src_row_idx + (src_x * 4);

				// copy RGBA direct
				raw_rgba[dest_pixel]     = orig_rgba[src_pixel];
				raw_rgba[dest_pixel + 1] = orig_rgba[src_pixel + 1];
				raw_rgba[dest_pixel + 2] = orig_rgba[src_pixel + 2];
				raw_rgba[dest_pixel + 3] = orig_rgba[src_pixel + 3];
			}
		}
        
		width = new_w;
		height = new_h;
		is_scaled = 1;
		stbi_image_free(orig_rgba);
	}
     
    // 3. Blend against the page background and reduce to 256 colours
	image = rgbaToImage(raw_rgba, width, height, bg);

	if (is_scaled) {
		free_mem(raw_rgba);
	} else {
		stbi_image_free(raw_rgba);
	}

	return(image);
}

Image *svgLoad(char *data, int datalen, RGBColor *bg, int target_width, int target_height) {
    NSVGimage       *svg_img = NULL;
    NSVGrasterizer  *rast = NULL;
    unsigned char   *raw_rgba = NULL;
    Image           *image;
    int             width, height;
    float           scale = 1.0f;
    char            *svg_copy;

    /* nanosvg requires a null-terminated string. We'll make a quick copy. */
    svg_copy = (char *)alloc_mem(datalen + 1);
    memcpy(svg_copy, data, datalen);
    svg_copy[datalen] = '\0';

    /* Parse the SVG source code */
    svg_img = nsvgParse(svg_copy, "px", 96.0f);
    free_mem(svg_copy);

    if (svg_img == NULL) {
        printf("ERROR: Could not parse SVG data!\n");
        return(NULL);
    }

    /* Original dimensions from the SVG file */
    int orig_w = (int)svg_img->width;
    int orig_h = (int)svg_img->height;
    if (orig_w < 1) orig_w = 1;
    if (orig_h < 1) orig_h = 1;
    
    width = orig_w;
    height = orig_h;

    const int MAX_WIDTH = 600;

    if (target_width > 0 || target_height > 0) {
        if (target_width > 0 && target_height > 0) {
            int w_from_h = (orig_w * target_height) / orig_h;
            int h_from_w = (orig_h * target_width) / orig_w;
            if (w_from_h <= target_width) {
                width = w_from_h;
                height = target_height;
            } else {
                width = target_width;
                height = h_from_w;
            }
        } else if (target_width > 0) {
            width = target_width;
            height = (orig_h * target_width) / orig_w;
        } else {
            height = target_height;
            width = (orig_w * target_height) / orig_h;
        }
    }

    /* Avoid upscaling and respect MAX_WIDTH */
    if (width > orig_w) {
        height = (height * orig_w) / width;
        width = orig_w;
    }
    if (height > orig_h) {
        width = (width * orig_h) / height;
        height = orig_h;
    }
    if (width > MAX_WIDTH) {
        height = (height * MAX_WIDTH) / width;
        width = MAX_WIDTH;
    }

    if (width < 1) width = 1;
    if (height < 1) height = 1;

    /* Calculate the exact scaling factor for the vector graphic */
    scale = (float)width / svg_img->width;

    /* Allocate buffer for the raw RGBA pixels */
    raw_rgba = (unsigned char *)alloc_mem(width * height * 4);

    /* Rasterize SVG directly to the desired target size */
    rast = nsvgCreateRasterizer();
    nsvgRasterize(rast, svg_img, 0.0f, 0.0f, scale, raw_rgba, width, height, width * 4);

    /* Free the nanosvg structures immediately – we have our pixels now */
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg_img);

    /* Blend against the page background and reduce to 256 colors */
    image = rgbaToImage(raw_rgba, width, height, bg);

    free_mem(raw_rgba);

    return(image);
}

#endif

extern Image *gifLoad();
extern Image *xbitmapLoad();
extern Image *xpixmapLoad();
extern Image *pbmLoad();
extern void gammacorrect();

// AllocImageInfo
static ImageInfo *AllocImageInfo(void) {
	ImageInfo *iinfo;

    iinfo = (ImageInfo *)alloc_mem(sizeof(ImageInfo));
    if (iinfo == NULL)
        return NULL;

    memset(iinfo, 0, sizeof(ImageInfo));

    return iinfo;
}

// ConvertRGBImageToImageInfo
static ImageInfo *
ConvertRGBImageToImageInfo(Image *img) {
	ImageInfo *iinfo;
	int csize;
	int dsize;

	iinfo = AllocImageInfo();

	iinfo->width = img->width;
	iinfo->height = img->height;

	csize = sizeof(unsigned short) * img->rgb.used;
	iinfo->reds = (unsigned short *)alloc_mem(csize);
	iinfo->greens = (unsigned short *)alloc_mem(csize);
	iinfo->blues = (unsigned short *)alloc_mem(csize);
	iinfo->num_colors = img->rgb.used;
	memcpy(iinfo->reds, img->rgb.red, csize);
	memcpy(iinfo->greens, img->rgb.green, csize);
	memcpy(iinfo->blues, img->rgb.blue, csize);

	dsize = img->width * img->height * sizeof(unsigned char);
	iinfo->image_data = (unsigned char *)alloc_mem(dsize);

	memcpy(iinfo->image_data, img->data, dsize);

	return(iinfo);
}

// ConvertTrueImageToImageInfo
static ImageInfo *ConvertTrueImageToImageInfo(Image *img) {
	ImageInfo *iinfo = NULL;
	int csize;
	int dsize;
	int i, j;
	int n;
	int ccount = 0;
	int maxcount = 256;
	unsigned short xr, xg, xb;
	int k = 0;

	iinfo = AllocImageInfo();

	iinfo->width = img->width;
	iinfo->height = img->height;

	dsize = img->width * img->height * sizeof(unsigned char);
	iinfo->image_data = (unsigned char *)alloc_mem(dsize);

	csize = sizeof(unsigned short) * maxcount;
	iinfo->reds = (unsigned short *)alloc_mem(csize);
	iinfo->greens = (unsigned short *)alloc_mem(csize);
	iinfo->blues = (unsigned short *)alloc_mem(csize);

	n = img->width * img->height * 3;

	for (i = 0; i < n; i += 3) {
		xr = (unsigned short)(img->data[i]) * 0xffff / 0xff;
		xg = (unsigned short)(img->data[i + 1]) * 0xffff / 0xff;
		xb = (unsigned short)(img->data[i + 2]) * 0xffff / 0xff;
		
		for (j = 0; j < ccount; j++)  {
			if (iinfo->reds[j] == xr && iinfo->greens[j] == xg && iinfo->blues[j] == xb) {
				iinfo->image_data[k++] = j;
				break;
			}
		}
		if (j == ccount) {
			if (ccount == maxcount) {
				iinfo->image_data[k++] = 0;
			} else {
				iinfo->reds[j] = xr;
				iinfo->greens[j] = xg;
				iinfo->blues[j] = xb;
				iinfo->image_data[k++] = j;
				ccount++;
			}
		}
	}

	iinfo->num_colors = ccount;

	return(iinfo);
}


// CreateImageInfo - Creates an Image structure from image data.
ImageInfo *CreateImageInfo(Widget w, Document *d, Convert *clist, char *path, int stype, int maxcolors, XColor *bg, double gamma) {
	Image *img=NULL, *nimg;
	RGBColor rgbc;
	Document *x;
	char *content;
	char *data;
	int datalen;
	ImageInfo *iinfo = NULL;	/* unless proven otherwise */

	if (d->ptext != NULL) {
		data = d->ptext;
		datalen = d->plen;
		content = d->pcontent;
	} else {
		data = d->text;
		datalen = d->len;
		content = d->content;
	}

	rgbc.red = bg->red;
	rgbc.green = bg->green;
	rgbc.blue = bg->blue;

	if (strcasecmp(content, "image/gif") == 0) {
#ifdef IMAGENEW  
		img = imgLoad(data, datalen, &rgbc, d->req_width, d->req_height);	//test if internalgif handling can be bypassed
#else
		img = gifLoad(data, datalen, &rgbc);
#endif
	}
#ifdef IMAGENEW
	else if (strcasecmp(content, "image/png") == 0||
		strcasecmp(content, "image/x-ms-bmp") == 0 ||
		strcasecmp(content, "image/jpeg") == 0 ||
		strcasecmp(content, "image/tga") == 0 ||
		strcasecmp(content, "image/vnd.adobe.photoshop") == 0 ||
		strcasecmp(content, "image/x-photoshop") == 0 ||	
		strcasecmp(content, "image/psd") == 0) 
	{
		//printf("DBG content to imgLoad: %s\n", content);
		img = imgLoad(data, datalen, &rgbc, d->req_width, d->req_height);
	}
	else if (strcasecmp(content, "image/svg+xml") == 0)
	{
		img = svgLoad(data, datalen, &rgbc, d->req_width, d->req_height);
	}
#endif      
 	else if (strcasecmp(content, "image/x-xbitmap") == 0 ||
		strcasecmp(content, "image/xbm") == 0 ||
		strcasecmp(content, "image/x-xbm") == 0)
	{
		img = xbitmapLoad(data, datalen, &rgbc);
	}
	else if (strcasecmp(content, "image/x-xpixmap") == 0 ||
		strcasecmp(content, "image/xpm") == 0 ||
		strcasecmp(content, "image/x-xpm") == 0)
	{
		img = xpixmapLoad(XtDisplay(w), DefaultScreen(XtDisplay(w)), data, datalen, &rgbc);
	}
	else if (strcasecmp(content, "image/x-portable-anymap") == 0 ||
		strcasecmp(content, "image/x-portable-bitmap") == 0 ||
		strcasecmp(content, "image/x-portable-graymap") == 0 ||
		strcasecmp(content, "image/x-portable-pixmap") == 0)
	{
		img = pbmLoad(data, datalen, &rgbc);
	} else {
		if (d->ptext != NULL) return(NULL);
		//printf("DBG content to external load: %s\n", content);
		x = ConvertDocument(d, clist, "inline", path);
		if (x != NULL) {
			if (x->ptext != NULL && x->pcontent != NULL && strncasecmp(x->pcontent, "image/", 6) == 0) {
				iinfo = CreateImageInfo (w, x, clist, path, stype, maxcolors, bg, gamma);  /* recursive call */
			}
			if (x != d) DestroyDocument(x);
		}
		return(iinfo);
	}

	if (img != NULL) {
		if (img->type != IBAD) {
			if (img->rgb.used < 3 ) {
				int i;
				for(i = 0; i < img->rgb.used; ++i) {
					if ( RGB_TO_TRUE (*(img->rgb.red + i),  *(img->rgb.green + i), *(img->rgb.blue + i) ) == 0xffffff) {
						*(img->rgb.red + i) = bg->red;
						*(img->rgb.green + i) = bg->green;
						*(img->rgb.blue + i) = bg->blue;
						break;
					}
				}
			}
			if ((stype == MONO_DISPLAY || maxcolors == 2) && img->type != IBITMAP) {
				nimg = dither(img, 0);
				freeImage(img);
				img = nimg;
			} else if (img->rgb.used > maxcolors) {
				printf("DBG chimera: triggering reduce\n");	  
				nimg = reduce(img, maxcolors, 0);
				freeImage(img);
				img = nimg;
			}

			if (img->type == IBITMAP) {
				nimg = expand(img);
				freeImage(img);
				img = nimg;
				gamma = 0.0;
			}

			if (img != NULL) {
				if (gamma > (double)0.0) gammacorrect(img, gamma);
			
				if (img->type == ITRUE) {
					iinfo = ConvertTrueImageToImageInfo(img);
				} else if (img->type == IRGB) {
					iinfo = ConvertRGBImageToImageInfo(img);
				}
				freeImage(img);
			}
		} else {
			free_mem((char *)img); /* probably need to free more than this. */
		}
	}
	return(iinfo);
}
