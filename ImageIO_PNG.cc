#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <png.h>

#include "ImageIO_PNG.h"
#include "CopyBits.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "main.h"
#include "util.h"


#define MSGLEN 1024
static const char *FW_CHUNK = "frwz";
static const char *FW_ID_STR = "Fractal Wizard 2.0";
static const int FW_ID_LEN = strlen(FW_ID_STR) + 1;


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

/* public virtual */ bool
ImageIO_PNG::read(const char *filename, char **plugin_name,
		  void **plugin_data, int *plugin_data_length,
		  FWPixmap *pm, char **message) {
    char msgbuf[MSGLEN];

    png_bytepp row_pointers = NULL;
    char *plbuf = NULL;
    int plbufsize = 0;
    char *plname = NULL;

    FILE *png = fopen(filename, "r");
    if (png == NULL) {
	snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).", filename,
		    strerror(errno));
	*message = strclone(msgbuf);
	return false;
    }

    png_structp png_ptr = png_create_read_struct(
		    PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
	*message = strclone("Could not allocate PNG data structures.");
	fclose(png);
	return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr); 
    if (info_ptr == NULL) {
	*message = strclone("Could not allocate PNG data structures.");
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	fclose(png);
	return false;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (end_info == NULL) {
	*message = strclone("Could not allocate PNG data structures.");
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(png);
	return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
	*message = strclone("Something went wrong with PNG input, but I'm not telling you what.");
	failed:
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(png);
	if (plbuf != NULL)
	    free(plbuf);
	if (plname != NULL)
	    free(plname);
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
    png_set_keep_unknown_chunks(png_ptr, 2, (png_byte *) FW_CHUNK, 1);
    // The libpng docs don't tell you this, but in the call above, the
    // 'keep' argument is ignored, so even though your list of chunk types
    // is recorded in the info structure, the flag that must be set to
    // tell libpng to actually read the unknown chunks stays unset. You have to
    // call png_set_keep_unknowns_chunks() with a num_chunks parameter of
    // zero for the flag to be changed. How fucked up is that? GRRRR!
    png_set_keep_unknown_chunks(png_ptr, 2, NULL, 0);

    png_read_info(png_ptr, info_ptr);
    
    png_unknown_chunk *chunks;
    int nchunks = png_get_unknown_chunks(png_ptr, info_ptr, &chunks);
    for (int i = 0; i < nchunks; i++) {
	png_unknown_chunk *chunk = chunks + i;
	unsigned char *data = chunk->data;
	if (strcmp((char *) data, FW_ID_STR) != 0)
	    continue;
	data += FW_ID_LEN;
	unsigned int length = (data[0] << 24)
			    | (data[1] << 16)
			    | (data[2] << 8)
			    | (data[3]);
	unsigned int crc = (data[4] << 24)
			    | (data[5] << 16)
			    | (data[6] << 8)
			    | (data[7]);
	data += 8;
	unsigned int size = chunk->size - FW_ID_LEN - 8;
	unsigned int actualcrc;
	if (size < length) {
	    snprintf(msgbuf, MSGLEN, "FW Plugin data block found, but it looks truncated\n(%d bytes read, %d bytes expected).", size, length);
	    *message = strclone(msgbuf);
	} else if ((actualcrc = crc32(data, length)) != crc) {
	    snprintf(msgbuf, MSGLEN, "FW Plugin data block found, but it looks corrupted\n(expected CRC: 0x%x, actual: 0x%x).", crc, actualcrc);
	    *message = strclone(msgbuf);
	} else {
	    // Looks good so far. Now, let's extract the plugin name. (No more
	    // sanity checks; we trust the CRC to protect us from random
	    // corruption. We don't expect plugins to check their data.)
	    // NOTE: unlike the corresponding code in the PNM, GIF, and JPEG
	    // modules, we don't accumulate a sequence of FW blocks into a
	    // plugin data block; this is because PNG chunks can be large
	    // (2^31, IIRC), so there's no need to split things up. I only read
	    // the first FW data block and stop looking after that.
	    plname = strclone((char *) data);
	    int pnl = strlen(plname);
	    length -= pnl + 1;
	    plbuf = (char *) malloc(length);
	    memcpy(plbuf, data + pnl + 1, length);
	    plbufsize = length;
	    break;
	}
    }

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
	    snprintf(msgbuf, MSGLEN, "PNGViewer: unknown color_type %d.",
		    color_type);
	    *message = strclone(msgbuf);
	    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	    goto failed;
    }

    switch (pm->depth) {
	case 1:
	    pm->bytesperline = (pm->width + 31 >> 3) & ~3;
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

    if (color_type == PNG_COLOR_TYPE_GRAY) {
	if (bit_depth == 1)
	    png_set_packswap(png_ptr);
	else if (bit_depth == 2 || bit_depth == 4)
	    png_set_gray_1_2_4_to_8(png_ptr);
    } else if (bit_depth < 8)
	png_set_packing(png_ptr);
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
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    free(row_pointers);

    if (plbuf != NULL) {
	*plugin_name = plname;
	*plugin_data = plbuf;
	*plugin_data_length = plbufsize;
    }
    return true;
}

/* public virtual */ bool
ImageIO_PNG::write(const char *filename, const char *plugin_name,
		   const void *plugin_data, int plugin_data_length,
		   const FWPixmap *pm, char **message) {
    char msgbuf[MSGLEN];

    unsigned char *chunkbuf = NULL;
    png_bytepp row_pointers = NULL;

    FILE *png = fopen(filename, "w");
    if (png == NULL) {
	snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).", filename,
		    strerror(errno));
	*message = strclone(msgbuf);
	return false;
    }

    png_structp png_ptr = png_create_write_struct(
		    PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
	*message = strclone("Could not allocate PNG data structures.");
	fclose(png);
	return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
	*message = strclone("Could not allocate PNG data structures.");
	png_destroy_write_struct(&png_ptr, NULL);
	fclose(png);
	return false;
    }


    if (setjmp(png_jmpbuf(png_ptr))) {
	*message = strclone("Something went wrong with PNG output, but I'm not telling you what.");
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(png);
	if (chunkbuf != NULL)
	    free(chunkbuf);
	if (row_pointers != NULL)
	    free(row_pointers);
	return false;
    }

    png_init_io(png_ptr, png);

    // Set compression level to 0 if you want to be able to easily
    // peek at a PNG file's contents.
    // png_set_compression_level(png_ptr, 0);

    // Quote from the libpng docs:
    //
    // > Writing unknown chunks
    // > 
    // > You can use the png_set_unknown_chunks function to queue up chunks
    // > for writing.  You give it a chunk name, raw data, and a size; that's
    // > all there is to it.  The chunks will be written by the next following
    // > png_write_info_before_PLTE, png_write_info, or png_write_end function.
    //
    // Lies!!! If you don't also call png_set_keep_unknown_chunks(), your
    // chunks are unceremoniously dropped on the floor. And even with this
    // call, they'll still land in the bit bucket unless you also set the chunk
    // location, and just *what* to set it to is obscure as hell. (The fact
    // that I got unknown chunk reading and writing to work at all is thanks to
    // a hair-tearing day spent with the debugger, single-stepping through the
    // libpng source code to find out what it *really* does.)
    // Apart from the fact that the jolly phrase "that's all there is to it"
    // is so wrong and misleading that it's just cruel, what's the idea of
    // having to register chunk types in order for them to be written? I mean,
    // having such a mechanism in order to prevent tons of unwanted data from
    // being *read*, sure, but what on Earth is the logic behind filtering
    // stuff THAT I JUST TOLD LIBPNG TO WRITE (when I called
    // png_set_unknown_chunks())?!? GRRRR!
    png_set_keep_unknown_chunks(png_ptr, 2, (png_byte *) FW_CHUNK, 1);

    bool gray = pm->depth == 8 && CopyBits::is_grayscale(pm->cmap);
    png_set_IHDR(png_ptr, info_ptr, pm->width, pm->height,
		 pm->depth == 1 ? 1 : 8,
		 pm->depth == 1 || gray ? PNG_COLOR_TYPE_GRAY
		    : pm->depth == 8 ? PNG_COLOR_TYPE_PALETTE
		    : PNG_COLOR_TYPE_RGB,
		 PNG_INTERLACE_NONE,
		 PNG_COMPRESSION_TYPE_DEFAULT,
		 PNG_FILTER_TYPE_DEFAULT);

    if (plugin_data != NULL && plugin_data_length > 0) {
	// I'm assuming that libpng does not make a copy of the chunk
	// chunk data. (Wouldn't it be nice if the PNG docs just *said*
	// so one way or the other?)
	// So, I keep it around until after I'm done writing the image,
	// and dispose it then. Of course, if libpng *does* take ownership,
	// that won't work either. Can't win. (TODO)

	// TODO: try wiping the chunk after the png_set_unknown_chunks()
	// call, to find out if libpng makes a copy of the data in the
	// chunk descriptor array, or just keeps the pointer to it.

	// TODO: try wiping the chunk data buffer after the
	// png_set_unknown_chunks() call, to find out if libpng makes a copy
	// of the data.
	//
	// TODO: Re: the comments above: looking at the libpng source code,
	// I discovered that libpng copies *everything*. So, there's no need
	// to hold on to anything after having given it to libpng.

	int size = plugin_data_length + FW_ID_LEN + 8;
	chunkbuf = (unsigned char *) malloc(size);
	strcpy((char *) chunkbuf, FW_ID_STR);
	unsigned char *pos = chunkbuf + FW_ID_LEN;
	*pos++ = plugin_data_length >> 24;
	*pos++ = plugin_data_length >> 16;
	*pos++ = plugin_data_length >> 8;
	*pos++ = plugin_data_length;
	unsigned int crc = crc32(plugin_data, plugin_data_length);
	*pos++ = crc >> 24;
	*pos++ = crc >> 16;
	*pos++ = crc >> 8;
	*pos++ = crc;
	memcpy(pos, plugin_data, plugin_data_length);
	
	png_unknown_chunk chunk;
	strcpy((char *) chunk.name, FW_CHUNK);
	chunk.data = chunkbuf;
	chunk.size = size;

	png_set_unknown_chunks(png_ptr, info_ptr, &chunk, 1);
	// The libpng docs tell you not to mess with the chunk 'location'.
	// Lies!!! If you do nothing, location is left at zero, and your
	// chunks are never written at all. GRRRR!
	png_set_unknown_chunk_location(png_ptr, info_ptr, 0, 1);
    }

    if (pm->depth == 8 && !gray) {
	png_color plte[256];
	for (int i = 0; i < 256; i++) {
	    plte[i].red = pm->cmap[i].r;
	    plte[i].green = pm->cmap[i].g;
	    plte[i].blue = pm->cmap[i].b;
	}
	png_set_PLTE(png_ptr, info_ptr, plte, 256);
    }

    png_write_info(png_ptr, info_ptr);

    if (pm->depth == 24)
	png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);
    if (pm->depth == 1)
	png_set_packswap(png_ptr);

    row_pointers = (png_bytepp) malloc(pm->height * sizeof(png_bytep));
    for (int i = 0; i < pm->height; i++)
	row_pointers[i] = (png_bytep) pm->pixels + (pm->bytesperline * i);

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(png);
    free(row_pointers);
    if (chunkbuf != NULL)
	free(chunkbuf);
    return true;
}
