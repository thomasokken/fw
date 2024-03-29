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

#include <stdlib.h>
#include <string.h>

#include "Plugin.h"

static const char *my_settings_layout[] = {
    "WIDTH 'Width'",
    "HEIGHT 'Height'",
    "int 'X contribution to Red'",
    "int 'Y contribution to Red'",
    "int 'X contribution to Green'",
    "int 'Y contribution to Green'",
    "int 'X contribution to Blue'",
    "int 'Y contribution to Blue'",
    NULL
};

class Ramp : public Plugin {
    private:
        int xr, yr, xg, yg, xb, yb;

    public:
        Ramp(void *dl) : Plugin(dl) {
            register_for_serialization(my_settings_layout, &xr);
        }
        virtual ~Ramp() {}
        virtual const char *name() {
            return "Ramp";
        }
        virtual bool does_depth(int depth) {
            return depth == 8 || depth == 24;
        }
        virtual void init_new() {
            pm->width = 256;
            pm->height = 256;
            xr = 1;
            yr = 0;
            xg = 0;
            yg = 1;
            xb = 0;
            yb = 0;
            get_settings_dialog();
        }
        virtual void get_settings_ok() {
            pm->bytesperline = pm->depth == 8 ? (pm->width + 3 & ~3) : (pm->width * 4);
            pm->pixels = (unsigned char *) malloc(pm->bytesperline * pm->height);
            unsigned char *dst = pm->pixels;

            int roffset = 0, boffset = 0, goffset = 0;
            int rshift = 0, bshift = 0, gshift = 0;

            if (xr < 0) {
                xr = -1;
                roffset += 255;
            } else if (xr > 0)
                xr = 1;
            if (yr < 0) {
                yr = -1;
                roffset += 255;
            } else if (yr > 0)
                yr = 1;

            if (xg < 0) {
                xg = -1;
                goffset += 255;
            } else if (xg > 0)
                xg = 1;
            if (yg < 0) {
                yg = -1;
                goffset += 255;
            } else if (yg > 0)
                yg = 1;

            if (xb < 0) {
                xb = -1;
                boffset += 255;
            } else if (xb > 0)
                xb = 1;
            if (yb < 0) {
                yb = -1;
                boffset += 255;
            } else if (yb > 0)
                yb = 1;

            rshift = xr != 0 && yr != 0 ? 1 : 0;
            gshift = xg != 0 && yg != 0 ? 1 : 0;
            bshift = xb != 0 && yb != 0 ? 1 : 0;

            for (int y = 0; y < pm->height; y++) {
                int yy = (y * 255) / (pm->height - 1);
                for (int x = 0; x < pm->width; x++) {
                    int xx = (x * 255) / (pm->width - 1);
                    if (pm->depth == 24)
                        *dst++ = 0;
                    *dst++ = (roffset + xr * xx + yr * yy) >> rshift;
                    if (pm->depth == 24) {
                        *dst++ = (goffset + xg * xx + yg * yy) >> gshift;
                        *dst++ = (boffset + xb * xx + yb * yy) >> bshift;
                    }
                }
            }

            if (pm->depth == 8) {
                pm->cmap = new FWColor[256];
                for (int k = 0; k < 256; k++)
                    pm->cmap[k].r = pm->cmap[k].g = pm->cmap[k].b = k;
            }

            init_proceed();
        }
        virtual bool start() {
            paint();
            return true;
        }
};

#ifndef STATICPLUGINS
extern "C" Plugin *factory(void *dl) {
    return new Ramp(dl);
}
#else
Plugin *Ramp_factory(void *dl) {
    return new Ramp(dl);
}
#endif
