#ifndef PLUGIN_H
#define PLUGIN_H 1


// These two headers aren't actually needed here, but they're included for the
// convenience of plugin writers: now plugins only need to include Plugin.h
// and nothing else.

#include "FWColor.h"
#include "FWPixmap.h"


// We don't want to include Viewer.h here; the plugin interface should be
// as independent as possible of the FW internals, so that changes to FW don't
// require plugins to be recompiled any more than strictly necessary.
// So, all calls to Viewer (and other FW code) are hidden in Plugin.cc; all
// that is directly exposed to the plugin is the FWPixmap, which is a sub-
// record of Viewer.

class Viewer;


class Plugin {
    private:
	void *dl;
	Viewer *viewer;

    protected:
	FWPixmap *pm;
	void *settings;
	char **settings_layout;

    public:
	static Plugin *get(const char *name);
	static void release(Plugin *plugin);
	static char **list();
	void setPixmap(FWPixmap *pm) { this->pm = pm; }
	void setViewer(Viewer *viewer) { this->viewer = viewer; }
	virtual int can_open(const char *filename);
	virtual bool does_depth(int depth);
	virtual void set_depth(int depth);
	virtual void init_new();
	virtual bool init_clone(const Plugin *src);
	virtual bool init_load(const char *filename);
	virtual void save(const char *filename);
	virtual void get_settings_ok();
	virtual void get_settings_cancel();
	virtual void run();
	virtual void restart();
	virtual void stop();
	virtual bool work();
	virtual const char *name() const = 0;
	virtual const char *help();

    protected:
	Plugin(void *dl);
	virtual ~Plugin();
	static void beep();
	void get_settings_dialog();
	void init_proceed();
	void init_abort();
	void paint();
	void paint(int top, int left, int bottom, int right);
	void colormapChanged();
	void start_working();
	void stop_working();
	static int debug_level();
};

#endif
