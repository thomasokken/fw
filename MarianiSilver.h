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

#ifndef MARIANISILVER_H
#define MARIANISILVER_H 1

#include "Plugin.h"

class MarianiSilver : public Plugin {
    private:
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

    protected:
	// Start of serialized data
	double xmin, xmax, ymin, ymax;
	int bands;
	int maxValue;

    private:
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
	// End of serialized data

	MarianiSilver *clonee;
    
    protected:
	// Subclasses should implement these methods
	virtual const char *name() = 0;
	// virtual const char *help(); -- optional; default impl. returns NULL
	virtual void ms_init_new() = 0;
	virtual void ms_init_clone(MarianiSilver *src) = 0;
	virtual void ms_start() = 0;
	virtual int ms_calc_pixel(double x, double y) = 0;

    public:
	MarianiSilver(void *dl);
	virtual ~MarianiSilver();
	virtual bool does_depth(int depth);
	virtual void init_new();
	virtual void init_clone(Plugin *src);
	virtual void get_settings_ok();
	virtual bool start();
	virtual void stop();
	virtual bool restart();
	virtual bool work();

    private:
	void fillrect(int top, int left, int bottom, int right, int value);
	void recurse();
	void pop();
	int calcandset(int h, int v, double x, double y);
};

#endif
