#ifndef PLUGIN_H
#define PLUGIN_H 1


// These two headers aren't actually needed here, but they're included for
// the convenience of plugin writers (so plugins only need to include Plugin.h
// and nothing else).

#include "FWColor.h"
#include "FWPixmap.h"


// We don't want to include Viewer.h here; the plugin interface should be
// as independent as possible of the FW internals, so that changes to FW don't
// require plugins to be recompiled any more than strictly necessary.
// So, all calls to Viewer (and other FW code) are hidden in Plugin.cc; all
// that is directly exposed to the plugin is the FWPixmap, which is a sub-
// record of Viewer.

class SettingsHelper;
class Viewer;

class Plugin {
    private:
	void *dl;
	Viewer *viewer;
	int settings_count;
	const char ***settings_layout;
	void **settings_base;
	SettingsHelper *settings_helper;

	// These two members are serialized (unless the actual plugin does not
	// register anything for serialization). Do not add anything to the
	// list of members to be serialized, or you'll lose file compatibility.
	// To maintain compatibility, store any additional FW data (that is,
	// data that isn't owned by the plugin itself) in a null-terminated
	// string pointed to by 'extra'. There is no length limit to 'extra'.

	bool finished;
	char *extra;

	friend class SettingsHelper;
	friend class SlaveDriver;

    protected:
	FWPixmap *pm;

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
	char *dumpSettings();

	// Methods implemented by Plugins
	// (Reasonable default implementations are provided for all of
	// these, except for name().)
	virtual bool does_depth(int depth);
	virtual void init_new();
	virtual void init_clone(Plugin *src);
	virtual void get_settings_ok();
	virtual void get_settings_cancel();
	virtual void start();
	virtual void stop();
	virtual void restart();
	virtual bool work();
	virtual const char *name() = 0;
	virtual const char *help();

    protected:
	// Utility methods provided for use by plugins
	void register_for_serialization(const char **layout, void *base);
	static void beep();
	static int debug_level();
	void get_settings_dialog();
	void init_proceed();
	void init_abort();
	void start_working();
	void stop_working();
	void get_recommended_size(int *width, int *height);
	void get_screen_size(int *width, int *height);
	void get_selection(int *x, int *y, int *width, int *height);
	int get_scale();

	// Methods used by plugins to tell FW that the image has changed
	void paint();
	void paint(int top, int left, int bottom, int right);
	void colormapChanged();
};

#endif
