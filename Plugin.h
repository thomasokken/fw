#ifndef PLUGIN_H
#define PLUGIN_H 1


// These three headers aren't actually needed here, but they're included for
// the convenience of plugin writers: now plugins only need to include Plugin.h
// and nothing else.

#include "FWColor.h"
#include "FWPixmap.h"
#include "PluginSettings.h"


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
	void *dump_info;

    protected:
	FWPixmap *pm;
	PluginSettings *settings;

	Plugin(void *dl);
	virtual ~Plugin();

    public:
	// Methods called by FW to manage plugins
	static Plugin *get(const char *name);
	static void release(Plugin *plugin);
	static char **list();
	void setPixmap(FWPixmap *pm) { this->pm = pm; }
	void setViewer(Viewer *viewer) { this->viewer = viewer; }
	void serialize(void **buf, int *nbytes);
	void deserialize(void *buf, int nbytes);

	// Methods implemented by Plugins
	// (Reasonable default implementations are provided for all of
	// these, except for name().)
	virtual bool does_depth(int depth);
	virtual void set_depth(int depth);
	virtual void init_new();
	virtual bool init_clone(const Plugin *src);
	virtual void init_undump();
	virtual void get_settings_ok();
	virtual void get_settings_cancel();
	virtual void run();
	virtual void restart();
	virtual void stop();
	virtual bool work();
	virtual void dump();
	virtual const char *name() const = 0;
	virtual const char *help();

    protected:
	// Utility methods provided for use by plugins
	static PluginSettings *getSettingsInstance();
	static void beep();
	static int debug_level();
	void get_settings_dialog();
	void init_proceed();
	void init_abort();
	void start_working();
	void stop_working();

	// Methods used by plugins to tell FW that the image has changed
	void paint();
	void paint(int top, int left, int bottom, int right);
	void colormapChanged();

	// The next 5 methods should only be called from dump()
	void dumpchar(int c);
	void dumpbuf(void *buf, int nbytes);
	void dumpint(int n);
	void dumpdouble(double d);
	void dumpstring(const char *s);

	// The next 5 methods should only be called from init_undump()
	int undumpchar();
	void undumpbuf(void *buf, int nbytes);
	int undumpint();
	double undumpdouble();
	char *undumpstring();
};

#endif
