/*-
 * Copyright (c) 2002 by Thomas Okken <thomas.okken@erols.com>
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Revision History:
 * 26-Jan-2002: First version.
 * 31-Jul-2004: Fractal Wizard Plugin version.
 */

#include <stdlib.h>
#include <string.h>

#include "Plugin.h"

static unsigned short crc16[] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/* number of generations to keep track of for detecting loops */
#define HISTORY 32

/* number of times a loop must be detected before I actually restart */
#define PATIENCE 100

#define swap_bytes(x) (((x) >> 24) \
		     | (((x) & 0xff0000) >> 8) \
		     | (((x) & 0xff00) << 8) \
		     | ((x) << 24))

static const char *my_settings_layout[] = {
    "WIDTH 'Width'",	// width
    "HEIGHT 'Height'",	// height
    "int",		// repeats
    "REPEAT 32",	// history[HISTORY]
    "int",
    "ENDREP",
    NULL
};


class Life : public Plugin {
    private:
	int repeats;
	unsigned int history[HISTORY];
	bool initialized;
	int hwords;
	int bitmapsize;
	unsigned *bitmap1, *bitmap2;
	unsigned int rightedgemask;
	bool bigendian;

    public:
	Life(void *dl) : Plugin(dl) {
	    register_for_serialization(my_settings_layout, &repeats);
	    initialized = false;
	    bitmap1 = NULL;
	    bitmap2 = NULL;

	    int foo = 0x01020304;
	    bigendian = *((char *) &foo) == 0x01;
	}

	virtual ~Life() {
	    if (bitmap1 != NULL && bitmap1 != (unsigned int *) pm->pixels)
		free(bitmap1);
	    if (bitmap2 != NULL && bitmap2 != (unsigned int *) pm->pixels)
		free(bitmap2);
	}

	virtual const char *name() {
	    return "Life";
	}

	virtual bool does_depth(int depth) {
	    return depth == 1;
	}

	virtual void init_new() {
	    pm->width = 300;
	    pm->height = 300;
	    get_settings_dialog();
	}

	virtual void get_settings_ok() {
	    pm->bytesperline = (pm->width + 31 >> 3) & ~3;
	    int size = pm->bytesperline * pm->height;
	    pm->pixels = (unsigned char *) malloc(size);
	    memset(pm->pixels, 0, size);
	    init_proceed();
	}

	virtual void start() {
	    hwords = pm->width + 31 >> 5;
	    bitmapsize = hwords * sizeof(unsigned int) * pm->height;
	    if (bigendian)
		bitmap1 = (unsigned int *) malloc(bitmapsize);
	    else
		bitmap1 = (unsigned int *) pm->pixels;
	    bitmap2 = (unsigned int *) malloc(bitmapsize);

	    repeats = 0;
	    rightedgemask = 0xffffffff >> 31 - (pm->width - 1 & 31);
	    start_working();
	}

	virtual void stop() {
	    stop_working();
	}
	
	virtual void restart() {
	    if (!initialized) {
		// Only pm->pixels is saved when FW serializes a plugin;
		// we need to reallocate and repopulate our other dynamic
		// arrays.
		hwords = pm->width + 31 >> 5;
		bitmapsize = hwords * sizeof(unsigned int) * pm->height;
		if (bigendian) {
		    bitmap1 = (unsigned int *) malloc(bitmapsize);
		    for (int i = 0; i < bitmapsize >> 2; i++) {
			unsigned int tmp = ((unsigned int *) pm->pixels)[i];
			bitmap1[i] = swap_bytes(tmp);
		    }
		} else
		    bitmap1 = (unsigned int *) pm->pixels;
		bitmap2 = (unsigned int *) malloc(bitmapsize);
		rightedgemask = 0xffffffff >> 31 - (pm->width - 1 & 31);
	    }

	    start_working();
	}

	virtual bool work() {
	    int index, aboveindex, belowindex, x, y, i, hit;
	    unsigned short crc;
	    unsigned int *temp;
	    int notattop, notatbottom;

	    if (repeats == 0) {
		repeats = PATIENCE;
		memset(bitmap1, 0, bitmapsize);
		memset(history, 0, HISTORY * sizeof(unsigned int));

		for (y = 0; y < pm->height; y++)
		    for (x = 0; x < pm->width; x++)
			if (rand() < RAND_MAX >> 1) {
			    int a = (x >> 5) + y * hwords;
			    int b = x & 31;
			    bitmap1[a] |= 1 << b;
			}
		if (bigendian)
		    for (i = 0; i < bitmapsize >> 2; i++) {
			unsigned int tmp = bitmap1[i];
			((unsigned int *) pm->pixels)[i] = swap_bytes(tmp);
		    }

		paint();
		return false;
	    }

	    crc = 0;
	    index = 0;
	    aboveindex = -hwords;
	    belowindex = hwords;
	    for (y = 0; y < pm->height; y++) {
		notattop = y != 0;
		notatbottom = y != pm->height - 1;
		for (x = 0; x < hwords; x++) {
		    unsigned int s0, s1, s2, s3, r;
		    unsigned int w00, w01, w02, w10, w11, w12, w20, w21, w22;
		    w10 = notattop ? bitmap1[aboveindex] : 0;
		    w11 = bitmap1[index];
		    w12 = notatbottom ? bitmap1[belowindex] : 0;
		    w00 = w10 << 1;
		    w01 = w11 << 1;
		    w02 = w12 << 1;
		    if (x != 0) {
			if (notattop && bitmap1[aboveindex - 1] & 0x80000000)
			    w00 |= 1;
			if (bitmap1[index - 1] & 0x80000000)
			    w01 |= 1;
			if (notatbottom && bitmap1[belowindex - 1] & 0x80000000)
			    w02 |= 1;
		    }
		    w20 = w10 >> 1;
		    w21 = w11 >> 1;
		    w22 = w12 >> 1;
		    if (x != hwords - 1) {
			if (notattop && bitmap1[aboveindex + 1] & 1)
			    w20 |= 0x80000000;
			if (bitmap1[index + 1] & 1)
			    w21 |= 0x80000000;
			if (notatbottom && bitmap1[belowindex + 1] & 1)
			    w22 |= 0x80000000;
		    }

		    s1 =      w00;
		    s0 =                 ~w00;

		    s2 = s1 & w01;
		    s1 = s0 & w01 | s1 & ~w01;
		    s0 =            s0 & ~w01;

		    s3 = s2 & w02;
		    s2 = s1 & w02 | s2 & ~w02;
		    s1 = s0 & w02 | s1 & ~w02;
		    s0 =            s0 & ~w02;

		    s3 = s2 & w10 | s3 & ~w10;
		    s2 = s1 & w10 | s2 & ~w10;
		    s1 = s0 & w10 | s1 & ~w10;
		    s0 =            s0 & ~w10;

		    s3 = s2 & w12 | s3 & ~w12;
		    s2 = s1 & w12 | s2 & ~w12;
		    s1 = s0 & w12 | s1 & ~w12;
		    s0 =            s0 & ~w12;

		    s3 = s2 & w20 | s3 & ~w20;
		    s2 = s1 & w20 | s2 & ~w20;
		    s1 = s0 & w20 | s1 & ~w20;
		    s0 =            s0 & ~w20;

		    s3 = s2 & w21 | s3 & ~w21;
		    s2 = s1 & w21 | s2 & ~w21;
		    s1 = s0 & w21 | s1 & ~w21;

		    s3 = s2 & w22 | s3 & ~w22;
		    s2 = s1 & w22 | s2 & ~w22;
		    
		    r = s3 | s2 & w11;
		    if (x == hwords - 1)
			r &= rightedgemask;
		    bitmap2[index] = r;
		    if (bigendian)
			((unsigned int *) pm->pixels)[index] = swap_bytes(r);

		    for (i = 0; i < 4; i++) {
			crc = crc >> 8 ^ crc16[crc & 0xFF ^ r & 0xFF];
			r >>= 8;
		    }
		    index++;
		    aboveindex++;
		    belowindex++;
		}
	    }
	    if (!bigendian)
		pm->pixels = (unsigned char *) bitmap2;

	    temp = bitmap1;
	    bitmap1 = bitmap2;
	    bitmap2 = temp;

	    /* I use 16-bit CRCs to detect loops.
	     * Finding a matching CRC is not an absolute guarantee that a loop
	     * is occurring; for this reason, I wait until I detect PATIENCE
	     * consecutive matches. The maximum loop length that I detect is
	     * given by HISTORY. Don't set this to a too-high value; long loops
	     * are bound to be interesting so you don't want to miss them just
	     * because you left the room for a minute.
	     * TODO: how about setting HISTORY to something way high, and
	     * adding some code that checks game and loop length after
	     * termination, and saves the initial state for games with long
	     * durations and/or long loops?
	    */
	    hit = 0;
	    for (i = 0; i < HISTORY; i++) {
		if (crc == history[i])
		    hit = 1;
		if (i == HISTORY - 1)
		    history[i] = crc;
		else
		    history[i] = history[i + 1];
	    }
	    if (hit)
		repeats--;
	    else
		repeats = PATIENCE;

	    paint();
	    return false;
	}
};

extern "C" Plugin *factory(void *dl) {
    return new Life(dl);
}
