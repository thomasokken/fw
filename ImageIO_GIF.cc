#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ImageIO_GIF.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "main.h"
#include "util.h"

/* TO DO:
 * Better error handling.
 * GIF87a support is complete, as far as I have been able to determine.
 * For GIF89a support, it would be nice to handle the Plain Text extension,
 * and maybe honor the Graphic Control extension (e.g., if a sub-image is
 * specified to be restored to previous, don't even paint it to begin with;
 * or erase its extent if the specified behaviour is to restore to the
 * background color, etc.
 * Also, the Application Extension is made to order for using GIF as a base
 * format for some FW plugins -- basically, any that don't use 24-bit mode
 * and don't store outrageous amounts of application data. MandelbrotMS should
 * be a perfect fit. Of course, I'll also have to write a GIF encoder, and
 * then move both decoder and encoder to some nice GIF support class.
 */

static bool read_byte(FILE *file, int *n);
static bool read_short(FILE *file, int *n);

/* public virtual */ bool
ImageIO_GIF::can_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL)
	return false;
    char magic[7];
    int n = fread(magic, 1, 6, file);
    fclose(file);
    if (n != 6)
	return false;
    magic[6] = 0;
    return strcmp(magic, "GIF87a") == 0 || strcmp(magic, "GIF89a") == 0;
}

/* public virtual */ bool
ImageIO_GIF::read(const char *filename, char **plugin_name, void **plugin_data,
		  int *plugin_data_length, FWPixmap *pm, char **message) {
    *plugin_name = NULL;
    *plugin_data = NULL;
    *plugin_data_length = 0;
    *message = strclone("foo");

    FILE *gif = fopen(filename, "r");
    if (gif == NULL) {
	fprintf(stderr, "GIFViewer: Can't open \"%s\".\n", filename);
	return false;
    }

    char sig[7];
    if (fread(sig, 1, 6, gif) != 6) {
	fprintf(stderr, "GIFViewer: GIF signature not found.\n");
	fclose(gif);
	return false;
    }
    sig[6] = 0;
    bool gif89a;
    if (strcmp(sig, "GIF87a") == 0)
	gif89a = false;
    else if (strcmp(sig, "GIF89a") == 0)
	gif89a = true;
    else {
	fprintf(stderr, "GIFViewer: GIF87a or GIF89a signature not found.\n");
	fclose(gif);
	return false;
    }

    int info;
    int background;
    int zero;

    if (!read_short(gif, &pm->width)
	    || !read_short(gif, &pm->height)
	    || !read_byte(gif, &info)
	    || !read_byte(gif, &background)
	    || !read_byte(gif, &zero)
	    || zero != 0) {
	fprintf(stderr, "GIFViewer: fatally premature EOF.\n");
	fclose(gif);
	return false;
    }

    bool has_global_cmap = (info & 128) != 0;
    int bpp = (info & 7) + 1;
    int ncolors = 1 << bpp;

    /* Bits 6..4 of info contain one less than the "color resolution",
     * defined as the number of significant bits per RGB component in
     * the source image's color palette. If the source image (from
     * which the GIF was generated) was 24-bit true color, the color
     * resolution is 8, so the value in bits 6..4 is 7. If the source
     * image had an EGA color cube (2x2x2), the color resolution would
     * be 2, etc.
     * Bit 3 of info must be zero in GIF87a; in GIF89a, if it is set,
     * it indicates that the global colormap is sorted, the most
     * important entries being first. In PseudoColor environments this
     * can be used to make sure to get the most important colors from
     * the X server first, to optimize the image's appearance in the
     * event that not all the colors from the colormap can actually be
     * obtained at the same time.
     * The 'zero' field is always 0 in GIF87a; in GIF89a, it indicates
     * the pixel aspect ratio, as (PAR + 15) : 64. If PAR is zero,
     * this means no aspect ratio information is given, PAR = 1 means
     * 1:4 (narrow), PAR = 49 means 1:1 (square), PAR = 255 means
     * slightly over 4:1 (wide), etc.
     */

    pm->depth = 8;
    pm->bytesperline = (pm->width + 3) & ~3;

    int size = pm->bytesperline * pm->height;
    pm->pixels = (unsigned char *) malloc(size);
    memset(pm->pixels, background, size);
    pm->cmap = new FWColor[256];
    if (has_global_cmap) {
	for (int i = 0; i < ncolors; i++) {
	    int r, g, b;
	    if (!read_byte(gif, &r)
		    || !read_byte(gif, &g)
		    || !read_byte(gif, &b)) {
		fprintf(stderr, "GIFViewer: fatally premature EOF.\n");
		goto failed;
	    }
	    pm->cmap[i].r = r;
	    pm->cmap[i].g = g;
	    pm->cmap[i].b = b;
	}
    } else {
	for (int i = 0; i < ncolors; i++) {
	    int k = (i * 255) / (ncolors - 1);
	    pm->cmap[i].r = k;
	    pm->cmap[i].g = k;
	    pm->cmap[i].b = k;
	}
    }

    while (1) {
	int whatnext;
	if (!read_byte(gif, &whatnext)) {
	    fprintf(stderr, "GIFViewer: unexpected EOF.\n");
	    goto done;
	}
	if (whatnext == ',') {
	    // Image
	    int ileft, itop, iwidth, iheight;
	    int info;
	    if (!read_short(gif, &ileft)
		    || !read_short(gif, &itop)
		    || !read_short(gif, &iwidth)
		    || !read_short(gif, &iheight)
		    || !read_byte(gif, &info)) {
		fprintf(stderr, "GIFViewer: unexpected EOF.\n");
		goto done;
	    }
	    
	    if (itop + iheight > pm->height
		    || ileft + iwidth > pm->width) {
		fprintf(stderr, "Image position and size not contained within screen size!\n");
		goto failed;
	    }

	    /* Bit 3 of info must be zero in GIF87a; in GIF89a, if it
		* is set, it indicates that the local colormap is sorted,
		* the most important entries being first. In PseudoColor
		* environments this can be used to make sure to get the
		* most important colors from the X server first, to
		* optimize the image's appearance in the event that not
		* all the colors from the colormap can actually be
		* obtained at the same time.
		*/

	    FWColor *lcmap;
	    int lbpp;
	    int lncolors;
	    if ((info & 128) == 0) {
		// Using global color map
		lcmap = pm->cmap;
		lbpp = bpp;
		lncolors = ncolors;
	    } else {
		// Using local color map
		lbpp = (info & 7) + 1;
		lncolors = 1 << lbpp;
		lcmap = new FWColor[lncolors];
		for (int i = 0; i < lncolors; i++) {
		    int r, g, b;
		    if (!read_byte(gif, &r)
			    || !read_byte(gif, &g)
			    || !read_byte(gif, &b)) {
			delete[] lcmap;
			fprintf(stderr, "GIFViewer: unexpected EOF.\n");
			goto done;
		    }
		    lcmap[i].r = r;
		    lcmap[i].g = g;
		    lcmap[i].b = b;
		}
		if (pm->depth == 8) {
		    if (g_verbosity >= 1)
			fprintf(stderr, "GIFViewer: Local colormap found, switching to 24-bit mode!\n");
		    int newbytesperline = pm->width * 4;
		    unsigned char *newpixels = (unsigned char *)
				malloc(newbytesperline * pm->height);
		    for (int v = 0; v < pm->height; v++)
			for (int h = 0; h < pm->width; h++) {
			    unsigned char pixel = pm->pixels[pm->bytesperline * v + h];
			    unsigned char *newpixel = newpixels
				    + (newbytesperline * v + 4 * h);
			    newpixel[0] = 0;
			    newpixel[1] = pm->cmap[pixel].r;
			    newpixel[2] = pm->cmap[pixel].g;
			    newpixel[3] = pm->cmap[pixel].b;
			}
		    free(pm->pixels);
		    pm->pixels = newpixels;
		    pm->bytesperline = newbytesperline;
		    pm->depth = 24;
		}
	    }

	    bool interlaced = (info & 64) != 0;
	    int h = 0;
	    int v = 0;
	    int codesize;
	    int bytecount;
	    if (!read_byte(gif, &codesize)
		    || !read_byte(gif, &bytecount)) {
		fprintf(stderr, "GIFViewer: unexpected EOF.\n");
		goto done;
	    }

	    short prefix_table[4096];
	    short code_table[4096];

	    int maxcode = 1 << codesize;
	    for (int i = 0; i < maxcode + 2; i++) {
		prefix_table[i] = -1;
		code_table[i] = i;
	    }
	    int clear_code = maxcode++;
	    int end_code = maxcode++;

	    int curr_code_size = codesize + 1;
	    int curr_code = 0;
	    int old_code = -1;
	    int bits_needed = curr_code_size;
	    bool end_code_seen = false;

	    while (bytecount != 0) {
		for (int i = 0; i < bytecount; i++) {
		    int currbyte;
		    if (!read_byte(gif, &currbyte)) {
			fprintf(stderr, "GIFViewer: unexpected EOF.\n");
			if (lcmap != pm->cmap)
			    delete[] lcmap;
			goto done;
		    }
		    if (end_code_seen)
			continue;

		    int bits_available = 8;
		    while (bits_available != 0) {
			int bits_copied = bits_needed < bits_available ?
				    bits_needed : bits_available;
			int bits = currbyte >> (8 - bits_available);
			bits &= 255 >> (8 - bits_copied);
			curr_code |= bits << (curr_code_size - bits_needed);
			bits_available -= bits_copied;
			bits_needed -= bits_copied;

			if (bits_needed == 0) {
			    if (curr_code == end_code) {
				end_code_seen = true;
			    } else if (curr_code == clear_code) {
				maxcode = (1 << codesize) + 2;
				curr_code_size = codesize + 1;
				old_code = -1;
			    } else {
				if (curr_code < maxcode) {
				    if (maxcode < 4096 && old_code != -1) {
					int c = curr_code;
					while (prefix_table[c] != -1)
					    c = prefix_table[c];
					c = code_table[c];
					prefix_table[maxcode] = old_code;
					code_table[maxcode] = c;
					maxcode++;
					if (maxcode == (1 << curr_code_size)
						&& curr_code_size < 12)
					    curr_code_size++;
				    }
				} else if (curr_code == maxcode) {
				    // Once maxcode == 4096, we can't get here any
				    // more, because we refuse to raise curr_code_size
				    // above 12 -- so we can never read a bigger code
				    // than 4095.
				    int c = old_code;
				    while (prefix_table[c] != -1)
					c = prefix_table[c];
				    c = code_table[c];
				    prefix_table[maxcode] = old_code;
				    code_table[maxcode] = c;
				    maxcode++;
				    if (maxcode == (1 << curr_code_size)
					    && curr_code_size < 12)
					curr_code_size++;
				} else {
				    fprintf(stderr, "GIFViewer: currcode > maxcode!\n");
				    end_code_seen = true;
				}

				old_code = curr_code;

				// Output curr_code!
				unsigned char expanded[4096];
				int explen = 0;
				while (curr_code != -1) {
				    expanded[explen++] = code_table[curr_code];
				    curr_code = prefix_table[curr_code];
				}
				for (int i = explen - 1; i >= 0; i--) {
				    int pixel = expanded[i];
				    if (pm->depth == 8)
					pm->pixels[pm->bytesperline * (itop + v) + ileft + h] = pixel;
				    else {
					unsigned char *rgb = pm->pixels + (pm->bytesperline * (itop + v) + 4 * (ileft + h));
					rgb[0] = 0;
					rgb[1] = lcmap[pixel].r;
					rgb[2] = lcmap[pixel].g;
					rgb[3] = lcmap[pixel].b;
				    }
				    if (++h == iwidth) {
					h = 0;
					if (interlaced) {
					    switch (v & 7) {
						case 0:
						    v += 8;
						    if (v < iheight)
							break;
						    /* Some GIF en/decoders go straight
							* from the '0' pass to the '4'
							* pass without checking the height,
							* and blow up on 2/3/4 pixel high
							* interlaced images.
							*/
						    if (iheight > 4)
							v = 4;
						    else if (iheight > 2)
							v = 2;
						    else if (iheight > 1)
							v = 1;
						    else
							end_code_seen = true;
						    break;
						case 4:
						    v += 8;
						    if (v >= iheight)
							v = 2;
						    break;
						case 2:
						case 6:
						    v += 4;
						    if (v >= iheight)
							v = 1;
						    break;
						case 1:
						case 3:
						case 5:
						case 7:
						    v += 2;
						    if (v >= iheight)
							end_code_seen = true;
						    break;
					    }
					    if (end_code_seen)
						break;
					} else {
					    if (++v == iheight) {
						end_code_seen = true;
						break;
					    }
					}
				    }
				}
			    }
			    curr_code = 0;
			    bits_needed = curr_code_size;
			}
		    }
		}
		if (!read_byte(gif, &bytecount)) {
		    fprintf(stderr, "GIFViewer: unexpected EOF.\n");
		    if (lcmap != pm->cmap)
			delete[] lcmap;
		    goto done;
		}
	    }

	    if (lcmap != pm->cmap)
		delete[] lcmap;
	} else if (whatnext == '!') {
	    // Extension block
	    int function_code, byte_count;
	    read_byte(gif, &function_code);
	    if (!read_byte(gif, &byte_count)) {
		fprintf(stderr, "GIFViewer: unexpected EOF.\n");
		goto done;
	    }
	    while (byte_count != 0) {
		for (int i = 0; i < byte_count; i++) {
		    int dummy;
		    if (!read_byte(gif, &dummy)) {
			fprintf(stderr, "GIFViewer: unexpected EOF.\n");
			goto done;
		    }
		}
		if (!read_byte(gif, &byte_count)) {
		    fprintf(stderr, "GIFViewer: unexpected EOF.\n");
		    goto done;
		}
	    }
	} else if (whatnext == ';') {
	    // Terminator
	    break;
	} else {
	    fprintf(stderr, "GIFViewer: unrecognized tag '%c' (0x%02x).\n", whatnext, whatnext);
	    goto failed;
	}
    }

    done:
    if (pm->depth == 24) {
	delete[] pm->cmap;
	pm->cmap = NULL;
    }
    fclose(gif);
    return true;

    failed:
    if (pm->cmap != NULL) {
	delete[] pm->cmap;
	pm->cmap = NULL;
    }
    if (pm->pixels != NULL) {
	free(pm->pixels);
	pm->pixels = NULL;
    }
    fclose(gif);
    return false;
}

/* public virtual */ bool
ImageIO_GIF::write(const char *filename, const char *plugin_name,
		   const void *plugin_data, int plugin_data_length,
		   const FWPixmap *pm, char **message) {
    *message = strclone("foo");
    return false;
}

static bool read_byte(FILE *file, int *n) {
    int c = fgetc(file);
    if (c == EOF)
	return false;
    *n = c;
    return true;
}

static bool read_short(FILE *file, int *n) {
    int c1 = fgetc(file);
    if (c1 == EOF)
	return false;
    int c2 = fgetc(file);
    if (c2 == EOF)
	return false;
    *n = c1 + (c2 << 8);
    return true;
}
