#include <X11/Intrinsic.h>
#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>

#include "Plugin.h"
#include "SettingsDialog.h"
#include "SettingsHelper.h"
#include "Viewer.h"
#include "main.h"

#include "About.xbm"


static int list_compar(const void *a, const void *b);

struct WorkNode {
    Plugin *worker;
    WorkNode *next;
};
static WorkNode *worklist = NULL;
static XtWorkProcId workproc_id;
static Boolean workproc(XtPointer ud);


class Null : public Plugin {
    // This plugin is used to display files for which no plugin is available,
    // either because it wasn't specified in the file, or it was specified but
    // could not be loaded. This plugin won't show up in the plugin lists
    // that are used to build the File->New and Help menus, because it is
    // statically linked.

    public:
	Null() : Plugin(NULL) {}
	virtual ~Null() {}
	virtual const char *name() {
	    return "Null";
	}
	virtual void start() {
	    paint();
	}
};


class About : public Plugin {
    public:
	About() : Plugin(NULL) {}
	virtual ~About() {}
	virtual const char *name() {
	    return "About";
	}
	virtual void init_new() {
	    pm->width = About_width;
	    pm->height = About_height;
	    pm->bytesperline = (About_width + 31 >> 3) & ~3;
	    int srcbpl = About_width + 7 >> 3;
	    int size = pm->bytesperline * pm->height;
	    pm->pixels = (unsigned char *) malloc(size);
	    for (int v = 0; v < pm->height; v++) {
		unsigned char *src = About_bits + v * srcbpl;
		unsigned char *dst = pm->pixels + v * pm->bytesperline;
		for (int h = 0; h < srcbpl; h++)
		    *dst++ = ~*src++;
	    }
	    pm->depth = 1;
	    init_proceed();
	}
	virtual void start() {
	    paint();
	}
};


/* protected */
Plugin::Plugin(void *dl) {
    this->dl = dl;
    settings_count = 0;
    settings_layout = NULL;
    settings_base = NULL;
    settings_helper = NULL;
}

/* protected virtual */
Plugin::~Plugin() {
    stop_working();
    if (settings_helper != NULL)
	delete settings_helper;
}

/* public static */ Plugin *
Plugin::get(const char *name) {
    if (name == NULL || strcmp(name, "Null") == 0)
	return new Null();
    else if (strcmp(name, "About") == 0)
	return new About();

    char dlname[_POSIX_PATH_MAX];
    snprintf(dlname, _POSIX_PATH_MAX, "%s/.fw/%s.so", getenv("HOME"), name);
    void *dl = dlopen(dlname, RTLD_NOW);
    if (dl == NULL) {
	if (g_verbosity >= 1)
	    fprintf(stderr, "Loading \"%s\" failed: %s\n", name, dlerror());
	return NULL;
    }
    Plugin *(*factory)(void *) = (Plugin *(*)(void *)) dlsym(dl, "factory");
    return (*factory)(dl);
}

/* public static */ void
Plugin::release(Plugin *plugin) {
    void *dl = plugin->dl;
    delete plugin;
    if (dl != NULL)
	dlclose(dl);
}

/* public static */ char **
Plugin::list() {
    char dirname[_POSIX_PATH_MAX];
    snprintf(dirname, _POSIX_PATH_MAX, "%s/.fw", getenv("HOME"));
    DIR *dir = opendir(dirname);
    if (dir == NULL)
	return NULL;
    int n = 0;
    char **names = (char **) malloc(sizeof(char *));
    names[0] = NULL;
    struct dirent *dent;
    while ((dent = readdir(dir)) != NULL) {
	int len = strlen(dent->d_name);
	if (len >= 3 && strcmp(dent->d_name + (len - 3), ".so") == 0) {
	    char *name = (char *) malloc(len - 2);
	    strncpy(name, dent->d_name, len - 3);
	    name[len - 3] = 0;
	    n++;
	    names = (char **) realloc(names, (n + 1) * sizeof(char *));
	    names[n - 1] = name;
	    names[n] = NULL;
	}
    }
    closedir(dir);
    if (n == 0) {
	free(names);
	return NULL;
    }
    qsort(names, n, sizeof(char *), list_compar);
    return names;
}

/* protected */ void
Plugin::register_for_serialization(const char **layout, void *base) {
    settings_count++;
    settings_layout = (const char ***) realloc(settings_layout,
					 settings_count * sizeof(char **));
    settings_base = (void **) realloc(settings_base,
				      settings_count * sizeof(void *));
    settings_layout[settings_count - 1] = layout;
    settings_base[settings_count - 1] = base;
}

/* public */ void
Plugin::serialize(void **buf, int *nbytes) {
    if (settings_count == 0) {
	*buf = NULL;
	*nbytes = 0;
	return;
    }
    if (settings_helper == NULL)
	settings_helper = new SettingsHelper(this);
    settings_helper->serialize(buf, nbytes);
}

/* public */ void
Plugin::deserialize(void *buf, int nbytes) {
    if (settings_count == 0)
	return;
    if (settings_helper == NULL)
	settings_helper = new SettingsHelper(this);
    settings_helper->deserialize(buf, nbytes);
}

/* public virtual */ bool
Plugin::does_depth(int depth) {
    return false;
}

/* public virtual */ void
Plugin::init_new() {
    beep();
    viewer->deleteLater();
}

/* public virtual */ void
Plugin::init_clone(Plugin *src) {
    init_new();
}

/* public virtual */ void
Plugin::get_settings_ok() {
    init_proceed();
}

/* public virtual */ void
Plugin::get_settings_cancel() {
    init_abort();
}

/* public virtual */ void
Plugin::start() {
    //
}

/* public virtual */ void
Plugin::stop() {
    //
}

/* public virtual */ void
Plugin::restart() {
    //
}

/* public virtual */ bool
Plugin::work() {
    // 'true' means we're done, don't call us again
    return true;
}

/* public virtual */ const char *
Plugin::help() {
    return NULL;
}

/* protected static */ void
Plugin::beep() {
    XBell(g_display, 100);
}

/* protected static */ int
Plugin::debug_level() {
    return g_verbosity;
}

/* protected */ void
Plugin::get_settings_dialog() {
    if (settings_helper == NULL)
	settings_helper = new SettingsHelper(this);
    new SettingsDialog(this, settings_helper);
}

/* protected */ void
Plugin::init_proceed() {
    viewer->finish_init();
}

/* protected */ void
Plugin::init_abort() {
    viewer->deleteLater();
}

/* protected */ void
Plugin::start_working() {
    WorkNode *node = worklist;
    bool found = false;
    while (node != NULL) {
	if (node->worker == this) {
	    found = true;
	    break;
	}
	node = node->next;
    }
    if (!found) {
	if (worklist == NULL)
	    workproc_id = XtAppAddWorkProc(g_appcontext, workproc, NULL);
	node = new WorkNode;
	node->worker = this;
	node->next = worklist;
	worklist = node;
    }
}

/* protected */ void
Plugin::stop_working() {
    WorkNode *node = worklist;
    WorkNode **prev = &worklist;
    bool found = false;
    while (node != NULL) {
	if (node->worker == this) {
	    found = true;
	    break;
	}
	prev = &node->next;
	node = node->next;
    }
    if (found) {
	*prev = node->next;
	delete node;
	if (worklist == NULL)
	    XtRemoveWorkProc(workproc_id);
    }
}

/* protected */ void
Plugin::get_recommended_size(int *width, int *height) {
    viewer->get_recommended_size(width, height);
}

/* protected */ void
Plugin::get_screen_size(int *width, int *height) {
    viewer->get_screen_size(width, height);
}

/* protected */ void
Plugin::get_selection(int *x, int *y, int *width, int *height) {
    viewer->get_selection(x, y, width, height);
}

/* protected */ int
Plugin::get_scale() {
    return viewer->get_scale();
}

/* protected */ void
Plugin::paint() {
    viewer->paint(0, 0, pm->height, pm->width);
}

/* protected */ void
Plugin::paint(int top, int left, int bottom, int right) {
    viewer->paint(top, left, bottom, right);
}

/* protected */ void
Plugin::colormapChanged() {
    viewer->colormapChanged();
}

static int list_compar(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **) b);
}

static Boolean workproc(XtPointer ud) {
    static int n = 0;

    // Just to be safe
    if (worklist == NULL)
	return True;

    WorkNode *node = worklist;
    WorkNode **prev = &worklist;
    for (int i = 0; i < n; i++) {
	prev = &node->next;
	node = node->next;
	if (node == NULL) {
	    n = 0;
	    prev = &worklist;
	    node = worklist;
	    break;
	}
    }

    if (node->worker->work()) {
	*prev = node->next;
	delete node;
	if (worklist == NULL)
	    return True;
    }
    n++;
    return False;
}
