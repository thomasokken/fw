#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Plugin.h"

class MandelbrotMono : public Plugin {
    private:
	unsigned int x, y;
    public:
	MandelbrotMono(void *dl) : Plugin(dl) {}
	virtual ~MandelbrotMono() {}
	virtual const char *name() const {
	    return "MandelbrotMono";
	}
	virtual void init_new() {
	    width = 500;
	    height = 300;
	    bytesperline = width + 7 >> 3;
	    unsigned int size = bytesperline * height;
	    pixels = (char *) malloc(size);
	    for (unsigned int i = 0; i < size; i++)
		pixels[i] = 0;
	    depth = 1;
	    viewer->finish_init();
	}
	virtual void run() {
	    paint();
	    x = 0;
	    y = 0;
	    start_prodding();
	}
	virtual void restart() {
	    if (y < height)
		start_prodding();
	}
	virtual void stop() {
	    stop_prodding();
	    paint();
	}
	virtual bool work() {
	    unsigned int firsty = y;
	    unsigned int endy = y + 10;
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
		    pixels[y * bytesperline + (x >> 3)] |= 1 << (x & 7);
		if (++x == width) {
		    x = 0;
		    if (++y == height) {
			paint(firsty, 0, height, width);
			return true;
		    }
		}
	    }
	    paint(firsty, 0, endy, width);
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
