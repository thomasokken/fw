#include <stdio.h>

#include "ImageIO.h"
#include "util.h"

/* private static */ Map *
ImageIO::map = new Map;

/* public */
ImageIO::ImageIO() {
    //
}

/* public virtual */
ImageIO::~ImageIO() {
    //
}

/* public static */ void
ImageIO::regist(ImageIO *imgio) {
    map->put(imgio->name(), imgio);
}

/* public static */ Iterator *
ImageIO::list() {
    return map->keys();
}

/* public static */ bool
ImageIO::sread(const char *filename, char **type,
	       char **plugin_name, void **plugin_data,
	       int *plugin_data_length, FWPixmap *pm,
	       char **message) {
    Iterator *iter = map->values();
    while (iter->hasNext()) {
	ImageIO *imgio = (ImageIO *) iter->next();
	if (imgio->can_read(filename)) {
	    delete iter;
	    bool result = imgio->read(filename, plugin_name, plugin_data,
				      plugin_data_length, pm, message);
	    if (result)
		*type = strclone(imgio->name());
	    return result;
	}
    }
    delete iter;
    *message = strclone("Unsupported file type.");
    return false;
}

/* public static */ bool
ImageIO::swrite(const char *filename, const char *type,
		const char *plugin_name, const void *plugin_data,
		int plugin_data_length, const FWPixmap *pm,
		char **message) {
    fprintf(stderr, "Trying to write \"%s\" (type %s)...\n", filename, type);
    ImageIO *imgio = (ImageIO *) map->get(type);
    if (imgio == NULL) {
	*message = strclone("Unsupported file type.");
	return false;
    }
    return imgio->write(filename, plugin_name, plugin_data,
			plugin_data_length, pm, message);
}
