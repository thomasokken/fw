#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Plugin.h"

struct my_settings {
    int width, height;
    int xr, yr, xg, yg, xb, yb;
    my_settings() {
	width = 256;
	height = 256;
	xr = 1;
	yr = 0;
	xg = 0;
	yg = 1;
	xb = 0;
	yb = 0;
    }
};
static char *my_settings_layout[] = {
    "iWidth",
    "iHeight",
    "iX contribution to Red",
    "iY contribution to Red",
    "iX contribution to Green",
    "iY contribution to Green",
    "iX contribution to Blue",
    "iY contribution to Blue",
    NULL
};

class Ramp : public Plugin {
    private:
	my_settings s;
    public:
	Ramp(void *dl) : Plugin(dl) {
	    settings = &s;
	    settings_layout = my_settings_layout;
	}
	virtual ~Ramp() {}
	virtual const char *name() const {
	    return "Ramp";
	}
	virtual bool does_depth(int depth) {
	    return depth == 8 || depth == 24;
	}
	virtual void set_depth(int depth) {
	    this->depth = depth;
	}
	virtual void init_new() {
	    get_settings();
	}
	virtual void get_settings_ok() {
	    width = s.width;
	    height = s.height;
	    bytesperline = depth == 8 ? (width + 3 & ~3) : (width * 4);
	    pixels = (char *) malloc(bytesperline * height);
	    char *dst = pixels;

	    int roffset = 0, boffset = 0, goffset = 0;
	    int rshift = 0, bshift = 0, gshift = 0;

	    if (s.xr < 0) {
		s.xr = -1;
		roffset += 255;
	    } else if (s.xr > 0)
		s.xr = 1;
	    if (s.yr < 0) {
		s.yr = -1;
		roffset += 255;
	    } else if (s.yr > 0)
		s.yr = 1;

	    if (s.xg < 0) {
		s.xg = -1;
		goffset += 255;
	    } else if (s.xg > 0)
		s.xg = 1;
	    if (s.yg < 0) {
		s.yg = -1;
		goffset += 255;
	    } else if (s.yg > 0)
		s.yg = 1;

	    if (s.xb < 0) {
		s.xb = -1;
		boffset += 255;
	    } else if (s.xb > 0)
		s.xb = 1;
	    if (s.yb < 0) {
		s.yb = -1;
		boffset += 255;
	    } else if (s.yb > 0)
		s.yb = 1;

	    rshift = s.xr != 0 && s.yr != 0 ? 1 : 0;
	    gshift = s.xg != 0 && s.yg != 0 ? 1 : 0;
	    bshift = s.xb != 0 && s.yb != 0 ? 1 : 0;

	    for (unsigned int y = 0; y < height; y++) {
		int yy = (y * 255) / (height - 1);
		for (unsigned int x = 0; x < width; x++) {
		    int xx = (x * 255) / (width - 1);
		    if (depth == 24)
			*dst++ = 0;
		    *dst++ = (roffset + s.xr * xx + s.yr * yy) >> rshift;
		    if (depth == 24) {
			*dst++ = (goffset + s.xg * xx + s.yg * yy) >> gshift;
			*dst++ = (boffset + s.xb * xx + s.yb * yy) >> bshift;
		    }
		}
	    }

	    if (depth == 8) {
		cmap = new Color[256];
		for (int k = 0; k < 256; k++)
		    cmap[k].r = cmap[k].g = cmap[k].b = k;
	    }

	    viewer->finish_init();
	}
	virtual void run() {
	    paint();
	}

	friend Plugin *factory2(void *dl);
};

Plugin *factory2(void *dl) {
    return new Ramp(dl);
}

extern "C" Plugin *factory(void *dl) {
    return factory2(dl);
}
