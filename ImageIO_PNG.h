#ifndef IMAGEIO_PNG_H
#define IMAGEIO_PNG_H 1

#include "ImageIO.h"

class ImageIO_PNG : public ImageIO {
    public:
	ImageIO_PNG() {}
	virtual ~ImageIO_PNG() {}

	virtual const char *name() { return "PNG"; };
	virtual bool can_read(const char *filename);
	virtual bool read(const char *filename, char **plugin_name,
			  void **plugin_data, int *plugin_data_length,
			  FWPixmap *pm, char **message);
	virtual bool write(const char *filename, const char *plugin_name,
			   const void *plugin_data, int plugin_data_length,
			   const FWPixmap *pm, char **message);
};

#endif
