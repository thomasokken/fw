#ifndef PLUGIN_H
#define PLUGIN_H 1

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "Color.h"
#include "Viewer.h"

class Plugin {
    private:
	void *dl;
	struct ProdNode {
	    Plugin *proddee;
	    ProdNode *next;
	};
	static ProdNode *prodlist;
	static XtWorkProcId workproc_id;

    protected:
	Viewer *vw;
	void *settings;
	char **settings_layout;

    public:
	static Plugin *get(const char *name);
	static void release(Plugin *plugin);
	static char **list();
	void setviewer(Viewer *vw) { this->vw = vw; }
	void getsize(unsigned int *width, unsigned int *height);
	virtual int can_open(const char *filename) { return false; }
	virtual bool does_depth(int depth) { return false; }
	virtual void set_depth(int depth) {}
	virtual void init_new() { beep(); vw->deleteLater(); }
	virtual bool init_clone(const Plugin *src) { beep(); return false; }
	virtual bool init_load(const char *filename) { beep(); return false; }
	virtual void save(const char *filename) { beep(); }
	virtual void get_settings();
	virtual void get_settings_ok();
	virtual void get_settings_cancel();
	virtual void run() {}
	virtual void restart() {}
	virtual void stop() {}
	virtual bool work() { return true; }
	virtual const char *name() const = 0;
	virtual const char *help() { return NULL; }

    protected:
	Plugin(void *dl);
	virtual ~Plugin();
	static void beep();
	void paint();
	void paint(int top, int left, int bottom, int right);
	void colormapChanged();
	void start_prodding();
	void stop_prodding();
	void make_graymap();

    private:
	static Boolean workproc(XtPointer ud);
	static int list_compar(const void *a, const void *b);
};

#endif
