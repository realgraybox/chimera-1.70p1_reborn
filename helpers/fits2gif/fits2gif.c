/*
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

//images can be found here: https://fits.gsfc.nasa.gov/nrao_data/samples/
#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <gif_lib.h>
#include <unistd.h>

#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR >= 5
#define MakeMapObject GifMakeMapObject
#define FreeMapObject GifFreeMapObject
#endif

#define BLOCK_SIZE 2880

typedef struct {
    int width;
    int height;
    uint8_t *indexed_pixels;
} FitsImage;

/* -------------------------------------------------- */
/* FITS header helpers                                */
/* -------------------------------------------------- */

static int find_keyword(
    const char *header,
    size_t header_size,
    const char *key,
    char *value,
    size_t value_size)
{
    size_t i;

    for(i = 0; i + 80 <= header_size; i += 80)
    {
        if(strncmp(header + i, key, strlen(key)) == 0)
        {
            const char *eq = memchr(header + i, '=', 80);

            if(!eq)
                return 0;

            eq++;

            while(*eq == ' ')
                eq++;

            size_t n = 0;

            while(n < value_size - 1 &&
                  eq + n < header + i + 80 &&
                  eq[n] != '/' &&
                  eq[n] != '\n' &&
                  eq[n] != '\r')
            {
                value[n] = eq[n];
                n++;
            }

            while(n > 0 && value[n - 1] == ' ')
                n--;

            value[n] = 0;

            return 1;
        }
    }

    return 0;
}

static long get_long_keyword(
    const char *header,
    size_t header_size,
    const char *key,
    long defval)
{
    char value[128];

    if(find_keyword(header, header_size,
                    key, value, sizeof(value)))
    {
        return strtol(value, NULL, 10);
    }

    return defval;
}

static double get_double_keyword(
    const char *header,
    size_t header_size,
    const char *key,
    double defval)
{
    char value[128];

    if(find_keyword(header, header_size,
                    key, value, sizeof(value)))
    {
        return strtod(value, NULL);
    }

    return defval;
}

/* -------------------------------------------------- */
/* Big-endian readers                                 */
/* -------------------------------------------------- */

static int16_t read_be_int16(FILE *f)
{
    unsigned char b[2];

    if(fread(b,1,2,f) != 2)
        return 0;

    return (int16_t)(
        ((uint16_t)b[0] << 8) |
         (uint16_t)b[1]);
}

static int32_t read_be_int32(FILE *f)
{
    unsigned char b[4];

    if(fread(b,1,4,f) != 4)
        return 0;

    return (int32_t)(
        ((uint32_t)b[0] << 24) |
        ((uint32_t)b[1] << 16) |
        ((uint32_t)b[2] << 8 ) |
         (uint32_t)b[3]);
}

static float read_be_float(FILE *f)
{
    unsigned char b[4];

    union {
        uint32_t u;
        float f;
    } v;

    if(fread(b,1,4,f) != 4)
        return 0.0f;

    v.u =
        ((uint32_t)b[0] << 24) |
        ((uint32_t)b[1] << 16) |
        ((uint32_t)b[2] << 8 ) |
         (uint32_t)b[3];

    return v.f;
}

static double read_be_double(FILE *f)
{
    unsigned char b[8];

    union {
        uint64_t u;
        double d;
    } v;

    if(fread(b,1,8,f) != 8)
        return 0.0;

    v.u =
        ((uint64_t)b[0] << 56) |
        ((uint64_t)b[1] << 48) |
        ((uint64_t)b[2] << 40) |
        ((uint64_t)b[3] << 32) |
        ((uint64_t)b[4] << 24) |
        ((uint64_t)b[5] << 16) |
        ((uint64_t)b[6] << 8 ) |
         (uint64_t)b[7];

    return v.d;
}

/* -------------------------------------------------- */
/* FITS loader                                        */
/* -------------------------------------------------- */

static int load_fits(
    const char *filename,
    FitsImage *img)
{
    FILE *f = NULL;
    char *header = NULL;

    size_t header_size = 0;
    size_t header_capacity = 0;

    int end_found = 0;

    long naxis;
    long width;
    long height;
    long bitpix;

    double bscale;
    double bzero;

    double *pixels = NULL;

    f = fopen(filename, "rb");

    if(!f)
    {
        perror(filename);
        return 1;
    }

    while(!end_found)
    {
        header_capacity += BLOCK_SIZE;

        char *tmp =
            realloc(header, header_capacity);

        if(!tmp)
        {
            fprintf(stderr,
                    "Out of memory\n");
            goto fail;
        }

        header = tmp;

        if(fread(header + header_size,
                 1,
                 BLOCK_SIZE,
                 f) != BLOCK_SIZE)
        {
            fprintf(stderr,
                    "Header read error\n");
            goto fail;
        }

        size_t block_start = header_size;

        header_size += BLOCK_SIZE;

        size_t i;

        for(i = block_start;
            i + 80 <= header_size;
            i += 80)
        {
            if(strncmp(header + i,
                       "END", 3) == 0)
            {
                end_found = 1;
                break;
            }
        }
    }

    naxis =
        get_long_keyword(
            header,
            header_size,
            "NAXIS",
            0);
            
   width =
    get_long_keyword(
        header,
        header_size,
        "NAXIS1",
        -1);

	height =
		get_long_keyword(
			header,
			header_size,
			"NAXIS2",
			-1);

	bitpix =
		get_long_keyword(
			header,
			header_size,
			"BITPIX",
			0);         
            
      bscale =
    get_double_keyword(
        header,
        header_size,
        "BSCALE",
        1.0);

	bzero =
		get_double_keyword(
			header,
			header_size,
			"BZERO",
			0.0);         

#ifdef DEBUG
	printf("NAXIS  = %ld\n", naxis);

	for(int k=1;k<=naxis;k++)
	{
		char key[16];

		sprintf(key,"NAXIS%d",k);

		printf("%s = %ld\n",
			   key,
			   get_long_keyword(
				   header,
				   header_size,
				   key,
				   -1));
	}
#endif    
   
    
if(naxis == 2)
{
#ifdef DEBUG
	width =
        get_long_keyword(
            header,
            header_size,
            "NAXIS1",
            -1);

    height =
        get_long_keyword(
            header,
            header_size,
            "NAXIS2",
            -1);

    bitpix =
        get_long_keyword(
            header,
            header_size,
            "BITPIX",
            0);

    bscale =
        get_double_keyword(
            header,
            header_size,
            "BSCALE",
            1.0);

    bzero =
        get_double_keyword(
            header,
            header_size,
            "BZERO",
            0.0);
#endif
} else if (naxis == 3) {
	
	long naxis3 =
        get_long_keyword(
            header,
            header_size,
            "NAXIS3",
            0);

    printf("NAXIS3 = %ld\n", naxis3);
    printf("Using first image plane\n");
}
else
{
    fprintf(stderr,
            "Unsupported NAXIS=%ld\n",
            naxis);
    goto fail;
}
	
	
    if(width <= 0 || height <= 0)
    {
        fprintf(stderr, "Invalid dimensions\n");
        goto fail;
    }

    printf("Width  : %ld\n", width);
    printf("Height : %ld\n", height);
    printf("BITPIX : %ld\n", bitpix);
    printf("BSCALE : %g\n", bscale);
    printf("BZERO  : %g\n", bzero);

    size_t npixels =
        (size_t)width * (size_t)height;

    pixels =
        malloc(npixels * sizeof(double));

    if(!pixels)
    {
        fprintf(stderr,
                "Out of memory\n");
        goto fail;
    }

    size_t i;

    switch(bitpix)
    {
        case 8:
        {
            for(i=0;i<npixels;i++)
            {
                unsigned char v;

                if(fread(&v,1,1,f) != 1)
                {
                    fprintf(stderr,
                            "Unexpected EOF\n");
                    goto fail;
                }

                pixels[i] =
                    bscale * v + bzero;
            }
            break;
        }

        case 16:
        {
            for(i=0;i<npixels;i++)
            {
                pixels[i] =
                    bscale *
                    read_be_int16(f)
                    + bzero;
            }
            break;
        }

        case 32:
        {
            for(i=0;i<npixels;i++)
            {
                pixels[i] =
                    bscale *
                    read_be_int32(f)
                    + bzero;
            }
            break;
        }

        case -32:
        {
            for(i=0;i<npixels;i++)
            {
                pixels[i] =
                    bscale *
                    read_be_float(f)
                    + bzero;
            }
            break;
        }

        case -64:
        {
            for(i=0;i<npixels;i++)
            {
                pixels[i] =
                    bscale *
                    read_be_double(f)
                    + bzero;
            }
            break;
        }

        default:
        {
            fprintf(stderr,
                    "Unsupported BITPIX=%ld\n",
                    bitpix);
            goto fail;
        }
    }

    double minv = pixels[0];
    double maxv = pixels[0];

    for(i=1;i<npixels;i++)
    {
        if(pixels[i] < minv)
            minv = pixels[i];

        if(pixels[i] > maxv)
            maxv = pixels[i];
    }

    printf("Min pixel: %g\n", minv);
    printf("Max pixel: %g\n", maxv);

    img->indexed_pixels =
        malloc(npixels);

    if(!img->indexed_pixels)
    {
        fprintf(stderr,
                "Out of memory\n");
        goto fail;
    }

    img->width = (int)width;
    img->height = (int)height;

    if(maxv == minv)
    {
        memset(img->indexed_pixels,
               0,
               npixels);
    }
    else
    {
        double scale =
            255.0 / (maxv - minv);

        for(i=0;i<npixels;i++)
        {
            double x =
                (pixels[i] - minv)
                * scale;

            if(x < 0)
                x = 0;

            if(x > 255)
                x = 255;

            img->indexed_pixels[i] =
                (uint8_t)(x + 0.5);
        }
    }

    free(pixels);
    free(header);
    fclose(f);

    return 0;

fail:

    free(header);
    free(pixels);

    if(f)
        fclose(f);

    return 1;
}

/* -------------------------------------------------- */
/* GIF writer                                         */
/* -------------------------------------------------- */

static int save_gif(
    const char *filename,
    const uint8_t *indexed_pixels,
    int width,
    int height)
{
    int err = 0;

    ColorMapObject *palette =
        MakeMapObject(256, NULL);

    if(!palette)
    {
        fprintf(stderr,
                "Palette allocation failed\n");
        return 1;
    }

    int i;

    for(i=0;i<256;i++)
    {
        palette->Colors[i].Red   = i;
        palette->Colors[i].Green = i;
        palette->Colors[i].Blue  = i;
    }

    GifFileType *gif;

#if GIFLIB_MAJOR >= 5
    int giferr = 0;

    if(strcmp(filename, "-") == 0)
        gif = EGifOpenFileHandle(
            fileno(stdout),
            &giferr);
    else
        gif = EGifOpenFileName(
            filename,
            0,
            &giferr);
#else
    if(strcmp(filename, "-") == 0)
        gif = EGifOpenFileHandle(
            fileno(stdout));
    else
        gif = EGifOpenFileName(
            filename,
            0);
#endif

    if(!gif)
    {
        FreeMapObject(palette);
        return 1;
    }

    if(EGifPutScreenDesc(
           gif,
           width,
           height,
           8,
           0,
           palette) == GIF_ERROR)
    {
        err = 1;
        goto done;
    }

    if(EGifPutImageDesc(
           gif,
           0,
           0,
           width,
           height,
           0,
           NULL) == GIF_ERROR)
    {
        err = 1;
        goto done;
    }

    int y;

    for(y=0;y<height;y++)
    {
        if(EGifPutLine(
               gif,
               (GifPixelType *)
               &indexed_pixels[y * width],
               width) == GIF_ERROR)
        {
            err = 1;
            goto done;
        }
    }

done:

#if GIFLIB_MAJOR >= 5
    EGifCloseFile(gif, &giferr);
#else
    EGifCloseFile(gif);
#endif

    FreeMapObject(palette);

    return err;
}

/* -------------------------------------------------- */
/* Main                                               */
/* -------------------------------------------------- */

int main(int argc, char **argv)
{
    FitsImage img;

    img.width = 0;
    img.height = 0;
    img.indexed_pixels = NULL;

    if(argc != 3)
    {
        fprintf(stderr,
                "Usage: %s input.fits output.gif\n",
                argv[0]);

        fprintf(stderr,
                "Use '-' for stdout\n");

        return 1;
    }

    if(load_fits(argv[1], &img))
        return 1;

    int result =
        save_gif(
            argv[2],
            img.indexed_pixels,
            img.width,
            img.height);

    free(img.indexed_pixels);

    return result;
}
