#include <X11/Intrinsic.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>

#include "Plugin.h"
#include "SettingsDialog.h"
#include "Viewer.h"
#include "main.h"


struct ProdNode {
    Plugin *proddee;
    ProdNode *next;
};
static ProdNode *prodlist = NULL;
static XtWorkProcId workproc_id;

static Boolean workproc(XtPointer ud);
static int list_compar(const void *a, const void *b);


/* protected */
Plugin::Plugin(void *dl) {
    this->dl = dl;
    settings = NULL;
    settings_layout = NULL;
}

/* protected virtual */
Plugin::~Plugin() {
    stop_prodding();
}

/* public static */ Plugin *
Plugin::get(const char *name) {
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

static int list_compar(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **) b);
}

/* public virtual */ int
Plugin::can_open(const char *filename) {
    return false;
}

/* public virtual */ bool
Plugin::does_depth(int depth) {
    return false;
}

/* public virtual */ void
Plugin::set_depth(int depth) {
    //
}

/* public virtual */ void
Plugin::init_new() {
    beep();
    viewer->deleteLater();
}

/* public virtual */ bool
Plugin::init_clone(const Plugin *src) {
    beep();
    return false;
}

/* public virtual */ bool
Plugin::init_load(const char *filename) {
    beep();
    return false;
}

/* public virtual */ void
Plugin::save(const char *filename) {
    beep();
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
Plugin::run() {
    //
}

/* public virtual */ void
Plugin::restart() {
    //
}

/* public virtual */ void
Plugin::stop() {
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

/* protected */ void
Plugin::get_settings_dialog() {
    new SettingsDialog(settings, settings_layout, this);
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

/* protected */ void
Plugin::start_prodding() {
    ProdNode *node = prodlist;
    bool found = false;
    while (node != NULL) {
	if (node->proddee == this) {
	    found = true;
	    break;
	}
	node = node->next;
    }
    if (!found) {
	if (prodlist == NULL)
	    workproc_id = XtAppAddWorkProc(g_appcontext, workproc, NULL);
	node = new ProdNode;
	node->proddee = this;
	node->next = prodlist;
	prodlist = node;
    }
}

/* protected */ void
Plugin::stop_prodding() {
    ProdNode *node = prodlist;
    ProdNode **prev = &prodlist;
    bool found = false;
    while (node != NULL) {
	if (node->proddee == this) {
	    found = true;
	    break;
	}
	prev = &node->next;
	node = node->next;
    }
    if (found) {
	*prev = node->next;
	delete node;
	if (prodlist == NULL)
	    XtRemoveWorkProc(workproc_id);
    }
}

/* protected static */ int
Plugin::debug_level() {
    return g_verbosity;
}

static Boolean workproc(XtPointer ud) {
    static int n = 0;

    // Just to be safe
    if (prodlist == NULL)
	return True;

    ProdNode *node = prodlist;
    ProdNode **prev = &prodlist;
    for (int i = 0; i < n; i++) {
	prev = &node->next;
	node = node->next;
	if (node == NULL) {
	    n = 0;
	    prev = &prodlist;
	    node = prodlist;
	    break;
	}
    }

    if (node->proddee->work()) {
	*prev = node->next;
	delete node;
	if (prodlist == NULL)
	    return True;
    }
    n++;
    return False;
}
