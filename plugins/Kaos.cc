#include <stdlib.h>

#include "Plugin.h"

struct my_settings {
    int width, height;
    my_settings() {
	width = 700;
	height = 500;
    }
};
static char *my_settings_layout[] = {
    "iWidth", "iHeight"
};

const int NMAX = 10;
const int ITERS = 10000;

struct q_struct {
    double a11, a12, a21, a22, b1, b2;
};
struct rggb_struct {
    int rg, gb;
};


class Kaos : public Plugin {
    private:
	my_settings s;
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
	    settings = &s;
	    settings_layout = my_settings_layout;
	    jOud1 = 0;
	}
	virtual ~Kaos() {}
	virtual const char *name() const {
	    return "Kaos";
	}
	virtual bool does_depth(int depth) {
	    return depth == 8 || depth == 24;
	}
	virtual void init_new() {
	    get_settings();
	}
	virtual void set_depth(int depth) {
	    this->depth = depth;
	}
	virtual void get_settings_ok() {
	    width = s.width;
	    height = s.height;
	    bytesperline = depth == 8 ? (width + 3 & ~3) : (width * 4);
	    pixels = (char *) malloc(bytesperline * height);
	    if (depth == 8) { 
		cmap = new Color[256];
		for (int k = 0; k < 256; k++)
		    cmap[k].r = cmap[k].g = cmap[k].b = k;
	    }

	    memset(pixels, 0, bytesperline * height);

	    // Reinoud's Kaos initializations

	    n = rand() % (NMAX - 4) + 5;
	    double tscale = 0.4;
	    for (int i = 0; i < n; i++) {
		q_struct *q = Q + i;
		q->a11 = tscale;
		q->a22 = tscale;
		q->a12 = (((double) rand()) / RAND_MAX) * tscale;
		q->a21 = (((double) rand()) / RAND_MAX) * tscale;
		q->b1 = (((double) rand()) / RAND_MAX) * tscale;
		q->b2 = (((double) rand()) / RAND_MAX) * tscale;

		if (depth == 24) {
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
	    for (int i = 0; i < 100; i++) {
		int j = rand() % n;
		double xOud = x;
		q_struct *q = Q + j;
		x = q->a11 * x + q->a12 * y + q->b1;
		y = q->a21 * xOud + q->a22 * y + q->b2;
	    }

	    xmid = width / 2;
	    ymid = height / 2;
	    maxmid = xmid > ymid ? xmid : ymid;
	    jop = 0;
	    jopmax = ((long ) xmid) * ymid;
	    if (depth == 8)
		jopmax /= 4;

	    Plugin::get_settings_ok();
	}
	virtual void run() {
	    finished = false;
	    paint();
	    start_prodding();
	}
	virtual void stop() {
	    stop_prodding();
	    paint();
	}
	virtual void restart() {
	    if (!finished)
		start_prodding();
	}
	virtual bool work() {
	    if (depth == 8) {
		for (int k = 0; k < ITERS && !finished; k++) {
		    int j = rand() % n;
		    double xOud = x;
		    q_struct *q = Q + j;
		    x = q->a11 * x + q->a12 * y + q->b1;
		    y = q->a21 * xOud + q->a22 * y + q->b2;
		    int xs = xmid + (int) (x * maxmid + 0.5);
		    int ys = ymid + (int) (y * maxmid + 0.5);
		    if (xs >= 0 && ys >= 0 && xs < (int) width && ys < (int) height) {
			char *p = pixels + (ys * bytesperline + xs);
			unsigned char c = *((unsigned char *) p);
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
		    int xs = xmid + (int) (x * maxmid + 0.5);
		    int ys = ymid + (int) (y * maxmid + 0.5);
		    if (xs >= 0 && ys >= 0 && xs < (int) width && ys < (int) height) {
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
			char *p = pixels + (ys * bytesperline + xs * 4 + comp);
			unsigned char c = *((unsigned char *) p);
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

    private:
	friend Plugin *factory2(void *dl);
};

Plugin *factory2(void *dl) {
    return new Kaos(dl);
}

extern "C" Plugin *factory(void *dl) {
    return factory2(dl);
}
