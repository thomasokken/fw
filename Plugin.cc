#include <X11/Intrinsic.h>
#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>

#include "Plugin.h"
#include "BasicPluginSettings.h"
#include "SettingsDialog.h"
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

struct DumpInfo {
    char *buf;
    int bufsize;
    int bufptr;
    bool failed;
    DumpInfo() {
	buf = NULL;
	bufsize = 0;
	bufptr = 0;
    }
    void append(const char *b, int n) {
	if (bufptr + n > bufsize) {
	    // Grow the buffer by the number of bytes needed, add another
	    // 1024 for good measure (if we're too stingy, we only end up
	    // having to realloc() more often). Finally, round the new size
	    // up to the nearest kilobyte.
	    int newsize = bufptr + n + 1024;
	    newsize = (newsize + 1023) & ~1023;
	    buf = (char *) realloc(buf, newsize);
	    bufsize = newsize;
	}
	memcpy(buf + bufptr, b, n);
	bufptr += n;
    }
    int getchar() {
	if (bufptr >= bufsize)
	    return -1;
	else
	    return buf[bufptr++];
    }
};


class Null : public Plugin {
    // This plugin is used to display files for which no plugin is available,
    // either because it wasn't specified in the file, or it was specified but
    // could not be loaded. This plugin won't show up in the plugin lists
    // that are used to build the File->New and Help menus, because it is
    // statically linked.

    public:
	Null() : Plugin(NULL) {}
	virtual ~Null() {}
	virtual const char *name() const {
	    return "Null";
	}
	virtual void run() {
	    paint();
	}
};


class About : public Plugin {
    public:
	About() : Plugin(NULL) {}
	virtual ~About() {}
	virtual const char *name() const {
	    return "About";
	}
	virtual void init_new() {
	    pm->width = About_width;
	    pm->height = About_height;
	    pm->bytesperline = About_width + 7 >> 3;
	    int size = pm->bytesperline * pm->height;
	    pm->pixels = (unsigned char *) malloc(size);
	    for (int i = 0; i < size; i++)
		pm->pixels[i] = ~About_bits[i];
	    pm->depth = 1;
	    init_proceed();
	}
	virtual void run() {
	    paint();
	}
};


/* protected */
Plugin::Plugin(void *dl) {
    this->dl = dl;
    settings = NULL;
}

/* protected virtual */
Plugin::~Plugin() {
    stop_working();
    if (settings != NULL)
	delete settings;
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

/* public */ void
Plugin::serialize(void **buf, int *nbytes) {
    DumpInfo *di = new DumpInfo;
    dump_info = di;
    const char *pn = name();
    dumpstring(pn == NULL ? "Null" : pn);
    dump();
    *buf = di->buf;
    *nbytes = di->bufptr;
    delete di;
}

/* public */ void
Plugin::deserialize(void *buf, int nbytes) {
    DumpInfo *di = new DumpInfo;
    dump_info = di;
    di->buf = (char *) buf;
    di->bufsize = nbytes;
    init_undump();
    delete di;
}

/* public virtual */ bool
Plugin::does_depth(int depth) {
    return false;
}

/* public virtual */ void
Plugin::set_depth(int depth) {
    pm->depth = depth;
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

/* public virtual */ void
Plugin::init_undump() {
    //
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

/* public virtual */ void
Plugin::dump() {
    //
}

/* public virtual */ const char *
Plugin::help() {
    return NULL;
}

/* protected static */ PluginSettings *
Plugin::getSettingsInstance() {
    return new BasicPluginSettings();
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
    new SettingsDialog(this, settings);
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
Plugin::dumpchar(int c) {
    DumpInfo *di = (DumpInfo *) dump_info;
    char ch = c;
    di->append(&ch, 1);
}

/* protected */ void
Plugin::dumpbuf(void *buf, int nbytes) {
    DumpInfo *di = (DumpInfo *) dump_info;
    di->append((char *) buf, nbytes);
}

/* protected */ void
Plugin::dumpint(int i) {
    DumpInfo *di = (DumpInfo *) dump_info;
    char buf[32];
    snprintf(buf, 32, "%d", i);
    di->append(buf, strlen(buf) + 1);
}

/* protected */ void
Plugin::dumpdouble(double d) {
    DumpInfo *di = (DumpInfo *) dump_info;
    char buf[32];
    snprintf(buf, 32, "%.16g", d);
    di->append(buf, strlen(buf) + 1);
}

/* protected */ void
Plugin::dumpstring(const char *s) {
    DumpInfo *di = (DumpInfo *) dump_info;
    di->append(s, strlen(s) + 1);
}

/* protected */ int
Plugin::undumpchar() {
    DumpInfo *di = (DumpInfo *) dump_info;
    return di->getchar();
}

/* protected */ void
Plugin::undumpbuf(void *buf, int nbytes) {
    DumpInfo *di = (DumpInfo *) dump_info;
    if (di->bufptr + nbytes > di->bufsize) {
	int new_nbytes = di->bufsize - di->bufptr;
	memset(((char *) buf) + new_nbytes, 0, nbytes - new_nbytes);
	nbytes = new_nbytes;
    }
    memcpy(buf, di->buf, nbytes);
    di->bufptr += nbytes;
}

/* protected */ int
Plugin::undumpint() {
    DumpInfo *di = (DumpInfo *) dump_info;
    char buf[32];
    int p = 0, c;
    do {
	c = di->getchar();
	if (c == -1)
	    c = 0;
	buf[p++] = c;
    } while (p < 31 && c != 0);
    buf[p] = 0;
    int i = 0;
    sscanf(buf, "%d", &i);
    return i;
}

/* protected */ double
Plugin::undumpdouble() {
    DumpInfo *di = (DumpInfo *) dump_info;
    char buf[32];
    int p = 0, c;
    do {
	c = di->getchar();
	if (c == -1)
	    c = 0;
	buf[p++] = c;
    } while (p < 31 && c != 0);
    buf[p] = 0;
    double d = 0;
    sscanf(buf, "%lf", &d);
    return d;
}

/* protected */ char *
Plugin::undumpstring() {
    DumpInfo *di = (DumpInfo *) dump_info;
    char *buf = (char *) malloc(256);
    int size = 256;
    int length = 0;
    int c;
    do {
	c = di->getchar();
	if (c == -1)
	    c = 0;
	if (length == size) {
	    length += 256;
	    buf = (char *) realloc(buf, length);
	}
	buf[length++] = c;
    } while (c != 0);
    return buf;
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
