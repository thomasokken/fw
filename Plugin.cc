#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include "Plugin.h"
#include "SettingsDialog.h"
#include "Viewer.h"
#include "main.h"

/* private static */ Plugin::ProdNode *
Plugin::prodlist = NULL;

/* private static */ XtWorkProcId
Plugin::workproc_id;

/* protected */
Plugin::Plugin(void *dl) {
    this->dl = dl;
    pixels = NULL;
    cmap = NULL;
    settings = NULL;
    settings_layout = NULL;
}

/* protected virtual */
Plugin::~Plugin() {
    stop_prodding();
    if (pixels != NULL)
	free(pixels);
    if (cmap != NULL)
	delete[] cmap;
}

/* public static */ Plugin *
Plugin::get(const char *name) {
    char dlname[_POSIX_PATH_MAX];
    snprintf(dlname, _POSIX_PATH_MAX, "%s/.fw/%s.so", getenv("HOME"), name);
    void *dl = dlopen(dlname, RTLD_NOW);
    if (dl == NULL)
	return NULL;
    Plugin *(*factory)(void *) = (Plugin *(*)(void *)) dlsym(dl, "factory");
    return (*factory)(dl);
}

/* public static */ void
Plugin::release(Plugin *plugin) {
    void *dl = plugin->dl;
    delete plugin;
    dlclose(dl);
}

/* public */ void
Plugin::getsize(unsigned int *width, unsigned int *height) {
    *width = this->width;
    *height = this->height;
}

/* public virtual */ void
Plugin::get_settings() {
    new SettingsDialog(settings, settings_layout, this);
}

/* public virtual */ void
Plugin::get_settings_ok() {
    viewer->finish_init();
}

/* public virtual */ void
Plugin::get_settings_cancel() {
    viewer->deleteLater();
}

/* protected static */ void
Plugin::beep() {
    XBell(display, 100);
}

/* public */ void
Plugin::paint() {
    paint(0, 0, height, width);
}

/* protected */ void
Plugin::paint(unsigned int top, unsigned int left,
	      unsigned int bottom, unsigned int right) {
    viewer->paint(pixels, cmap, depth, width, height, bytesperline,
		  top, left, bottom, right);
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
	    workproc_id = XtAppAddWorkProc(appcontext, workproc, NULL);
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

/* private static */ Boolean
Plugin::workproc(XtPointer ud) {
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

/* private */ void
Plugin::make_graymap() {
    cmap = new Color[256];
    for (int i = 0; i < 256; i++)
	cmap[i].r = cmap[i].g = cmap[i].b = i;
}
