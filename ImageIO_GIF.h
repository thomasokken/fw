#ifndef IMAGEIO_GIF_H
#define IMAGEIO_GIF_H 1

#include "ImageIO.h"

class ImageIO_GIF : public ImageIO {
    public:
	ImageIO_GIF() {}
	virtual ~ImageIO_GIF() {}

	virtual const char *name() { return "GIF"; };
	virtual bool can_read(const char *filename);
	virtual bool read(const char *filename, char **plugin_name,
			  void **plugin_data, int *plugin_data_length,
			  FWPixmap *pm, char **message);
	virtual bool write(const char *filename, char *plugin_name,
			   const void *plugin_data, int plugin_data_length,
			   const FWPixmap *pm, char **message);
};

#endif
