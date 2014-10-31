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

static unsigned char cool_colors[] = {
    242, 10, 230, 238, 10, 229, 234, 10, 228, 230, 10, 227, 226, 10, 226,
    222, 10, 225, 219, 10, 224, 215, 11, 224, 211, 11, 223, 207, 11, 222,
    203, 11, 221, 199, 11, 220, 195, 11, 219, 192, 11, 218, 188, 12, 217,
    184, 12, 216, 180, 12, 215, 176, 12, 214, 172, 12, 213, 169, 12, 212,
    165, 12, 211, 161, 13, 211, 157, 13, 210, 153, 13, 209, 149, 13, 208,
    146, 13, 207, 142, 13, 206, 138, 13, 205, 134, 14, 204, 130, 14, 203,
    126, 14, 202, 122, 14, 201, 119, 14, 200, 115, 14, 199, 111, 14, 198,
    107, 15, 198, 103, 15, 197, 99, 15, 196, 96, 15, 195, 92, 15, 194,
    88, 15, 193, 84, 15, 192, 80, 16, 191, 76, 16, 190, 73, 16, 189,
    69, 16, 188, 65, 16, 187, 61, 16, 186, 57, 16, 185, 53, 17, 185,
    49, 17, 184, 46, 17, 183, 42, 17, 182, 38, 17, 181, 34, 17, 180,
    30, 17, 179, 26, 18, 178, 23, 18, 177, 19, 18, 176, 15, 18, 175,
    11, 18, 174, 7, 18, 173, 3, 18, 172, 0, 18, 172, 244, 9, 231,
    240, 10, 230, 236, 10, 229, 232, 10, 228, 228, 10, 227, 224, 10, 226,
    220, 10, 225, 217, 10, 224, 213, 11, 223, 209, 11, 222, 205, 11, 221,
    201, 11, 220, 197, 11, 219, 194, 11, 218, 190, 11, 217, 186, 12, 217,
    182, 12, 216, 178, 12, 215, 174, 12, 214, 170, 12, 213, 167, 12, 212,
    163, 12, 211, 159, 13, 210, 155, 13, 209, 151, 13, 208, 147, 13, 207,
    144, 13, 206, 140, 13, 205, 136, 13, 204, 132, 14, 204, 128, 14, 203,
    124, 14, 202, 121, 14, 201, 117, 14, 200, 113, 14, 199, 109, 14, 198,
    105, 15, 197, 101, 15, 196, 97, 15, 195, 94, 15, 194, 90, 15, 193,
    86, 15, 192, 82, 15, 191, 78, 16, 191, 74, 16, 190, 71, 16, 189,
    67, 16, 188, 63, 16, 187, 59, 16, 186, 55, 16, 185, 51, 17, 184,
    48, 17, 183, 44, 17, 182, 40, 17, 181, 36, 17, 180, 32, 17, 179,
    28, 17, 178, 24, 18, 178, 21, 18, 177, 17, 18, 176, 13, 18, 175,
    9, 18, 174, 5, 18, 173, 1, 18, 172, 254, 253, 1, 254, 249, 1,
    254, 245, 1, 254, 241, 1, 254, 237, 1, 254, 233, 2, 254, 229, 2,
    254, 225, 2, 254, 221, 2, 254, 217, 2, 254, 213, 3, 254, 209, 3,
    254, 205, 3, 254, 201, 3, 254, 197, 3, 254, 193, 4, 253, 189, 4,
    253, 185, 4, 253, 181, 4, 253, 177, 4, 253, 173, 5, 253, 170, 5,
    253, 166, 5, 253, 162, 5, 253, 158, 6, 253, 154, 6, 253, 150, 6,
    253, 146, 6, 253, 142, 6, 253, 138, 7, 253, 134, 7, 253, 130, 7,
    252, 126, 7, 252, 122, 7, 252, 118, 8, 252, 114, 8, 252, 110, 8,
    252, 106, 8, 252, 102, 8, 252, 98, 9, 252, 94, 9, 252, 90, 9,
    252, 87, 9, 252, 83, 9, 252, 79, 10, 252, 75, 10, 252, 71, 10,
    252, 67, 10, 251, 63, 10, 251, 59, 11, 251, 55, 11, 251, 51, 11,
    251, 47, 11, 251, 43, 11, 251, 39, 12, 251, 35, 12, 251, 31, 12,
    251, 27, 12, 251, 23, 12, 251, 19, 13, 251, 15, 13, 251, 11, 13,
    251, 7, 13, 251, 4, 13, 255, 255, 0, 254, 251, 1, 254, 247, 1,
    254, 243, 1, 254, 239, 1, 254, 235, 2, 254, 231, 2, 254, 227, 2,
    254, 223, 2, 254, 219, 2, 254, 215, 3, 254, 211, 3, 254, 207, 3,
    254, 203, 3, 254, 199, 3, 254, 195, 4, 253, 191, 4, 253, 187, 4,
    253, 183, 4, 253, 179, 4, 253, 175, 5, 253, 171, 5, 253, 168, 5,
    253, 164, 5, 253, 160, 5, 253, 156, 6, 253, 152, 6, 253, 148, 6,
    253, 144, 6, 253, 140, 6, 253, 136, 7, 253, 132, 7, 252, 128, 7,
    252, 124, 7, 252, 120, 7, 252, 116, 8, 252, 112, 8, 252, 108, 8,
    252, 104, 8, 252, 100, 8, 252, 96, 9, 252, 92, 9, 252, 88, 9,
    252, 85, 9, 252, 81, 10, 252, 77, 10, 252, 73, 10, 252, 69, 10,
    251, 65, 10, 251, 61, 11, 251, 57, 11, 251, 53, 11, 251, 49, 11,
    251, 45, 11, 251, 41, 12, 251, 37, 12, 251, 33, 12, 251, 29, 12,
    251, 25, 12, 251, 21, 13, 251, 17, 13, 251, 13, 13, 251, 9, 13,
    251, 5, 13
};

static const char *my_settings_layout[] = {
    "int 'Maximum Iterations'",         // maxiter
    "double 'Convergence Limit'",       // conv
    "double 'Root (real)'",             // root_re
    "double 'Root (imaginary)'",        // root_im
    "double",           // conv2
    NULL
};

class Newton : public MarianiSilver {
    private:
        int maxiter;
        double conv;
        double root_re, root_im;
        double conv2;
        // End of serialized data
        
        bool i_am_a_clone;

    public:
        Newton(void *dl) : MarianiSilver(dl) {
            register_for_serialization(my_settings_layout, &maxiter);
        }

        virtual ~Newton() {}

        virtual const char *name() {
            return "Newton";
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
            conv = 0.001;
            root_re = 1;
            root_im = 0;
            i_am_a_clone = false;
        }

        virtual void ms_init_clone(MarianiSilver *src) {
            Newton *clonee = (Newton *) src;

            maxiter = clonee->maxiter;
            conv = clonee->conv;
            root_re = clonee->root_re;
            root_im = clonee->root_im;

            i_am_a_clone = true;
        }

        virtual void ms_start() {
            conv2 = conv * conv;
            maxValue = maxiter * 2;

            if (!i_am_a_clone) {
                // If we're a clone, we get the colormap from our original,
                // but if we're not, we get a boring grayscale colormap.
                // Let's use something funkier.
                for (int i = 0; i < 256; i++) {
                    pm->cmap[i].r = cool_colors[3 * i];
                    pm->cmap[i].g = cool_colors[3 * i + 1];
                    pm->cmap[i].b = cool_colors[3 * i + 2];
                }
            }
        }

        virtual int ms_calc_pixel(double re, double im) {
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
                a = re - root_re;
                b = im - root_im;
                if (a * a + b * b <= conv2)
                    return count + maxiter;
                else
                    return count;
            } else
                return 0;

            return value;
        }
};

#ifndef STATICPLUGINS
extern "C" Plugin *factory(void *dl) {
    return new Newton(dl);
}
#else
Plugin *Newton_factory(void *dl) {
    return new Newton(dl);
}
#endif
