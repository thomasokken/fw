#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "ImageIO_JPEG.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "util.h"

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
ImageIO_JPEG::read(const char *filename, char **plugin_name,
		   void **plugin_data, int *plugin_data_length,
		   FWPixmap *pm, char **message) {

    *message = strclone("foo");

    FILE *jpg = fopen(filename, "r");
    if (jpg == NULL)
	return false;

    // Buffer for receiving scanlines in 3-color images
    unsigned char *buf = NULL;

    struct jpeg_decompress_struct jcs;
    my_error_mgr jerr;
    if (setjmp(jerr.env)) {
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
	return false;
    }

    jcs.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    jpeg_create_decompress(&jcs);
    jpeg_stdio_src(&jcs, jpg);
    jpeg_read_header(&jcs, TRUE);

    pm->width = jcs.image_width;
    pm->height = jcs.image_height;
    if (jcs.num_components != 1 && jcs.num_components != 3) {
	fprintf(stderr, "JPEGViewer: weird num_components (%d)!\n",
		jcs.num_components);
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
    return true;
}

/* public virtual */ bool
ImageIO_JPEG::write(const char *filename, char *plugin_name,
		    const void *plugin_data, int plugin_data_length,
		    const FWPixmap *pm, char **message) {
    *message = strclone("foo");
    return false;
}
