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
