#include <stdlib.h>
#include <string.h>

#include "MarianiSilver.h"

static const char *my_settings_layout[] = {
    "WIDTH 'Width'",			// pm->width
    "HEIGHT 'Height'",			// pm->height
    "double 'Lower Real Bound'",	// xmin
    "double 'Upper Real Bound'",	// xmax
    "double 'Lower Imaginary Bound'",	// ymin
    "double 'Upper Imaginary Bound'",	// ymax
    "int 'Color Bands'",		// bands
    "int",				// maxValue (set by subclass)
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


/* public */
MarianiSilver::MarianiSilver(void *dl) : Plugin(dl) {
    register_for_serialization(my_settings_layout, &xmin);
    clonee = NULL;
}

/* public virtual */
MarianiSilver::~MarianiSilver() {
    //
}

/* public virtual */ bool
MarianiSilver::does_depth(int depth) {
    return depth == 8;
}

/* public virtual */ void
MarianiSilver::init_new() {
    get_recommended_size(&pm->width, &pm->height);
    bands = 1;

    // Subclasses should set
    //
    //   pm->width
    //   pm->height
    //   xmin
    //   xmax
    //   ymin
    //   ymax
    //   bands
    //
    // pm->width and pm->height are prepopulated with the recommended
    // size (the size that maximizes the image without requiring
    // scrolling to see it all); bands is prepopulated as 1.

    ms_init_new();
    get_settings_dialog();
}

/* public virtual */ void
MarianiSilver::init_clone(Plugin *src) {
    clonee = (MarianiSilver *) src;

    int sx, sy, sw, sh;
    clonee->get_selection(&sx, &sy, &sw, &sh);
    if (sx == -1) {
	sx = 0;
	sy = 0;
	sw = clonee->pm->width;
	sh = clonee->pm->height;
    }

    int scale = clonee->get_scale();
    if (scale >= 1) {
	pm->width = sw * scale;
	pm->height = sh * scale;
    } else {
	pm->width = sw / (-scale);
	pm->height = sh / (-scale);
    }

    xmin = clonee->xmin + ((clonee->xmax - clonee->xmin)
		    * sx) / (clonee->pm->width - 1);
    xmax = clonee->xmin + ((clonee->xmax - clonee->xmin)
		    * (sx + sw - 1)) / (clonee->pm->width - 1);
    ymax = clonee->ymax - ((clonee->ymax - clonee->ymin)
		    * sy) / (clonee->pm->height - 1);
    ymin = clonee->ymax - ((clonee->ymax - clonee->ymin)
		    * (sy + sh - 1)) / (clonee->pm->height - 1);

    bands = clonee->bands;
    maxValue = clonee->maxValue;

    ms_init_clone(clonee);
    get_settings_dialog();
}

/* public virtual */ void
MarianiSilver::get_settings_ok() {
    pm->bytesperline = pm->width + 3 & ~3;
    pm->pixels = (unsigned char *) malloc(pm->bytesperline * pm->height);
    pm->cmap = new FWColor[256];
    if (clonee != NULL && clonee->pm->depth == 8) {
	FWPixmap *cpm = clonee->pm;

	// We're a clone. We copy our original's colormap, and we copy a
	// greyed-out version of the original bitmap so the user has something
	// to look at while the computation is under way. Since there is no
	// guarantee that there's white in the color map, we look for the
	// brightest color.
	int brightest_pixel = 0;
	int brightest_value = 0;
	for (int k = 0; k < 256; k++) {
	    pm->cmap[k] = cpm->cmap[k];
	    int brightness = 306 * pm->cmap[k].r
			   + 601 * pm->cmap[k].g
			   + 117 * pm->cmap[k].b;
	    if (brightness > brightest_value) {
		brightest_value = brightness;
		brightest_pixel = k;
	    }
	}

	// Preparations for clonee->clone pixel coordinate transformation
	int sx, sy, sw, sh;
	clonee->get_selection(&sx, &sy, &sw, &sh);
	if (sx == -1) {
	    sx = 0;
	    sy = 0;
	    sw = cpm->width;
	    sh = cpm->height;
	}
	float xscale = ((float) sw) / pm->width;
	float yscale = ((float) sh) / pm->height;

	for (int y = 0; y < pm->height; y++)
	    for (int x = 0; x < pm->width; x++) {
		unsigned char *addr = pm->pixels + y * pm->bytesperline + x;
		if (((x ^ y) & 1) == 0)
		    *addr = brightest_pixel;
		else {
		    int xx = ((int) (x * xscale)) + sx;
		    if (xx >= 0 && xx < cpm->width) {
			int yy = ((int) (y * yscale)) + sy;
			if (yy >= 0 && yy < cpm->width)
			    *addr = cpm->pixels[yy * cpm->bytesperline + xx];
			else
			    *addr = brightest_pixel;
		    } else
			*addr = brightest_pixel;
		}
	    }
    } else {
	for (int k = 0; k < 256; k++) {
	    pm->cmap[k].r = k;
	    pm->cmap[k].g = k;
	    pm->cmap[k].b = k;
	}
	memset(pm->pixels, 255, pm->bytesperline * pm->height);
    }
    init_proceed();
}

/* public virtual */ bool
MarianiSilver::start() {
    ms_start();
    b2 = bands * 254;
    step = (xmax - xmin) / pm->width;
    sp = 0;
    stack[sp].set(0, 0, pm->height - 1, pm->width - 1);
    state = 0;
    ndirty = 0;
    dirty.empty();
    finished = false;
    paint();
    start_working();
    return false;
}

/* public virtual */ void
MarianiSilver::stop() {
    stop_working();
    if (!dirty.isEmpty()) {
	paint(dirty.top, dirty.left, dirty.bottom + 1, dirty.right + 1);
	dirty.empty();
	ndirty = 0;
    }
}

/* public virtual */ bool
MarianiSilver::restart() {
    if (!finished)
	start_working();
    return finished;
}
	
/* public virtual */ bool
MarianiSilver::work() {
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

/* private */ void
MarianiSilver::fillrect(int top, int left, int bottom, int right, int value) {
    for (int y = top; y <= bottom; y++)
	for (int x = left; x <= right; x++)
	    pm->pixels[y * pm->bytesperline + x] = value;
}

/* private */ void
MarianiSilver::recurse() {
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

/* private */ void
MarianiSilver::pop() {
    dirty.merge(tos);
    ndirty += (tos.right - tos.left + 1) * (tos.bottom - tos.top + 1);
    sp--;
    state = 0;
    finished = sp < 0;
}

/* private */ int
MarianiSilver::calcandset(int h, int v, double x, double y) {
    int n = ms_calc_pixel(x, y);
    int value = n == 0 ? 0
		: (((n - 1) * b2) / (maxValue - 1)) % 255 + 1;
    pm->pixels[v * pm->bytesperline + h] = value;
    return value;
}
