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
	    vw->width = 500;
	    vw->height = 300;
	    vw->bytesperline = vw->width + 3 & ~3;
	    int size = vw->bytesperline * vw->height;
	    vw->pixels = (unsigned char *) malloc(size);
	    for (int i = 0; i < size; i++)
		vw->pixels[i] = 0;
	    vw->cmap = new Color[256];
	    for (int i = 0; i < 255; i++)
		vw->cmap[i].r = vw->cmap[i].g = vw->cmap[i].b = i;
	    vw->depth = 8;
	    vw->finish_init();
	}
	virtual void run() {
	    paint();
	    x = 0;
	    y = 0;
	    start_prodding();
	}
	virtual void restart() {
	    if (y < vw->height)
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
		    vw->pixels[y * vw->bytesperline + x] = 0;
		else
		    vw->pixels[y * vw->bytesperline + x] = ((n % 100) * 255) / 100;
		if (++x == vw->width) {
		    x = 0;
		    if (++y == vw->height) {
			paint(firsty, 0, vw->height, vw->width);
			return true;
		    }
		}
	    }
	    paint(firsty, 0, endy, vw->width);
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
