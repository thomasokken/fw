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
	Viewer *viewer;
	char *pixels;
	Color *cmap;
	int depth;
	unsigned int width, height, bytesperline;
	void *settings;
	char **settings_layout;

    public:
	static Plugin *get(const char *name);
	static void release(Plugin *plugin);
	void setviewer(Viewer *viewer) { this->viewer = viewer; }
	void getsize(unsigned int *width, unsigned int *height);
	virtual bool does_depth(int depth) { return false; }
	virtual void set_depth(int depth) {}
	virtual void init_new() { beep(); delete viewer; }
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
	void paint();

    protected:
	Plugin(void *dl);
	virtual ~Plugin();
	static void beep();
	void paint(unsigned int top, unsigned int left,
		   unsigned int bottom, unsigned int right);
	void start_prodding();
	void stop_prodding();
	void make_graymap();

    private:
	static Boolean workproc(XtPointer ud);
};

#endif
