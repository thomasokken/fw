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

#ifndef FWPIXMAP_H
#define FWPIXMAP_H 1

class FWColor;

struct FWPixmap {
    // NOTE: 'pixels' should be allocated using malloc() and freed using
    // free(); 'cmap' should be allocated using new FWColor[] and freed
    // using delete[].
    // Only depths 1, 8, and 24 are supported. When depth == 8, 'cmap' is
    // assumed to point to a colormap. Maybe FW should also support a mode
    // where depth == 8 and cmap == NULL, and treat that as 8 bit greyscale;
    // the rendering code in Viewer::paint() could be optimized for that
    // case.
    // If 'cmap' is not NULL, it should point to an array of 256 FWColors,
    // never fewer. If there are more than 256 elements in the array, the
    // excess is ignored.

    unsigned char *pixels;
    FWColor *cmap;
    int depth;
    int width, height, bytesperline;

    FWPixmap() {
	pixels = NULL;
	cmap = NULL;
    }

    void put_pixel(int x, int y, unsigned long p) {
	if (depth == 1) {
	    unsigned char m = 1 << (x & 7);
	    unsigned char *b = pixels + y * bytesperline + (x >> 3);
	    if (p == 0)
		*b &= ~m;
	    else
		*b |= m;
	} else if (depth == 8) {
	    pixels[y * bytesperline + x] = p;
	} else if (depth == 24) {
	    unsigned char *ptr = pixels + y * bytesperline + 4 * x;
	    *ptr++ = 0;
	    *ptr++ = p >> 16;
	    *ptr++ = p >> 8;
	    *ptr = p;
	}
    }
};

#endif
