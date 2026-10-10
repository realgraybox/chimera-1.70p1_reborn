 #include <X11/xpm.h>
 #include <stdlib.h>
 #include <string.h>
 #include "common.h"
 #include "copyright.h"
 #include "xloadimage.h"
 
int parseHexColor(const char *hex, unsigned short *red, unsigned short *green, unsigned short *blue) {
    if (sscanf(hex, "#%2hx%2hx%2hx", red, green, blue) == 3) {
        *red = (*red << 8) | *red;   // Expand to 16 bits
        *green = (*green << 8) | *green;
        *blue = (*blue << 8) | *blue;
        return 1;
     }
    return 0;  // Invalid format
}
 
Image *xpixmapLoad(Display *Disp, int Scrn, unsigned char *data, int datalen, RGBColor *bg) {
    XpmImage xpm_image = {0};
    XpmInfo xpm_info = {0};
    int status = 0;

    // Convert data to a null-terminated string if necessary
    char *xpm_data = (char *)malloc(datalen + 1);
    memcpy(xpm_data, data, datalen);
    xpm_data[datalen] = '\0';

    // Load XPM data using libXpm
    status = XpmCreateXpmImageFromBuffer(xpm_data, &xpm_image, &xpm_info);
    free(xpm_data);  // Free allocated buffer

    if (status != XpmSuccess) {
        fprintf(stderr, "Failed to load XPM image: %s\n", XpmGetErrorString(status));
        return NULL;
    }
    
   	// Calculate depth dynamically
	unsigned int depth = 1;
	unsigned int colors = xpm_image.ncolors;
	while ((1U << depth) < colors) {
		depth++;
	}
	
	// Debug print for depth
	printf("Detected depth: %u bits\n", depth);

 
    // Create custom Image structure
    Image *image = newRGBImage(xpm_image.width, xpm_image.height, depth);
    if (!image) {
        fprintf(stderr, "Failed to allocate memory for image.\n");
        XpmFreeXpmImage(&xpm_image);
        return NULL;
    }

    image->rgb.used = xpm_image.ncolors;
 
    // Map colors to the custom Image structure
    unsigned int i = 0; for (i = 0; i < xpm_image.ncolors; i++) {
        const char *colorString = xpm_image.colorTable[i].c_color;
        unsigned short red, green, blue;
        if (!parseHexColor(colorString, &red, &green, &blue)) {
            fprintf(stderr, "Failed to parse color: %s\n", colorString);
            red = green = blue = 0;  // Default to black
        }
        *(image->rgb.red + i) = red;
        *(image->rgb.green + i) = green;
        *(image->rgb.blue + i) = blue;
     }
 
    // Map pixels
    unsigned int y = 0; for (y = 0; y < xpm_image.height; y++) {
        unsigned int x = 0; for (x = 0; x < xpm_image.width; x++) {
            unsigned int pixel = xpm_image.data[y * xpm_image.width + x];
            valToMem(pixel, &image->data[(y * xpm_image.width + x) * image->pixlen], image->pixlen);
        }
    }

   // Free XpmImage resources
   XpmFreeXpmImage(&xpm_image);

    return image;
}
