#ifndef IMAGEIO_JPEG_H
#define IMAGEIO_JPEG_H 1

#include "ImageIO.h"

class ImageIO_JPEG : public ImageIO {
    public:
	ImageIO_JPEG() {}
	virtual ~ImageIO_JPEG() {}

	virtual const char *name() { return "JPEG"; };
	virtual bool can_read(const char *filename);
	virtual bool read(const char *filename, char **plugin_name,
			  void **plugin_data, int *plugin_data_length,
			  FWPixmap *pm, char **message);
	virtual bool write(const char *filename, const char *plugin_name,
			   const void *plugin_data, int plugin_data_length,
			   const FWPixmap *pm, char **message);
};

#endif
