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

#ifndef COPYBITS_H
#define COPYBITS_H 1

#include <X11/Xlib.h>

class FWColor;
class FWPixmap;
class InverseCMap;

class CopyBits {
    public:
	static void copy_unscaled(FWPixmap *pm, XImage *image,
				  bool priv_cmap, bool no_grays, bool dither,
				  int top, int left, int bottom, int right);
	static void copy_enlarged(int factor,
				  FWPixmap *pm, XImage *image,
				  bool priv_cmap, bool no_grays, bool dither,
				  int top, int left, int bottom, int right);
	static void copy_reduced(int factor,
				 FWPixmap *pm, XImage *image,
				 bool priv_cmap, bool no_grays, bool dither,
				 int top, int left, int bottom, int right,
				 InverseCMap *invcmap = NULL);

	static int halftone(unsigned char value, int x, int y);

	static bool is_grayscale(const FWColor *cmap);
	static int realdepth();

	static unsigned long rgb2pixel(unsigned char r,
				       unsigned char g,
				       unsigned char b);
	static void rgb2nearestgrays(unsigned char *r,
				     unsigned char *g,
				     unsigned char *b,
				     unsigned long *pixels);
	static void rgb2nearestcolors(unsigned char *r,
				      unsigned char *g,
				      unsigned char *b,
				      unsigned long *pixels);

	static void hsl2rgb(float h, float s, float l,
			    unsigned char *r, unsigned char *g,
			    unsigned char *b);
	static void rgb2hsl(unsigned char r, unsigned char g,
			    unsigned char b,
			    float *h, float *s, float *l);
	static void hsv2rgb(float h, float s, float v,
			    unsigned char *r, unsigned char *g,
			    unsigned char *b);
	static void rgb2hsv(unsigned char r, unsigned char g,
			    unsigned char b,
			    float *h, float *s, float *v);

};

#endif
