#include <stdlib.h>
#include <string.h>

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
    "double 'Convergence Limit'",	// conv
    "double 'Root (real)'",		// root_re
    "double 'Root (imaginary)'",	// root_im
    "int 'Color Bands'",		// bands
    "double",		// conv2
    "int",		// ndirty
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
    "REPEAT 4",		// tos
    "int",
    "ENDREP",
    "REPEAT 4",		// dirty
    "int",
    "ENDREP",
    "int",		// hm
    "int",		// vm
    "int",		// hc
    "int",		// vc
    "REPEAT 50",	// stack (50 levels of 4 ints each)
    "REPEAT 4",
    "int",
    "ENDREP",
    "ENDREP",
    "bool",		// finished
    NULL
};

class Newton : public Plugin {
    private:
	double xmin, xmax, ymin, ymax;
	int maxiter;
	double conv;
	double root_re, root_im;
	int bands;
	double conv2;
	int ndirty;
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

	Newton(void *dl) : Plugin(dl) {
	    register_for_serialization(my_settings_layout, &xmin);
	}

	virtual ~Newton() {}

	virtual const char *name() {
	    return "Newton";
	}

	virtual bool does_depth(int depth) {
	    return depth == 8;
	}

	virtual void init_new() {
	    pm->width = 700;
	    pm->height = 500;
	    xmin = -1.0 * pm->width / pm->height;
	    xmax = 1.0 * pm->width / pm->height;
	    ymin = -1;
	    ymax = 1;
	    maxiter = 25;
	    conv = 0.001;
	    root_re = 1;
	    root_im = 0;
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
	    conv2 = conv * conv;
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

	int calcandset(int h, int v, double re, double im) {
	    double re2, im2, a, b, c, d, e, re_p, im_p;
	    int count;
	    bool converged;

	    count = 0;
	    do {
		count = count + 1;
		re_p = re;
		im_p = im;
		re2 = re * re;
		im2 = im * im;
		a = 2 * re * (re2 - 3 * im2) + 1;   // (a,b) = 2*z^3+1
		b = 2 * im * (3 * re2 - im2);
		c = 3 * (re2 - im2);                // (c,d) = 3z^2
		d = 6 * re * im;
		e = c * c + d * d;
		re = (a * c + b * d) / e;           // z = (a,b)/(c,d)
		im = (b * c - a * d) / e;
		a = re - re_p;
		b = im - im_p;
		converged = (a * a + b * b) <= conv2;
	    } while (!converged && count < maxiter);

	    int value;
	    if (converged) {
		value = (count * 127 + maxiter / 2) / maxiter;
		a = re - root_re;
		b = im - root_im;
		if (a * a + b * b <= conv2)
		    value += 127;
		value += 2;
	    } else
		value = 0;

	    pm->pixels[v * pm->bytesperline + h] = value;
	    return value;
	}
};

extern "C" Plugin *factory(void *dl) {
    return new Newton(dl);
}
