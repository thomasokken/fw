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
ImageIO::sread(const char *filename, char **plugin_name,
	      void **plugin_data, int *plugin_data_length,
	      FWPixmap *pm, char **message) {
    Iterator *iter = map->values();
    while (iter->hasNext()) {
	ImageIO *imgio = (ImageIO *) iter->next();
	if (imgio->can_read(filename)) {
	    delete iter;
	    return imgio->read(filename, plugin_name, plugin_data,
			       plugin_data_length, pm, message);
	}
    }
    delete iter;
    *message = strclone("Unsupported file type.");
    return false;
}
