#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Plugin.h"

class Ramp : public Plugin {
    public:
	Ramp(void *dl) : Plugin(dl) {
	    settings = getSettingsInstance();
	    settings->addField(PluginSettings::INT, "Width");
	    settings->addField(PluginSettings::INT, "Height");
	    settings->addField(PluginSettings::INT, "X contribution to Red");
	    settings->addField(PluginSettings::INT, "Y contribution to Red");
	    settings->addField(PluginSettings::INT, "X contribution to Green");
	    settings->addField(PluginSettings::INT, "Y contribution to Green");
	    settings->addField(PluginSettings::INT, "X contribution to Blue");
	    settings->addField(PluginSettings::INT, "Y contribution to Blue");
	    settings->setIntField(0, 256);
	    settings->setIntField(1, 256);
	    settings->setIntField(2, 1);
	    settings->setIntField(5, 1);
	}
	virtual ~Ramp() {}
	virtual const char *name() const {
	    return "Ramp";
	}
	virtual bool does_depth(int depth) {
	    return depth == 8 || depth == 24;
	}
	virtual void set_depth(int depth) {
	    pm->depth = depth;
	}
	virtual void init_new() {
	    get_settings_dialog();
	}
	virtual void get_settings_ok() {
	    int width = settings->getIntField(0);
	    int height = settings->getIntField(1);
	    int xr = settings->getIntField(2);
	    int yr = settings->getIntField(3);
	    int xg = settings->getIntField(4);
	    int yg = settings->getIntField(5);
	    int xb = settings->getIntField(6);
	    int yb = settings->getIntField(7);

	    pm->width = width;
	    pm->height = height;
	    pm->bytesperline = pm->depth == 8 ? (pm->width + 3 & ~3) : (pm->width * 4);
	    pm->pixels = (unsigned char *) malloc(pm->bytesperline * pm->height);
	    unsigned char *dst = pm->pixels;

	    int roffset = 0, boffset = 0, goffset = 0;
	    int rshift = 0, bshift = 0, gshift = 0;

	    if (xr < 0) {
		xr = -1;
		roffset += 255;
	    } else if (xr > 0)
		xr = 1;
	    if (yr < 0) {
		yr = -1;
		roffset += 255;
	    } else if (yr > 0)
		yr = 1;

	    if (xg < 0) {
		xg = -1;
		goffset += 255;
	    } else if (xg > 0)
		xg = 1;
	    if (yg < 0) {
		yg = -1;
		goffset += 255;
	    } else if (yg > 0)
		yg = 1;

	    if (xb < 0) {
		xb = -1;
		boffset += 255;
	    } else if (xb > 0)
		xb = 1;
	    if (yb < 0) {
		yb = -1;
		boffset += 255;
	    } else if (yb > 0)
		yb = 1;

	    rshift = xr != 0 && yr != 0 ? 1 : 0;
	    gshift = xg != 0 && yg != 0 ? 1 : 0;
	    bshift = xb != 0 && yb != 0 ? 1 : 0;

	    for (int y = 0; y < pm->height; y++) {
		int yy = (y * 255) / (pm->height - 1);
		for (int x = 0; x < pm->width; x++) {
		    int xx = (x * 255) / (pm->width - 1);
		    if (pm->depth == 24)
			*dst++ = 0;
		    *dst++ = (roffset + xr * xx + yr * yy) >> rshift;
		    if (pm->depth == 24) {
			*dst++ = (goffset + xg * xx + yg * yy) >> gshift;
			*dst++ = (boffset + xb * xx + yb * yy) >> bshift;
		    }
		}
	    }

	    if (pm->depth == 8) {
		pm->cmap = new FWColor[256];
		for (int k = 0; k < 256; k++)
		    pm->cmap[k].r = pm->cmap[k].g = pm->cmap[k].b = k;
	    }

	    init_proceed();
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
