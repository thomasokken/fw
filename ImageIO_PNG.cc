#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <png.h>

#include "ImageIO_PNG.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "main.h"
#include "util.h"


/* public virtual */ bool
ImageIO_PNG::can_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL)
	return false;
    unsigned char magic[8];
    int n = fread(magic, 1, 8, file);
    fclose(file);
    if (n != 8)
	return false;
    return !png_sig_cmp(magic, 0, 8);
}

static int read_chunk_callback(png_structp png_ptr, png_unknown_chunkp chunk) {
    //PNGViewer *This = (PNGViewer *) png_get_user_chunk_ptr(png_ptr);
    if (g_verbosity >= 1)
    fprintf(stderr, "Found '%s' chunk (%d bytes)\n", chunk->name, chunk->size);
    // return value: -n means error, 0 means did not recognize,
    // n means success.
    return 1;
}

/* public virtual */ bool
ImageIO_PNG::read(const char *filename, char **plugin_name,
		  void **plugin_data, int *plugin_data_length,
		  FWPixmap *pm, char **message) {
    *message = strclone("foo");

    FILE *png = fopen(filename, "r");
    if (png == NULL)
	return false;

    png_structp png_ptr = png_create_read_struct(
		    PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
	fclose(png);
	return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr); 
    if (info_ptr == NULL) {
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	fclose(png);
	return false;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (end_info == NULL) {
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(png);
	return false;
    }

    png_bytepp row_pointers = NULL;

    if (setjmp(png_jmpbuf(png_ptr))) {
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(png);
	if (pm->cmap != NULL) {
	    delete[] pm->cmap;
	    pm->cmap = NULL;
	}
	if (pm->pixels != NULL) {
	    free(pm->pixels);
	    pm->pixels = NULL;
	}
	if (row_pointers != NULL)
	    free(row_pointers);
	return false;
    }

    png_init_io(png_ptr, png);
    png_set_read_user_chunk_fn(png_ptr, (png_voidp) this,
		    read_chunk_callback);

    png_read_info(png_ptr, info_ptr);
    
    pm->width = png_get_image_width(png_ptr, info_ptr);
    pm->height = png_get_image_height(png_ptr, info_ptr);

    int color_type = png_get_color_type(png_ptr, info_ptr);
    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    bool needgrayramp = false;

    switch (color_type) {
	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    if (bit_depth == 1)
		pm->depth = 1;
	    else {
		pm->depth = 8;
		needgrayramp = true;
	    }
	    break;
	case PNG_COLOR_TYPE_PALETTE:
	    pm->depth = 8;
	    break;
	case PNG_COLOR_TYPE_RGB:
	case PNG_COLOR_TYPE_RGB_ALPHA:
	    pm->depth = 24;
	    break;
	default:
	    fprintf(stderr, "PNGViewer: unknown color_type %d.\n",
		    color_type);
	    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	    fclose(png);
	    return false;
    }

    switch (pm->depth) {
	case 1:
	    pm->bytesperline = (pm->width + 7) >> 3;
	    break;
	case 8:
	    pm->bytesperline = (pm->width + 3) & ~3;
	    break;
	case 24:
	    pm->bytesperline = pm->width * 4;
	    break;
    }

    pm->pixels = (unsigned char *) malloc(pm->bytesperline * pm->height);

    if (pm->depth == 8) {
	if (!needgrayramp) {
	    png_colorp palette;
	    int palettesize;
	    if (png_get_PLTE(png_ptr, info_ptr, &palette, &palettesize)
		    == 0) {
		if (g_verbosity >= 1)
		    fprintf(stderr, "PNGViewer: no palette found in colormapped image; using gray ramp.\n");
		needgrayramp = true;
	    } else {
		int s = palettesize > 256 ? 256 : palettesize;
		pm->cmap = new FWColor[256];
		for (int i = 0; i < s; i++) {
		    pm->cmap[i].r = palette[i].red;
		    pm->cmap[i].g = palette[i].green;
		    pm->cmap[i].b = palette[i].blue;
		}
		for (int i = s; i < 256; i++) {
		    pm->cmap[i].r = 0;
		    pm->cmap[i].g = 255;
		    pm->cmap[i].b = 0;
		}
	    }
	}
	if (needgrayramp) {
	    pm->cmap = new FWColor[256];
	    for (int k = 0; k < 256; k++) {
		pm->cmap[k].r = k;
		pm->cmap[k].g = k;
		pm->cmap[k].b = k;
	    }
	}
    }

    if (color_type == PNG_COLOR_TYPE_GRAY
	    && (bit_depth == 2 || bit_depth == 4))
	png_set_gray_1_2_4_to_8(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth == 1)
	png_set_packswap(png_ptr);
    if (bit_depth == 16)
	png_set_strip_16(png_ptr);
    if (color_type & PNG_COLOR_MASK_ALPHA)
	png_set_strip_alpha(png_ptr);
    if (pm->depth == 24)
	png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

    row_pointers = (png_bytepp) malloc(pm->height * sizeof(png_bytep));
    for (int i = 0; i < pm->height; i++)
	row_pointers[i] = (png_bytep) pm->pixels + (pm->bytesperline * i);

    png_read_image(png_ptr, row_pointers);

    free(row_pointers);
    return true;
}

/* public virtual */ bool
ImageIO_PNG::write(const char *filename, const char *plugin_name,
		   const void *plugin_data, int plugin_data_length,
		   const FWPixmap *pm, char **message) {
    *message = strclone("foo");
    return false;
}
