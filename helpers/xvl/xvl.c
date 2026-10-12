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
// xvl.c
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STBI_ONLY_TGA
#define STBI_ONLY_PSD

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <X11/XWDFile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef USE_TIFF
#include <tiffio.h>
#endif

#ifdef WEBP
#include <webp/decode.h>
#endif

#include "stb_image.h"
#include "stb_image_write.h"

static char loaded_path[4096];
static void make_save_path(char *buf, size_t bufsz)
{
    const char *base = "output";
    char dir[4096] = ".";

    if (loaded_path[0]) {
        const char *slash = strrchr(loaded_path, '/');

        if (slash) {
            size_t dirlen = (size_t)(slash - loaded_path);

            if (dirlen >= sizeof dir)
                dirlen = sizeof dir - 1;

            memcpy(dir, loaded_path, dirlen);
            dir[dirlen] = '\0';

            base = slash + 1;
        } else {
            base = loaded_path;
        }
    }

    char stem[4096];
    snprintf(stem, sizeof stem, "%s", base);

    char *dot = strrchr(stem, '.');
    if (dot && dot != stem)
        *dot = '\0';

    if (strcmp(dir, ".") == 0)
        snprintf(buf, bufsz, "%s_view.png", stem);
    else
        snprintf(buf, bufsz, "%s/%s_view.png", dir, stem);
}

void set_window_icon(Display *dpy, Window win) {
    unsigned long icon_data[] = {
        2, 2,                     // width, height
        0xFFFF0000, 0xFF00FF00,   // red, green
        0xFF0000FF, 0xFFFFFFFF    // blue, white
    };
    
    int data_length = 2 + (2 * 2); // 2 (measure) + (width * height)

    // find or create the correct Atom for netwm-icon
    Atom wm_icon = XInternAtom(dpy, "_NET_WM_ICON", False);
    Atom cardinal = XInternAtom(dpy, "CARDINAL", False);

    // write data to window property
    XChangeProperty(dpy, win, wm_icon, cardinal, 32, PropModeReplace, (unsigned char*)icon_data, data_length);
}

// global or static variables for the filemanagers mouse click
typedef enum {
    FS_REGION_NONE = 0,
    FS_REGION_CANCEL,
    FS_REGION_OPEN
} FsRegion;

static int fs_cancel_x1, fs_cancel_x2;
static int fs_open_x1, fs_open_x2;
static int fs_text_y;

float zoom = 1.0f;
int offset_x = 0, offset_y = 0;
const int ui_height = 40;

typedef enum {
    UI_REGION_NONE = 0,
    UI_REGION_OPEN,
    UI_REGION_ZOOM,
    UI_REGION_PAN,
    UI_REGION_SAVE,
    UI_REGION_QUIT
} UiRegion;

int ui_open_x1, ui_open_x2;
int ui_zoom_x1, ui_zoom_x2;
int ui_pan_x1, ui_pan_x2;
int ui_save_x1, ui_save_x2;
int ui_quit_x1, ui_quit_x2;
int ui_text_y;
XFontStruct *ui_font = NULL;

static UiRegion ui_hit_region(int x, int y) {
    if (y < ui_text_y - 15 || y > ui_text_y + 5) return UI_REGION_NONE;
	if (x >= ui_open_x1 && x <= ui_open_x2) return UI_REGION_OPEN;
    if (x >= ui_zoom_x1 && x <= ui_zoom_x2) return UI_REGION_ZOOM;
    if (x >= ui_pan_x1 && x <= ui_pan_x2) return UI_REGION_PAN;
    if (x >= ui_save_x1 && x <= ui_save_x2) return UI_REGION_SAVE;
    if (x >= ui_quit_x1 && x <= ui_quit_x2) return UI_REGION_QUIT;
    return UI_REGION_NONE;
}

static void zoom_in(void) {
    zoom *= 1.25f;
}

static void zoom_out(void) {
    zoom /= 1.25f;
    if (zoom < 0.1f) zoom = 0.1f;
}

static void pan_left(void) {
    offset_x -= (int)(20 / zoom);
}

static void pan_right(void) {
    offset_x += (int)(20 / zoom);
}

static void pan_up(void) {
    offset_y -= (int)(20 / zoom);
}

static void pan_down(void) {
    offset_y += (int)(20 / zoom);
}

// filemanager
#define MAX_FILES 1024
static char fm_cwd[4096];
static char *fm_files[MAX_FILES];
static int fm_file_count = 0;
static int fm_selected = 0;
static int fm_scroll_offset = 0;

static int compare_files(const void *a, const void *b) {
    return strcoll(*(const char **)a, *(const char **)b);
}

static void load_files(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;
    int i;
    for (i = 0; i < fm_file_count; i++) free(fm_files[i]);
    fm_file_count = 0;
    fm_files[fm_file_count++] = strdup("..");
    struct dirent *entry;
    while ((entry = readdir(dir)) && fm_file_count < MAX_FILES) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;
        fm_files[fm_file_count++] = strdup(entry->d_name);
    }
    closedir(dir);
    qsort(fm_files, fm_file_count, sizeof(char *), compare_files);
    for (i = 0; i < fm_file_count; i++) {
        if (strcmp(fm_files[i], "..") == 0) {
            char *tmp = fm_files[i];
            int j;
            for (j = i; j > 0; j--) fm_files[j] = fm_files[j - 1];
            fm_files[0] = tmp;
            break;
        }
    }
    fm_selected = 0;
}

static FsRegion fs_hit_region(int x, int y) {

    if (y < fs_text_y - 15 || y > fs_text_y + 5)
        return FS_REGION_NONE;

    if (x >= fs_cancel_x1 && x <= fs_cancel_x2) return FS_REGION_CANCEL;
    if (x >= fs_open_x1 && x <= fs_open_x2) return FS_REGION_OPEN;

    return FS_REGION_NONE;
}

static void draw_fm(Display *dpy, Window win, GC gc, XFontStruct *font, int win_w, int win_h) {
    /* 1. Dark background in the entire window (Matches the image viewer without image) */
    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
    XFillRectangle(dpy, win, gc, 0, 0, win_w, win_h);

    int line_height = font ? font->ascent + font->descent + 2 : 14;
    
    /* 2. Use global ui_height (40) instead of dynamic height */
    int usable_h = win_h - ui_height;
    int visible_lines = usable_h / line_height;

    if (fm_file_count <= 0) { fm_scroll_offset = 0; fm_selected = 0; }
    else {
        if (fm_scroll_offset > fm_file_count - visible_lines) fm_scroll_offset = fm_file_count - visible_lines;
        if (fm_scroll_offset < 0) fm_scroll_offset = 0;
        if (fm_selected >= fm_file_count) fm_selected = fm_file_count - 1;
    }
    int i;
    for (i = 0; i < visible_lines; i++) {
        int idx = i + fm_scroll_offset;
        if (idx >= fm_file_count) break;
        int y = (i + 1) * line_height;

        if (idx == fm_selected) {
            /* Selected file: Nice dark gray selection (or white if you want full contrast) */
            XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
            XFillRectangle(dpy, win, gc, 0, y - line_height + 2, win_w, line_height);
            XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
        } else {
            /* Plain text: White */
            XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", fm_cwd, fm_files[idx]);
        struct stat st;
        char info[64] = "";
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) snprintf(info, sizeof(info), "<DIR>");
            else snprintf(info, sizeof(info), "%ld bytes", st.st_size);
        } else { snprintf(info, sizeof(info), "?"); }

        XDrawString(dpy, win, gc, 10, y, fm_files[idx], strlen(fm_files[idx]));
        XDrawString(dpy, win, gc, win_w > 350 ? win_w - 150 : 200, y, info, strlen(info));
    }

    /* 3. Status bar at the bottom: White bar with black text (Matches draw_ui exactly) */
    XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
    XFillRectangle(dpy, win, gc, 0, win_h - ui_height, win_w, ui_height);
    
    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));

    /* Divide text into zones to measure precise click coordinates */
    const char *cancel_text = "[Esc/q]: Cancel";
    const char *scroll_text = "  [Arrows/Wheel]: Scroll  ";
    const char *open_text   = "[Enter/Click]: Open/Select";

    int x = 10;
    fs_text_y = win_h - ui_height + 20; /* Center the text vertically in the 40 pixel high bar */
    
    // Zone 1: Cancel button
    fs_cancel_x1 = x;
    XDrawString(dpy, win, gc, x, fs_text_y, cancel_text, strlen(cancel_text));
    fs_cancel_x2 = fs_cancel_x1 + (font ? XTextWidth(font, cancel_text, (int)strlen(cancel_text)) : 100);

    // Zone 2: Scroll info (space)
    x = fs_cancel_x2;
    XDrawString(dpy, win, gc, x, fs_text_y, scroll_text, strlen(scroll_text));
    int scroll_w = font ? XTextWidth(font, scroll_text, (int)strlen(scroll_text)) : 150;

    // Zone 3: Open/Select button
    x = fs_cancel_x2 + scroll_w;
    fs_open_x1 = x;
    XDrawString(dpy, win, gc, x, fs_text_y, open_text, strlen(open_text));
    fs_open_x2 = fs_open_x1 + (font ? XTextWidth(font, open_text, (int)strlen(open_text)) : 150);
}

char* run_file_picker(Display *dpy, Window win, GC gc, XFontStruct *font, int *win_w, int *win_h) {
    if (fm_cwd[0] == '\0') getcwd(fm_cwd, sizeof(fm_cwd));
    load_files(fm_cwd);

    XEvent ev;
    char *result_path = NULL;

    while (1) {
        draw_fm(dpy, win, gc, font, *win_w, *win_h);
        XNextEvent(dpy, &ev);

        int line_height = font ? font->ascent + font->descent + 2 : 14;
        
        int usable_h = *win_h - ui_height;
        int visible_lines = usable_h / line_height;

        if (ev.type == Expose) {
            /* Only drawn in the next iteration of the loop */
        } 
        else if (ev.type == ConfigureNotify) {
            *win_w = ev.xconfigure.width;
            *win_h = ev.xconfigure.height;
        } 
        else if (ev.type == KeyPress) {
            KeySym key = XLookupKeysym(&ev.xkey, 0);
            if (key == XK_q || key == XK_Escape) {
                break;
            } 
            else if (key == XK_Up) {
                if (fm_selected > 0) {
                    fm_selected--;
                    if (fm_selected < fm_scroll_offset) fm_scroll_offset--;
                }
            } 
            else if (key == XK_Down) {
                if (fm_selected < fm_file_count - 1) {
                    fm_selected++;
                    if (fm_selected >= fm_scroll_offset + visible_lines) fm_scroll_offset++;
                }
            } 
            else if (key == XK_Return) {
                char path[1024];
                snprintf(path, sizeof(path), "%s/%s", fm_cwd, fm_files[fm_selected]);
                struct stat st;
                if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    chdir(path);
                    getcwd(fm_cwd, sizeof(fm_cwd));
                    load_files(fm_cwd);
                } else {
                    result_path = strdup(path);
                    break;
                }
            }
        } 
        else if (ev.type == ButtonPress) {
            if (ev.xbutton.button == Button1) {
                /* First check if the bottom bar UI elements are pressed */
                FsRegion hit = fs_hit_region(ev.xbutton.x, ev.xbutton.y);

                if (hit == FS_REGION_CANCEL) {
                    /* Cancel immediately (equivalent to Esc/q) */
                    break;
                } 
                else if (hit == FS_REGION_OPEN) {
                    /* Open the selected file or folder (equivalent to Enter) */
                    if (fm_file_count > 0 && fm_selected >= 0 && fm_selected < fm_file_count) {
                        char path[1024];
                        snprintf(path, sizeof(path), "%s/%s", fm_cwd, fm_files[fm_selected]);
                        struct stat st;
                        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                            chdir(path);
                            getcwd(fm_cwd, sizeof(fm_cwd));
                            load_files(fm_cwd);
                        } else {
                            result_path = strdup(path);
                            break;
                        }
                    }
                } 
                else {
                   /* Nothing hit at the bottom: Handle clicks on the file list as normal */
                    int idx = ev.xbutton.y / line_height;
                    if (idx < visible_lines) {
                        int file_idx = idx + fm_scroll_offset;
                        if (file_idx >= 0 && file_idx < fm_file_count) {
                            if (fm_selected == file_idx) {
                                char path[1024];
                                snprintf(path, sizeof(path), "%s/%s", fm_cwd, fm_files[fm_selected]);
                                struct stat st;
                                if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                                    chdir(path);
                                    getcwd(fm_cwd, sizeof(fm_cwd));
                                    load_files(fm_cwd);
                                } else {
                                    result_path = strdup(path);
                                    break;
                                }
                            } else {
                                fm_selected = file_idx;
                            }
                        }
                    }
                }
            } 
            else if (ev.xbutton.button == Button4) { 
                if (fm_scroll_offset > 0) fm_scroll_offset--;
            } 
            else if (ev.xbutton.button == Button5) { 
                if (fm_scroll_offset < fm_file_count - visible_lines) {
                    fm_scroll_offset++;
                }
            }
        }
    }
    int i;
    for (i = 0; i < fm_file_count; i++) free(fm_files[i]);
    fm_file_count = 0;

    return result_path;
}
//filemanager end

/* 
* Analogous to: data = stbi_load(filename, w, h, channels, 3);
* Returns an RGB pixel buffer (3 bytes per pixel) or NULL on error.
* IMPORTANT: The output must be freed with WebPFree() instead of stbi_image_free().
*/
#ifdef WEBP
unsigned char* stbi_load_webp_rgb(const char* filename, int* w, int* h, int* channels) {
    FILE* f = NULL;
    long file_size = 0;
    unsigned char* file_buffer = NULL;
    unsigned char* pixel_buffer = NULL;

    if (!filename || !w || !h || !channels) {
        return NULL;
    }

    // 1. open file
    f = fopen(filename, "rb");
    if (!f) {
        return NULL;
    }

    // 2. find filesize
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    file_size = ftell(f);
    if (file_size <= 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    // 3. allocate temp buffer for raw WebP-filedata
    file_buffer = (unsigned char*)malloc(file_size);
    if (!file_buffer) {
        fclose(f);
        return NULL;
    }

    // 4. read file to memory
    if (fread(file_buffer, 1, file_size, f) != (size_t)file_size) {
        free(file_buffer);
        fclose(f);
        return NULL;
    }
    fclose(f);

    // 5. Check WebP features to see if the image has an alpha channel
    WebPBitstreamFeatures features;
    if (WebPGetFeatures(file_buffer, (size_t)file_size, &features) != VP8_STATUS_OK) {
        free(file_buffer);
        return NULL; // Not a valid WebP file or corrupt header
    }

    *w = features.width;
    *h = features.height;

    // 6. Set the number of channels dynamically based on whether there is transparency
    if (features.has_alpha) {
        *channels = 4;
        // 7. Afkod direkte til RGBA-pixelbuffer
        pixel_buffer = WebPDecodeRGBA(file_buffer, (size_t)file_size, w, h);
    } else {
        *channels = 3;
        // 7. Decode directly to RGB pixel buffer
        pixel_buffer = WebPDecodeRGB(file_buffer, (size_t)file_size, w, h);
    }

    // 8. release temp filebuffer and return pixeldata
    free(file_buffer);
    return pixel_buffer;
}
#endif

/* Reads 2 bytes correctly into an array and swaps to Big-Endian */
static uint16_t sgi_read_ushort(FILE *f) {
    unsigned char b[2];
    if (fread(b, 1, 2, f) != 2) return 0;
    return (uint16_t)((b[0] << 8) | b[1]);
}

/* SGI RLE Scanline expander */
static void sgi_expand_rle_row(unsigned char *src, unsigned char *dst, int xsize) {
    int x = 0;
    while (x < xsize) {
        unsigned char pixel = *src++;
        int count = pixel & 0x7f;
        if (!count) break;
        if (pixel & 0x80) {
            while (count-- && x < xsize) {
                dst[x++] = *src++;
            }
        } else {
            unsigned char val = *src++;
            while (count-- && x < xsize) {
                dst[x++] = val;
            }
        }
    }
}

unsigned char* load_sgi_to_rgb(const char* filename, int* w, int* h) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    /* 1. Read and check Magic Number (SGI is always 474 / 0x01DA) */
    uint16_t magic = sgi_read_ushort(f);
    if (magic != 474) {
        fclose(f);
        return NULL;
    }

    unsigned char storage = (unsigned char)fgetc(f); /* 0 = Raw, 1 = RLE */
    unsigned char bpc = (unsigned char)fgetc(f);     /* Bytes per channel (must be 1) */
    
    uint16_t dimension = sgi_read_ushort(f);
    (void)dimension;
    uint16_t xsize = sgi_read_ushort(f);
    uint16_t ysize = sgi_read_ushort(f);
    uint16_t zsize = sgi_read_ushort(f); /* Channels: 1=Grayscale, 3=RGB, 4=RGBA */

    if (bpc != 1 || xsize <= 0 || ysize <= 0 || (zsize != 1 && zsize != 3 && zsize != 4)) {
        fclose(f);
        return NULL;
    }

    /* Skip the rest of the 512-byte header */
    fseek(f, 512, SEEK_SET);

    /* Allocate standard RGB buffer (3 bytes per pixel) */
    unsigned char *rgb_data = (unsigned char*)malloc((size_t)xsize * ysize * 3);
    if (!rgb_data) {
        fclose(f);
        return NULL;
    }

    *w = xsize;
    *h = ysize;

    /* Temporary buffer for a single channel plane (1 byte per pixel per row) */
    unsigned char *channel_row = (unsigned char*)malloc(xsize);
    if (!channel_row) {
        free(rgb_data);
        fclose(f);
        return NULL;
    }

    if (storage == 0) {
        /* --- UNCOMPRESSED (RAW) SETUP --- */
		/* SGI saves plan-wise: All R rows, then all G rows, then all B rows */
        int c, y, x;
        for (c = 0; c < zsize; ++c) {
            for (y = 0; y < ysize; ++y) {
                if (fread(channel_row, 1, xsize, f) != xsize) goto error_cleanup;
                
                /* SGI is bottom-up, flip the y-axis to your top-down format */
                int target_y = ysize - 1 - y;
                
                for (x = 0; x < xsize; ++x) {
                    int dst_idx = (target_y * xsize + x) * 3;
                    if (zsize >= 3) {
                        if (c < 3) rgb_data[dst_idx + c] = channel_row[x];
                    } else {
                        /* Grayscale: Copy to R, G and B */
                        rgb_data[dst_idx + 0] = channel_row[x];
                        rgb_data[dst_idx + 1] = channel_row[x];
                        rgb_data[dst_idx + 2] = channel_row[x];
                    }
                }
            }
        }
    } else {
        /* --- RLE COMPRESSED SETUP --- */
        int num_rows = ysize * zsize;
        uint32_t *start_tab = (uint32_t*)malloc((size_t)num_rows * sizeof(uint32_t));
        uint32_t *length_tab = (uint32_t*)malloc((size_t)num_rows * sizeof(uint32_t));
        
        int i;
        for (i = 0; i < num_rows; ++i) {
            unsigned char b[4];
            if (fread(b, 1, 4, f) != 4) goto rle_error;
            start_tab[i] = (uint32_t)((b[0]<<24)|(b[1]<<16)|(b[2]<<8)|b[3]);
        }
        for (i = 0; i < num_rows; ++i) {
            unsigned char b[4];
            if (fread(b, 1, 4, f) != 4) goto rle_error;
            length_tab[i] = (uint32_t)((b[0]<<24)|(b[1]<<16)|(b[2]<<8)|b[3]);
        }

        unsigned char *rle_buf = NULL;
        uint32_t max_len = 0;
        for (i = 0; i < num_rows; ++i) {
            if (length_tab[i] > max_len) max_len = length_tab[i];
        }
        rle_buf = (unsigned char*)malloc(max_len);

        int c, y, x;
        for (c = 0; c < zsize; ++c) {
            for (y = 0; y < ysize; ++y) {
                int row_idx = y + c * ysize;
                fseek(f, (long)start_tab[row_idx], SEEK_SET);
                if (fread(rle_buf, 1, length_tab[row_idx], f) != length_tab[row_idx]) {
                    free(rle_buf);
                    goto rle_error;
                }

                sgi_expand_rle_row(rle_buf, channel_row, xsize);
                int target_y = ysize - 1 - y;

                for (x = 0; x < xsize; ++x) {
                    int dst_idx = (target_y * xsize + x) * 3;
                    if (zsize >= 3) {
                        if (c < 3) rgb_data[dst_idx + c] = channel_row[x];
                    } else {
                        rgb_data[dst_idx + 0] = channel_row[x];
                        rgb_data[dst_idx + 1] = channel_row[x];
                        rgb_data[dst_idx + 2] = channel_row[x];
                    }
                }
            }
        }
        free(rle_buf);
        free(start_tab);
        free(length_tab);
        goto success;

    rle_error:
        free(start_tab);
        free(length_tab);
        goto error_cleanup;
    }

success:
    free(channel_row);
    fclose(f);
    return rgb_data;

error_cleanup:
    free(channel_row);
    free(rgb_data);
    fclose(f);
    return NULL;
}

static int next_token(FILE *f, char *buf, int size) {
    int c;

    if (size <= 0)
        return 0;

    for (;;) {
        do {
            c = fgetc(f);
        } while (c != EOF && isspace((unsigned char)c));

        if (c == '#') {
            while (c != EOF && c != '\n')
                c = fgetc(f);
            continue;
        }
        break;
    }

    if (c == EOF)
        return 0;

    int i = 0;
    while (c != EOF && !isspace((unsigned char)c) && c != '#') {
        if (i < size - 1)
            buf[i++] = (char)c;
        c = fgetc(f);
    }
    buf[i] = '\0';

    if (c == '#') {
        while (c != EOF && c != '\n')
            c = fgetc(f);
    }

    return 1;
}

static int read_pnm_header(FILE *f, char *magic, int *width, int *height, int *maxval) {
    char tok[64];

    if (!next_token(f, tok, sizeof(tok)))
        return 0;
    strncpy(magic, tok, 7);
    magic[7] = '\0';

    if (!next_token(f, tok, sizeof(tok)))
        return 0;
    *width = atoi(tok);

    if (!next_token(f, tok, sizeof(tok)))
        return 0;
    *height = atoi(tok);

    if (strcmp(magic, "P1") == 0 || strcmp(magic, "P4") == 0) {
        *maxval = 1;
        return (*width > 0 && *height > 0);
    }

    if (!next_token(f, tok, sizeof(tok)))
        return 0;
    *maxval = atoi(tok);

    return (*width > 0 && *height > 0 && *maxval > 0 && *maxval <= 255);
}
//PPM P1-P6
unsigned char *load_pnm_to_rgb(const char *filename, int *width, int *height) {
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;

    char magic[8];
    int maxval = 0;

    if (!read_pnm_header(f, magic, width, height, &maxval)) {
        fclose(f);
        return NULL;
    }

    size_t pixels = (size_t)(*width) * (size_t)(*height);
    unsigned char *rgb = (unsigned char *)malloc(pixels * 3);
    if (!rgb) {
        fclose(f);
        return NULL;
    }

    if (strcmp(magic, "P1") == 0) {
		size_t i ;
        for (i = 0; i < pixels; i++) {
            char tok[16];
            if (!next_token(f, tok, sizeof(tok))) {
                free(rgb);
                fclose(f);
                return NULL;
            }
            int v = atoi(tok) ? 0 : 255;
            rgb[i * 3 + 0] = (unsigned char)v;
            rgb[i * 3 + 1] = (unsigned char)v;
            rgb[i * 3 + 2] = (unsigned char)v;
        }
    } else if (strcmp(magic, "P2") == 0) {
		size_t i;
        for (i = 0; i < pixels; i++) {
            char tok[16];
            if (!next_token(f, tok, sizeof(tok))) {
                free(rgb);
                fclose(f);
                return NULL;
            }
            int g = atoi(tok);
            g = (g * 255) / maxval;
            rgb[i * 3 + 0] = (unsigned char)g;
            rgb[i * 3 + 1] = (unsigned char)g;
            rgb[i * 3 + 2] = (unsigned char)g;
        }
    } else if (strcmp(magic, "P3") == 0) {
		size_t i;
        for (i = 0; i < pixels; i++) {
            char tok[16];
            int r, g, b;

            if (!next_token(f, tok, sizeof(tok))) {
                free(rgb);
                fclose(f);
                return NULL;
            }
            r = (atoi(tok) * 255) / maxval;

            if (!next_token(f, tok, sizeof(tok))) {
                free(rgb);
                fclose(f);
                return NULL;
            }
            g = (atoi(tok) * 255) / maxval;

            if (!next_token(f, tok, sizeof(tok))) {
                free(rgb);
                fclose(f);
                return NULL;
            }
            b = (atoi(tok) * 255) / maxval;

            rgb[i * 3 + 0] = (unsigned char)r;
            rgb[i * 3 + 1] = (unsigned char)g;
            rgb[i * 3 + 2] = (unsigned char)b;
        }
    } else if (strcmp(magic, "P4") == 0) {
        int rowbytes = (*width + 7) / 8;
        size_t bytes = (size_t)rowbytes * (size_t)(*height);
        unsigned char *bits = (unsigned char *)malloc(bytes);

        if (!bits) {
            free(rgb);
            fclose(f);
            return NULL;
        }

        if (fread(bits, 1, bytes, f) != bytes) {
            free(bits);
            free(rgb);
            fclose(f);
            return NULL;
        }

        size_t p = 0;
        int y;
        int x;
        for (y = 0; y < *height; y++) {
            unsigned char *row = bits + (size_t)y * rowbytes;
            for (x = 0; x < *width; x++) {
                int byte = row[x / 8];
                int bit = 7 - (x % 8);
                unsigned char v = (byte & (1 << bit)) ? 0 : 255;
                rgb[p++] = v;
                rgb[p++] = v;
                rgb[p++] = v;
            }
        }

        free(bits);
    } else if (strcmp(magic, "P5") == 0) {
		size_t i;
        for (i = 0; i < pixels; i++) {
            int g = fgetc(f);
            if (g == EOF) {
                free(rgb);
                fclose(f);
                return NULL;
            }
            g = (g * 255) / maxval;
            rgb[i * 3 + 0] = (unsigned char)g;
            rgb[i * 3 + 1] = (unsigned char)g;
            rgb[i * 3 + 2] = (unsigned char)g;
        }
    } else if (strcmp(magic, "P6") == 0) {
        size_t size = pixels * 3;
        if (fread(rgb, 1, size, f) != size) {
            free(rgb);
            fclose(f);
            return NULL;
        }

        if (maxval != 255) {
			size_t i;
            for (i = 0; i < size; i++)
                rgb[i] = (unsigned char)((rgb[i] * 255) / maxval);
        }
    } else {
        free(rgb);
        fclose(f);
        return NULL;
    }

    fclose(f);
    return rgb;
}

static int pcx_rle_read_byte(FILE *f, int *value) {
    int c = fgetc(f);
    if (c == EOF)
        return 0;

    if ((c & 0xC0) == 0xC0) {
        int count = c & 0x3F;
        int v = fgetc(f);
        if (v == EOF)
            return 0;
        *value = v;
        return count;
    }

    *value = c;
    return 1;
}

static uint16_t read_le16(const unsigned char *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

unsigned char* load_pcx_to_rgb(const char* filename, int* width, int* height) {
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;

    unsigned char h[128];
    if (fread(h, 1, sizeof(h), f) != sizeof(h)) {
        fclose(f);
        return NULL;
    }

    if (h[0] != 0x0A || h[2] != 1) {
        fclose(f);
        return NULL;
    }

    int bits_per_pixel = h[3];
    int xmin = read_le16(&h[4]);
    int ymin = read_le16(&h[6]);
    int xmax = read_le16(&h[8]);
    int ymax = read_le16(&h[10]);
    int nplanes = h[65];
    int bytes_per_line = read_le16(&h[66]);

    int w = xmax - xmin + 1;
    int hgt = ymax - ymin + 1;

    if (w <= 0 || hgt <= 0 || bytes_per_line <= 0 || nplanes <= 0) {
        fclose(f);
        return NULL;
    }

    unsigned char *rgb = (unsigned char *)malloc((size_t)w * (size_t)hgt * 3);
    if (!rgb) {
        fclose(f);
        return NULL;
    }

    if (bits_per_pixel == 8 && nplanes == 1) {
        //long palette_pos;	//must be there despite gcc says its unused TODO CHECK
        unsigned char palette[256 * 3];
        int has_palette = 0;

        if (fseek(f, -769, SEEK_END) == 0) {
            int marker = fgetc(f);
            if (marker == 12) {
                if (fread(palette, 1, sizeof(palette), f) == sizeof(palette))
                    has_palette = 1;
            }
        }

        fseek(f, 128, SEEK_SET);

        unsigned char *scan = (unsigned char *)malloc((size_t)bytes_per_line);
        if (!scan) {
            free(rgb);
            fclose(f);
            return NULL;
        }
		int y;
        for (y = 0; y < hgt; y++) {
            //int out_x = 0;	//must be there despite gcc says its unused TODO CHECK
            int got = 0;

            while (got < bytes_per_line) {
                int v, count = pcx_rle_read_byte(f, &v);
                if (count <= 0) {
                    free(scan);
                    free(rgb);
                    fclose(f);
                    return NULL;
                }
                while (count-- && got < bytes_per_line)
                    scan[got++] = (unsigned char)v;
            }
			int x;
            for (x = 0; x < w; x++) {
                unsigned char idx = scan[x];
                unsigned char r, g, b;

                if (has_palette) {
                    r = palette[idx * 3 + 0];
                    g = palette[idx * 3 + 1];
                    b = palette[idx * 3 + 2];
                } else {
                    unsigned char v = idx;
                    r = g = b = v;
                }

                size_t p = ((size_t)y * (size_t)w + (size_t)x) * 3;
                rgb[p + 0] = r;
                rgb[p + 1] = g;
                rgb[p + 2] = b;
            }
        }

        free(scan);
        fclose(f);
        *width = w;
        *height = hgt;
        return rgb;
    }

    if (bits_per_pixel == 8 && nplanes == 3) {
        unsigned char *planes = (unsigned char *)malloc((size_t)bytes_per_line * 3);
        if (!planes) {
            free(rgb);
            fclose(f);
            return NULL;
        }

        fseek(f, 128, SEEK_SET);
		int y;
		int plane;
        for (y = 0; y < hgt; y++) {
            for (plane = 0; plane < 3; plane++) {
                int got = 0;
                while (got < bytes_per_line) {
                    int v, count = pcx_rle_read_byte(f, &v);
                    if (count <= 0) {
                        free(planes);
                        free(rgb);
                        fclose(f);
                        return NULL;
                    }
                    while (count-- && got < bytes_per_line)
                        planes[plane * bytes_per_line + got++] = (unsigned char)v;
                }
            }
			int x;
            for (x = 0; x < w; x++) {
                size_t p = ((size_t)y * (size_t)w + (size_t)x) * 3;
                rgb[p + 0] = planes[0 * bytes_per_line + x];
                rgb[p + 1] = planes[1 * bytes_per_line + x];
                rgb[p + 2] = planes[2 * bytes_per_line + x];
            }
        }

        free(planes);
        fclose(f);
        *width = w;
        *height = hgt;
        return rgb;
    }

    free(rgb);
    fclose(f);
    return NULL;
}

unsigned char* load_xbm_to_rgb(const char* filename, int* width, int* height) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;

    char line[512];
    int w = 0, h = 0;
    unsigned char* bits = NULL;
    int bits_count = 0, bits_capacity = 0;

    // 1. read line
    while (fgets(line, sizeof(line), f)) {

        if (strstr(line, "width")) {
            char* p = strrchr(line, ' ');
            if (p) w = atoi(p + 1);
        }
        if (strstr(line, "height")) {
            char* p = strrchr(line, ' ');
            if (p) h = atoi(p + 1);
        }
        
        if (strstr(line, "_bits") && strchr(line, '{')) {
            if (w <= 0 || h <= 0) break; 

            bits_capacity = ((w + 7) / 8) * h;
            bits = malloc(bits_capacity);
            if (!bits) break;

            unsigned int val;

            while (fscanf(f, " 0x%x ,", &val) == 1 && bits_count < bits_capacity) {
                bits[bits_count++] = (unsigned char)val;
            }
            break;
        }
    }
    fclose(f);

    if (!bits || bits_count < bits_capacity || w <= 0 || h <= 0) {
        if (bits) free(bits);
        return NULL;
    }

    // 2. convert 1-bit to 24-bit RGB formate
    unsigned char* rgb = malloc(w * h * 3);
    int bytes_per_line = ((w + 7) / 8);
	
	int y;
	int x;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int byte_idx = y * bytes_per_line + (x / 8);
            int bit_idx = x % 8;
            int pixel_black = (bits[byte_idx] >> bit_idx) & 1;

            int rgb_idx = (y * w + x) * 3;
            unsigned char color = pixel_black ? 0 : 255; // black on white background
            
            rgb[rgb_idx + 0] = color; // R
            rgb[rgb_idx + 1] = color; // G
            rgb[rgb_idx + 2] = color; // B
        }
    }

    free(bits);
    *width = w;
    *height = h;
    return rgb;
}

unsigned char* load_xpm_to_rgba(const char* filename, int* width, int* height, int* channels) {
    XpmImage xpm_img;
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) return NULL;

    if (XpmReadFileToXpmImage((char*)filename, &xpm_img, NULL) != XpmSuccess) {
        XCloseDisplay(dpy);
        return NULL;
    }

    int w = xpm_img.width;
    int h = xpm_img.height;
    
    // 1. Allocate space for 4 channels (RGBA) instead of 3
    unsigned char* rgba = (unsigned char*)malloc((size_t)w * h * 4);
    
    // We now store both the RGB color (in the top bits) and the alpha value in our temporary palette
    unsigned long* palette = (unsigned long*)malloc((size_t)xpm_img.ncolors * sizeof(unsigned long));
    
    if (!rgba || !palette) {
        free(rgba);
        free(palette);
        XpmFreeXpmImage(&xpm_img);
        XCloseDisplay(dpy);
        return NULL;
    }

    Colormap cmap = DefaultColormap(dpy, DefaultScreen(dpy));
    unsigned int i;
    for (i = 0; i < xpm_img.ncolors; i++) {
		XColor xcol;
        // Check if this color in the XPM file is marked as transparent ("None")
        if (xpm_img.colorTable[i].c_color && 
            strcasecmp(xpm_img.colorTable[i].c_color, "None") == 0) {
            // Set RGB to 0 and Alpha to 0 (transparent)
            palette[i] = 0x00000000; 
        } else if (xpm_img.colorTable[i].c_color &&
                   XParseColor(dpy, cmap, xpm_img.colorTable[i].c_color, &xcol)) {
            XColor xcol;
            XParseColor(dpy, cmap, xpm_img.colorTable[i].c_color, &xcol);
            
            unsigned char r = xcol.red >> 8;
            unsigned char g = xcol.green >> 8;
            unsigned char b = xcol.blue >> 8;
            unsigned char a = 255; // Completely opaque color
            
            palette[i] = ((unsigned long)r << 24) | 
                         ((unsigned long)g << 16) | 
                         ((unsigned long)b << 8)  | 
                         a;
        } else {
           // Fallback if the color cannot be interpreted (Black, opaque)
            palette[i] = 0x000000FF; 
        }
    }

    int y;
    int x;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            unsigned int pixel_data = xpm_img.data[y * w + x];
            unsigned long color = (pixel_data < xpm_img.ncolors) ? palette[pixel_data] : 0x000000FF;
            
            // Index springer nu med 4 bytes pr. pixel
            int idx = (y * w + x) * 4;
            rgba[idx + 0] = (color >> 24) & 0xFF; // R
            rgba[idx + 1] = (color >> 16) & 0xFF; // G
            rgba[idx + 2] = (color >> 8) & 0xFF;  // B
            rgba[idx + 3] = color & 0xFF;         // A (Alpha channel!)
        }
    }

    free(palette);
    XpmFreeXpmImage(&xpm_img);
    XCloseDisplay(dpy);
    
    *width = w;
    *height = h;
    *channels = 4; // Tell the program that the image has 4 channels
    return rgba;
}

unsigned char* load_xwd_to_rgb(const char* filename, int* width, int* height) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;

    unsigned char hdr_buf[100];
    if (fread(hdr_buf, 1, sizeof(hdr_buf), f) != sizeof(hdr_buf)) {
        fclose(f);
        return NULL;
    }

    int swap = 0;
    unsigned long file_version = *(unsigned long*)(hdr_buf + 4);
    if (file_version != XWD_FILE_VERSION) {
        unsigned long swapped = ((file_version & 0x000000FFUL) << 24) |
                                ((file_version & 0x0000FF00UL) << 8) |
                                ((file_version & 0x00FF0000UL) >> 8) |
                                ((file_version & 0xFF000000UL) >> 24);
        if (swapped != XWD_FILE_VERSION) {
            fclose(f);
            return NULL;
        }
        swap = 1;
    }

    fseek(f, 0, SEEK_SET);

    XWDFileHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return NULL;
    }

    if (swap) {
        header.header_size = ((header.header_size & 0x000000FFUL) << 24) |
                             ((header.header_size & 0x0000FF00UL) << 8) |
                             ((header.header_size & 0x00FF0000UL) >> 8) |
                             ((header.header_size & 0xFF000000UL) >> 24);
        header.file_version = XWD_FILE_VERSION;
        header.pixmap_format = ((header.pixmap_format & 0x000000FFUL) << 24) |
                               ((header.pixmap_format & 0x0000FF00UL) << 8) |
                               ((header.pixmap_format & 0x00FF0000UL) >> 8) |
                               ((header.pixmap_format & 0xFF000000UL) >> 24);
        header.pixmap_depth = ((header.pixmap_depth & 0x000000FFUL) << 24) |
                              ((header.pixmap_depth & 0x0000FF00UL) << 8) |
                              ((header.pixmap_depth & 0x00FF0000UL) >> 8) |
                              ((header.pixmap_depth & 0xFF000000UL) >> 24);
        header.pixmap_width = ((header.pixmap_width & 0x000000FFUL) << 24) |
                              ((header.pixmap_width & 0x0000FF00UL) << 8) |
                              ((header.pixmap_width & 0x00FF0000UL) >> 8) |
                              ((header.pixmap_width & 0xFF000000UL) >> 24);
        header.pixmap_height = ((header.pixmap_height & 0x000000FFUL) << 24) |
                               ((header.pixmap_height & 0x0000FF00UL) << 8) |
                               ((header.pixmap_height & 0x00FF0000UL) >> 8) |
                               ((header.pixmap_height & 0xFF000000UL) >> 24);
        header.xoffset = ((header.xoffset & 0x000000FFUL) << 24) |
                         ((header.xoffset & 0x0000FF00UL) << 8) |
                         ((header.xoffset & 0x00FF0000UL) >> 8) |
                         ((header.xoffset & 0xFF000000UL) >> 24);
        header.byte_order = ((header.byte_order & 0x000000FFUL) << 24) |
                            ((header.byte_order & 0x0000FF00UL) << 8) |
                            ((header.byte_order & 0x00FF0000UL) >> 8) |
                            ((header.byte_order & 0xFF000000UL) >> 24);
        header.bitmap_unit = ((header.bitmap_unit & 0x000000FFUL) << 24) |
                             ((header.bitmap_unit & 0x0000FF00UL) << 8) |
                             ((header.bitmap_unit & 0x00FF0000UL) >> 8) |
                             ((header.bitmap_unit & 0xFF000000UL) >> 24);
        header.bitmap_bit_order = ((header.bitmap_bit_order & 0x000000FFUL) << 24) |
                                  ((header.bitmap_bit_order & 0x0000FF00UL) << 8) |
                                  ((header.bitmap_bit_order & 0x00FF0000UL) >> 8) |
                                  ((header.bitmap_bit_order & 0xFF000000UL) >> 24);
        header.bitmap_pad = ((header.bitmap_pad & 0x000000FFUL) << 24) |
                            ((header.bitmap_pad & 0x0000FF00UL) << 8) |
                            ((header.bitmap_pad & 0x00FF0000UL) >> 8) |
                            ((header.bitmap_pad & 0xFF000000UL) >> 24);
        header.bits_per_pixel = ((header.bits_per_pixel & 0x000000FFUL) << 24) |
                                ((header.bits_per_pixel & 0x0000FF00UL) << 8) |
                                ((header.bits_per_pixel & 0x00FF0000UL) >> 8) |
                                ((header.bits_per_pixel & 0xFF000000UL) >> 24);
        header.bytes_per_line = ((header.bytes_per_line & 0x000000FFUL) << 24) |
                                ((header.bytes_per_line & 0x0000FF00UL) << 8) |
                                ((header.bytes_per_line & 0x00FF0000UL) >> 8) |
                                ((header.bytes_per_line & 0xFF000000UL) >> 24);
        header.visual_class = ((header.visual_class & 0x000000FFUL) << 24) |
                              ((header.visual_class & 0x0000FF00UL) << 8) |
                              ((header.visual_class & 0x00FF0000UL) >> 8) |
                              ((header.visual_class & 0xFF000000UL) >> 24);
        header.red_mask = ((header.red_mask & 0x000000FFUL) << 24) |
                          ((header.red_mask & 0x0000FF00UL) << 8) |
                          ((header.red_mask & 0x00FF0000UL) >> 8) |
                          ((header.red_mask & 0xFF000000UL) >> 24);
        header.green_mask = ((header.green_mask & 0x000000FFUL) << 24) |
                            ((header.green_mask & 0x0000FF00UL) << 8) |
                            ((header.green_mask & 0x00FF0000UL) >> 8) |
                            ((header.green_mask & 0xFF000000UL) >> 24);
        header.blue_mask = ((header.blue_mask & 0x000000FFUL) << 24) |
                           ((header.blue_mask & 0x0000FF00UL) << 8) |
                           ((header.blue_mask & 0x00FF0000UL) >> 8) |
                           ((header.blue_mask & 0xFF000000UL) >> 24);
        header.bits_per_rgb = ((header.bits_per_rgb & 0x000000FFUL) << 24) |
                              ((header.bits_per_rgb & 0x0000FF00UL) << 8) |
                              ((header.bits_per_rgb & 0x00FF0000UL) >> 8) |
                              ((header.bits_per_rgb & 0xFF000000UL) >> 24);
        header.colormap_entries = ((header.colormap_entries & 0x000000FFUL) << 24) |
                                  ((header.colormap_entries & 0x0000FF00UL) << 8) |
                                  ((header.colormap_entries & 0x00FF0000UL) >> 8) |
                                  ((header.colormap_entries & 0xFF000000UL) >> 24);
        header.ncolors = ((header.ncolors & 0x000000FFUL) << 24) |
                         ((header.ncolors & 0x0000FF00UL) << 8) |
                         ((header.ncolors & 0x00FF0000UL) >> 8) |
                         ((header.ncolors & 0xFF000000UL) >> 24);
    }

    if (header.file_version != XWD_FILE_VERSION) {
        fclose(f);
        return NULL;
    }

    if (header.header_size < sizeof(XWDFileHeader)) {
        fclose(f);
        return NULL;
    }

    fseek(f, header.header_size, SEEK_SET);

    int w = (int)header.pixmap_width;
    int h = (int)header.pixmap_height;
    int ncolors = (int)header.ncolors;
    int bpp = (int)header.bits_per_pixel;
    int bytes_per_line = (int)header.bytes_per_line;
    if (w <= 0 || h <= 0 || bytes_per_line <= 0) {
        fclose(f);
        return NULL;
    }

    unsigned int* palette = NULL;
    if (ncolors > 0) {
        palette = (unsigned int*)calloc((size_t)ncolors, sizeof(unsigned int));
        if (!palette) {
            fclose(f);
            return NULL;
        }
        int i;
        for (i = 0; i < ncolors; i++) {
            XWDColor c;
            if (fread(&c, sizeof(c), 1, f) != 1) {
                free(palette);
                fclose(f);
                return NULL;
            }
            if (swap) {
                c.pixel = ((c.pixel & 0x000000FFUL) << 24) |
                          ((c.pixel & 0x0000FF00UL) << 8) |
                          ((c.pixel & 0x00FF0000UL) >> 8) |
                          ((c.pixel & 0xFF000000UL) >> 24);
                c.red = (unsigned short)((c.red >> 8) | (c.red << 8));
                c.green = (unsigned short)((c.green >> 8) | (c.green << 8));
                c.blue = (unsigned short)((c.blue >> 8) | (c.blue << 8));
            }
            palette[i] = ((c.red >> 8) << 16) | ((c.green >> 8) << 8) | (c.blue >> 8);
        }
    }

    long pixel_bytes = (long)bytes_per_line * h;
    unsigned char* raw = (unsigned char*)malloc((size_t)pixel_bytes);
    unsigned char* rgb = (unsigned char*)malloc((size_t)w * h * 3);
    if (!raw || !rgb) {
        free(palette);
        free(raw);
        free(rgb);
        fclose(f);
        return NULL;
    }

    if (fread(raw, 1, (size_t)pixel_bytes, f) != (size_t)pixel_bytes) {
        free(palette);
        free(raw);
        free(rgb);
        fclose(f);
        return NULL;
    }
    fclose(f);

    int bytes_per_pixel = (bpp + 7) / 8;
    if (bytes_per_pixel != 1 && bytes_per_pixel != 2 && bytes_per_pixel != 3 && bytes_per_pixel != 4) {
        free(palette);
        free(raw);
        free(rgb);
        return NULL;
    }
	int y;
	int x;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            unsigned int color = 0;
            int idx = y * bytes_per_line + x * bytes_per_pixel;

            if (bpp == 1 && palette) {
                int bit = (raw[y * bytes_per_line + (x >> 3)] >> (x & 7)) & 1;
                color = palette[bit];
            } else if (palette && ncolors > 0 && bpp <= 8) {
                color = palette[raw[idx]];
            } else if (bytes_per_pixel == 4) {
                if (header.byte_order == MSBFirst) {
                    color = ((unsigned int)raw[idx + 1] << 16) |
                            ((unsigned int)raw[idx + 2] << 8) |
                            ((unsigned int)raw[idx + 3]);
                } else {
                    color = ((unsigned int)raw[idx + 2] << 16) |
                            ((unsigned int)raw[idx + 1] << 8) |
                            ((unsigned int)raw[idx + 0]);
                }
            } else if (bytes_per_pixel == 3) {
                if (header.byte_order == MSBFirst) {
                    color = ((unsigned int)raw[idx + 0] << 16) |
                            ((unsigned int)raw[idx + 1] << 8) |
                            ((unsigned int)raw[idx + 2]);
                } else {
                    color = ((unsigned int)raw[idx + 2] << 16) |
                            ((unsigned int)raw[idx + 1] << 8) |
                            ((unsigned int)raw[idx + 0]);
                }
            }

            int out = (y * w + x) * 3;
            rgb[out + 0] = (color >> 16) & 0xFF;
            rgb[out + 1] = (color >> 8) & 0xFF;
            rgb[out + 2] = color & 0xFF;
        }
    }

    free(palette);
    free(raw);
    *width = w;
    *height = h;
    return rgb;
}

//tiff
#ifdef USE_TIFF
unsigned char* load_tiff_rgba_fallback(const char* filename, int* w, int* h, int* channels) {
    TIFF* tif = TIFFOpen(filename, "r");
    if (!tif) {
        return NULL;
    }

    uint32 width, height;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    *w = (int)width;
    *h = (int)height;

    // 1. Check if the TIFF file contains an alpha channel (extra samples)
    uint16 extra_samples_count = 0;
    uint16* extra_samples_types = NULL;
    int has_alpha = 0;

    if (TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &extra_samples_count, &extra_samples_types)) {
        // If there are extra channels and the first one is marked as associated or unassociated alpha
        if (extra_samples_count > 0 && extra_samples_types != NULL) {
            if (extra_samples_types[0] == EXTRASAMPLE_ASSOCALPHA || 
                extra_samples_types[0] == EXTRASAMPLE_UNASSALPHA) {
                has_alpha = 1;
            }
        }
    }

    // Determine number of output channels based on whether there is real transparency
    int out_channels = has_alpha ? 4 : 3;
    *channels = out_channels; 

    // Allocate memory for temporary RGBA buffer (4 bytes per pixel, which libtiff requires)
    uint32* raster = (uint32*)_TIFFmalloc(width * height * sizeof(uint32));
    if (!raster) {
        TIFFClose(tif);
        return NULL;
    }

    // Read the image directly as RGBA with top-left orientation
    if (!TIFFReadRGBAImageOriented(tif, width, height, raster, ORIENTATION_TOPLEFT, 0)) {
        _TIFFfree(raster);
        TIFFClose(tif);
        return NULL;
    }

    unsigned char* data = (unsigned char*)malloc(width * height * out_channels);
    if (!data) {
        _TIFFfree(raster);
        TIFFClose(tif);
        return NULL;
    }

    // Convert from uint32 raster to our output buffer
    uint32 i;
    for (i = 0; i < width * height; i++) {
        uint32 pixel = raster[i];
        
        data[i * out_channels + 0] = (unsigned char)(TIFFGetR(pixel));
        data[i * out_channels + 1] = (unsigned char)(TIFFGetG(pixel));
        data[i * out_channels + 2] = (unsigned char)(TIFFGetB(pixel));
        
        if (out_channels == 4) {
            data[i * out_channels + 3] = (unsigned char)(TIFFGetA(pixel));
        }
    }

    _TIFFfree(raster);
    TIFFClose(tif);
    return data;
}
#endif //TIFF

unsigned char* minimalist_load_image(const char* filename, int* w, int* h, int* channels) {
    unsigned char* data = NULL;
    // Common starting point, but let the functions override this
    *channels = 3; 

    // 1. Try PNM (PBM/PGM/PPM) - these typically only have 3 channels
    data = load_pnm_to_rgb(filename, w, h);
    if (data != NULL) {
        *channels = 3;
        return data;
    }

    // 2. Try stb_image (PNG, JPEG, GIF, BMP, TGA)
    // By changing the last argument from 3 to 0, the image retains its original channels (RGB or RGBA)
    data = stbi_load(filename, w, h, channels, 0); 
    if (data != NULL) return data;
    

    
#ifdef WEBP   
    // Check if your stbi_load_webp_rgb forces RGB. If it does, it may need to be upgraded later
    data = stbi_load_webp_rgb(filename, w, h, channels);
    if (data != NULL) return data;
#endif	


    // 3. Try XWD
    data = load_xwd_to_rgb(filename, w, h);
    if (data != NULL) { *channels = 3; return data; }

    // 4. Try XPM Common in older X11 apps. Quick text-check for XPM
    data = load_xpm_to_rgba(filename, w, h, channels);
    if (data != NULL) return data;

    // 5. Try PCX
    data = load_pcx_to_rgb(filename, w, h);
    if (data != NULL) { *channels = 3; return data; }
    
    // 6. Try SGI/RGB
    data = load_sgi_to_rgb(filename, w, h);
    if (data != NULL) { *channels = 3; return data; }
    
	// 7. Try XBM
    data = load_xbm_to_rgb(filename, w, h);
    if (data != NULL) { *channels = 3; return data;}
    
#ifdef USE_TIFF
    data = load_tiff_rgba_fallback(filename, w, h, channels);
    if (data != NULL) return data;
#endif

	return NULL;
    
}

void save_visible_area(unsigned char* image_data, int img_w, int img_h,
                       int img_channels, int screen_w, int screen_h, float zoom,
                       int offset_x, int offset_y) {
    int out_w = screen_w;
    int out_h = screen_h - ui_height;
    
    if (out_w <= 0 || out_h <= 0) return;

	// DO YOU WANT CHECKERBOARD IN THE SAVED FILE?
	// Set to 1 if you want to save with checkerboard (3 channel output)
	// Set to 0 if you want to keep true transparency/black background (3 or 4 channel output)
    int burn_checkerboard = 0; 

    // Find out how many channels the output file should have
    int out_channels = (img_channels == 4 && !burn_checkerboard) ? 4 : 3;

    unsigned char* out_data = (unsigned char*)malloc((size_t)out_w * out_h * out_channels);
    if (!out_data) {
        fprintf(stderr, "Error: Could not allocate memory to save image\n");
        return;
    }

    int y, x;
    for (y = 0; y < out_h; ++y) {
        for (x = 0; x < out_w; ++x) {
            int src_x = offset_x + (int)(x / zoom);
            int src_y = offset_y + (int)(y / zoom);
            int dst_idx = (y * out_w + x) * out_channels;

            // Generate checkerboard color to use if we are outside the image, or if we bake it in
            unsigned char bg_r = 240; unsigned char bg_g = 240; unsigned char bg_b = 240;
            if (((x / 16) + (y / 16)) % 2 == 0) {
                bg_r = 200; bg_g = 200; bg_b = 200;
            }

            if (src_x < 0 || src_y < 0 || src_x >= img_w || src_y >= img_h) {
                // The area OUTSIDE the image's borders
                if (burn_checkerboard) {
                    out_data[dst_idx + 0] = bg_r;
                    out_data[dst_idx + 1] = bg_g;
                    out_data[dst_idx + 2] = bg_b;
                } else {
                    // If we want to preserve true transparency, we make the area outside the image transparent
                    out_data[dst_idx + 0] = 0;
                    out_data[dst_idx + 1] = 0;
                    out_data[dst_idx + 2] = 0;
                    if (out_channels == 4) out_data[dst_idx + 3] = 0; // Completely transparent background
                }
            } else {
                // The area WITHIN the image's borders
                int src_idx = (src_y * img_w + src_x) * img_channels;

                if (img_channels == 4) {
                    unsigned char src_r = image_data[src_idx + 0];
                    unsigned char src_g = image_data[src_idx + 1];
                    unsigned char src_b = image_data[src_idx + 2];
                    unsigned char src_a = image_data[src_idx + 3];

                    if (burn_checkerboard) {
                        // Blend the colors with checkerboard and save as 3-channel RGB
                        out_data[dst_idx + 0] = (src_r * src_a + bg_r * (255 - src_a)) / 255;
                        out_data[dst_idx + 1] = (src_g * src_a + bg_g * (255 - src_a)) / 255;
                        out_data[dst_idx + 2] = (src_b * src_a + bg_b * (255 - src_a)) / 255;
                    } else {
                        // Save as raw, true 4-channel RGBA
                        out_data[dst_idx + 0] = src_r;
                        out_data[dst_idx + 1] = src_g;
                        out_data[dst_idx + 2] = src_b;
                        out_data[dst_idx + 3] = src_a;
                    }
                } else {
                    // Standard 3-channel RGB source
                    out_data[dst_idx + 0] = image_data[src_idx + 0];
                    out_data[dst_idx + 1] = image_data[src_idx + 1];
                    out_data[dst_idx + 2] = image_data[src_idx + 2];
                    // If the output absolutely needs to be 4 channels (but the source is only 3), we set alpha to full
                    if (out_channels == 4) out_data[dst_idx + 3] = 255; 
                }
            }
        }
    }
	char save_path[4096];
	make_save_path(save_path, sizeof save_path);
    // We change '3' and 'out_w * 3' to use the dynamic 'out_channels'
    if (stbi_write_png(/*"output.png"*/save_path, out_w, out_h, out_channels, out_data, out_w * out_channels)) {
        //printf("Saved image as output.png (%d channels)\n", out_channels);
        printf("Saved image as %s (%d channels)\n",
           save_path, out_channels);
        
    } else {
        fprintf(stderr, "Failed to write output.png\n");
    }
    free(out_data);
}

// --- XVL DISPLAY LOGIC --- 
void draw_ui(Display* dpy, Drawable drawable, GC gc, int win_w, int win_h) {
    const char* open_text = "[o]: Open";
    const char* zoom_text = "[+/-]: Zoom";
    const char* pan_text = "[Arrows]: Pan";
    const char* save_text = "[s]: Save";
    const char* quit_text = "[q]: Quit";

    int x = 10;
    ui_text_y = win_h - ui_height + 20;
    
    ui_open_x1 = x;
	XDrawString(dpy, drawable, gc, x, ui_text_y, open_text, strlen(open_text));
	ui_open_x2 = ui_open_x1 + XTextWidth(ui_font, open_text, (int)strlen(open_text));
	
	x = ui_open_x2 + 15;
    ui_zoom_x1 = x;
    XDrawString(dpy, drawable, gc, x, ui_text_y, zoom_text, strlen(zoom_text));
    ui_zoom_x2 = ui_zoom_x1 + XTextWidth(ui_font, zoom_text, (int)strlen(zoom_text));

    x = ui_zoom_x2 + 15;
    ui_pan_x1 = x;
    XDrawString(dpy, drawable, gc, x, ui_text_y, pan_text, strlen(pan_text));
    ui_pan_x2 = ui_pan_x1 + XTextWidth(ui_font, pan_text, (int)strlen(pan_text));

    x = ui_pan_x2 + 15;
    ui_save_x1 = x;
    XDrawString(dpy, drawable, gc, x, ui_text_y, save_text, strlen(save_text));
    ui_save_x2 = ui_save_x1 + XTextWidth(ui_font, save_text, (int)strlen(save_text));

    x = ui_save_x2 + 15;
    ui_quit_x1 = x;
    XDrawString(dpy, drawable, gc, x, ui_text_y, quit_text, strlen(quit_text));
    ui_quit_x2 = ui_quit_x1 + XTextWidth(ui_font, quit_text, (int)strlen(quit_text));
}

XImage* create_scaled_ximage(Display* dpy, Visual* visual, int depth,
                             unsigned char* image_data, int img_w, int img_h,
                             int channels, int screen_w, int screen_h,
                             float zoom, int offset_x, int offset_y) {
    int disp_w = screen_w;
    int disp_h = screen_h;
    int max_image_y = disp_h - ui_height;

    XImage* img = XCreateImage(dpy, visual, depth, ZPixmap, 0, NULL, disp_w, disp_h, 32, 0);
    if (!img) return NULL;

    img->data = (char*)malloc((size_t)img->bytes_per_line * img->height);
    if (!img->data) {
        XDestroyImage(img);
        return NULL;
    }
       
    int is_truecolor = (visual->class == TrueColor || visual->class == DirectColor);

    int y;
    int x;
    for (y = 0; y < max_image_y; ++y) {
        for (x = 0; x < disp_w; ++x) {
            // 1. Generate checkerboard background for this pixel (e.g. 16x16 pixels checkerboard)
			// We alternate between a light gray and a slightly darker gray
            unsigned char bg_r = 240;
            unsigned char bg_g = 240;
            unsigned char bg_b = 240;
            if (((x / 16) + (y / 16)) % 2 == 0) {
                bg_r = 200;
                bg_g = 200;
                bg_b = 200;
            }

            int src_x = offset_x + (int)(x / zoom);
            int src_y = offset_y + (int)(y / zoom);
            
            unsigned char r = bg_r;
            unsigned char g = bg_g;
            unsigned char b = bg_b;

            // If we hit within the image's bounds, we get the color from there
            if (src_x >= 0 && src_y >= 0 && src_x < img_w && src_y < img_h) {
                int src_idx = (src_y * img_w + src_x) * channels; // Uses dynamic 'channels' (3 or 4)
                
                if (channels == 4) {
                    unsigned char src_r = image_data[src_idx + 0];
                    unsigned char src_g = image_data[src_idx + 1];
                    unsigned char src_b = image_data[src_idx + 2];
                    unsigned char src_a = image_data[src_idx + 3]; // Alpha channel (0-255)

                    if (src_a == 255) {
                        // Completely opaque
                        r = src_r; g = src_g; b = src_b;
                    } else if (src_a == 0) {
                        // Completely transparent - keep the background (checkerboard)
						// r, g, b remain bg_r, bg_g, bg_b
                    } else {
                        // Semi-transparent: Alpha blending formula
						// Blend the image color with the checkerboard background
                        r = (src_r * src_a + bg_r * (255 - src_a)) / 255;
                        g = (src_g * src_a + bg_g * (255 - src_a)) / 255;
                        b = (src_b * src_a + bg_b * (255 - src_a)) / 255;
                    }
                } else {
                    // Standard 3 channels (RGB) - no transparency
                    r = image_data[src_idx + 0];
                    g = image_data[src_idx + 1];
                    b = image_data[src_idx + 2];
                }
            }

            unsigned long pixel_val;
            if (is_truecolor) {
                pixel_val = 
                        (((unsigned long)r * (visual->red_mask   + 1) / 256) & visual->red_mask)   |
                        (((unsigned long)g * (visual->green_mask + 1) / 256) & visual->green_mask) |
                        (((unsigned long)b * (visual->blue_mask  + 1) / 256) & visual->blue_mask);
            } else if (depth == 8) {
                unsigned char r_3bit = r & 0xE0;
                unsigned char g_3bit = (g & 0xE0) >> 3;
                unsigned char b_2bit = (b & 0xC0) >> 6;
                pixel_val = r_3bit | g_3bit | b_2bit;
            } else {
                pixel_val = 0;
            }

            XPutPixel(img, x, y, pixel_val);
        }
    }

    // Fill the rest of the window (if the image area doesn't cover all the way down to the UI) with checkers
	// (This replaces your old memset(img->data, 0, ...) and ensures that empty areas also have checkers)
    for (y = max_image_y; y < disp_h; ++y) {
        for (x = 0; x < disp_w; ++x) {
            XPutPixel(img, x, y, 0); // Keeps the bottom black/clear for UI overwriting
        }
    }

    return img;
}

void redraw(Display* dpy, Window win, Pixmap buffer, GC gc,
            Visual* visual, int depth, unsigned char* image_data,
            int img_w, int img_h, int img_channels,
            int win_w, int win_h) {
    
    // We're passing img_channels along now
    XImage* ximg = create_scaled_ximage(dpy, visual, depth,
                                        image_data, img_w, img_h, img_channels,
                                        win_w, win_h, zoom, offset_x, offset_y);
    if (!ximg) return;

    XPutImage(dpy, buffer, gc, ximg, 0, 0, 0, 0, ximg->width, ximg->height);
    XDestroyImage(ximg);

    XSetForeground(dpy, gc, WhitePixel(dpy, DefaultScreen(dpy)));
    XFillRectangle(dpy, buffer, gc, 0, win_h - ui_height, win_w, ui_height);

    XSetForeground(dpy, gc, BlackPixel(dpy, DefaultScreen(dpy)));
    draw_ui(dpy, buffer, gc, win_w, win_h);

    XCopyArea(dpy, buffer, win, gc, 0, 0, win_w, win_h, 0, 0);
}

//8bit color display
void setup_8bit_colormap(Display* dpy, Window win, Visual* visual) {
    // only if dispaly is 8-bit PseudoColor
    if (visual->class != PseudoColor) return;

    //Screen* screen = DefaultScreenOfDisplay(dpy);		//unused
    Colormap cmap = XCreateColormap(dpy, win, visual, AllocAll);

    XColor colors[256];
    int i;

    // fill color table with exact 3:3:2 RGB-format
    for (i = 0; i < 256; i++) {
        colors[i].pixel = i;
        colors[i].flags = DoRed | DoGreen | DoBlue;

        // decode 3 bits red (0-7), 3 bits green (0-7), 2 bits blue (0-3)
        unsigned char r = (i & 0xE0) >> 5; // top 3 bits
        unsigned char g = (i & 0x1C) >> 2; // mid 3 bits
        unsigned char b = (i & 0x03) >> 0; // lower 2 bits

        // X11 color values from 0 til 65535 (16-bit pr. channel)
        colors[i].red   = (unsigned short)((r * 65535) / 7);
        colors[i].green = (unsigned short)((g * 65535) / 7);
        colors[i].blue  = (unsigned short)((b * 65535) / 3);
    }
    
    // force index 1 to be white
	colors[1].red   = 65535;
	colors[1].green = 65535;
	colors[1].blue  = 65535;

    // save pallet in X-server and parse to window
    XStoreColors(dpy, cmap, colors, 256);
    XSetWindowColormap(dpy, win, cmap);
}

static void write_stdout(void *context, void *data, int size)
{
    FILE *fp = (FILE *)context;

    if (size > 0)
        fwrite(data, 1, (size_t)size, fp);
}

static int convert_to_png_stdout(const char *filename)
{
    int w, h, channels;
    unsigned char *data;

    data = minimalist_load_image(filename, &w, &h, &channels);
    if (!data) {
        fprintf(stderr, "xvl: cannot load '%s'\n", filename);
        return 1;
    }

    if (!stbi_write_png_to_func(write_stdout, stdout,
                                w, h, channels,
                                data, w * channels)) {
        fprintf(stderr, "xvl: cannot write PNG to stdout\n");
        free(data);
        return 1;
    }

    free(data);

    if (fflush(stdout) == EOF || ferror(stdout)) {
        fprintf(stderr, "xvl: error writing PNG to stdout\n");
        return 1;
    }

    return 0;
}

int main(int argc, char** argv) {
    int img_w = 0, img_h = 0, img_channels = 0;
    unsigned char* image_data = NULL;
    char current_filename[512] = "No image loaded";
    
    //climode
    if (argc == 3 && strcmp(argv[1], "-c") == 0)
        return convert_to_png_stdout(argv[2]);
    
    //

    // if any argument given read as starting image
    if (argc >= 2) {
        image_data = minimalist_load_image(argv[1], &img_w, &img_h, &img_channels);
        if (image_data) {
            strncpy(current_filename, argv[1], sizeof(current_filename) - 1);
            snprintf(loaded_path, sizeof loaded_path, "%s", current_filename);
        } else {
            fprintf(stderr, "Could not read initial image: %s\n", argv[1]);
        }
    }
	
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;

    int screen = DefaultScreen(dpy);
    Visual* visual = DefaultVisual(dpy, screen);
    int depth = DefaultDepth(dpy, screen);
    int win_w = 600, win_h = 500;
	
    XSetWindowAttributes attrs;
    attrs.background_pixel = WhitePixel(dpy, screen);
    attrs.border_pixel = BlackPixel(dpy, screen);
    attrs.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask | ButtonPressMask;

    Window win = XCreateWindow(dpy, RootWindow(dpy, screen), 0, 0, 
                               win_w, win_h, 1, depth, InputOutput, 
                               visual, CWBackPixel | CWBorderPixel | CWEventMask, &attrs);
                               
    setup_8bit_colormap(dpy, win, visual);     //8bit display
			
    char window_title[1024];
    snprintf(window_title, sizeof(window_title), "xvl - %s", current_filename);
    XStoreName(dpy, win, window_title);
    XMapWindow(dpy, win);

    // set icon
	set_window_icon(dpy, win);

    GC gc = XCreateGC(dpy, win, 0, NULL);
    
    ui_font = XLoadQueryFont(dpy, "9x15");
    // 3. FALLBACK: If 9x15 is not found, try "fixed"
	if (ui_font == NULL) {
		fprintf(stderr, "Warning: Could not load font '9x15'. Using 'fixed'\n");
		ui_font = XLoadQueryFont(dpy, "fixed");
	}

	// 4. CRITICAL SAFETY: If all else fails (which is extremely rare with "fixed")
	if (ui_font == NULL) {
		fprintf(stderr, "Error: No usefull fonts found on the system - closing.\n");
		// Close the program controlled here so it doesn't crash later
		exit(1); 
	}

	// 5. Put the found font in GC
	XSetFont(dpy, gc, ui_font->fid);
    
    Pixmap buffer = XCreatePixmap(dpy, win, win_w, win_h, depth);
    
    // force start draw once
        if (!image_data) {
        XSetForeground(dpy, gc, BlackPixel(dpy, screen));
        XFillRectangle(dpy, buffer, gc, 0, 0, win_w, win_h - ui_height);
        XSetForeground(dpy, gc, WhitePixel(dpy, screen));
        XFillRectangle(dpy, buffer, gc, 0, win_h - ui_height, win_w, ui_height);
        XSetForeground(dpy, gc, BlackPixel(dpy, screen));
        draw_ui(dpy, buffer, gc, win_w, win_h);
        XCopyArea(dpy, buffer, win, gc, 0, 0, win_w, win_h, 0, 0);
    }

    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);

        int trigger_open = 0;

            if (ev.type == Expose) {
            if (image_data) {
                redraw(dpy, win, buffer, gc, visual, depth, image_data, img_w, img_h, img_channels, win_w, win_h);
            } else {
                // draw empty menu
                XSetForeground(dpy, gc, BlackPixel(dpy, screen));
                XFillRectangle(dpy, buffer, gc, 0, 0, win_w, win_h - ui_height);
                XSetForeground(dpy, gc, WhitePixel(dpy, screen));
                XFillRectangle(dpy, buffer, gc, 0, win_h - ui_height, win_w, ui_height);
                XSetForeground(dpy, gc, BlackPixel(dpy, screen));
                draw_ui(dpy, buffer, gc, win_w, win_h);
                XCopyArea(dpy, buffer, win, gc, 0, 0, win_w, win_h, 0, 0);
            }
        } 
        else if (ev.type == ConfigureNotify) {
            win_w = ev.xconfigure.width;
            win_h = ev.xconfigure.height;
            XFreePixmap(dpy, buffer);
            buffer = XCreatePixmap(dpy, win, win_w, win_h, depth);
            if (image_data) redraw(dpy, win, buffer, gc, visual, depth, image_data, img_w, img_h, img_channels, win_w, win_h);
        }
        else if (ev.type == ButtonPress) {
            UiRegion region = ui_hit_region(ev.xbutton.x, ev.xbutton.y);
			if (region == UI_REGION_QUIT && ev.xbutton.button == Button1) {
				break;
			}
            if (region == UI_REGION_OPEN && ev.xbutton.button == Button1) {
                trigger_open = 1;
            }
            else if (image_data) {
                if (region == UI_REGION_QUIT && ev.xbutton.button == Button1) break;
                if (region == UI_REGION_SAVE && ev.xbutton.button == Button1) {
                    save_visible_area(image_data, img_w, img_h, img_channels, win_w, win_h, zoom, offset_x, offset_y);
                    redraw(dpy, win, buffer, gc, visual, depth, image_data, img_w, img_h, img_channels, win_w, win_h);
                }
                if (region == UI_REGION_ZOOM) {
                    if (ev.xbutton.button == Button4) zoom_in();
                    else if (ev.xbutton.button == Button5) zoom_out();
                    redraw(dpy, win, buffer, gc, visual, depth, image_data, img_w, img_h, img_channels, win_w, win_h);
                }
                if (region == UI_REGION_PAN) {
                    if (ev.xbutton.button == Button4) pan_up();
                    else if (ev.xbutton.button == Button5) pan_down();
                    else if (ev.xbutton.button == Button1) pan_left();
                    else if (ev.xbutton.button == Button3) pan_right();
                    redraw(dpy, win, buffer, gc, visual, depth, image_data, img_w, img_h, img_channels, win_w, win_h);
                }
            }
        }
        else if (ev.type == KeyPress) {
            KeySym key = XLookupKeysym(&ev.xkey, 0);

            if (key == XK_q) break;
            else if (key == XK_o) trigger_open = 1;
            else if (image_data) {
                if (key == XK_plus || key == XK_equal || key == XK_KP_Add) zoom_in();
                else if (key == XK_minus || key == XK_underscore || key == XK_KP_Subtract) zoom_out();
                else if (key == XK_Left) pan_left();
                else if (key == XK_Right) pan_right();
                else if (key == XK_Up) pan_up();
                else if (key == XK_Down) pan_down();
                else if (key == XK_s) save_visible_area(image_data, img_w, img_h, img_channels, win_w, win_h, zoom, offset_x, offset_y);

                redraw(dpy, win, buffer, gc, visual, depth, image_data, img_w, img_h, img_channels, win_w, win_h);
            }
        }

        // filemanager trigger logic
        if (trigger_open) {
            char *chosen_file = run_file_picker(dpy, win, gc, ui_font, &win_w, &win_h);
            if (chosen_file) {
                unsigned char *new_data = minimalist_load_image(chosen_file, &img_w, &img_h, &img_channels);
                if (new_data) {
					if (image_data) free(image_data);
                    image_data = new_data;
                    zoom = 1.0f; offset_x = 0; offset_y = 0;
                    strncpy(current_filename, chosen_file, sizeof(current_filename) - 1);
                    current_filename[sizeof(current_filename) - 1] = '\0';
                    snprintf(loaded_path, sizeof loaded_path, "%s", current_filename);
                    printf("debug: %s\n", current_filename);
                    snprintf(window_title, sizeof(window_title), "xvl - %s", current_filename);
                    XStoreName(dpy, win, window_title);
                } else {
                    printf("Failed to load chosen file: %s\n", chosen_file);
                }
                free(chosen_file);
            }
            // force full redraw after closure of filemanager
            XFreePixmap(dpy, buffer);
            buffer = XCreatePixmap(dpy, win, win_w, win_h, depth);
            if (image_data) {
                redraw(dpy, win, buffer, gc, visual, depth, image_data, img_w, img_h, img_channels, win_w, win_h);
            } else {
                // Clean and redraw the empty menu if cancel was pressed without an image
                XSetForeground(dpy, gc, BlackPixel(dpy, screen));
                XFillRectangle(dpy, buffer, gc, 0, 0, win_w, win_h - ui_height);
                XSetForeground(dpy, gc, WhitePixel(dpy, screen));
                XFillRectangle(dpy, buffer, gc, 0, win_h - ui_height, win_w, ui_height);
                XSetForeground(dpy, gc, BlackPixel(dpy, screen));
                draw_ui(dpy, buffer, gc, win_w, win_h);
                XCopyArea(dpy, buffer, win, gc, 0, 0, win_w, win_h, 0, 0);
            }
        }
    }

    XFreeFont(dpy, ui_font);
    XFreeGC(dpy, gc);
    XFreePixmap(dpy, buffer);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    if (image_data) free(image_data);
    return 0;
}
