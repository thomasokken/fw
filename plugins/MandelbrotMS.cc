#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "Plugin.h"

struct rect {
    int top, left, bottom, right;
    void set(int top, int left, int bottom, int right) {
	this->top = top;
	this->left = left;
	this->bottom = bottom;
	this->right = right;
    }
    void empty() {
	top = left = bottom = right = -1;
    }
    bool isEmpty() const {
	return top == -1 && left == -1 && bottom == -1 && right == -1;
    }
    void merge(const rect &r) {
	if (isEmpty()) {
	    top = r.top;
	    left = r.left;
	    bottom = r.bottom;
	    right = r.right;
	} else {
	    if (!r.isEmpty()) {
		if (r.top < top)
		    top = r.top;
		if (r.left < left)
		    left = r.left;
		if (r.bottom > bottom)
		    bottom = r.bottom;
		if (r.right > right)
		    right = r.right;
	    }
	}
    }
};

static const char *my_settings_layout[] = {
    "WIDTH 'Width'",			// pm->width
    "HEIGHT 'Height'",			// pm->height
    "double 'Lower Real Bound'",	// xmin
    "double 'Upper Real Bound'",	// xmax
    "double 'Lower Imaginary Bound'",	// ymin
    "double 'Upper Imaginary Bound'",	// ymax
    "int 'Maximum Iterations'",		// maxiter
    "double 'Cutoff Value'",		// limit
    "int 'Color Bands'",		// bands
    "double",		// limit2
    "int",		// ndirty
    "int",		// b2
    "double",		// step
    "double",		// x1
    "double",		// y1
    "double",		// x2
    "double",		// y2
    "double",		// xm
    "double",		// ym
    "int",		// state
    "int",		// value
    "int",		// sp
    "REPEAT 200",	// stack (50 levels of 4 ints each)
    "int",
    "ENDREP",
    "bool",		// finished
    NULL
};

class MandelbrotMS : public Plugin {
    private:
	double xmin, xmax, ymin, ymax;
	int maxiter;
	double limit;
	int bands;
	double limit2;
	int ndirty;
	int b2;
	double step;
	double x1, y1;
	double x2, y2;
	double xm, ym;
	int state;
	int value;
	int sp;
	rect tos;
	rect dirty;
	int hm, vm;
	int hc, vc;
	rect stack[50];
	bool finished;

    public:
	/* move this to Plugin later */
	void fillrect(int top, int left, int bottom, int right, int value) {
	    for (int y = top; y <= bottom; y++)
		for (int x = left; x <= right; x++)
		    pm->pixels[y * pm->bytesperline + x] = value;
	}

	MandelbrotMS(void *dl) : Plugin(dl) {
	    settings_layout = my_settings_layout;
	    settings_base = &xmin;
	}

	virtual ~MandelbrotMS() {}

	virtual const char *name() {
	    return "MandelbrotMS";
	}

	virtual bool does_depth(int depth) {
	    return depth == 8;
	}

	virtual void init_new() {
	    pm->width = 700;
	    pm->height = 500;
	    xmin = -1.6 * pm->width / pm->height;
	    xmax = 0.55 * pm->width / pm->height;
	    ymin = -1.075;
	    ymax = 1.075;
	    maxiter = 100;
	    limit = 2.0;
	    bands = 1;
	    get_settings_dialog();
	}

	virtual void get_settings_ok() {
	    pm->bytesperline = pm->width + 3 & ~3;
	    pm->pixels = (unsigned char *) malloc(pm->bytesperline * pm->height);
	    memset(pm->pixels, 255, pm->bytesperline * pm->height);
	    pm->cmap = new FWColor[256];
	    for (int k = 0; k < 256; k++) {
		pm->cmap[k].r = k;
		pm->cmap[k].g = k;
		pm->cmap[k].b = k;
	    }
	    init_proceed();
	}

	virtual void start() {
	    b2 = bands * 253;
	    limit2 = limit * limit;
	    step = (xmax - xmin) / pm->width;
	    sp = 0;
	    stack[sp].set(0, 0, pm->height - 1, pm->width - 1);
	    state = 0;
	    ndirty = 0;
	    dirty.empty();
	    finished = false;
	    paint();
	    start_working();
	}

	virtual void stop() {
	    stop_working();
	    if (!dirty.isEmpty()) {
		paint(dirty.top, dirty.left, dirty.bottom + 1, dirty.right + 1);
		dirty.empty();
		ndirty = 0;
	    }
	}

	virtual void restart() {
	    if (!finished)
		start_working();
	}
	
	virtual bool work() {
	    int count = 1000;
	    do {
		switch (state) {
		    case 0:
			tos = stack[sp];
			x1 = xmin + (tos.left + 0.5) * step;
			y1 = ymax - (tos.top + 0.5) * step;
			if (tos.right - tos.left < 5 || tos.bottom - tos.top < 5) {
			    hc = tos.left;
			    vc = tos.top;
			    xm = x1;
			    ym = y1;
			    state = 9;
			} else {
			    value = calcandset(tos.left, tos.top, x1, y1);
			    state = 1;
			}
			break;

		    case 1:
			hm = (tos.left + tos.right) >> 1;
			vm = (tos.top + tos.bottom) >> 1;
			xm = xmin + (hm + 0.5) * step;
			ym = ymax - (vm + 0.5) * step;
			if (value != calcandset(hm, vm, xm, ym))
			    recurse();
			else
			    state = 2;
			break;

		    case 2:
			x2 = xmin + (tos.right + 0.5) * step;
			if (value != calcandset(tos.right, tos.top, x2, y1))
			    recurse();
			else
			    state = 3;
			break;

		    case 3:
			y2 = ymax - (tos.bottom + 0.5) * step;
			if (value != calcandset(tos.right, tos.bottom, x2, y2))
			    recurse();
			else
			    state = 4;
			break;

		    case 4:
			if (value != calcandset(tos.left, tos.bottom, x1, y2))
			    recurse();
			else {
			    xm = x1;
			    ym = y1;
			    hc = tos.left;
			    vc = tos.top;
			    state = 5;
			}
			break;

		    case 5:
			hc = hc + 1;
			xm = xm + step;
			if (hc == tos.right)
			    state = 6;
			else if (value != calcandset(hc, vc, xm, ym))
			    recurse();
			break;

		    case 6:
			vc = vc + 1;
			ym = ym - step;
			if (vc == tos.bottom)
			    state = 7;
			else if (value != calcandset(hc, vc, xm, ym))
			    recurse();
			break;

		    case 7:
			hc = hc - 1;
			xm = xm - step;
			if (hc == tos.left)
			    state = 8;
			else if (value != calcandset(hc, vc, xm, ym))
			    recurse();
			break;

		    case 8:
			vc = vc - 1;
			ym = ym + step;
			if (vc == tos.top) {
			    fillrect(tos.top + 1, tos.left + 1,
				     tos.bottom - 1, tos.right - 1, value);
			    pop();
			} else if (value != calcandset(hc, vc, xm, ym))
			    recurse();
			break;

		    case 9:
			value = calcandset(hc, vc, xm, ym);
			if (hc < tos.right) {
			    hc = hc + 1;
			    xm = xm + step;
			} else if (vc < tos.bottom) {
			    hc = tos.left;
			    xm = x1;
			    vc = vc + 1;
			    ym = ym - step;
			} else
			    pop();
			break;
		}
	    } while (sp >= 0 && --count >= 0);
	    if (ndirty > 1000 || finished) {
		paint(dirty.top, dirty.left, dirty.bottom + 1, dirty.right + 1);
		dirty.empty();
		ndirty = 0;
	    }
	    return finished;
	}

    private:
	void recurse() {
	    stack[sp + 1].top = vm;
	    stack[sp + 1].left = stack[sp + 0].left;
	    stack[sp + 1].bottom = stack[sp + 0].bottom;
	    stack[sp + 1].right = hm;
	    stack[sp + 2].top = stack[sp + 0].top;
	    stack[sp + 2].left = hm;
	    stack[sp + 2].bottom = vm;
	    stack[sp + 2].right = stack[sp + 0].right;
	    stack[sp + 3].top = stack[sp + 0].top;
	    stack[sp + 3].left = stack[sp + 0].left;
	    stack[sp + 3].bottom = vm;
	    stack[sp + 3].right = hm;
	    stack[sp + 0].top = vm;
	    stack[sp + 0].left = hm;
	    sp += 3;
	    state = 0;
	}

	void pop() {
	    dirty.merge(tos);
	    ndirty += (tos.right - tos.left + 1) * (tos.bottom - tos.top + 1);
	    sp--;
	    state = 0;
	    finished = sp < 0;
	}

	int calcandset(int h, int v, double x, double y) {
	    int n = 0;
	    double re = x, im = y;
	    double re2 = re * re;
	    double im2 = im * im;
	    while (n < maxiter && re2 + im2 < limit2) {
		double tmp = re2 - im2 + x;
		im = 2 * re * im + y;
		re = tmp;
		re2 = re * re;
		im2 = im * im;
		n++;
	    }
	    int value = n == maxiter ? 0
			: ((n * b2) / (maxiter - 1)) % 254 + 1;
	    pm->pixels[v * pm->bytesperline + h] = value;
	    return value;
	}
};

extern "C" Plugin *factory(void *dl) {
    return new MandelbrotMS(dl);
}
