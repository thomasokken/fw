#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Plugin.h"

class MandelbrotMono : public Plugin {
    private:
	int x, y;
    public:
	MandelbrotMono(void *dl) : Plugin(dl) {}
	virtual ~MandelbrotMono() {}
	virtual const char *name() const {
	    return "MandelbrotMono";
	}
	virtual void init_new() {
	    vw->width = 500;
	    vw->height = 300;
	    vw->bytesperline = vw->width + 7 >> 3;
	    int size = vw->bytesperline * vw->height;
	    vw->pixels = (unsigned char *) malloc(size);
	    for (int i = 0; i < size; i++)
		vw->pixels[i] = 0;
	    vw->depth = 1;
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
		if (n & 1)
		    vw->pixels[y * vw->bytesperline + (x >> 3)] |= 1 << (x & 7);
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
    return new MandelbrotMono(dl);
}

extern "C" Plugin *factory(void *dl) {
    return factory2(dl);
}
