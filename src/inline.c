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

#include "libimagequant/libimagequant.h"


#ifndef HAVE_POWF
float powf (float x, float y) { return (float) pow( (double)x, (double)y ); }
#endif

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
     
    // 3. ALPHA BLENDING: Blend against Chimeras background color (bg)
	unsigned char bg_r = (bg->red >> 8) & 0xFF;
	unsigned char bg_g = (bg->green >> 8) & 0xFF;
	unsigned char bg_b = (bg->blue >> 8) & 0xFF;
    
	int total_pixels = width * height;
	int i;
	for (i = 0; i < total_pixels; i++) {
		int idx = i * 4;
		unsigned char r = raw_rgba[idx];
		unsigned char g = raw_rgba[idx+1];
		unsigned char b = raw_rgba[idx+2];
		unsigned char a = raw_rgba[idx+3];
        
		if (a == 0) {
			raw_rgba[idx]   = bg_r;
			raw_rgba[idx+1] = bg_g;
			raw_rgba[idx+2] = bg_b;
		} else if (a < 255) {
			raw_rgba[idx]   = (r * a + bg_r * (255 - a)) / 255;
			raw_rgba[idx+1] = (g * a + bg_g * (255 - a)) / 255;
			raw_rgba[idx+2] = (b * a + bg_b * (255 - a)) / 255;
		}
		raw_rgba[idx+3] = 255; 
	}
    
    // 4. INITIALIZE CHIMERA IMAGE STRUCTURE
    image = (Image *)alloc_mem(sizeof(Image));
    memset(image, 0, sizeof(Image));
    
    image->type = IRGB; 
    image->width = width;
    image->height = height;
    image->pixlen = 1;  
    image->data = (unsigned char *)alloc_mem(width * height);

	// GIF/PNG/JPEG/BMP VIA LIBIMAGEQUANT
    liq_attr *attr = liq_attr_create();
    liq_set_max_colors(attr, 256); 
    liq_set_speed(attr, 10);
        
    liq_image *liq_img = liq_image_create_rgba(attr, raw_rgba, width, height, 0);
    liq_result *res = NULL;
        
    if (liq_image_quantize(liq_img, attr, &res) != LIQ_OK) {
        printf("ERROR: libimagequant could not quantize image\n");
        if (is_scaled) free_mem(raw_rgba); else stbi_image_free(raw_rgba);
        liq_image_destroy(liq_img);
        liq_attr_destroy(attr);
        free_mem(image->data);
        free_mem(image);
        return(NULL);
    }
        
        //liq_set_dithering_level(res, 0.80);
        liq_set_dithering_level(res, 0.0);		//testing for speed/gprof
        liq_write_remapped_image(res, liq_img, image->data, width * height);
        
        const liq_palette *pal = liq_get_palette(res);
        
        image->rgb.red = (unsigned short *)alloc_mem(sizeof(unsigned short) * pal->count);
        image->rgb.green = (unsigned short *)alloc_mem(sizeof(unsigned short) * pal->count);
        image->rgb.blue = (unsigned short *)alloc_mem(sizeof(unsigned short) * pal->count);
        image->rgb.used = pal->count;
        image->rgb.size = pal->count;
        
        for (i = 0; i < pal->count; i++) {
            image->rgb.red[i]   = (unsigned short)pal->entries[i].r * 0xffff / 0xff;
            image->rgb.green[i] = (unsigned short)pal->entries[i].g * 0xffff / 0xff;
            image->rgb.blue[i]  = (unsigned short)pal->entries[i].b * 0xffff / 0xff;
        }
        
        liq_result_destroy(res);
        liq_image_destroy(liq_img);
        liq_attr_destroy(attr);
        
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
    int             width, height, i;
    float           scale = 1.0f;
    char            *svg_copy;

    /* nanosvg kræver en null-termineret streng. Vi laver en hurtig kopi. */
    svg_copy = (char *)alloc_mem(datalen + 1);
    memcpy(svg_copy, data, datalen);
    svg_copy[datalen] = '\0';

    /* Parse SVG kildekoden */
    svg_img = nsvgParse(svg_copy, "px", 96.0f);
    free_mem(svg_copy);

    if (svg_img == NULL) {
        printf("ERROR: Kunne ikke parse SVG data!\n");
        return(NULL);
    }

    /* Originale dimensioner fra SVG-filen */
    int orig_w = (int)svg_img->width;
    int orig_h = (int)svg_img->height;
    if (orig_w < 1) orig_w = 1;
    if (orig_h < 1) orig_h = 1;
    
   /* printf("NANO says width         : %d pixels\n", orig_w);
    printf("HTML request width : %d pixels\n", target_width);
    printf("NANO says height       : %d pixels\n", orig_h);
    printf("HTML request height: %d pixels\n", target_height);
    printf("\n");*/

    width = orig_w;
    height = orig_h;

    /* --- Genbrug af din smarte boks-skaleringslogik --- */
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

    /* Undgå upscaling og respekter MAX_WIDTH */
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

    /* Beregn den præcise skaleringsfaktor til vektorgrafikken */
    scale = (float)width / svg_img->width;

    /* Alloker buffer til de rå RGBA pixels */
    raw_rgba = (unsigned char *)alloc_mem(width * height * 4);

    /* Rasteriser SVG direkte til den ønskede målstørrelse */
    rast = nsvgCreateRasterizer();
    nsvgRasterize(rast, svg_img, 0.0f, 0.0f, scale, raw_rgba, width, height, width * 4);

    /* Frigør nanosvg strukturer med det samme – vi har vores pixels nu */
    nsvgDeleteRasterizer(rast);
    nsvgDelete(svg_img);

    /* --- ALPHA BLENDING (Nøjagtig kopi fra din imgLoad) --- */
    unsigned char bg_r = (bg->red >> 8) & 0xFF;
    unsigned char bg_g = (bg->green >> 8) & 0xFF;
    unsigned char bg_b = (bg->blue >> 8) & 0xFF;
    
    int total_pixels = width * height;
    for (i = 0; i < total_pixels; i++) {
        int idx = i * 4;
        unsigned char r = raw_rgba[idx];
        unsigned char g = raw_rgba[idx+1];
        unsigned char b = raw_rgba[idx+2];
        unsigned char a = raw_rgba[idx+3];
        
        if (a == 0) {
            raw_rgba[idx]   = bg_r;
            raw_rgba[idx+1] = bg_g;
            raw_rgba[idx+2] = bg_b;
        } else if (a < 255) {
            raw_rgba[idx]   = (r * a + bg_r * (255 - a)) / 255;
            raw_rgba[idx+1] = (g * a + bg_g * (255 - a)) / 255;
            raw_rgba[idx+2] = (b * a + bg_b * (255 - a)) / 255;
        }
        raw_rgba[idx+3] = 255; 
    }

    /* --- INITIALIZE CHIMERA IMAGE & LIBIMAGEQUANT (Ligeledes genbrug) --- */
    image = (Image *)alloc_mem(sizeof(Image));
    memset(image, 0, sizeof(Image));
    
    image->type = IRGB; 
    image->width = width;
    image->height = height;
    image->pixlen = 1;  
    image->data = (unsigned char *)alloc_mem(width * height);

    liq_attr *attr = liq_attr_create();
    liq_set_max_colors(attr, 256); 
   // liq_set_max_colors(attr, 128); 	//for speedtest/gprof
    liq_set_speed(attr, 10);
        
    liq_image *liq_img = liq_image_create_rgba(attr, raw_rgba, width, height, 0);
    liq_result *res = NULL;
        
    if (liq_image_quantize(liq_img, attr, &res) != LIQ_OK) {
        printf("ERROR: libimagequant kunne ikke kvantisere SVG\n");
        free_mem(raw_rgba);
        liq_image_destroy(liq_img);
        liq_attr_destroy(attr);
        free_mem(image->data);
        free_mem(image);
        return(NULL);
    }
        
    liq_set_dithering_level(res, 0.80);
    liq_write_remapped_image(res, liq_img, image->data, width * height);
    
    const liq_palette *pal = liq_get_palette(res);
    
    image->rgb.red = (unsigned short *)alloc_mem(sizeof(unsigned short) * pal->count);
    image->rgb.green = (unsigned short *)alloc_mem(sizeof(unsigned short) * pal->count);
    image->rgb.blue = (unsigned short *)alloc_mem(sizeof(unsigned short) * pal->count);
    image->rgb.used = pal->count;
    image->rgb.size = pal->count;
    
    for (i = 0; i < pal->count; i++) {
        image->rgb.red[i]   = (unsigned short)pal->entries[i].r * 0xffff / 0xff;
        image->rgb.green[i] = (unsigned short)pal->entries[i].g * 0xffff / 0xff;
        image->rgb.blue[i]  = (unsigned short)pal->entries[i].b * 0xffff / 0xff;
    }
    
    liq_result_destroy(res);
    liq_image_destroy(liq_img);
    liq_attr_destroy(attr);
    
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
