///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

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
    ImageIO *imgio = (ImageIO *) map->get(type);
    if (imgio == NULL) {
	*message = strclone("Unsupported file type.");
	return false;
    }
    return imgio->write(filename, plugin_name, plugin_data,
			plugin_data_length, pm, message);
}
