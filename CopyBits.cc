#include "CopyBits.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "main.h"


static bool inited = false;
static int rmax, rmult, bmax, bmult, gmax, gmult;

static void calc_rgb_masks() {
    if (inited)
	return;
    if (g_visual->c_class != TrueColor && g_visual->c_class != DirectColor) {
	inited = true;
	return;
    }
    rmax = g_visual->red_mask;
    rmult = 0;
    while ((rmax & 1) == 0) {
	rmax >>= 1;
	rmult++;
    }
    gmax = g_visual->green_mask;
    gmult = 0;
    while ((gmax & 1) == 0) {
	gmax >>= 1;
	gmult++;
    }
    bmax = g_visual->blue_mask;
    bmult = 0;
    while ((bmax & 1) == 0) {
	bmax >>= 1;
	bmult++;
    }
    inited = true;
}


/* public static */ void
CopyBits::copy_unscaled(FWPixmap *pm, XImage *image,
			bool priv_cmap, bool no_grays, bool dither,
			int top, int left, int bottom, int right) {
    calc_rgb_masks();

    if (pm->depth == 1) {
	// Black and white are always available, so we never have to
	// do anything fancy to render 1-bit images, so we use this simple
	// special-case code that does not do the error diffusion thing.
	unsigned long black = BlackPixel(g_display, g_screennumber);
	unsigned long white = WhitePixel(g_display, g_screennumber);
	for (int y = top; y < bottom; y++)
	    for (int x = left; x < right; x++)
		XPutPixel(image, x, y,
			(pm->pixels[y * pm->bytesperline + (x >> 3)] & 1 << (x & 7))
			    == 0 ? black : white);
    } else if (pm->depth == 8 && priv_cmap) {
	// As in the 1-bit case, in this case we know all our colors are
	// available, so no dithering is required and all we do is copy pixels.
	for (int y = top; y < bottom; y++)
	    for (int x = left; x < right; x++)
		XPutPixel(image, x, y, pm->pixels[y * pm->bytesperline + x]);
    } else if (!dither && (g_grayramp == NULL || priv_cmap)) {
	// Color display, not dithered
	for (int y = top; y < bottom; y++) {
	    for (int x = left; x < right; x++) {
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		unsigned long pixel;
		if (priv_cmap) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp)
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int rerr = (r + 25) % 51 - 25;
		    int gerr = (g + 25) % 51 - 25;
		    int berr = (b + 25) % 51 - 25;
		    int color_err = rerr * rerr + gerr * gerr + berr * berr;
		    int k = (r * 306 + g * 601 + b * 117) / 1024;
		    rerr = r - k;
		    gerr = g - k;
		    berr = b - k;
		    int gray_err = rerr * rerr + gerr * gerr + berr * berr;
		    if (color_err < gray_err || no_grays) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
		    } else {
			int kk = (k * 6 + 17) / 34;
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
		    }
		} else if (g_colorcube != NULL) {
		    int index = (((r * (g_cubesize - 1) + 127) / 255)
			    * g_cubesize
			    + ((g * (g_cubesize - 1) + 127) / 255))
			    * g_cubesize
			    + ((b * (g_cubesize - 1) + 127) / 255);
		    pixel = g_colorcube[index].pixel;
		} else {
		    pixel = (((r * rmax + 127) / 255) << rmult)
			    + (((g * gmax + 127) / 255) << gmult)
			    + (((b * bmax + 127) / 255) << bmult);
		}
		XPutPixel(image, x, y, pixel);
	    }
	}
    } else if (dither && (g_grayramp == NULL || priv_cmap)) {
	// Color display, dithered
	int *dr = new int[right - left];
	int *dg = new int[right - left];
	int *db = new int[right - left];
	int *nextdr = new int[right - left];
	int *nextdg = new int[right - left];
	int *nextdb = new int[right - left];
	for (int i = 0; i < right - left; i++)
	    dr[i] = dg[i] = db[i] = nextdr[i] = nextdg[i] = nextdb[i] = 0;
	int dR = 0, dG = 0, dB = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((y & 1) << 1) - 1;
	    int start, end;
	    if (dir == 1) {
		start = left;
		end = right;
	    } else {
		start = right - 1;
		end = left - 1;
	    }
	    int *temp;
	    temp = nextdr; nextdr = dr; dr = temp;
	    temp = nextdg; nextdg = dg; dg = temp;
	    temp = nextdb; nextdb = db; db = temp;
	    dR = dG = dB = 0;
	    for (int x = start; x != end; x += dir) {
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		r += (dr[x - left] + dR) >> 4;
		if (r < 0) r = 0; else if (r > 255) r = 255;
		dr[x - left] = 0;
		g += (dg[x - left] + dG) >> 4;
		if (g < 0) g = 0; else if (g > 255) g = 255;
		dg[x - left] = 0;
		b += (db[x - left] + dB) >> 4;
		if (b < 0) b = 0; else if (b > 255) b = 255;
		db[x - left] = 0;
		unsigned long pixel;
		if (priv_cmap) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int dr1 = (r + 25) % 51 - 25;
		    int dg1 = (g + 25) % 51 - 25;
		    int db1 = (b + 25) % 51 - 25;
		    int color_err = dr1 * dr1 + dg1 * dg1 + db1 * db1;
		    int k = (r * 306 + g * 601 + b * 117) / 1024;
		    int dr2 = r - k;
		    int dg2 = g - k;
		    int db2 = b - k;
		    int gray_err = dr2 * dr2 + dg2 * dg2 + db2 * db2;
		    if (color_err < gray_err || no_grays) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
			dR = dr1;
			dG = dg1;
			dB = db1;
		    } else {
			int kk = (k * 6 + 17) / 34;
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
			dR = dr2;
			dG = dg2;
			dB = db2;
		    }
		} else if (g_colorcube != NULL) {
		    int index = (((r * (g_cubesize - 1) + 127) / 255)
			    * g_cubesize
			    + ((g * (g_cubesize - 1) + 127) / 255))
			    * g_cubesize
			    + ((b * (g_cubesize - 1) + 127) / 255);
		    dR = r - (g_colorcube[index].red >> 8);
		    dG = g - (g_colorcube[index].green >> 8);
		    dB = b - (g_colorcube[index].blue >> 8);
		    pixel = g_colorcube[index].pixel;
		} else {
		    int ri = (r * rmax + 127) / 255;
		    int gi = (g * gmax + 127) / 255;
		    int bi = (b * bmax + 127) / 255;
		    pixel = (ri << rmult) + (gi << gmult) + (bi << bmult);
		    dR = r - ri * 255 / rmax;
		    dG = g - gi * 255 / gmax;
		    dB = b - bi * 255 / bmax;
		}
		XPutPixel(image, x, y, pixel);
		int prevx = x - dir;
		int nextx = x + dir;
		if (prevx >= left && prevx < right) {
		    nextdr[prevx - left] += dR * 3;
		    nextdg[prevx - left] += dG * 3;
		    nextdb[prevx - left] += dB * 3;
		}
		nextdr[x - left] += dR * 5;
		nextdg[x - left] += dG * 5;
		nextdb[x - left] += dB * 5;
		if (nextx >= left && nextx < right) {
		    nextdr[nextx - left] += dR;
		    nextdg[nextx - left] += dG;
		    nextdb[nextx - left] += dB;
		}
		dR *= 7;
		dG *= 7;
		dB *= 7;
	    }
	}
	delete[] dr;
	delete[] dg;
	delete[] db;
	delete[] nextdr;
	delete[] nextdg;
	delete[] nextdb;
    } else if (!dither) {
	// Grayscale display, not dithered
	for (int y = top; y < bottom; y++) {
	    for (int x = left; x < right; x++) {
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		int k = (r * 306 + g * 601 + b * 117) / 1024;
		int graylevel = (k * (g_rampsize - 1) + 127) / 255;
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		unsigned long pixel = g_grayramp[graylevel].pixel;
		XPutPixel(image, x, y, pixel);
	    }
	}
    } else {
	// Grayscale display, dithered
	int *dk = new int[right - left];
	int *nextdk = new int[right - left];
	for (int i = 0; i < right - left; i++)
	    dk[i] = nextdk[i] = 0;
	int dK = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((y & 1) << 1) - 1;
	    int start, end;
	    if (dir == 1) {
		start = left;
		end = right;
	    } else {
		start = right - 1;
		end = left - 1;
	    }
	    int *temp;
	    temp = nextdk; nextdk = dk; dk = temp;
	    dK = 0;
	    for (int x = start; x != end; x += dir) {
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		int k = (r * 306 + g * 601 + b * 117) / 1024;
		k += (dk[x - left] + dK) >> 4;
		if (k < 0) k = 0; else if (k > 255) k = 255;
		dk[x - left] = 0;
		int graylevel = (k * (g_rampsize - 1) + 127) / 255;
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		dK = k - (g_grayramp[graylevel].red >> 8);
		unsigned long pixel = g_grayramp[graylevel].pixel;
		XPutPixel(image, x, y, pixel);
		int prevx = x - dir;
		int nextx = x + dir;
		if (prevx >= left && prevx < right)
		    nextdk[prevx - left] += dK * 3;
		nextdk[x - left] += dK * 5;
		if (nextx >= left && nextx < right)
		    nextdk[nextx - left] += dK;
		dK *= 7;
	    }
	}
	delete[] dk;
	delete[] nextdk;
    }
}

/* public static */ void
CopyBits::copy_enlarged(int factor,
			FWPixmap *pm, XImage *image,
			bool priv_cmap, bool no_grays, bool dither,
			int top, int left, int bottom, int right) {
    calc_rgb_masks();

    int TOP = top * factor;
    int BOTTOM = bottom * factor;
    int LEFT = left * factor;
    int RIGHT = right * factor;

    if (pm->depth == 1) {
	// Black and white are always available, so we never have to
	// do anything fancy to render 1-bit images, so we use this simple
	// special-case code that does not do the error diffusion thing.
	unsigned long black = BlackPixel(g_display, g_screennumber);
	unsigned long white = WhitePixel(g_display, g_screennumber);
	for (int y = top; y < bottom; y++) {
	    int Y = y * factor;
	    for (int x = left; x < right; x++) {
		int X = x * factor;
		unsigned long pixel =
			(pm->pixels[y * pm->bytesperline + (x >> 3)] & 1 << (x & 7))
			    == 0 ? black : white;
		for (int YY = Y; YY < Y + factor; YY++)
		    for (int XX = X; XX < X + factor; XX++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else if (pm->depth == 8 && priv_cmap) {
	// As in the 1-bit case, in this case we know all our colors are
	// available, so no dithering is required and all we do is copy pixels.
	for (int y = top; y < bottom; y++) {
	    int Y = y * factor;
	    for (int x = left; x < right; x++) {
		int X = x * factor;
		unsigned long pixel = pm->pixels[y * pm->bytesperline + x];
		for (int YY = Y; YY < Y + factor; YY++)
		    for (int XX = X; XX < X + factor; XX++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else if (!dither && (g_grayramp == NULL || priv_cmap)) {
	// Color display, not dithered
	for (int y = top; y < bottom; y++) {
	    int Y = y * factor;
	    for (int x = left; x < right; x++) {
		int X = x * factor;
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		unsigned long pixel;
		if (priv_cmap) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int rerr = (r + 25) % 51 - 25;
		    int gerr = (g + 25) % 51 - 25;
		    int berr = (b + 25) % 51 - 25;
		    int color_err = rerr * rerr + gerr * gerr + berr * berr;
		    int k = (r * 306 + g * 601 + b * 117) / 1024;
		    rerr = r - k;
		    gerr = g - k;
		    berr = b - k;
		    int gray_err = rerr * rerr + gerr * gerr + berr * berr;
		    if (color_err < gray_err || no_grays) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
		    } else {
			int kk = (k * 6 + 17) / 34;
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
		    }
		} else if (g_colorcube != NULL) {
		    int index = (((r * (g_cubesize - 1) + 127) / 255)
			    * g_cubesize
			    + ((g * (g_cubesize - 1) + 127) / 255))
			    * g_cubesize
			    + ((b * (g_cubesize - 1) + 127) / 255);
		    pixel = g_colorcube[index].pixel;
		} else {
		    pixel = (((r * rmax + 127) / 255) << rmult)
			    + (((g * gmax + 127) / 255) << gmult)
			    + (((b * bmax + 127) / 255) << bmult);
		}
		for (int XX = X; XX < X + factor; XX++)
		    for (int YY = Y; YY < Y + factor; YY++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else if (dither && (g_grayramp == NULL || priv_cmap)) {
	// Color display, dithered
	int *dr = new int[RIGHT - LEFT];
	int *dg = new int[RIGHT - LEFT];
	int *db = new int[RIGHT - LEFT];
	int *nextdr = new int[RIGHT - LEFT];
	int *nextdg = new int[RIGHT - LEFT];
	int *nextdb = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dr[i] = dg[i] = db[i] = nextdr[i] = nextdg[i] = nextdb[i] = 0;
	int dR = 0, dG = 0, dB = 0;
	for (int Y = TOP; Y < BOTTOM; Y++) {
	    int y = Y / factor;
	    int dir = ((Y & 1) << 1) - 1;
	    int START, END;
	    if (dir == 1) {
		START = LEFT;
		END = RIGHT;
	    } else {
		START = RIGHT - 1;
		END = LEFT - 1;
	    }
	    int *temp;
	    temp = nextdr; nextdr = dr; dr = temp;
	    temp = nextdg; nextdg = dg; dg = temp;
	    temp = nextdb; nextdb = db; db = temp;
	    dR = dG = dB = 0;
	    for (int X = START; X != END; X += dir) {
		int x = X / factor;
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		r += (dr[X - LEFT] + dR) >> 4;
		if (r < 0) r = 0; else if (r > 255) r = 255;
		dr[X - LEFT] = 0;
		g += (dg[X - LEFT] + dG) >> 4;
		if (g < 0) g = 0; else if (g > 255) g = 255;
		dg[X - LEFT] = 0;
		b += (db[X - LEFT] + dB) >> 4;
		if (b < 0) b = 0; else if (b > 255) b = 255;
		db[X - LEFT] = 0;
		unsigned long pixel;
		if (priv_cmap) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int dr1 = (r + 25) % 51 - 25;
		    int dg1 = (g + 25) % 51 - 25;
		    int db1 = (b + 25) % 51 - 25;
		    int color_err = dr1 * dr1 + dg1 * dg1 + db1 * db1;
		    int k = (r * 306 + g * 601 + b * 117) / 1024;
		    int dr2 = r - k;
		    int dg2 = g - k;
		    int db2 = b - k;
		    int gray_err = dr2 * dr2 + dg2 * dg2 + db2 * db2;
		    if (color_err < gray_err || no_grays) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
			dR = dr1;
			dG = dg1;
			dB = db1;
		    } else {
			int kk = (k * 6 + 17) / 34;
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
			dR = dr2;
			dG = dg2;
			dB = db2;
		    }
		} else if (g_colorcube != NULL) {
		    int index = (((r * (g_cubesize - 1) + 127) / 255)
			    * g_cubesize
			    + ((g * (g_cubesize - 1) + 127) / 255))
			    * g_cubesize
			    + ((b * (g_cubesize - 1) + 127) / 255);
		    dR = r - (g_colorcube[index].red >> 8);
		    dG = g - (g_colorcube[index].green >> 8);
		    dB = b - (g_colorcube[index].blue >> 8);
		    pixel = g_colorcube[index].pixel;
		} else {
		    int ri = (r * rmax + 127) / 255;
		    int gi = (g * gmax + 127) / 255;
		    int bi = (b * bmax + 127) / 255;
		    pixel = (ri << rmult) + (gi << gmult) + (bi << bmult);
		    dR = r - ri * 255 / rmax;
		    dG = g - gi * 255 / gmax;
		    dB = b - bi * 255 / bmax;
		}
		XPutPixel(image, X, Y, pixel);
		int PREVX = X - dir;
		int NEXTX = X + dir;
		if (PREVX >= LEFT && PREVX < RIGHT) {
		    nextdr[PREVX - LEFT] += dR * 3;
		    nextdg[PREVX - LEFT] += dG * 3;
		    nextdb[PREVX - LEFT] += dB * 3;
		}
		nextdr[X - LEFT] += dR * 5;
		nextdg[X - LEFT] += dG * 5;
		nextdb[X - LEFT] += dB * 5;
		if (NEXTX >= LEFT && NEXTX < RIGHT) {
		    nextdr[NEXTX - LEFT] += dR;
		    nextdg[NEXTX - LEFT] += dG;
		    nextdb[NEXTX - LEFT] += dB;
		}
		dR *= 7;
		dG *= 7;
		dB *= 7;
	    }
	}
	delete[] dr;
	delete[] dg;
	delete[] db;
	delete[] nextdr;
	delete[] nextdg;
	delete[] nextdb;
    } else if (!dither) {
	// Grayscale display, not dithered
	for (int y = top; y < bottom; y++) {
	    int Y = y * factor;
	    for (int x = left; x < right; x++) {
		int X = x * factor;
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		int k = (r * 306 + g * 601 + b * 117) / 1024;
		int graylevel = (k * (g_rampsize - 1) + 127) / 255;
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		unsigned long pixel = g_grayramp[graylevel].pixel;
		for (int XX = X; XX < X + factor; XX++)
		    for (int YY = Y; YY < Y + factor; YY++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else {
	// Grayscale display, dithered
	int *dk = new int[RIGHT - LEFT];
	int *nextdk = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dk[i] = nextdk[i] = 0;
	int dK = 0;
	for (int Y = TOP; Y < BOTTOM; Y++) {
	    int y = Y / factor;
	    int dir = ((Y & 1) << 1) - 1;
	    int START, END;
	    if (dir == 1) {
		START = LEFT;
		END = RIGHT;
	    } else {
		START = RIGHT - 1;
		END = LEFT - 1;
	    }
	    int *temp;
	    temp = nextdk; nextdk = dk; dk = temp;
	    dK = 0;
	    for (int X = START; X != END; X += dir) {
		int x = X / factor;
		int r, g, b;
		if (pm->depth == 8) {
		    unsigned char index = pm->pixels[y * pm->bytesperline + x];
		    r = pm->cmap[index].r;
		    g = pm->cmap[index].g;
		    b = pm->cmap[index].b;
		} else /* pm->depth == 24 */ {
		    r = pm->pixels[y * pm->bytesperline + (x << 2) + 1];
		    g = pm->pixels[y * pm->bytesperline + (x << 2) + 2];
		    b = pm->pixels[y * pm->bytesperline + (x << 2) + 3];
		}
		int k = (r * 306 + g * 601 + b * 117) / 1024;
		k += (dk[X - LEFT] + dK) >> 4;
		if (k < 0) k = 0; else if (k > 255) k = 255;
		dk[X - LEFT] = 0;
		int graylevel = (k * (g_rampsize - 1) + 127) / 255;
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		dK = k - (g_grayramp[graylevel].red >> 8);
		unsigned long pixel = g_grayramp[graylevel].pixel;
		XPutPixel(image, X, Y, pixel);
		int PREVX = X - dir;
		int NEXTX = X + dir;
		if (PREVX >= LEFT && PREVX < RIGHT)
		    nextdk[PREVX - LEFT] += dK * 3;
		nextdk[X - LEFT] += dK * 5;
		if (NEXTX >= LEFT && NEXTX < RIGHT)
		    nextdk[NEXTX - LEFT] += dK;
		dK *= 7;
	    }
	}
	delete[] dk;
	delete[] nextdk;
    }
}

/* public static */ void
CopyBits::copy_reduced(int factor,
		       FWPixmap *pm, XImage *image,
		       bool priv_cmap, bool no_grays, bool dither,
		       int top, int left, int bottom, int right) {
    calc_rgb_masks();

    int TOP = top / factor;
    int BOTTOM = (bottom + factor - 1) / factor;
    if (BOTTOM > image->height)
	BOTTOM = image->height;
    int LEFT = left / factor;
    int RIGHT = (right + factor - 1) / factor;
    if (RIGHT > image->width)
	RIGHT = image->width;

    // Normalize source coordinates
    top = TOP * factor;
    bottom = BOTTOM * factor;
    if (bottom > pm->height)
	bottom = pm->height;
    left = LEFT * factor;
    right = RIGHT * factor;
    if (right > pm->width)
	right = pm->width;

    int R[RIGHT - LEFT];
    int G[RIGHT - LEFT];
    int B[RIGHT - LEFT];
    int W[RIGHT - LEFT];
    for (int i = 0; i < RIGHT - LEFT; i++)
	R[i] = G[i] = B[i] = W[i] = 0;


    if (!dither || (pm->depth == 1 && priv_cmap)) {
	// Color or Gray, no dithering
	int Y = TOP;
	int YY = 0;
	for (int y = top; y < bottom; y++) {
	    int X = 0;
	    int XX = 0;
	    for (int x = left; x < right; x++) {
		if (pm->depth == 1) {
		    int p = ((pm->pixels[y * pm->bytesperline + (x >> 3)] >> (x & 7)) & 1)
				* 255;
		    R[X] += p;
		    G[X] += p;
		    B[X] += p;
		} else if (pm->depth == 8) {
		    int p = pm->pixels[y * pm->bytesperline + x];
		    R[X] += pm->cmap[p].r;
		    G[X] += pm->cmap[p].g;
		    B[X] += pm->cmap[p].b;
		} else {
		    unsigned char *p = pm->pixels + (y * pm->bytesperline + x * 4 + 1);
		    R[X] += *p++;
		    G[X] += *p++;
		    B[X] += *p;
		}
		W[X]++;
		if (++XX == factor) {
		    XX = 0;
		    X++;
		}
	    }

	    if (YY == factor - 1 || Y == BOTTOM - 1) {
		for (X = 0; X < RIGHT - LEFT; X++) {
		    int w = W[X];
		    int r = (R[X] + w / 2) / w; if (r > 255) r = 255;
		    int g = (G[X] + w / 2) / w; if (g > 255) g = 255;
		    int b = (B[X] + w / 2) / w; if (b > 255) b = 255;
		    R[X] = G[X] = B[X] = W[X] = 0;

		    if (priv_cmap) {
			// Find the closest match to (R, G, B) in the colormap...
			if (pm->depth == 1) {
			    int k = (r * 306 + g * 601 + b * 117) / 1024;
			    if (k > 255)
				k = 255;
			    XPutPixel(image, X + LEFT, Y, k);
			} else if (pm->depth == 24) {
			    // 256-entry color map, with first 216 entries
			    // containing a 6x6x6 color cube, and the remaining
			    // 40 entries containing 40 shades of gray (which,
			    // together with the 6 shades of gray in the color
			    // cube, make up a 46-entry gray ramp We look for a
			    // color match and a graylevel match, and pick the
			    // best fit.
			    int rerr = (r + 25) % 51 - 25;
			    int gerr = (g + 25) % 51 - 25;
			    int berr = (b + 25) % 51 - 25;
			    int color_err = rerr * rerr + gerr * gerr + berr * berr;
			    int k = (r * 306 + g * 601 + b * 117) / 1024;
			    rerr = r - k; 
			    gerr = g - k;
			    berr = b - k;
			    int gray_err = rerr * rerr + gerr * gerr + berr * berr;
			    unsigned long pixel;
			    if (color_err < gray_err || no_grays) {
				int rr = (r + 25) / 51;
				int gg = (g + 25) / 51;
				int bb = (b + 25) / 51;
				pixel = 36 * rr + 6 * gg + bb;
			    } else {
				int kk = (k * 6 + 17) / 34;
				if (kk % 9 == 0)
				    pixel = kk * 43 / 9;
				else
				    pixel = (kk / 9) * 8 + (kk % 9) + 215;
			    }
			    XPutPixel(image, X + LEFT, Y, pixel);
			} else {
			    // TODO -- this is pathetic, of course
			    unsigned long best_pixel;
			    int best_error = 1000000;
			    for (int i = 0; i < 256; i++) {
				int dr = r - pm->cmap[i].r;
				int dg = g - pm->cmap[i].g;
				int db = b - pm->cmap[i].b;
				int err = dr * dr + dg * dg + db * db;
				if (err < best_error) {
				    best_pixel = i;
				    best_error = err;
				}
			    }
			    XPutPixel(image, X + LEFT, Y, best_pixel);
			}
		    } else if (g_grayramp == NULL) {
			unsigned long pixel;
			if (g_colorcube != NULL) {
			    int index = (((r * (g_cubesize - 1) + 127) / 255)
				    * g_cubesize
				    + ((g * (g_cubesize - 1) + 127) / 255))
				    * g_cubesize
				    + ((b * (g_cubesize - 1) + 127) / 255);
			    pixel = g_colorcube[index].pixel;
			} else {
			    pixel = (((r * rmax + 127) / 255) << rmult)
				    + (((g * gmax + 127) / 255) << gmult)
				    + (((b * bmax + 127) / 255) << bmult);
			}
			XPutPixel(image, X + LEFT, Y, pixel);
		    } else {
			int k = (r * 306 + g * 601 + b * 117) / 1024;
			int graylevel = (k * (g_rampsize - 1) + 127) / 255;
			if (graylevel >= g_rampsize)
			    graylevel = g_rampsize - 1;
			unsigned long pixel = g_grayramp[graylevel].pixel;
			XPutPixel(image, X + LEFT, Y, pixel);
		    }
		}
	    }

	    if (++YY == factor) {
		YY = 0;
		Y++;
	    }
	}
    } else if (g_grayramp == NULL || priv_cmap) {
	// Color dithering
	int *dr = new int[RIGHT - LEFT];
	int *dg = new int[RIGHT - LEFT];
	int *db = new int[RIGHT - LEFT];
	int *nextdr = new int[RIGHT - LEFT];
	int *nextdg = new int[RIGHT - LEFT];
	int *nextdb = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dr[i] = dg[i] = db[i] = nextdr[i] = nextdg[i] = nextdb[i] = 0;
	int dR = 0, dG = 0, dB = 0;

	int Y = TOP;
	int YY = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((Y & 1) << 1) - 1;
	    int start, end, START, END;
	    if (dir == 1) {
		start = left;
		end = right;
		START = 0;
		END = RIGHT - LEFT;
	    } else {
		start = right - 1;
		end = left - 1;
		START = RIGHT - LEFT - 1;
		END = -1;
	    }

	    int X = dir == 1 ? 0 : RIGHT - LEFT - 1;
	    int XX = dir == 1 ? 0 : (right - 1) % factor + 1;
	    for (int x = start; x != end; x += dir) {
		if (pm->depth == 1) {
		    int p = ((pm->pixels[y * pm->bytesperline + (x >> 3)]
				>> (x & 7)) & 1) * 255;
		    R[X] += p;
		    G[X] += p;
		    B[X] += p;
		} else if (pm->depth == 8) {
		    int p = pm->pixels[y * pm->bytesperline + x];
		    R[X] += pm->cmap[p].r;
		    G[X] += pm->cmap[p].g;
		    B[X] += pm->cmap[p].b;
		} else {
		    unsigned char *p = pm->pixels + (y * pm->bytesperline + x * 4 + 1);
		    R[X] += *p++;
		    G[X] += *p++;
		    B[X] += *p;
		}
		W[X]++;
		if (dir == 1) {
		    if (++XX == factor) {
			XX = 0;
			X++;
		    }
		} else {
		    if (--XX == 0) {
			XX = factor;
			X--;
		    }
		}
	    }

	    if (YY == factor - 1 || y == bottom - 1) {
		int *temp;
		temp = nextdr; nextdr = dr; dr = temp;
		temp = nextdg; nextdg = dg; dg = temp;
		temp = nextdb; nextdb = db; db = temp;
		dR = dG = dB = 0;

		for (X = START; X != END; X += dir) {
		    int w = W[X];
		    int r = (R[X] + w / 2) / w; if (r > 255) r = 255;
		    int g = (G[X] + w / 2) / w; if (g > 255) g = 255;
		    int b = (B[X] + w / 2) / w; if (b > 255) b = 255;
		    R[X] = G[X] = B[X] = W[X] = 0;
		    
		    r += (dr[X] + dR) >> 4;
		    if (r < 0) r = 0; else if (r > 255) r = 255;
		    dr[X] = 0;
		    g += (dg[X] + dG) >> 4;
		    if (g < 0) g = 0; else if (g > 255) g = 255;
		    dg[X] = 0;
		    b += (db[X] + dB) >> 4;
		    if (b < 0) b = 0; else if (b > 255) b = 255;
		    db[X] = 0;

		    if (priv_cmap && pm->depth == 24) {
			// 256-entry color map, with first 216 entries
			// containing a 6x6x6 color cube, and the remaining 40
			// entries containing 40 shades of gray (which,
			// together with the 6 shades of gray in the color
			// cube, make up a 46-entry gray ramp We look for a
			// color match and a graylevel match, and pick the best
			// fit.
			int dr1 = (r + 25) % 51 - 25;
			int dg1 = (g + 25) % 51 - 25;
			int db1 = (b + 25) % 51 - 25;
			int color_err = dr1 * dr1 + dg1 * dg1 + db1 * db1;
			int k = (r * 306 + g * 601 + b * 117) / 1024;
			int dr2 = r - k; 
			int dg2 = g - k;
			int db2 = b - k;
			int gray_err = dr2 * dr2 + dg2 * dg2 + db2 * db2;
			unsigned long pixel;
			if (color_err < gray_err || no_grays) {
			    int rr = (r + 25) / 51;
			    int gg = (g + 25) / 51;
			    int bb = (b + 25) / 51;
			    pixel = 36 * rr + 6 * gg + bb;
			    dR = dr1;
			    dG = dg1;
			    dB = db1;
			} else {
			    int kk = (k * 6 + 17) / 34;
			    if (kk % 9 == 0)
				pixel = kk * 43 / 9;
			    else
				pixel = (kk / 9) * 8 + (kk % 9) + 215;
			    dR = dr2;
			    dG = dg2;
			    dB = db2;
			}
			XPutPixel(image, X + LEFT, Y, pixel);
		    } else if (priv_cmap) {
			// Find the closest match to (R, G, B) in the colormap...
			// TODO -- this is pathetic, of course! Ssslllooowww...
			int best_pixel;
			int best_error = 1000000;
			for (int i = 0; i < 256; i++) {
			    int rerr = r - pm->cmap[i].r;
			    int gerr = g - pm->cmap[i].g;
			    int berr = b - pm->cmap[i].b;
			    int err = rerr * rerr + gerr * gerr + berr * berr;
			    if (err < best_error) {
				best_pixel = i;
				best_error = err;
			    }
			}
			XPutPixel(image, X + LEFT, Y, best_pixel);
			dR = r - pm->cmap[best_pixel].r;
			dG = g - pm->cmap[best_pixel].g;
			dB = b - pm->cmap[best_pixel].b;
		    } else {
			unsigned long pixel;
			if (g_colorcube != NULL) {
			    int index = (((r * (g_cubesize - 1) + 127) / 255)
				    * g_cubesize
				    + ((g * (g_cubesize - 1) + 127) / 255))
				    * g_cubesize
				    + ((b * (g_cubesize - 1) + 127) / 255);
			    pixel = g_colorcube[index].pixel;
			    dR = r - (g_colorcube[index].red >> 8);
			    dG = g - (g_colorcube[index].green >> 8);
			    dB = b - (g_colorcube[index].blue >> 8);
			} else {
			    int ri = (r * rmax + 127) / 255;
			    int gi = (g * gmax + 127) / 255;
			    int bi = (b * bmax + 127) / 255;
			    pixel = (ri << rmult) + (gi << gmult) + (bi << bmult);
			    dR = r - ri * 255 / rmax;
			    dG = g - gi * 255 / gmax;
			    dB = b - bi * 255 / bmax;
			}
			XPutPixel(image, X + LEFT, Y, pixel);
		    }
		    
		    int PREVX = X - dir;
		    int NEXTX = X + dir;
		    if (PREVX >= 0 && PREVX < RIGHT - LEFT) {
			nextdr[PREVX] += dR * 3;
			nextdg[PREVX] += dG * 3;
			nextdb[PREVX] += dB * 3;
		    }
		    nextdr[X] += dR * 5;
		    nextdg[X] += dG * 5;
		    nextdb[X] += dB * 5;
		    if (NEXTX >= 0 && NEXTX < RIGHT - LEFT) {
			nextdr[NEXTX] += dR;
			nextdg[NEXTX] += dG;
			nextdb[NEXTX] += dB;
		    }
		    dR *= 7;
		    dG *= 7;
		    dB *= 7;
		}
	    }

	    if (++YY == factor) {
		YY = 0;
		Y++;
	    }
	}
    } else {
	// Grayscale dithering
	int *dk = new int[RIGHT - LEFT];
	int *nextdk = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dk[i] = nextdk[i] = 0;
	int dK = 0;

	int Y = TOP;
	int YY = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((Y & 1) << 1) - 1;
	    int start, end, START, END;
	    if (dir == 1) {
		start = left;
		end = right;
		START = 0;
		END = RIGHT - LEFT;
	    } else {
		start = right - 1;
		end = left - 1;
		START = RIGHT - LEFT - 1;
		END = -1;
	    }

	    int X = dir == 1 ? 0 : RIGHT - LEFT - 1;
	    int XX = dir == 1 ? 0 : (right - 1) % factor + 1;
	    for (int x = start; x != end; x += dir) {
		if (pm->depth == 1) {
		    int p = ((pm->pixels[y * pm->bytesperline + (x >> 3)]
				>> (x & 7)) & 1) * 255;
		    R[X] += p;
		} else if (pm->depth == 8) {
		    int p = pm->pixels[y * pm->bytesperline + x];
		    R[X] += pm->cmap[p].r;
		    G[X] += pm->cmap[p].g;
		    B[X] += pm->cmap[p].b;
		} else {
		    unsigned char *p = pm->pixels + (y * pm->bytesperline + x * 4 + 1);
		    R[X] += *p++;
		    G[X] += *p++;
		    B[X] += *p;
		}
		W[X]++;
		if (dir == 1) {
		    if (++XX == factor) {
			XX = 0;
			X++;
		    }
		} else {
		    if (--XX == 0) {
			XX = factor;
			X--;
		    }
		}
	    }

	    if (YY == factor - 1 || y == bottom - 1) {
		int *temp;
		temp = nextdk; nextdk = dk; dk = temp;
		dK = 0;

		for (X = START; X != END; X += dir) {
		    int w = W[X];
		    int k;
		    if (pm->depth == 1) {
			k = (R[X] + w / 2) / w;
			if (k > 255)
			    k = 255;
			R[X] = W[X] = 0;
		    } else {
			int r = (R[X] + w / 2) / w; if (r > 255) r = 255;
			int g = (G[X] + w / 2) / w; if (g > 255) g = 255;
			int b = (B[X] + w / 2) / w; if (b > 255) b = 255;
			int k = (r * 306 + g * 601 + b * 117) / 1024;
			if (k > 255)
			    k = 255;
			R[X] = G[X] = B[X] = W[X] = 0;
		    }

		    k += (dk[X] + dK) >> 4;
		    if (k < 0) k = 0; else if (k > 255) k = 255;
		    dk[X] = 0;

		    int graylevel = (k * (g_rampsize - 1) + 127) / 255;
		    if (graylevel >= g_rampsize)
			graylevel = g_rampsize - 1;
		    unsigned long pixel = g_grayramp[graylevel].pixel;
		    XPutPixel(image, X + LEFT, Y, pixel);
		    dK = k - (g_grayramp[graylevel].red >> 8);
		    int PREVX = X - dir;
		    int NEXTX = X + dir;
		    if (PREVX >= 0 && PREVX < RIGHT - LEFT)
			nextdk[PREVX] += dK * 3;
		    nextdk[X] += dK * 5;
		    if (NEXTX >= 0 && NEXTX < RIGHT - LEFT)
			nextdk[NEXTX] += dK;
		    dK *= 7;
		}
	    }

	    if (++YY == factor) {
		YY = 0;
		Y++;
	    }
	}
	delete[] dk;
	delete[] nextdk;
    }
}

static bool halftone_inited = false;
static unsigned char halftone_pattern[256][16][2];

static void halftone_initialize() {
    if (halftone_inited)
	return;

    for (int i = 0; i < 256; i++) {
	// Each halftone patten differs by its predecessor by exactly one bit.
	// First, copy the predecessor into the current slot:

	if (i == 0) {
	    for (int j = 0; j < 16; j++)
		for (int k = 0; k < 2; k++)
		    halftone_pattern[0][j][k] = 0;
	} else {
	    for (int j = 0; j < 16; j++)
		for (int k = 0; k < 2; k++)
		    halftone_pattern[i][j][k] = halftone_pattern[i - 1][j][k];
	}

	// Next, find the bit to set. This is a bit tricky. One possibility is
	// to paint a gradually growing clump, which leads to clustered-dot
	// dithering. Clustered-dot is appropriate mostly for printers, since
	// it offers a measure of immunity to pixel smear; on computer screens,
	// pattern dithering usually looks better, because the artefacts tend
	// to be smaller. I use a very simplistic approach to generating these
	// patterns, which seems to work very well (and generalizes nicely to
	// other pattern sizes, not that that's important right here).

	int v = i;
	int x = 0;
	int y = 0;

	for (int k = 0; k < 4; k++) {
	    y = (y << 1) | (v & 1); v >>= 1;
	    x = (x << 1) | (v & 1); v >>= 1;
	}
	x ^= y;

	halftone_pattern[i][y][x >> 3] |= 1 << (x & 7);
    }

    halftone_inited = true;
}

/* public static */ int
CopyBits::halftone(unsigned char value, int x, int y) {
    // A little hack: since halftone_pattern[0] has one pixel off (in a 16x16
    // pattern, you have 257 possible levels of brightness, and our table only
    // has 256 entries), we couldn't represent 'black' exactly. We fudge that
    // by handling black directly...

    if (value == 0)
	return 0;
    else if (value < 128)
	// This means halftone_pattern[127] (50% gray) is never used. Hopefully
	// no one will notice. It should be less apparent than an inaccuracy at
	// pure white or pure black!
	value--;

    halftone_initialize();
    x &= 15;
    y &= 15;
    unsigned char c = halftone_pattern[value][y][x >> 3];
    return (c >> (x & 7)) & 1;
}

/* public static */ bool
CopyBits::is_grayscale(const FWColor *cmap) {
    for (int i = 0; i < 256; i++)
	if (cmap[i].r != i || cmap[i].g != i || cmap[i].b != i)
	    return false;
    return true;
}

/* public static */ unsigned long
CopyBits::rgb2pixel(unsigned char r, unsigned char g, unsigned char b) {
    if (g_grayramp != NULL) {
	int p = ((306 * r + 601 * g + 117 * b)
		    * (g_rampsize - 1) + 130560) / 261120;
	return g_grayramp[p].pixel;
    } else if (g_colorcube != NULL) {
	int index =  (((r * (g_cubesize - 1) + 127) / 255)
		    * g_cubesize
		    + ((g * (g_cubesize - 1) + 127) / 255))
		    * g_cubesize
		    + ((b * (g_cubesize - 1) + 127) / 255);
	return g_colorcube[index].pixel;
    } else {
	calc_rgb_masks();
	return  (((r * rmax + 127) / 255) << rmult)
	      + (((g * gmax + 127) / 255) << gmult)
	      + (((b * bmax + 127) / 255) << bmult);
    }
}

// rgb2nearestgrays and rgb2nearestcolors are helpers for halftoning; they
// return the 2 grays or the 8 colors that the halftone pattern should be
// built from. The r, g, and b parameters are two-way: when these functions
// return, they contain the color remainder. For example, if a color has r=80,
// and the destination has a 6x6x6 color cube, the nearest colors in the color
// cube will have r=51 and r=102, and the remainder will be 145 (out of a range
// of 0..255, as for the input values). This means that the halftone pattern
// should contain a fraction of (255-145)/255 of r=51, and 145/255 of r=102.
// If the remainder is zero, that means that the returned colors are exact in
// that component, so the halftone pattern lookup can be skipped (for that
// component).

/* public static */ void
CopyBits::rgb2nearestgrays(unsigned char *r, unsigned char *g,
			   unsigned char *b, unsigned long *pixels) {
    int V = 306 * *r + 601 * *g + 117 * *b;
    unsigned char v = (V * 255 + 130560) / 261120;

    int p = (V * (g_rampsize - 1) + 130560) / 261120;
    unsigned char k = g_grayramp[p].red / 257;
    if (k == v) {
	// Exact match
	pixels[0] = pixels[1] = g_grayramp[p].pixel;
	*r = *g = *b = 0;
    } else {
	int p2;
	int k2;
	if (k < v) {
	    p2 = p + 1;
	    k2 = g_grayramp[p2].red;
	} else {
	    p2 = p;
	    k2 = k;
	    p = p - 1;
	    k = g_grayramp[p].red;
	}
	pixels[0] = g_grayramp[p].pixel;
	pixels[1] = g_grayramp[p2].pixel;
	int dk = k2 - k;
	*r = *g = *b = ((v - k) * 255 + dk / 2) / dk;
    }
}

/* public static */ void
CopyBits::rgb2nearestcolors(unsigned char *r, unsigned char *g,
			    unsigned char *b, unsigned long *pixels) {
    int rp, rp2, gp, gp2, bp, bp2;
    unsigned char rk, rk2, gk, gk2, bk, bk2;

    if (g_colorcube != NULL) {

	int cs = g_cubesize;
	int cs2 = g_cubesize * g_cubesize;
	rp = (*r * (g_cubesize - 1) + 127) / 255;
	rk = g_colorcube[rp * cs2].red / 257;
	if (rk == *r) {
	    rp2 = rp;
	    rk2 = rk;
	    *r = 0;
	} else {
	    if (rk < *r) {
		rp2 = rp + 1;
		rk2 = g_colorcube[rp2 * cs2].red / 257;
	    } else {
		rp2 = rp;
		rk2 = rk;
		rp = rp - 1;
		rk = g_colorcube[rp * cs2].red / 257;
	    }
	    int dk = rk2 - rk;
	    *r = ((*r - rk) * 255 + dk / 2) / dk;
	}

	gp = (*g * (g_cubesize - 1) + 127) / 255;
	gk = g_colorcube[gp * cs].green / 257;
	if (gk == *g) {
	    gp2 = gp;
	    gk2 = gk;
	    *g = 0;
	} else {
	    if (gk < *g) {
		gp2 = gp + 1;
		gk2 = g_colorcube[gp2 * cs].green / 257;
	    } else {
		gp2 = gp;
		gk2 = gk;
		gp = gp - 1;
		gk = g_colorcube[gp * cs].green / 257;
	    }
	    int dk = gk2 - gk;
	    *g = ((*g - gk) * 255 + dk / 2) / dk;
	}

	bp = (*b * (g_cubesize - 1) + 127) / 255;
	bk = g_colorcube[bp].blue / 257;
	if (bk == *b) {
	    bp2 = bp;
	    bk2 = bk;
	    *b = 0;
	} else {
	    if (bk < *b) {
		bp2 = bp + 1;
		bk2 = g_colorcube[bp2].blue / 257;
	    } else {
		bp2 = bp;
		bk2 = bk;
		bp = bp - 1;
		bk = g_colorcube[bp].blue / 257;
	    }
	    int dk = bk2 - bk;
	    *b = ((*b - bk) * 255 + dk / 2) / dk;
	}

	int ro = (rp2 - rp) * cs2;
	int go = (gp2 - gp) * cs;
	int bo = bp2 - bp;
	int p = rp * cs2 + gp * cs + bp;
	pixels[0] = g_colorcube[p               ].pixel;
	pixels[1] = g_colorcube[p           + bo].pixel;
	pixels[2] = g_colorcube[p      + go     ].pixel;
	pixels[3] = g_colorcube[p      + go + bo].pixel;
	pixels[4] = g_colorcube[p + ro          ].pixel;
	pixels[5] = g_colorcube[p + ro      + bo].pixel;
	pixels[6] = g_colorcube[p + ro + go     ].pixel;
	pixels[7] = g_colorcube[p + ro + go + bo].pixel;

    } else {

	calc_rgb_masks();
	rp = (*r * rmax + 127) / 255;
	rk = rp * 255 / rmax;
	if (rk == *r) {
	    rp2 = rp;
	    rk2 = rk;
	    *r = 0;
	} else {
	    if (rk < *r) {
		rp2 = rp + 1;
		rk2 = rp2 * 255 / rmax;
	    } else {
		rp2 = rp;
		rk2 = rk;
		rp = rp - 1;
		rk = rp * 255 / rmax;
	    }
	    int dk = rk2 - rk;
	    *r = ((*r - rk) * 255 + dk / 2) / dk;
	}

	gp = (*g * gmax + 127) / 255;
	gk = gp * 255 / gmax;
	if (gk == *g) {
	    gp2 = gp;
	    gk2 = gk;
	    *g = 0;
	} else {
	    if (gk < *g) {
		gp2 = gp + 1;
		gk2 = gp2 * 255 / gmax;
	    } else {
		gp2 = gp;
		gk2 = gk;
		gp = gp - 1;
		gk = gp * 255 / gmax;
	    }
	    int dk = gk2 - gk;
	    *g = ((*g - gk) * 255 + dk / 2) / dk;
	}

	bp = (*b * bmax + 127) / 255;
	bk = bp * 255 / bmax;
	if (bk == *b) {
	    bp2 = bp;
	    bk2 = bk;
	    *b = 0;
	} else {
	    if (bk < *b) {
		bp2 = bp + 1;
		bk2 = bp2 * 255 / bmax;
	    } else {
		bp2 = bp;
		bk2 = bk;
		bp = bp - 1;
		bk = bp * 255 / bmax;
	    }
	    int dk = bk2 - bk;
	    *b = ((*b - bk) * 255 + dk / 2) / dk;
	}

	int ro = (rp2 - rp) << rmult;
	int go = (gp2 - gp) << gmult;
	int bo = (bp2 - bp) << bmult;
	int p = (rp << rmult) | (bp << bmult) | (gp << gmult);
	pixels[0] = p               ;
	pixels[1] = p           + bo;
	pixels[2] = p      + go     ;
	pixels[3] = p      + go + bo;
	pixels[4] = p + ro          ;
	pixels[5] = p + ro      + bo;
	pixels[6] = p + ro + go     ;
	pixels[7] = p + ro + go + bo;
    }
}

/* public static */ void
CopyBits::rgb2hsl(unsigned char R, unsigned char G, unsigned char B,
		  float *H, float *S, float *L) {
    float var_R = R / 255.0;
    float var_G = G / 255.0;
    float var_B = B / 255.0;

    float var_Min = var_R < var_G ? var_R : var_G;
    if (var_B < var_Min)
	var_Min = var_B;
    float var_Max = var_R > var_G ? var_R : var_G;
    if (var_B > var_Max)
	var_Max = var_B;
    float del_Max = var_Max - var_Min;

    *L = (var_Max + var_Min) / 2;

    if (del_Max == 0) {
	*H = 0;
	*S = 0;
    } else {
	if (*L < 0.5)
	    *S = del_Max / (var_Max + var_Min);
	else
	    *S = del_Max / (2 - var_Max - var_Min);

	float del_R = ((var_Max - var_R) / 6 + del_Max / 2) / del_Max;
	float del_G = ((var_Max - var_G) / 6 + del_Max / 2) / del_Max;
	float del_B = ((var_Max - var_B) / 6 + del_Max / 2) / del_Max;

	if (var_R == var_Max)
	    *H = del_B - del_G;
	else if (var_G == var_Max)
	    *H = 1.0 / 3 + del_R - del_B;
	else if (var_B == var_Max)
	    *H = 2.0 / 3 + del_G - del_R;

	if (*H < 0) *H += 1;
	if (*H > 1) *H -= 1;
    }
}

static float hue2rgb(float v1, float v2, float vH) {
   if (vH < 0) vH += 1;
   if (vH > 1) vH -= 1;
   if (6 * vH < 1) return v1 + (v2 - v1) * 6 * vH;
   if (2 * vH < 1) return v2;
   if (3 * vH < 2) return v1 + (v2 - v1) * (2.0 / 3 - vH) * 6;
   return v1;
}

/* public static */ void
CopyBits::hsl2rgb(float H, float S, float L,
		  unsigned char *R, unsigned char *G, unsigned char *B) {
    if (S == 0) {
	*R = (unsigned char) (L * 255);
	*G = (unsigned char) (L * 255);
	*B = (unsigned char) (L * 255);
    } else {
	float var_2;
	if (L < 0.5)
	    var_2 = L * (1 + S);
	else
	    var_2 = (L + S) - (S * L);

	float var_1 = 2 * L - var_2;

	*R = (unsigned char) (255 * hue2rgb(var_1, var_2, H + 1.0 / 3));
	*G = (unsigned char) (255 * hue2rgb(var_1, var_2, H));
	*B = (unsigned char) (255 * hue2rgb(var_1, var_2, H - 1.0 / 3));
    }
}

/* public static */ void
CopyBits::rgb2hsv(unsigned char R, unsigned char G, unsigned char B,
		  float *H, float *S, float *V) {
    float var_R = R / 255.0;
    float var_G = G / 255.0;
    float var_B = B / 255.0;

    float var_Min = var_R < var_G ? var_R : var_G;
    if (var_B < var_Min)
	var_Min = var_B;
    float var_Max = var_R > var_G ? var_R : var_G;
    if (var_B > var_Max)
	var_Max = var_B;
    float del_Max = var_Max - var_Min;

    *V = var_Max;

    if (del_Max == 0) {
	*H = 0;
	*S = 0;
    } else {
	*S = del_Max / var_Max;

	float del_R = ((var_Max - var_R) / 6 + del_Max / 2) / del_Max;
	float del_G = ((var_Max - var_G) / 6 + del_Max / 2) / del_Max;
	float del_B = ((var_Max - var_B) / 6 + del_Max / 2) / del_Max;

	if (var_R == var_Max)
	    *H = del_B - del_G;
	else if (var_G == var_Max)
	    *H = (1.0 / 3) + del_R - del_B;
	else if (var_B == var_Max)
	    *H = ( 2 / 3 ) + del_G - del_R;

	if (*H < 0) *H += 1;
	if (*H > 1) *H -= 1;
    }
}

/* public static */ void
CopyBits::hsv2rgb(float H, float S, float V,
		  unsigned char *R, unsigned char *G, unsigned char *B) {
    if (S == 0) {
	*R = (unsigned char) (V * 255);
	*G = (unsigned char) (V * 255);
	*B = (unsigned char) (V * 255);
    } else {
	float var_h = H * 6;
	int var_i = (int) var_h;
	float var_1 = V * (1 - S);
	float var_2 = V * (1 - S * (var_h - var_i));
	float var_3 = V * (1 - S * (1 - (var_h - var_i)));

	float var_r, var_g, var_b;
	if ( var_i == 0 ) {
	    var_r = V     ; var_g = var_3 ; var_b = var_1;
	} else if (var_i == 1) {
	    var_r = var_2 ; var_g = V     ; var_b = var_1;
	} else if (var_i == 2) {
	    var_r = var_1 ; var_g = V     ; var_b = var_3;
	} else if (var_i == 3) {
	    var_r = var_1 ; var_g = var_2 ; var_b = V;
	} else if (var_i == 4) {
	    var_r = var_3 ; var_g = var_1 ; var_b = V;
	} else {
	    var_r = V     ; var_g = var_1 ; var_b = var_2;
	}

	*R = (unsigned char) (var_r * 255);
	*G = (unsigned char) (var_g * 255);
	*B = (unsigned char) (var_b * 255);
   }
}
