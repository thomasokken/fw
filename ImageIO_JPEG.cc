///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "ImageIO_JPEG.h"
#include "CopyBits.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "util.h"


#define MSGLEN 1024
static const char *FW_ID_STR = "Fractal Wizard 2.0";
static const int FW_ID_LEN = strlen(FW_ID_STR) + 1;

typedef struct {
    struct jpeg_error_mgr pub;
    jmp_buf env;
} my_error_mgr;

static void my_error_exit(j_common_ptr p) {
    struct jpeg_decompress_struct *jcs = (struct jpeg_decompress_struct *) p;
    my_error_mgr *err = (my_error_mgr *) jcs->err;
    longjmp(err->env, 1);
}


/* public virtual */ bool
ImageIO_JPEG::can_read(const char *filename) {
    FILE *jpg = fopen(filename, "r");
    if (jpg == NULL)
        return false;
    unsigned char magic[4];
    int n = fread(magic, 1, 4, jpg);
    fclose(jpg);
    if (n != 4)
        return false;
    return magic[0] == 0xFF && magic[1] == 0xD8 && magic[2] == 0xFF;
}

/* public virtual */ bool
ImageIO_JPEG::read(const char *filename, char **plugin_name,
                   void **plugin_data, int *plugin_data_length,
                   FWPixmap *pm, char **message) {
    char msgbuf[MSGLEN];

    FILE *jpg = fopen(filename, "r");
    if (jpg == NULL) {
        snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).", filename, strerror(errno));
        *message = strclone(msgbuf);
        return false;
    }

    // Buffer for receiving scanlines in 3-color images
    unsigned char *buf = NULL;

    // Buffer for plugin data
    unsigned char *plbuf = NULL;
    char *plname = NULL;

    struct jpeg_decompress_struct jcs;
    my_error_mgr jerr;
    if (setjmp(jerr.env)) {
        snprintf(msgbuf, MSGLEN, "Something went wrong with the JPEG decompression, but I'm not telling you what.");
        *message = strclone(msgbuf);
        jpeg_destroy_decompress(&jcs);
        fclose(jpg);
        if (pm->pixels != NULL) {
            free(pm->pixels);
            pm->pixels = NULL;
        }
        if (pm->cmap != NULL) {
            delete[] pm->cmap;
            pm->cmap = NULL;
        }
        if (buf != NULL)
            delete[] buf;
        if (plbuf != NULL)
            free(plbuf);
        if (plname != NULL)
            free(plname);
        return false;
    }

    jcs.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    jpeg_create_decompress(&jcs);
    jpeg_stdio_src(&jcs, jpg);
    jpeg_save_markers(&jcs, JPEG_APP0 + 13, 65535);
    jpeg_read_header(&jcs, TRUE);
    
    jpeg_saved_marker_ptr marker = jcs.marker_list;
    int plbufsize = 0;
    int pllength;
    unsigned int crc;
    while (marker != NULL) {
        if (marker->marker == JPEG_APP0 + 13
                && (int) marker->data_length >
                                (plbuf == NULL ? FW_ID_LEN + 8 : FW_ID_LEN)
                && strncmp((char *) marker->data, FW_ID_STR, FW_ID_LEN) == 0) {
            // Looks like one of ours
            if (plbuf == NULL) {
                // first 8 bytes are length & crc
                pllength = (marker->data[FW_ID_LEN] << 24)
                        | (marker->data[FW_ID_LEN + 1] << 16)
                        | (marker->data[FW_ID_LEN + 2] << 8)
                        | (marker->data[FW_ID_LEN + 3]);
                crc = (marker->data[FW_ID_LEN + 4] << 24)
                        | (marker->data[FW_ID_LEN + 5] << 16)
                        | (marker->data[FW_ID_LEN + 6] << 8)
                        | (marker->data[FW_ID_LEN + 7]);
                int plbufpos = plbufsize;
                plbufsize += marker->data_length - FW_ID_LEN - 8;
                plbuf = (unsigned char *) malloc(plbufsize);
                memcpy(plbuf + plbufpos, marker->data + FW_ID_LEN + 8,
                        marker->data_length - FW_ID_LEN - 8);
            } else {
                int plbufpos = plbufsize;
                plbufsize += marker->data_length - FW_ID_LEN;
                plbuf = (unsigned char *) realloc(plbuf, plbufsize);
                memcpy(plbuf + plbufpos, marker->data + FW_ID_LEN,
                        marker->data_length - FW_ID_LEN);
            }
        }
        marker = marker->next;
    }
    if (plbuf != NULL) {
        // Let's see if we got anything good.
        unsigned int actualcrc;
        if (plbufsize < pllength) {
            snprintf(msgbuf, MSGLEN, "FW Plugin data block found, but it looks truncated\n(%d bytes read, %d bytes expected).", plbufsize, pllength);
            *message = strclone(msgbuf);
            free(plbuf);
            plbuf = NULL;
        } else if (crc != (actualcrc = crc32(plbuf, pllength))) {
            snprintf(msgbuf, MSGLEN, "FW Plugin data block found, but it looks corrupted\n(expected CRC: 0x%x, actual: 0x%x).", crc, actualcrc);
            *message = strclone(msgbuf);
            free(plbuf);
            plbuf = NULL;
        } else {
            // Looks good so far. Now, let's extract the plugin name. (No more
            // sanity checks; we trust the CRC to protect us from random
            // corruption. We don't expect plugins to check their data.)
            plname = strclone((char *) plbuf);
            int pnl = strlen(plname);
            int data_offset = pnl + 1;
            pllength -= pnl + 1;
            memmove(plbuf, plbuf + data_offset, pllength);
        }
    }

    pm->width = jcs.image_width;
    pm->height = jcs.image_height;
    if (jcs.num_components != 1 && jcs.num_components != 3) {
        snprintf(msgbuf, MSGLEN, "JPEGViewer: weird num_components (%d).",
                jcs.num_components);
        *message = strclone(msgbuf);
        jpeg_destroy_decompress(&jcs);
        fclose(jpg);
        return 0;
    }
    pm->depth = jcs.num_components * 8;
    if (pm->depth == 8)
        pm->bytesperline = (pm->width + 3) & ~3;
    else
        pm->bytesperline = pm->width * 4;
    pm->pixels = (unsigned char *) malloc(pm->bytesperline * pm->height);
    if (pm->depth == 8) {
        pm->cmap = new FWColor[256];
        for (int i = 0; i < 256; i++) {
            pm->cmap[i].r = i;
            pm->cmap[i].g = i;
            pm->cmap[i].b = i;
        }
    }

    jpeg_start_decompress(&jcs);
    if (pm->depth == 8) {
        while ((int) jcs.output_scanline < pm->height) {
            unsigned char *row = (unsigned char *)
                        pm->pixels + (jcs.output_scanline * pm->bytesperline);
            jpeg_read_scanlines(&jcs, &row, 1);
        }
    } else {
        buf = new unsigned char[pm->width * 3];
        while ((int) jcs.output_scanline < pm->height) {
            char *src = (char *) buf;
            unsigned char *dst = pm->pixels + (jcs.output_scanline * pm->bytesperline);
            jpeg_read_scanlines(&jcs, &buf, 1);
            for (int i = 0; i < pm->width; i++) {
                *dst++ = 0;
                *dst++ = *src++;
                *dst++ = *src++;
                *dst++ = *src++;
            }
        }
        delete[] buf;
        buf = NULL;
    }

    jpeg_finish_decompress(&jcs);
    jpeg_destroy_decompress(&jcs);
    fclose(jpg);

    if (plbuf != NULL) {
        *plugin_data = plbuf;
        *plugin_data_length = pllength;
        *plugin_name = plname;
    }

    return true;
}

/* public virtual */ bool
ImageIO_JPEG::write(const char *filename, const char *plugin_name,
                    const void *plugin_data, int plugin_data_length,
                    const FWPixmap *pm, char **message) {
    char msgbuf[MSGLEN];

    // Scanline buffer
    unsigned char *buf = NULL;

    FILE *jpg = fopen(filename, "w");
    if (jpg == NULL) {
        snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).", filename, strerror(errno));
        *message = strclone(msgbuf);
        return false;
    }
    struct jpeg_compress_struct jcs;
    my_error_mgr jerr;
    if (setjmp(jerr.env)) {
        snprintf(msgbuf, MSGLEN, "Something went wrong with the JPEG compression, but I'm not telling you what.");
        *message = strclone(msgbuf);
        jpeg_destroy_compress(&jcs);
        fclose(jpg);
        if (buf != NULL)
            delete[] buf;
        return false;
    }

    jcs.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    jpeg_create_compress(&jcs);
    jpeg_stdio_dest(&jcs, jpg);

    jcs.image_width = pm->width;
    jcs.image_height = pm->height;
    bool gray = pm->depth == 1 ||
                (pm->depth == 8 && CopyBits::is_grayscale(pm->cmap));
    jcs.input_components = gray ? 1 : 3;
    jcs.in_color_space = gray ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_set_defaults(&jcs);
    // My settings here. TODO... Something like xv would be nice
    // (dials for quality and smoothing, the latter being particularly
    // interesting since FW-generated images probably arent't the best
    // candidates for JPEG compression without it).

    jpeg_start_compress(&jcs, TRUE);

    if (plugin_data != NULL && plugin_data_length > 0) {
        char marker[65533];
        strcpy(marker, FW_ID_STR);
        char *datastart = marker + FW_ID_LEN;
        int maxdata = 65533 - FW_ID_LEN;

        int crc = crc32(plugin_data, plugin_data_length);
        char *remaining = (char *) plugin_data;
        int remaining_length = plugin_data_length;
        bool first = true;

        while (remaining_length > 0) {
            if (first) {
                datastart[0] = plugin_data_length >> 24;
                datastart[1] = plugin_data_length >> 16;
                datastart[2] = plugin_data_length >> 8;
                datastart[3] = plugin_data_length;
                datastart[4] = crc >> 24;
                datastart[5] = crc >> 16;
                datastart[6] = crc >> 8;
                datastart[7] = crc;
                datastart += 8;
                maxdata -= 8;
            }

            int marker_size = remaining_length < maxdata
                                    ? remaining_length : maxdata;
            memcpy(datastart, remaining, marker_size);
            jpeg_write_marker(&jcs, JPEG_APP0 + 13, (unsigned char *) marker,
                                        marker_size + (datastart - marker));
            remaining += marker_size;
            remaining_length -= marker_size;

            if (first) {
                datastart -= 8;
                maxdata += 8;
                first = false;
            }
        }
    }

    // Allocate row buffer UNLESS we have an 8-bit image with a pure
    // gray-scale palette; in that one specific case, the in-memory layout
    // of our images actually matches what libjpeg expects, so we don't
    // have to do any copying.

    if (pm->depth != 8 || !gray)
        buf = new unsigned char[pm->width * (gray ? 1 : 3)];
    unsigned char *buf2 = buf;

    while ((int) jcs.next_scanline < pm->height) {
        switch (pm->depth) {
            case 1:
                for (int x = 0; x < pm->width; x++)
                    buf[x] = ((pm->pixels[jcs.next_scanline * pm->bytesperline
                                    + (x >> 3)] >> (x & 7)) & 1) * 255;
                break;
            case 8:
                if (gray) {
                    buf2 = pm->pixels + jcs.next_scanline * pm->bytesperline;
                } else {
                    unsigned char *dst = buf;
                    for (int x = 0; x < pm->width; x++) {
                        unsigned char pixel = pm->pixels[jcs.next_scanline
                                                * pm->bytesperline + x];
                        *dst++ = pm->cmap[pixel].r;
                        *dst++ = pm->cmap[pixel].g;
                        *dst++ = pm->cmap[pixel].b;
                    }
                }
                break;
            case 24: {
                unsigned char *dst = buf;
                unsigned char *src = pm->pixels + jcs.next_scanline
                                                        * pm->bytesperline;
                for (int x = 0; x < pm->width; x++) {
                    src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                    *dst++ = *src++;
                }
            }
            break;
        }
        jpeg_write_scanlines(&jcs, &buf2, 1);
    }

    if (buf != NULL)
        delete[] buf;
    buf = NULL;

    jpeg_finish_compress(&jcs);
    jpeg_destroy_compress(&jcs);
    fclose(jpg);
    return true;
}
