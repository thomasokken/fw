#include <ctype.h>
#include <errno.h>
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

#define MSGLEN 1024

static bool read_byte(FILE *file, int *n);
static bool read_short(FILE *file, int *n);
static unsigned char hash(short prefix, short pixel);

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
    char msgbuf[MSGLEN];

    unsigned char *buf = NULL;
    int bufsize = 0;
    int bufpos = 0;
    FWColor *lcmap = NULL;

    FILE *gif = fopen(filename, "r");
    if (gif == NULL) {
	snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).", filename, strerror(errno));
	*message = strclone(msgbuf);
	return false;
    }

    char sig[7];
    if (fread(sig, 1, 6, gif) != 6) {
	snprintf(msgbuf, MSGLEN, "GIF signature not found.");
	*message = strclone(msgbuf);
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
	snprintf(msgbuf, MSGLEN, "GIF87a or GIF89a signature not found.");
	*message = strclone(msgbuf);
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
	snprintf(msgbuf, MSGLEN, "Fatally premature EOF.");
	*message = strclone(msgbuf);
	fclose(gif);
	return false;
    }

    bool has_global_cmap = (info & 128) != 0;
    int bpp = (info & 7) + 1;
    int ncolors = 1 << bpp;
    int size = 0;

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

    pm->cmap = new FWColor[256];
    if (has_global_cmap) {
	for (int i = 0; i < ncolors; i++) {
	    int r, g, b;
	    if (!read_byte(gif, &r)
		    || !read_byte(gif, &g)
		    || !read_byte(gif, &b)) {
		snprintf(msgbuf, MSGLEN, "Fatally premature EOF.");
		*message = strclone(msgbuf);
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

    bool mono;
    bool invert;
    if (ncolors == 2) {
	// Test for true black & white
	if (pm->cmap[0].r == 0
		&& pm->cmap[0].g == 0
		&& pm->cmap[0].b == 0
		&& pm->cmap[1].r == 255
		&& pm->cmap[1].g == 255
		&& pm->cmap[1].b == 255) {
	    mono = true;
	    invert = false;
	} else if (pm->cmap[0].r == 255
		&& pm->cmap[0].g == 255
		&& pm->cmap[0].b == 255
		&& pm->cmap[1].r == 0
		&& pm->cmap[1].g == 0
		&& pm->cmap[1].b == 0) {
	    mono = true;
	    invert = true;
	} else
	    mono = false;
    } else
	mono = false;

    if (mono) {
	pm->depth = 1;
	pm->bytesperline = (pm->width + 31 >> 3) & ~3;
    } else {
	pm->depth = 8;
	pm->bytesperline = (pm->width + 3) & ~3;
    }

    size = pm->bytesperline * pm->height;
    pm->pixels = (unsigned char *) malloc(size);
    memset(pm->pixels, pm->depth == 1 ? background * 255 : background, size);

    while (1) {
	int whatnext;
	if (!read_byte(gif, &whatnext))
	    goto unexp_eof;
	if (whatnext == ',') {
	    // Image
	    int ileft, itop, iwidth, iheight;
	    int info;
	    if (!read_short(gif, &ileft)
		    || !read_short(gif, &itop)
		    || !read_short(gif, &iwidth)
		    || !read_short(gif, &iheight)
		    || !read_byte(gif, &info))
		goto unexp_eof;
	    
	    if (itop + iheight > pm->height
		    || ileft + iwidth > pm->width) {
		snprintf(msgbuf, MSGLEN, "Image position and size not contained within screen size!");
		*message = strclone(msgbuf);
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
			    || !read_byte(gif, &b))
			goto unexp_eof;
		    lcmap[i].r = r;
		    lcmap[i].g = g;
		    lcmap[i].b = b;
		}
		if (pm->depth != 24) {
		    if (g_prefs->verbosity >= 1)
			fprintf(stderr, "GIFViewer: Local colormap found, switching to 24-bit mode!\n");
		    int newbytesperline = pm->width * 4;
		    unsigned char *newpixels = (unsigned char *)
				malloc(newbytesperline * pm->height);
		    for (int v = 0; v < pm->height; v++)
			for (int h = 0; h < pm->width; h++) {
			    unsigned char pixel;
			    if (pm->depth == 1)
				pixel = (pm->pixels[pm->bytesperline * v
						+ (h >> 3)] >> (h & 7)) & 1;
			    else
				pixel = pm->pixels[pm->bytesperline * v + h];
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
	    if (!read_byte(gif, &codesize) || !read_byte(gif, &bytecount))
		goto unexp_eof;

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
		    if (!read_byte(gif, &currbyte))
			goto unexp_eof;
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
				    // Once maxcode == 4096, we can't get here
				    // any more, because we refuse to raise
				    // curr_code_size above 12 -- so we can
				    // never read a bigger code than 4095.
				    if (old_code == -1) {
					snprintf(msgbuf, MSGLEN, "Out-of-sequence code in compressed data.");
					*message = strclone(msgbuf);
					goto done;
				    }
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
				    snprintf(msgbuf, MSGLEN, "Out-of-sequence code in compressed data.");
				    *message = strclone(msgbuf);
				    goto done;
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
				    else if (pm->depth == 24) {
					unsigned char *rgb = pm->pixels + (pm->bytesperline * (itop + v) + 4 * (ileft + h));
					rgb[0] = 0;
					rgb[1] = lcmap[pixel].r;
					rgb[2] = lcmap[pixel].g;
					rgb[3] = lcmap[pixel].b;
				    } else {
					// VERY inefficient. 16 memory accesses
					// to write one byte. Then again, 1-bit
					// images never consume very many bytes
					// and I'm in a hurry. Maybe TODO.
					int x = ileft + h;
					int index = pm->bytesperline
						* (itop + v) + (x >> 3);
					unsigned char mask = 1 << (x & 7);
					if (pixel)
					    pm->pixels[index] |= mask;
					else
					    pm->pixels[index] &= ~mask;
				    }
				    if (++h == iwidth) {
					h = 0;
					if (interlaced) {
					    switch (v & 7) {
						case 0:
						    v += 8;
						    if (v < iheight)
							break;
						    /* Some GIF en/decoders go
						     * straight from the '0'
						     * pass to the '4' pass
						     * without checking the
						     * height, and blow up on
						     * 2/3/4 pixel high
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
		if (!read_byte(gif, &bytecount))
		    goto unexp_eof;
	    }

	    if (lcmap != pm->cmap)
		delete[] lcmap;
	    lcmap = NULL;

	} else if (whatnext == '!') {
	    // Extension block
	    int function_code, byte_count;
	    if (!read_byte(gif, &function_code))
		goto unexp_eof;
	    if (function_code == 255) {
		if (!read_byte(gif, &byte_count))
		    goto unexp_eof;
		if (byte_count != 11) {
		    // Fall through to block-skipping code, but jump over the
		    // reading of the byte count character, since we have it
		    // already.
		    goto not_fw_app_blk;
		} else {
		    char app_id[12];
		    int c;
		    for (int i = 0; i < 11; i++) {
			if (!read_byte(gif, &c))
			    goto unexp_eof;
			app_id[i] = c;
		    }
		    app_id[11] = 0;
		    if (strcmp(app_id, "FractWiz2.0") == 0) {
			// Yup, it's ours, collect the data!
			if (!read_byte(gif, &byte_count))
			    goto unexp_eof;
			while (byte_count != 0) {
			    for (int i = 0; i < byte_count; i++) {
				if (!read_byte(gif, &c))
				    goto unexp_eof;
				if (bufpos == bufsize) {
				    bufsize += 1024;
				    buf = (unsigned char *) realloc(buf, bufsize);
				}
				buf[bufpos++] = c;
			    }
			    if (!read_byte(gif, &byte_count))
				goto unexp_eof;
			}
			// Done; don't fall through to block-skipping code
			goto app_blk_done;
		    }
		}
	    }
	    // Not one of our extension blocks; we just skip it.
	    if (!read_byte(gif, &byte_count))
		goto unexp_eof;
	    not_fw_app_blk:
	    while (byte_count != 0) {
		for (int i = 0; i < byte_count; i++) {
		    int dummy;
		    if (!read_byte(gif, &dummy))
			goto unexp_eof;
		}
		if (!read_byte(gif, &byte_count))
		    goto unexp_eof;
	    }
	    app_blk_done:;
	} else if (whatnext == ';') {
	    // Terminator
	    break;
	} else {
	    snprintf(msgbuf, MSGLEN, "Unrecognized tag '%c' (0x%02x).",
		    whatnext, whatnext);
	    *message = strclone(msgbuf);
	    goto failed;
	}
    }

    done:
    if (lcmap != NULL && lcmap != pm->cmap)
	delete[] lcmap;
    if (pm->depth == 24) {
	delete[] pm->cmap;
	pm->cmap = NULL;
    }
    if (pm->depth == 1) {
	delete[] pm->cmap;
	pm->cmap = NULL;
	if (invert)
	    for (int i = 0; i < size; i++)
		pm->pixels[i] = ~pm->pixels[i];
    }

    if (buf != NULL) {
	// One or more Application Extension Blocks with our signature
	// "FractWiz2.0" have been found.
	// Let's see if we got anything good.
	// The first 4 bytes should be the length; the next 4 bytes should be
	// the CRC for the following 'length' bytes.
	int length = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	unsigned int crc =
		     (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
	unsigned int actualcrc;
	if (length > bufpos - 8) {
	    char msgbuf[MSGLEN];
	    snprintf(msgbuf, MSGLEN, "FW Plugin data block found, but it looks truncated\n(%d bytes read, %d bytes expected).", bufpos - 8, length);
	    *message = strclone(msgbuf);
	    free(buf);
	} else if (crc != (actualcrc = crc32(buf + 8, length))) {
	    char msgbuf[MSGLEN];
	    snprintf(msgbuf, MSGLEN, "FW Plugin data block found, but it looks corrupted\n(expected CRC: 0x%x, actual: 0x%x).", crc, actualcrc);
	    *message = strclone(msgbuf);
	    free(buf);
	} else {
	    // Looks good so far. Now, let's extract the plugin name. (No more
	    // sanity checks; we trust the CRC to protect us from random
	    // corruption. We don't expect plugins to check their data.)
	    *plugin_name = strclone((char *) buf + 8);
	    int pnl = strlen(*plugin_name);
	    int data_offset = pnl + 9;
	    length -= pnl + 1;
	    memmove(buf, buf + data_offset, length);
	    *plugin_data = buf;
	    *plugin_data_length = length;
	}
    }
    
    fclose(gif);
    return true;


    unexp_eof:
    snprintf(msgbuf, MSGLEN, "Unexpected EOF.");
    *message = strclone(msgbuf);
    goto done;


    failed:
    if (lcmap != NULL && lcmap != pm->cmap)
	delete[] lcmap;
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
    char msgbuf[MSGLEN];

    if (pm->depth > 8) {
	snprintf(msgbuf, MSGLEN, "This image has %d bits per pixel.\nGIF only supports depths of up to 8 bits per pixel.", pm->depth);
	*message = strclone(msgbuf);
	return false;
    }

    FILE *gif = fopen(filename, "w");
    if (gif == NULL) {
	snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).",
		 filename, strerror(errno));
	*message = strclone(msgbuf);
	return false;
    }


    // GIF Header

    fprintf(gif, "GIF87a");

    
    // Screen descriptor

    fputc(pm->width & 255, gif);
    fputc(pm->width >> 8, gif);
    fputc(pm->height & 255, gif);
    fputc(pm->height >> 8, gif);
    fputc(pm->depth == 1 ? 0xF0 : 0xF7, gif);
    fputc(0, gif);
    fputc(0, gif);


    // Global color map

    if (pm->depth == 1) {
	fputc(0, gif);
	fputc(0, gif);
	fputc(0, gif);
	fputc(255, gif);
	fputc(255, gif);
	fputc(255, gif);
    } else if (pm->depth == 8) {
	for (int i = 0; i < 256; i++) {
	    fputc(pm->cmap[i].r, gif);
	    fputc(pm->cmap[i].g, gif);
	    fputc(pm->cmap[i].b, gif);
	}
    } else {
	// GIF only does up to 8 bits per pixel. So, for the 24-bit case,
	// we use a hard-coded 256-entry color map, consisting of a 6x6x6
	// color cube, plus an additional 40 shades of gray (spaced to fit
	// in the gaps between the 6 shades of gray that we already have
	// in the cube, for a total of 46 shades of gray).
	int i = 0;
	for (int r = 0; r < 6; r++)
	    for (int g = 0; g < 6; g++)
		for (int b = 0; b < 6; b++) {
		    fputc(51 * r, gif);
		    fputc(51 * g, gif);
		    fputc(51 * b, gif);
		    i++;
		}
	for (int m = 0; m < 5; m++)
	    for (int n = 1; n < 9; n++) {
		int p = 51 * m + (17 * n) / 3;
		fputc(p, gif);
		fputc(p, gif);
		fputc(p, gif);
		i++;
	    }
    }


    // Extension Block

    if (plugin_data != NULL && plugin_data_length > 0) {
	fputc('!', gif);
	fputc(255, gif);
	fputc(11, gif);
	fprintf(gif, "FractWiz2.0");

	int total_length = plugin_data_length + 8;
	int n = total_length < 255 ? total_length : 255;
	unsigned int crc = crc32(plugin_data, plugin_data_length);

	fputc(n, gif);
	fputc(plugin_data_length >> 24, gif);
	fputc(plugin_data_length >> 16, gif);
	fputc(plugin_data_length >> 8, gif);
	fputc(plugin_data_length, gif);
	fputc(crc >> 24, gif);
	fputc(crc >> 16, gif);
	fputc(crc >> 8, gif);
	fputc(crc, gif);

	total_length -= 8;
	n -= 8;
	char *ptr = (char *) plugin_data;
	do {
	    fwrite(ptr, n, 1, gif);
	    total_length -= n;
	    ptr += n;
	    n = total_length < 255 ? total_length : 255;
	    fputc(n, gif);
	} while (n > 0);
    }


    // Image Descriptor

    fputc(',', gif);
    fputc(0, gif);
    fputc(0, gif);
    fputc(0, gif);
    fputc(0, gif);
    fputc(pm->width & 255, gif);
    fputc(pm->width >> 8, gif);
    fputc(pm->height & 255, gif);
    fputc(pm->height >> 8, gif);
    fputc(pm->depth == 1 ? 0x00 : 0x07, gif);


    // Image Data

    int codesize = pm->depth == 1 ? 2 : 8;
    int bytecount = 0;
    char buf[255];

    short prefix_table[4096];
    short code_table[4096];
    short hash_next[4096];
    short hash_head[256];

    int maxcode = 1 << codesize;
    for (int i = 0; i < maxcode; i++) {
	prefix_table[i] = -1;
	code_table[i] = i;
	hash_next[i] = -1;
    }
    for (int i = 0; i < 256; i++)
	hash_head[i] = -1;

    int clear_code = maxcode++;
    int end_code = maxcode++;

    int curr_code_size = codesize + 1;
    int prefix = -1;
    int currbyte = 0;
    int bits_needed = 8;
    bool initial_clear = true;
    bool really_done = false;

    long long hash_comparisons = 0;
    long long hash_searches = 0;
    long long hash_max_comparisons = 0;

    fputc(codesize, gif);
    for (int v = 0; v <= pm->height; v++) {
	bool done = v == pm->height;
	for (int h = 0; h < pm->width; h++) {
	    int new_code;
	    unsigned char hash_code;
	    int hash_index;

	    if (really_done) {
		new_code = end_code;
		goto emit;
	    } else if (done) {
		new_code = prefix;
		goto emit;
	    }

	    int pixel;
	    if (pm->depth == 1)
		pixel = ((pm->pixels[pm->bytesperline * v + (h >> 3)])
			    >> (h & 7)) & 1;
	    else
		pixel = pm->pixels[pm->bytesperline * v + h];

	    // Look for concat(prefix, pixel) in string table
	    if (prefix == -1) {
		prefix = pixel;
		goto no_emit;
	    }
	    
	    hash_code = hash(prefix, pixel);
	    hash_index = hash_head[hash_code];
	    hash_searches++;
	    hash_max_comparisons += maxcode - end_code;
	    while (hash_index != -1) {
		hash_comparisons++;
		if (prefix_table[hash_index] == prefix
			 && code_table[hash_index] == pixel) {
		    prefix = hash_index;
		    goto no_emit;
		}
		hash_index = hash_next[hash_index];
	    }
	    
	    // Not found:
	    if (maxcode < 4096) {
		prefix_table[maxcode] = prefix;
		code_table[maxcode] = pixel;
		hash_next[maxcode] = hash_head[hash_code];
		hash_head[hash_code] = maxcode;
		maxcode++;
	    }
	    new_code = prefix;
	    prefix = pixel;
	    
	    emit: {
		int outcode = initial_clear ? clear_code
					: really_done ? end_code : new_code;
		int bits_available = curr_code_size;
		while (bits_available != 0) {
		    int bits_copied = bits_needed < bits_available ?
				bits_needed : bits_available;
		    int bits = outcode >> (curr_code_size - bits_available);
		    bits &= 255 >> (8 - bits_copied);
		    currbyte |= bits << (8 - bits_needed);
		    bits_available -= bits_copied;
		    bits_needed -= bits_copied;
		    if (bits_needed == 0 ||
				    (bits_available == 0 && really_done)) {
			buf[bytecount++] = currbyte;
			if (bytecount == 255) {
			    fputc(bytecount, gif);
			    fwrite(buf, 1, bytecount, gif);
			    bytecount = 0;
			}
			if (bits_available == 0 && really_done)
			    goto data_done;
			currbyte = 0;
			bits_needed = 8;
		    }
		}

		if (done) {
		    done = false;
		    really_done = true;
		    goto emit;
		}
		if (initial_clear) {
		    initial_clear = false;
		    goto emit;
		} else {
		    if (maxcode > (1 << curr_code_size)) {
			curr_code_size++;
		    } else if (new_code == clear_code) {
			maxcode = (1 << codesize) + 2;
			curr_code_size = codesize + 1;
			for (int i = 0; i < 256; i++)
			    hash_head[i] = -1;
		    } else if (maxcode == 4096) {
			new_code = clear_code;
			goto emit;
		    }
		}
	    }

	    no_emit:;
	}
    }

    data_done:

    if (g_prefs->verbosity >= 2) {
	if (hash_searches == 0)
	    fprintf(stderr, "No hashing statistics available.\n");
	else {
	    fprintf(stderr, "%lld searches, %lld comparisons - %g comp/search.\n",
		    hash_searches, hash_comparisons,
		    ((double) hash_comparisons) / hash_searches);
	    fprintf(stderr, "Search percentage: %g\n", (100.0f * hash_comparisons) / hash_max_comparisons);
	}
    }

    if (bytecount > 0) {
	fputc(bytecount, gif);
	fwrite(buf, 1, bytecount, gif);
    }
    fputc(0, gif);


    // GIF Trailer

    fputc(';', gif);


    fclose(gif);
    return true;
}

static unsigned char hash(short prefix, short pixel) {
    // TODO: There's a lot of room for improvement here!
    // I'm getting search percentages of over 30%; looking for something
    // in single digits.
    unsigned long x = (((long) prefix) << 20) + (((long) pixel) << 12);
    x /= 997;
    unsigned char b1 = x >> 16;
    unsigned char b2 = x >> 8;
    unsigned char b3 = x;
    return b1 ^ b2 ^ b3;
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
