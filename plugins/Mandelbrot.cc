#include <stdio.h>

#include "MarianiSilver.h"
#include "Mandelbrot.help"

static const char *my_settings_layout[] = {
    "int 'Maximum Iterations'",		// maxiter
    "double 'Cutoff Value'",		// limit
    "double",		// limit2
    NULL
};

class Mandelbrot : public MarianiSilver {
    private:
	int maxiter;
	double limit;
	double limit2;
	// End of serialized data

    public:
	Mandelbrot(void *dl) : MarianiSilver(dl) {
	    register_for_serialization(my_settings_layout, &maxiter);
	}

	virtual ~Mandelbrot() {}

	virtual const char *name() {
	    return "Mandelbrot";
	}

	virtual const char *help() {
	    return helptext;
	}

	virtual void ms_init_new() {
	    // MarianiSilver stuff
	    // pm->width = 700;
	    // pm->height = 500;
	    xmin = -1.6 * pm->width / pm->height;
	    xmax = 0.55 * pm->width / pm->height;
	    ymin = -1.075;
	    ymax = 1.075;
	    bands = 10;

	    // Our stuff
	    maxiter = 1000;
	    limit = 100;
	}

	virtual void ms_init_clone(MarianiSilver *src) {
	    Mandelbrot *clonee = (Mandelbrot *) src;

	    maxiter = clonee->maxiter;
	    limit = clonee->limit;
	}

	virtual void ms_start() {
	    limit2 = limit * limit;
	    maxValue = maxiter;
	}

	virtual int ms_calc_pixel(double x, double y) {
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
	    if (n == maxiter)
		return 0;
	    else
		return n + 1;
	}
};

#ifndef STATICPLUGINS
extern "C" Plugin *factory(void *dl) {
    return new Mandelbrot(dl);
}
#else
Plugin *Mandelbrot_factory(void *dl) {
    return new Mandelbrot(dl);
}
#endif
