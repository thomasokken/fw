#include "CopyBits.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "main.h"


static bool inited = false;
static int rmax, rmult, bmax, bmult, gmax, gmult;

static void calc_rgb_masks() {
    if (inited)
	return;
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
		    calc_rgb_masks();
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
		    calc_rgb_masks();
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
		    calc_rgb_masks();
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
		    calc_rgb_masks();
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
			    calc_rgb_masks();
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
			    calc_rgb_masks();
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
