#ifndef IMAGEIO_PNM_H
#define IMAGEIO_PNM_H 1

#include "ImageIO.h"

class ImageIO_PNM : public ImageIO {
    public:
	ImageIO_PNM() {}
	virtual ~ImageIO_PNM() {}

	virtual const char *name() { return "PNM"; };
	virtual bool can_read(const char *filename);
	virtual bool read(const char *filename, char **plugin_name,
			  void **plugin_data, int *plugin_data_length,
			  FWPixmap *pm, char **message);
	virtual bool write(const char *filename, char *plugin_name,
			   const void *plugin_data, int plugin_data_length,
			   const FWPixmap *pm, char **message);
};

#endif
