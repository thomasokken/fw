#include <stdlib.h>
#include <string.h>

#include "Plugin.h"

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
	    settings = getSettingsInstance();
	    settings->addField(PluginSettings::INT, "Width");
	    settings->addField(PluginSettings::INT, "Height");
	    settings->setIntField(0, 983);
	    settings->setIntField(1, 647);
	}
	virtual ~Kaos() {}
	virtual const char *name() const {
	    return "Kaos";
	}
	virtual bool does_depth(int depth) {
	    return depth == 8 || depth == 24;
	}
	virtual void init_new() {
	    get_settings_dialog();
	}
	virtual void set_depth(int depth) {
	    pm->depth = depth;
	}
	virtual void get_settings_ok() {
	    pm->width = settings->getIntField(0);
	    pm->height = settings->getIntField(1);
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
	    jopmax = ((long ) xmid) * ymid;
	    if (pm->depth == 8)
		jopmax /= 4;

	    init_proceed();
	}
	virtual void run() {
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

	virtual void dump() {
	    for (int i = 0; i < NMAX; i++) {
		dumpdouble(Q[i].a11);
		dumpdouble(Q[i].a12);
		dumpdouble(Q[i].a21);
		dumpdouble(Q[i].a22);
		dumpdouble(Q[i].b1);
		dumpdouble(Q[i].b2);
	    }
	    for (int i = 0; i < NMAX; i++) {
		dumpint(RGGB[i].rg);
		dumpint(RGGB[i].gb);
	    }
	    dumpint(n);
	    dumpint(xmid);
	    dumpint(ymid);
	    dumpint(maxmid);
	    dumpdouble(x);
	    dumpdouble(y);
	    dumpint(jop);
	    dumpint(jopmax);
	    dumpint(jOud1);
	    dumpint(finished ? 1 : 0);
	}

	virtual void init_undump() {
	    for (int i = 0; i < NMAX; i++) {
		Q[i].a11 = undumpdouble();
		Q[i].a12 = undumpdouble();
		Q[i].a21 = undumpdouble();
		Q[i].a22 = undumpdouble();
		Q[i].b1 = undumpdouble();
		Q[i].b2 = undumpdouble();
	    }
	    for (int i = 0; i < NMAX; i++) {
		RGGB[i].rg = undumpint();
		RGGB[i].gb = undumpint();
	    }
	    n = undumpint();
	    xmid = undumpint();
	    ymid = undumpint();
	    maxmid = undumpint();
	    x = undumpdouble();
	    y = undumpdouble();
	    jop = undumpint();
	    jopmax = undumpint();
	    jOud1 = undumpint();
	    finished = undumpint() != 0;
	}
};

extern "C" Plugin *factory(void *dl) {
    return new Kaos(dl);
}
