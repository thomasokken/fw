#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Plugin.h"

class Mandelbrot8Bit : public Plugin {
    private:
	int x, y;
    public:
	Mandelbrot8Bit(void *dl) : Plugin(dl) {}
	virtual ~Mandelbrot8Bit() {}
	virtual const char *name() const {
	    return "Mandelbrot8Bit";
	}
	virtual void init_new() {
	    pm->width = 500;
	    pm->height = 300;
	    pm->bytesperline = pm->width + 3 & ~3;
	    int size = pm->bytesperline * pm->height;
	    pm->pixels = (unsigned char *) malloc(size);
	    for (int i = 0; i < size; i++)
		pm->pixels[i] = 0;
	    pm->cmap = new FWColor[256];
	    for (int i = 0; i < 255; i++)
		pm->cmap[i].r = pm->cmap[i].g = pm->cmap[i].b = i;
	    pm->depth = 8;
	    init_proceed();
	}
	virtual void run() {
	    paint();
	    x = 0;
	    y = 0;
	    start_prodding();
	}
	virtual void restart() {
	    if (y < pm->height)
		start_prodding();
	}
	virtual void stop() {
	    stop_prodding();
	    paint();
	}
	virtual bool work() {
	    int firsty = y;
	    int endy = y + 10;
	    while (y < endy) {
		double dx = (((double) x) - 250) / 150;
		double dy = (((double) y) - 150) / 150;
		double cx = dx;
		double cy = dy;
		int n = 0;
		while (n < 1000 && (cx * cx + cy * cy < 4)) {
		    double t = cx * cx - cy * cy + dx;
		    cy = 2 * cx * cy + dy;
		    cx = t;
		    n++;
		}
		if (n == 1000)
		    pm->pixels[y * pm->bytesperline + x] = 0;
		else
		    pm->pixels[y * pm->bytesperline + x] = ((n % 100) * 255) / 100;
		if (++x == pm->width) {
		    x = 0;
		    if (++y == pm->height) {
			paint(firsty, 0, pm->height, pm->width);
			return true;
		    }
		}
	    }
	    paint(firsty, 0, endy, pm->width);
	    return false;
	}

	friend Plugin *factory2(void *dl);
};

Plugin *factory2(void *dl) {
    return new Mandelbrot8Bit(dl);
}

extern "C" Plugin *factory(void *dl) {
    return factory2(dl);
}
