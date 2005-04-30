///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

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

#ifndef STATICPLUGINS
extern "C" Plugin *factory(void *dl) {
    return new Julia(dl);
}
#else
Plugin *Julia_factory(void *dl) {
    return new Julia(dl);
}
#endif
