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
#include "Polynomial.help"

static const char *my_settings_layout[] = {
    "int 'Maximum Iterations'",         // maxiter
    "double 'Escape Limit'",            // limit
    "double 'a[5]'",                    // a[0] (watch out!)
    "double 'a[4]'",                    // a[1]
    "double 'a[3]'",                    // a[2]
    "double 'a[2]'",                    // a[3]
    "double 'a[1]'",                    // a[4]
    "double 'a[0]'",                    // a[5]
    "double 'b'",                       // b
    "int",              // degree
    "double",           // limit2
    NULL
};

class Polynomial : public MarianiSilver {
    private:
        int maxiter;
        double limit;
        double a[6];
        double b;
        int degree;
        double limit2;
        // End of serialized data

    public:
        Polynomial(void *dl) : MarianiSilver(dl) {
            register_for_serialization(my_settings_layout, &maxiter);
        }

        virtual ~Polynomial() {}

        virtual const char *name() {
            return "Polynomial";
        }

        virtual const char *help() {
            return helptext;
        }

        virtual void ms_init_new() {
            // MarianiSilver stuff
            // pm->width = 700;
            // pm->height = 500;
            xmin = -1.0 * pm->width / pm->height;
            xmax = 1.0 * pm->width / pm->height;
            ymin = -1;
            ymax = 1;
            bands = 1;

            // Our stuff
            maxiter = 25;
            limit = 10;
            a[0] = 0; // Remember, a[0] is the coefficient of x^5
            a[1] = 0; // x^4
            a[2] = 0; // x^3
            a[3] = 1; // x^2
            a[4] = 0; // x
            a[5] = 0; // 1
            b = 1;
        }

        virtual void ms_init_clone(MarianiSilver *src) {
            Polynomial *clonee = (Polynomial *) src;

            maxiter = clonee->maxiter;
            limit = clonee->limit;
            for (int i = 0; i < 6; i++)
                a[i] = clonee->a[i];
            b = clonee->b;
        }

        virtual void ms_start() {
            degree = 5;
            while (a[5 - degree] == 0 && degree > 0)
                degree--;
            limit2 = limit * limit;
            maxValue = maxiter;
        }

        virtual int ms_calc_pixel(double re, double im) {
            int i, count;
            double x1, y1, x2, y2, temp, b_re, b_im;

            x2 = re;
            y2 = im;
            b_re = b * re;
            b_im = b * im;
            count = 0;
            while (count < maxiter && x2 * x2 + y2 * y2 <= limit2) {
                x1 = 0;
                y1 = 0;
                for (i = degree; i > 0; i--) {
                    temp = x1 + a[5 - i];
                    x1 = temp * x2 - y1 * y2;
                    y1 = temp * y2 + y1 * x2;
                }
                x2 = x1 + a[5] + b_re;
                y2 = y1 + b_im;
                count = count + 1;
            }
            return count == maxiter ? 0 : count + 1;
        }
};

#ifndef STATICPLUGINS
extern "C" Plugin *factory(void *dl) {
    return new Polynomial(dl);
}
#else
Plugin *Polynomial_factory(void *dl) {
    return new Polynomial(dl);
}
#endif
