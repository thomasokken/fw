#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ImageIO_PNM.h"
#include "CopyBits.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "util.h"

static bool match_one_blank(FILE *file);
static void optional_blank(FILE *file);
static bool match_blank(FILE *file);
static bool match_blank_or_comment(FILE *file, unsigned char **buf,
				   int *bufsize, int *pufpos);
static bool match_uint(FILE *file, int *i);
static void write_encoded_pnm_comment(FILE *file, const void *data,
				      int length, unsigned int crc);

#define MSGLEN 1024


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
    char msgbuf[MSGLEN];

    FILE *pnm = fopen(filename, "r");
    if (pnm == NULL) {
	snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).",
		 filename, strerror(errno));
	*message = strclone(msgbuf);
	return false;
    }

    int ch = fgetc(pnm);
    if (ch != 'P') {
	snprintf(msgbuf, MSGLEN, "PNM signature not found.");
	*message = strclone(msgbuf);
	fclose(pnm);
	return false;
    }

    int ftype = fgetc(pnm);
    if (ftype < '1' || ftype > '6') {
	snprintf(msgbuf, MSGLEN, "PNM signature not found.");
	*message = strclone(msgbuf);
	fclose(pnm);
	return false;
    }

    unsigned char *buf = NULL;
    int bufsize = 0;
    int bufpos = 0;

    if (!match_blank_or_comment(pnm, &buf, &bufsize, &bufpos)) {
	snprintf(msgbuf, MSGLEN, "PNM file format error.");
	*message = strclone(msgbuf);
	fclose(pnm);
	if (buf != NULL)
	    free(buf);
	return false;
    }

    if (!match_uint(pnm, &pm->width)) {
	snprintf(msgbuf, MSGLEN, "PNM file format error.");
	*message = strclone(msgbuf);
	fclose(pnm);
	if (buf != NULL)
	    free(buf);
	return false;
    }

    if (!match_blank_or_comment(pnm, &buf, &bufsize, &bufpos)) {
	snprintf(msgbuf, MSGLEN, "PNM file format error.");
	*message = strclone(msgbuf);
	fclose(pnm);
	if (buf != NULL)
	    free(buf);
	return false;
    }

    if (!match_uint(pnm, &pm->height)) {
	snprintf(msgbuf, MSGLEN, "PNM file format error.");
	*message = strclone(msgbuf);
	fclose(pnm);
	if (buf != NULL)
	    free(buf);
	return false;
    }

    int maxval;
    if (ftype != '1' && ftype != '4') {
	if (!match_blank_or_comment(pnm, &buf, &bufsize, &bufpos)) {
	    snprintf(msgbuf, MSGLEN, "PNM file format error.");
	    *message = strclone(msgbuf);
	    fclose(pnm);
	    if (buf != NULL)
		free(buf);
	    return false;
	}
	if (!match_uint(pnm, &maxval) || maxval < 1 || maxval > 65535) {
	    snprintf(msgbuf, MSGLEN, "PNM file format error.");
	    *message = strclone(msgbuf);
	    fclose(pnm);
	    if (buf != NULL)
		free(buf);
	    return false;
	}
    }

    if (!match_one_blank(pnm)) {
	snprintf(msgbuf, MSGLEN, "PNM file format error.");
	*message = strclone(msgbuf);
	fclose(pnm);
	if (buf != NULL)
	    free(buf);
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
	    pm->bytesperline = (pm->width + 31 >> 3) & ~3;
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
	    int srcbpl = pm->width + 7 >> 3;
	    for (int v = pm->height - 1; v >= 0; v--) {
		unsigned char *src = pm->pixels + (v + 1) * srcbpl - 1;
		unsigned char *dst = pm->pixels + v * pm->bytesperline
							+ srcbpl - 1;
		for (int h = srcbpl - 1; h >= 0; h--) {
		    unsigned char c = *src--;
		    unsigned char d = 0;
		    for (int j = 0; j < 8; j++) {
			d <<= 1;
			d |= (~c & 1);
			c >>= 1;
		    }
		    *dst-- = d;
		}
	    }
	    break;
	}
	case '5':
	    if (maxval < 256) {
		for (int v = 0; v < pm->height; v++) {
		    unsigned char *p = pm->pixels + v * pm->bytesperline;
		    fread(p, 1, pm->width, pnm);
		    for (int h = 0; h < pm->width; h++)
			*p++ = (((int) *p) * 255) / maxval;
		}
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

    if (buf != NULL) {
	// Comments containing the magic incantation "FW:" have been found.
	// Let's see if we got anything good.
	// The first 4 bytes should be the length; the next 4 bytes should be
	// the CRC for the following 'length' bytes.
	int length = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	unsigned int crc =
		     (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
	unsigned int actualcrc;
	if (length > bufpos - 8) {
	    char msgbuf[1024];
	    snprintf(msgbuf, 1024, "FW Plugin data block found, but it looks truncated\n(%d bytes read, %d bytes expected).", bufpos - 8, length);
	    *message = strclone(msgbuf);
	    free(buf);
	} else if (crc != (actualcrc = crc32(buf + 8, length))) {
	    char msgbuf[1024];
	    snprintf(msgbuf, 1024, "FW Plugin data block found, but it looks corrupted\n(expected CRC: 0x%x, actual: 0x%x).", crc, actualcrc);
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

static bool match_blank_or_comment(FILE *file, unsigned char **buf,
				   int *bufsize, int *bufpos) {
    // This function reads the bit between the PNM signature and the width.
    // It looks for comments containing the string "FW:" and tries to decode
    // the stuff following that string, and store it in a data block in
    // memory, which will later be used to allow the matching plugin to
    // deserialize its state.

    int c = fgetc(file);
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '#')
	return false;
    ungetc(c, file);

    unsigned char inchar[4];
    int k = 0;

    while (true) {
	while ((c = fgetc(file)) == ' ' || c == '\t' || c == '\r' || c == '\n');
	if (c != '#') {
	    ungetc(c, file);
	    return true;
	}
	int bingo = 0;
	while ((c = fgetc(file)) != '\n' && c != '\r' && c != EOF) {
	    if (c == EOF)
		return false;
	    int n = -1;
	    switch (bingo) {
		case 0:
		    if (c == 'F')
			bingo = 1;
		    break;
		case 1:
		    if (c == 'W')
			bingo = 2;
		    else
			bingo = 4;
		    break;
		case 2:
		    if (c == ':')
			bingo = 3;
		    else
			bingo = 4;
		    break;
		case 3:
		    if (c >= '0' && c <= '9')
			n = c - '0';
		    else if (c >= 'A' && c <= 'Z')
			n = c - 'A' + 10;
		    else if (c >= 'a' && c <= 'z')
			n = c - 'a' + 36;
		    else if (c == '.')
			n = 62;
		    else if (c == '/')
			n = 63;
		    else
			bingo = 4;
	    }
	    if (n != -1) {
		inchar[k++] = n;
		k = k & 3;
		if (k == 0) {
		    // We have some bytes!
		    if (*bufpos + 3 > *bufsize) {
			*bufsize += 1024;
			*buf = (unsigned char *) realloc(*buf, *bufsize);
		    }
		    (*buf)[(*bufpos)++] = (inchar[0] << 2) | (inchar[1] >> 4);
		    (*buf)[(*bufpos)++] = (inchar[1] << 4) | (inchar[2] >> 2);
		    (*buf)[(*bufpos)++] = (inchar[2] << 6) | inchar[3];
		}
	    }
	}
    }
}

static bool match_blank(FILE *file) {
    if (!match_one_blank(file))
	return false;
    optional_blank(file);
    return true;
}

static bool match_uint(FILE *file, int *i) {
    int c = fgetc(file);
    if (!isdigit(c))
	return false;
    *i = c - '0';
    while (isdigit(c = fgetc(file)))
	*i = *i * 10 + c - '0';
    if (c != EOF)
	ungetc(c, file);
    return true;
}

/* public virtual */ bool
ImageIO_PNM::write(const char *filename, const char *plugin_name,
		   const void *plugin_data, int plugin_data_length,
		   const FWPixmap *pm, char **message) {
    char msgbuf[MSGLEN];

    FILE *pnm = fopen(filename, "w");
    if (pnm == NULL) {
	snprintf(msgbuf, MSGLEN, "Can't open \"%s\" (%s).",
		 filename, strerror(errno));
	*message = strclone(msgbuf);
	return false;
    }

    bool grayscale = pm->depth == 8 && CopyBits::is_grayscale(pm->cmap);

    switch (pm->depth) {
	case 1:
	    fprintf(pnm, "P4\n");
	    break;
	case 8:
	    if (grayscale)
		fprintf(pnm, "P5\n");
	    else
		fprintf(pnm, "P6\n");
	    break;
	case 24:
	    fprintf(pnm, "P6\n");
	    break;
	default:
	    crash();
    }

    if (plugin_data != NULL && plugin_data_length > 0) {
	write_encoded_pnm_comment(pnm, plugin_data, plugin_data_length,
				  crc32(plugin_data, plugin_data_length));
    }

    fprintf(pnm, "%d %d\n", pm->width, pm->height);
    if (pm->depth != 1)
	fprintf(pnm, "255\n");

    int len = pm->height * pm->bytesperline;
    if (pm->depth == 1) {
	int pbmbpl = pm->width + 7 >> 3;
	for (int v = 0; v < pm->height; v++) {
	    unsigned char *ptr = pm->pixels + v * pm->bytesperline;
	    for (int h = 0; h < pbmbpl; h++) {
		char c = *ptr++;
		char d = 0;
		for (int j = 0; j < 8; j++) {
		    d <<= 1;
		    d |= (~c & 1);
		    c >>= 1;
		}
		fputc(d, pnm);
	    }
	}
    } else if (pm->depth == 8 && grayscale) {
	for (int v = 0; v < pm->height; v++)
	    fwrite(pm->pixels + v * pm->bytesperline, pm->width, 1, pnm);
    } else if (pm->depth == 8) {
	unsigned char *ptr = pm->pixels;
	for (int i = 0; i < len; i++) {
	    unsigned char pix = *ptr++;
	    fputc(pm->cmap[pix].r, pnm);
	    fputc(pm->cmap[pix].g, pnm);
	    fputc(pm->cmap[pix].b, pnm);
	}
    } else {
	unsigned char *ptr = pm->pixels;
	for (int i = 0; i < len; i++) {
	    if ((i & 3) != 0)
		fputc(*ptr, pnm);
	    ptr++;
	}
    }

    fclose(pnm);
    return true;
}

static char chartab[] =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz./";

static void write_encoded_pnm_comment(FILE *file, const void *data, int length,
				      unsigned int crc) {

    // This function writes first the four-byte data block length,
    // then the four-byte CRC, and finally the data block itself. The data
    // are padded out with zeros to make the total length (length, crc, and
    // data) come out a multiple of 3 bytes.
    // The bytes are encoded 6 bits to the character (like uuencode or base64,
    // but probably not compatible with either -- I didn't check and I don't
    // really care!). All of this is written as PNM comments, that is, lines
    // starting with a '#' character, 64 encoded characters (corresponding to
    // 48 bytes) per line. The lines start with "FW:" to help allow FW to
    // distinguish between its encoded data, and other PNM comments.

    unsigned char *ptr = (unsigned char *) data;
    int ngroups = 0;
    int pos3 = 0;
    unsigned char group[3];
    bool newline = true;

    // Make sure we write an even multiple of 3 bytes, including the 8
    // bytes for length and crc. Note: the 'length' we write in the first 4
    // bytes includes neither the 8 bytes for length and crc, nor the
    // padding.
    int imax;
    switch (length % 3) {
	case 0: imax = length + 1; break;
	case 1: imax = length; break;
	case 2: imax = length + 2; break;
    }

    for (int i = -8; i < imax; i++) {
	unsigned char ch;
	if (i < 0) {
	    switch (i) {
		case -8: ch = length >> 24; break;
		case -7: ch = length >> 16; break;
		case -6: ch = length >> 8; break;
		case -5: ch = length; break;
		case -4: ch = crc >> 24; break;
		case -3: ch = crc >> 16; break;
		case -2: ch = crc >> 8; break;
		case -1: ch = crc; break;
	    }
	} else if (i >= length)
	    ch = 0;
	else
	    ch = *ptr++;

	group[pos3] = ch;
	pos3 = (pos3 + 1) % 3;

	if (pos3 == 0) {
	    if (newline) {
		fprintf(file, "# FW:");
		newline = false;
	    }
	    fputc(chartab[group[0] >> 2], file);
	    fputc(chartab[((group[0] & 0x03) << 4) | (group[1] & 0xF0) >> 4], file);
	    fputc(chartab[((group[1] & 0x0F) << 2) | (group[2] >> 6)], file);
	    fputc(chartab[group[2] & 0x3F], file);
	    if (++ngroups == 16) {
		fprintf(file, "\n");
		ngroups = 0;
		newline = true;
	    }
	}
    }

    if (!newline)
	fprintf(file, "\n");
}
