#include <stdlib.h>
#include <string.h>

#include "Plugin.h"
#include "Kaos.help"

const int NMAX = 10;
const int ITERS = 10000;

struct q_struct {
    double a11, a12, a21, a22, b1, b2;
};

struct rggb_struct {
    int rg, gb;
};

static int my_round(double x) {
    if (x >= 0)
	return (int) (x + 0.5);
    else
	return (int) (x - 0.5);
}

static double my_rand() {
    return ((double) (rand() - RAND_MAX / 2)) / RAND_MAX * 2;
}

static const char *my_settings_layout[] = {
    "WIDTH 'Width'",
    "HEIGHT 'Height'",
    "REPEAT 60", // Q array
    "double",
    "ENDREP",
    "REPEAT 20", // RGGB array
    "int",
    "ENDREP",
    "int",       // n
    "int",       // xmid
    "int",       // ymid
    "int",       // maxmid
    "double",    // x
    "double",    // y
    "int",       // jop
    "long",      // jopmax
    "long",      // jOud1
    "bool",      // finished
    NULL
};

class Kaos : public Plugin {
    private:
	q_struct Q[NMAX];
	rggb_struct RGGB[NMAX];
	int n;
	int xmid, ymid;
	int maxmid;
	double x, y;
	long jop, jopmax;
	int jOud1;
	bool finished;
	
    public:
	Kaos(void *dl) : Plugin(dl) {
	    settings_layout = my_settings_layout;
	    settings_base = &Q;
	}
	virtual ~Kaos() {}
	virtual const char *name() {
	    return "Kaos";
	}
	virtual const char *help() {
	    return helptext;
	}
	virtual bool does_depth(int depth) {
	    return depth == 8 || depth == 24;
	}
	virtual void init_new() {
	    pm->width = 983;
	    pm->height = 647;
	    pm->depth = 24;
	    get_settings_dialog();
	}
	virtual void get_settings_ok() {
	    pm->bytesperline = pm->depth == 8 ? (pm->width + 3 & ~3) : (pm->width * 4);
	    pm->pixels = (unsigned char *) malloc(pm->bytesperline * pm->height);
	    if (pm->depth == 8) { 
		pm->cmap = new FWColor[256];
		for (int k = 0; k < 256; k++)
		    pm->cmap[k].r = pm->cmap[k].g = pm->cmap[k].b = k;
	    }

	    memset(pm->pixels, 0, pm->bytesperline * pm->height);

	    // Reinoud's Kaos initializations

	    n = rand() % (NMAX - 4) + 5;
	    double tscale = 0.4;
	    for (int i = 0; i < n; i++) {
		q_struct *q = Q + i;
		q->a11 = tscale;
		q->a22 = tscale;
		q->a12 = my_rand() * tscale;
		q->a21 = my_rand() * tscale;
		q->b1 = my_rand() * tscale;
		q->b2 = my_rand() * tscale;

		if (pm->depth == 24) {
		    int r1 = rand();
		    int r2 = rand();
		    if (r1 > r2) {
			RGGB[i].rg = r2;
			RGGB[i].gb = r1;
		    } else {
			RGGB[i].rg = r1;
			RGGB[i].gb = r2;
		    }
		}
	    }

	    x = 0;
	    y = 0;
	    int j = 0;
	    for (int i = 0; i < 100; i++) {
		jOud1 = j;
		j = rand() % n;
		double xOud = x;
		q_struct *q = Q + j;
		x = q->a11 * x + q->a12 * y + q->b1;
		y = q->a21 * xOud + q->a22 * y + q->b2;
	    }

	    xmid = pm->width / 2;
	    ymid = pm->height / 2;
	    maxmid = xmid > ymid ? xmid : ymid;
	    jop = 0;
	    jopmax = ((long) xmid) * ymid;
	    if (pm->depth == 8)
		jopmax /= 4;

	    init_proceed();
	}
	virtual void start() {
	    finished = false;
	    paint();
	    start_working();
	}
	virtual void stop() {
	    stop_working();
	    paint();
	}
	virtual void restart() {
	    if (!finished)
		start_working();
	}
	virtual bool work() {
	    if (pm->depth == 8) {
		for (int k = 0; k < ITERS && !finished; k++) {
		    int j = rand() % n;
		    double xOud = x;
		    q_struct *q = Q + j;
		    x = q->a11 * x + q->a12 * y + q->b1;
		    y = q->a21 * xOud + q->a22 * y + q->b2;
		    int xs = xmid + my_round(x * maxmid);
		    int ys = ymid + my_round(y * maxmid);
		    if (xs >= 0 && ys >= 0 && xs < pm->width && ys < pm->height) {
			unsigned char *p = pm->pixels + (ys * pm->bytesperline + xs);
			unsigned char c = *p;
			if (c < 254)
			    *p = c + 1;
			else
			    finished = true;
		    }
		    jop++;
		}
	    } else {
		int j = 0;
		for (int k = 0; k < ITERS && !finished; k++) {
		    int jOud2 = jOud1;
		    jOud1 = j;
		    int rnd = rand();
		    j = rnd % n;
		    double xOud = x;
		    q_struct *q = Q + j;
		    x = q->a11 * x + q->a12 * y + q->b1;
		    y = q->a21 * xOud + q->a22 * y + q->b2;
		    int xs = xmid + my_round(x * maxmid);
		    int ys = ymid + my_round(y * maxmid);
		    if (xs >= 0 && ys >= 0 && xs < pm->width && ys < pm->height) {
			int i, comp;
			switch (rand() % 3) {
			    case 0:
				i = j;
				break;
			    case 1:
				i = jOud1;
				break;
			    case 2:
				i = jOud2;
				break;
			}
			if (rnd < RGGB[i].rg)
			    comp = 1;
			else if (rnd < RGGB[i].gb)
			    comp = 2;
			else
			    comp = 3;
			unsigned char *p = pm->pixels + (ys * pm->bytesperline + xs * 4 + comp);
			unsigned char c = *p;
			if (c < 254)
			    *p = c + 1;
			else
			    finished = true;
		    }
		    jop++;
		}
	    }

	    if (jop > jopmax || finished) {
		paint();
		jop = 0;
	    }
	    return finished;
	}
};

extern "C" Plugin *factory(void *dl) {
    return new Kaos(dl);
}
