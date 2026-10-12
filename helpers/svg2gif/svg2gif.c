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

#include <math.h>

#ifdef __UCLIBC__
float sinf (float x) {return (float) sin((double)x); }
float cosf(float x) { return (float) cos((double)x); }
float tanf (float x){return (float) tan ( (double)x);}
float atan2f (float x, float y){return (float) atan2((double)x, (double)y);}
float fmodf (float x, float y){return (float) fmod ((double)x, (double)y);}
float acosf (float x){return (float) acos((double)x );}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <gif_lib.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#if defined(GIFLIB_MAJOR) && GIFLIB_MAJOR >= 5
#define MakeMapObject GifMakeMapObject
#define FreeMapObject GifFreeMapObject
#define EGifOpenFileHandle(h) EGifOpenFileHandle(h, NULL)
#endif

static int clampi(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v;
}

int write_gif(FILE *out, const unsigned char *rgba, int width, int height) {
    int x, y;

    GifFileType *gif = EGifOpenFileHandle(fileno(out));
    if (!gif)
        return 0;

    // Palette:
    //   0      = transparent
    //   1..216 = 6x6x6 RGB cube
     
    ColorMapObject *cmap = MakeMapObject(256, NULL);
    if (!cmap)
        return 0;

    cmap->Colors[0].Red   = 0;
    cmap->Colors[0].Green = 0;
    cmap->Colors[0].Blue  = 0;

    int idx = 1;

    int r, g, b;

    for (r = 0; r < 6; r++) {
        for (g = 0; g < 6; g++) {
            for (b = 0; b < 6; b++) {
                cmap->Colors[idx].Red   = r * 51;
                cmap->Colors[idx].Green = g * 51;
                cmap->Colors[idx].Blue  = b * 51;
                idx++;
            }
        }
    }

    while (idx < 256){
        cmap->Colors[idx].Red   = 0;
        cmap->Colors[idx].Green = 0;
        cmap->Colors[idx].Blue  = 0;
        idx++;
    }

    if (EGifPutScreenDesc(gif, width, height, 8, 0, cmap) == GIF_ERROR){
        FreeMapObject(cmap);
        return 0;
    }

    // GIF89a transparency extension.
    unsigned char gce[4];

    gce[0] = 0x01; // transparency flag
    gce[1] = 0x00; // delay low
    gce[2] = 0x00; // delay high
    gce[3] = 0x00; // transparent color index

    if (EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, sizeof(gce), gce) == GIF_ERROR) {
        FreeMapObject(cmap);
        return 0;
    }

    if (EGifPutImageDesc(gif, 0, 0, width, height, 0, NULL) == GIF_ERROR) {
        FreeMapObject(cmap);
        return 0;
    }

    GifPixelType *indexed = (GifPixelType *)malloc(width * height);

    if (!indexed) {
        FreeMapObject(cmap);
        return 0;
    }

    // Error diffusion buffers.
    float *er = calloc(width + 2, sizeof(float));
    float *eg = calloc(width + 2, sizeof(float));
    float *eb = calloc(width + 2, sizeof(float));

    float *nr = calloc(width + 2, sizeof(float));
    float *ng = calloc(width + 2, sizeof(float));
    float *nb = calloc(width + 2, sizeof(float));

    if (!er || !eg || !eb || !nr || !ng || !nb) {
        free(indexed);
        free(er); free(eg); free(eb);
        free(nr); free(ng); free(nb);
        FreeMapObject(cmap);
        return 0;
    }

    for (y = 0; y < height; y++) {
        memset(nr, 0, (width + 2) * sizeof(float));
        memset(ng, 0, (width + 2) * sizeof(float));
        memset(nb, 0, (width + 2) * sizeof(float));

        for (x = 0; x < width; x++) {
            int p = (y * width + x) * 4;
            int alpha = rgba[p + 3];
            if (alpha < 128) {
                indexed[y * width + x] = 0;
                continue;
            }

            float fr = rgba[p + 0] + er[x + 1];
            float fg = rgba[p + 1] + eg[x + 1];
            float fb = rgba[p + 2] + eb[x + 1];

            int rr = clampi((int)(fr + 0.5f));
            int gg = clampi((int)(fg + 0.5f));
            int bb = clampi((int)(fb + 0.5f));

            int qr = rr / 51;
            int qg = gg / 51;
            int qb = bb / 51;

            int pal = 1 + qr * 36 + qg * 6 + qb;

            indexed[y * width + x] = pal;

            int pr = qr * 51;
            int pg = qg * 51;
            int pb = qb * 51;

            float dr = rr - pr;
            float dg = gg - pg;
            float db = bb - pb;

            // Floyd–Steinberg
            er[x + 2] += dr * (7.0f / 16.0f);
            eg[x + 2] += dg * (7.0f / 16.0f);
            eb[x + 2] += db * (7.0f / 16.0f);

            nr[x + 0] += dr * (3.0f / 16.0f);
            ng[x + 0] += dg * (3.0f / 16.0f);
            nb[x + 0] += db * (3.0f / 16.0f);

            nr[x + 1] += dr * (5.0f / 16.0f);
            ng[x + 1] += dg * (5.0f / 16.0f);
            nb[x + 1] += db * (5.0f / 16.0f);

            nr[x + 2] += dr * (1.0f / 16.0f);
            ng[x + 2] += dg * (1.0f / 16.0f);
            nb[x + 2] += db * (1.0f / 16.0f);
        }

        memcpy(er, nr, (width + 2) * sizeof(float));
        memcpy(eg, ng, (width + 2) * sizeof(float));
        memcpy(eb, nb, (width + 2) * sizeof(float));
    }

    for (y = 0; y < height; y++){
        if (EGifPutLine(gif, indexed + y * width, width) == GIF_ERROR) {
            free(indexed);
            free(er); free(eg); free(eb);
            free(nr); free(ng); free(nb);
            FreeMapObject(cmap);
            return 0;
        }
    }

    free(indexed);
    free(er); free(eg); free(eb);
    free(nr); free(ng); free(nb);

    FreeMapObject(cmap);

#if GIFLIB_MAJOR >= 5
    {
        int err = 0;
        EGifCloseFile(gif, &err);
    }
#else
    EGifCloseFile(gif);
#endif

    return 1;
}
/*
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input.svg output.gif|-\n", argv[0]);
        fprintf(stderr, "Use '-' for stdout\n");
        return 1;
    }

    const char *inputfile = argv[1];
    const char *outputfile = argv[2];
    int stdoutput = (strcmp(outputfile, "-") == 0);

    int width = 0, height = 0;
    unsigned char *img = NULL;
    NSVGimage *shapes = NULL;
    NSVGrasterizer *rast = NULL;
     
	shapes = nsvgParseFromFile(inputfile, "px", 96.0f);
       
	if (shapes) {
            width = (int)shapes->width;
            height = (int)shapes->height;
            rast = nsvgCreateRasterizer();
            if (width <= 0 || height <= 0) {
				fprintf(stderr, "Invalid SVG size %d x %d\n",
						width, height);
				return 1;
			}
            if ((size_t)width > SIZE_MAX / 4 / (size_t)height) {
				fprintf(stderr, "Image too large\n");
				return 1;
			}
			img = malloc(width * height * 4);
            if (!img) {
                fprintf(stderr, "Memory allocation failed\n");
                return 1;
            }
          			
			memset(img, 0, width * height * 4);
       
            nsvgRasterize(rast, shapes, 0, 0, 1, img, width, height, width * 4);
        } else {
			fprintf(stderr, "Failed to parse SVG file: %s\n", inputfile);
			return 1;
	}
        
     // Output to file or stdout
    FILE *out = stdoutput ? stdout : fopen(outputfile, "wb");
    if (!out) {
        fprintf(stderr, "Failed to open output file\n");
        free(img);
         if (shapes) nsvgDelete(shapes);
    if (rast) nsvgDeleteRasterizer(rast);
        return 1;
    }

    if (!write_gif(out, img, width, height)) {
        fprintf(stderr, "Failed to write GIF output\n");
        if (!stdoutput) fclose(out);
        free(img);
         if (shapes) nsvgDelete(shapes);
    if (rast) nsvgDeleteRasterizer(rast);
        return 1;
    }

    if (!stdoutput) fclose(out);
    free(img);
     if (shapes) nsvgDelete(shapes);
    if (rast) nsvgDeleteRasterizer(rast);
    return 0;
}*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// define max size of output image
#define MAX_ICON_SIZE 128

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input.svg output.gif|-\n", argv[0]);
        fprintf(stderr, "Use '-' for stdout\n");
        return 1;
    }

    const char *inputfile = argv[1];
    const char *outputfile = argv[2];
    int stdoutput = (strcmp(outputfile, "-") == 0);

    int width = 0, height = 0;
    unsigned char *img = NULL;
    NSVGimage *shapes = NULL;
    NSVGrasterizer *rast = NULL;
     
    shapes = nsvgParseFromFile(inputfile, "px", 96.0f);
       
    if (shapes) {
        // find original dimensions
        float orig_w = shapes->width;
        float orig_h = shapes->height;

        if (orig_w <= 0 || orig_h <= 0) {
            fprintf(stderr, "Invalid SVG size %f x %f\n", orig_w, orig_h);
            if (shapes) nsvgDelete(shapes);
            return 1;
        }

        // calculate scale factor based on MAX_ICON_SIZE
        float scale = 1.0f;
        if (orig_w > MAX_ICON_SIZE || orig_h > MAX_ICON_SIZE) {
            if (orig_w > orig_h) {
                scale = (float)MAX_ICON_SIZE / orig_w;
            } else {
                scale = (float)MAX_ICON_SIZE / orig_h;
            }
        }

        // set new dimensions of output-image
        width = (int)(orig_w * scale);
        height = (int)(orig_h * scale);

        rast = nsvgCreateRasterizer();
        if (width <= 0 || height <= 0) {
            fprintf(stderr, "Invalid calculated size %d x %d\n", width, height);
            if (shapes) nsvgDelete(shapes);
            if (rast) nsvgDeleteRasterizer(rast);
            return 1;
        }

        if ((size_t)width > SIZE_MAX / 4 / (size_t)height) {
            fprintf(stderr, "Image too large\n");
            if (shapes) nsvgDelete(shapes);
            if (rast) nsvgDeleteRasterizer(rast);
            return 1;
        }

        img = malloc(width * height * 4);
        if (!img) {
            fprintf(stderr, "Memory allocation failed\n");
            if (shapes) nsvgDelete(shapes);
            if (rast) nsvgDeleteRasterizer(rast);
            return 1;
        }
          			
        memset(img, 0, width * height * 4);
       
        nsvgRasterize(rast, shapes, 0, 0, scale, img, width, height, width * 4);
    } else {
        fprintf(stderr, "Failed to parse SVG file: %s\n", inputfile);
        return 1;
    }
        
    // Output to file or stdout
    FILE *out = stdoutput ? stdout : fopen(outputfile, "wb");
    if (!out) {
        fprintf(stderr, "Failed to open output file\n");
        free(img);
        if (shapes) nsvgDelete(shapes);
        if (rast) nsvgDeleteRasterizer(rast);
        return 1;
    }

    if (!write_gif(out, img, width, height)) {
        fprintf(stderr, "Failed to write GIF output\n");
        if (!stdoutput) fclose(out);
        free(img);
        if (shapes) nsvgDelete(shapes);
        if (rast) nsvgDeleteRasterizer(rast);
        return 1;
    }

    if (!stdoutput) fclose(out);
    free(img);
    if (shapes) nsvgDelete(shapes);
    if (rast) nsvgDeleteRasterizer(rast);
    return 0;
}

