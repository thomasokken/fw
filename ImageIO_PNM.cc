#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ImageIO_PNM.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "util.h"

static bool match_one_blank(FILE *file);
static void optional_blank(FILE *file);
static bool match_blank(FILE *file);
static bool match_uint(FILE *file, int *i);
static int get_one_char(FILE *file);

/* public virtual */ bool
ImageIO_PNM::can_read(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL)
	return false;
    char magic[2];
    int n = fread(magic, 1, 2, file);
    fclose(file);
    if (n != 2)
	return false;
    return magic[0] == 'P' && magic[1] >= '1' && magic[1] <= '6';
}

/* public virtual */ bool
ImageIO_PNM::read(const char *filename, char **plugin_name,
		  void **plugin_data, int *plugin_data_length,
		  FWPixmap *pm, char **message) {
    *message = strclone("foo");

    FILE *pnm = fopen(filename, "r");
    if (pnm == NULL) {
	fprintf(stderr, "PNMViewer: Can't open \"%s\".\n", filename);
	return false;
    }

    int ch = fgetc(pnm);
    if (ch != 'P') {
	fprintf(stderr, "PNMViewer: PNM signature not found.\n");
	fclose(pnm);
	return false;
    }

    int ftype = fgetc(pnm);
    if (ftype < '1' || ftype > '6') {
	fprintf(stderr, "PNMViewer: PNM signature not found.\n");
	fclose(pnm);
	return false;
    }

    if (!match_blank(pnm)) {
	fprintf(stderr, "PNMViewer: File format error.\n");
	fclose(pnm);
	return false;
    }

    if (!match_uint(pnm, &pm->width)) {
	fprintf(stderr, "PNMViewer: File format error.\n");
	fclose(pnm);
	return false;
    }

    if (!match_blank(pnm)) {
	fprintf(stderr, "PNMViewer: File format error.\n");
	fclose(pnm);
	return false;
    }

    if (!match_uint(pnm, &pm->height)) {
	fprintf(stderr, "PNMViewer: File format error.\n");
	fclose(pnm);
	return false;
    }

    int maxval;
    if (ftype != '1' && ftype != '4') {
	if (!match_blank(pnm)) {
	    fprintf(stderr, "PNMViewer: File format error.\n");
	    fclose(pnm);
	    return false;
	}
	if (!match_uint(pnm, &maxval) || maxval < 1 || maxval > 65535) {
	    fprintf(stderr, "PNMViewer: File format error.\n");
	    fclose(pnm);
	    return false;
	}
    }

    if (!match_one_blank(pnm)) {
	fprintf(stderr, "PNMViewer: File format error.\n");
	fclose(pnm);
	return false;
    }

    switch (ftype) {
	case '1':
	case '4':
	    pm->depth = 1;
	    break;
	case '2':
	case '5':
	    pm->depth = 8;
	    break;
	case '3':
	case '6':
	    pm->depth = 24;
	    break;
    }

    switch (pm->depth) {
	case 1:
	    // 0=black 1=white
	    pm->bytesperline = (pm->width + 7) >> 3;
	    break;
	case 8:
	    // 0=black 255=white
	    pm->bytesperline = pm->width + 3 & ~3;
	    break;
	case 24:
	    // #000000=black #ffffff=white
	    pm->bytesperline = pm->width * 4;
	    break;
    }

    int size = pm->bytesperline * pm->height;
    pm->pixels = (unsigned char *) malloc(size);
    memset(pm->pixels, 0, size);
    if (pm->depth == 8) {
	pm->cmap = new FWColor[256];
	for (int k = 0; k < 256; k++) {
	    pm->cmap[k].r = k;
	    pm->cmap[k].g = k;
	    pm->cmap[k].b = k;
	}
    }

    switch (ftype) {
	case '1':
	    for (int v = 0; v < pm->height; v++)
		for (int h = 0; h < pm->width; h++) {
		    int ch;
		    while ((ch = fgetc(pnm)) != EOF && ch != '0'
			    && ch != '1');
		    if (ch == EOF)
			// Premature end-of-file; not fatal.
			goto eof;
		    if (ch == '0')
			pm->pixels[pm->bytesperline * v + (h >> 3)] |=
			    1 << (h & 7);
		}
	    break;
	case '2':
	    for (int v = 0; v < pm->height; v++)
		for (int h = 0; h < pm->width; h++) {
		    if (v == 0 && h == 0)
			optional_blank(pnm);
		    else
			if (!match_blank(pnm))
			    // Format error; stop reading
			    goto eof;
		    int d;
		    if (!match_uint(pnm, &d))
			// Format error; stop reading
			goto eof;
		    d = (d * 255) / maxval;
		    pm->pixels[pm->bytesperline * v + h] = d;
		}
	    break;
	case '3':
	    for (int v = 0; v < pm->height; v++)
		for (int h = 0; h < pm->width; h++)
		    for (int k = 1; k < 4; k++) {
			if (v == 0 && h == 0 && k == 1)
			    optional_blank(pnm);
			else
			    if (!match_blank(pnm))
				// Format error; stop reading
				goto eof;
			int d;
			if (!match_uint(pnm, &d))
			    // Format error; stop reading
			    goto eof;
			d = (d * 255) / maxval;
			pm->pixels[pm->bytesperline * v + h * 4 + k] = d;
		    }
	    break;
	case '4': {
	    fread(pm->pixels, 1, size, pnm);
	    unsigned char *p = pm->pixels;
	    for (int i = 0; i < size; i++) {
		char c = *p;
		char d = 0;
		for (int j = 0; j < 8; j++) {
		    d |= (~c & 1);
		    if (j < 7)
			d <<= 1;
		    c >>= 1;
		}
		*p++ = d;
	    }
	    break;
	}
	case '5':
	    if (maxval < 256) {
		fread(pm->pixels, 1, size, pnm);
		unsigned char *p = pm->pixels;
		for (int i = 0; i < size; i++)
		    *p++ = (((int) *p) * 255) / maxval;
	    } else {
		unsigned char *p = pm->pixels;
		for (int i = 0; i < size; i++) {
		    int c1 = fgetc(pnm);
		    if (c1 == EOF)
			// Premature end-of-file; not fatal.
			goto eof;
		    int c2 = fgetc(pnm);
		    if (c2 == EOF)
			// Premature end-of-file; not fatal.
			goto eof;
		    int d = (c1 << 8) | c2;
		    d = (d * 255) / maxval;
		    *p++ = d;
		}
	    }
	    break;
	case '6': {
	    unsigned char *p = pm->pixels;
	    for (int i = 0; i < size; i++)
		for (int k = 1; k < 4; k++) {
		    int d = fgetc(pnm);
		    if (d == EOF)
			// Premature end-of-file; not fatal.
			goto eof;
		    if (maxval > 255) {
			int c2 = fgetc(pnm);
			if (d == EOF)
			    // Premature end-of-file; not fatal.
			    goto eof;
			d = (d << 8) | c2;
		    }
		    d = (d * 255) / maxval;
		    if (k == 1)
			p++;
		    *p++ = d;
		}
	    }
	    break;
    }
    eof:
    
    fclose(pnm);
    return true;
}

static bool match_one_blank(FILE *file) {
    int c = fgetc(file);
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void optional_blank(FILE *file) {
    int c;
    while ((c = fgetc(file)) == ' ' || c == '\t' || c == '\r' || c == '\n');
    if (c != EOF)
	ungetc(c, file);
}

static bool match_blank(FILE *file) {
    int c = get_one_char(file);
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
	return false;
    while ((c = fgetc(file)) == ' ' || c == '\t' || c == '\r' || c == '\n');
    if (c != EOF)
	ungetc(c, file);
    return true;
}

static bool match_uint(FILE *file, int *i) {
    int c = get_one_char(file);
    if (!isdigit(c))
	return false;
    *i = c - '0';
    while (isdigit(c = fgetc(file)))
	*i = *i * 10 + c - '0';
    if (c != EOF)
	ungetc(c, file);
    return true;
}

static int get_one_char(FILE *file) {
    int c;
    while ((c = fgetc(file)) == '#')
	while ((c = fgetc(file)) != '\n' && c != '\r' && c != EOF);
    return c;
}

/* public virtual */ bool
ImageIO_PNM::write(const char *filename, char *plugin_name,
		   const void *plugin_data, int plugin_data_length,
		   const FWPixmap *pm, char **message) {
    *message = strclone("foo");
    return false;
}
