#include <stdio.h>

#include "MarianiSilver.h"
#include "Julia.help"

static const char *my_settings_layout[] = {
    "double 'c[r]'",                    // cr
    "double 'c[i]'",                    // ci
    "int 'Maximum Iterations'",		// maxiter
    "double 'Cutoff Value'",		// limit
    "double",		// limit2
    NULL
};

class Julia : public MarianiSilver {
    private:
	double cr, ci;
	int maxiter;
	double limit;
	double limit2;
	// End of serialized data

    public:
	Julia(void *dl) : MarianiSilver(dl) {
	    register_for_serialization(my_settings_layout, &cr);
	}

	virtual ~Julia() {}

	virtual const char *name() {
	    return "Julia";
	}

	virtual const char *help() {
	    return helptext;
	}

	virtual void ms_init_new() {
	    // MarianiSilver stuff
	    // pm->width = 700;
	    // pm->height = 500;
	    xmin = -2.0 * pm->width / pm->height;
	    xmax = 2.0 * pm->width / pm->height;
	    ymin = -2;
	    ymax = 2;
	    bands = 10;

	    // Our stuff
	    cr = 0;
	    ci = -1;
	    maxiter = 1000;
	    limit = 100;
	}

	virtual void ms_init_clone(MarianiSilver *src) {
	    Julia *clonee = (Julia *) src;

	    cr = clonee->cr;
	    ci = clonee->ci;
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
		im = 2 * re * im + ci;
		re = re2 - im2 + cr;
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

extern "C" Plugin *factory(void *dl) {
    return new Julia(dl);
}
