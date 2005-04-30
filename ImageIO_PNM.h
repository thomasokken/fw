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

#ifndef IMAGEIO_PNM_H
#define IMAGEIO_PNM_H 1

#include "ImageIO.h"

class ImageIO_PNM : public ImageIO {
    public:
	ImageIO_PNM() {}
	virtual ~ImageIO_PNM() {}

	virtual const char *name() { return "PBM/PGM/PPM"; };
	virtual bool can_read(const char *filename);
	virtual bool read(const char *filename, char **plugin_name,
			  void **plugin_data, int *plugin_data_length,
			  FWPixmap *pm, char **message);
	virtual bool write(const char *filename, const char *plugin_name,
			   const void *plugin_data, int plugin_data_length,
			   const FWPixmap *pm, char **message);
};

#endif
