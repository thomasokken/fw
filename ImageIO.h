#ifndef IMAGEFILE_H
#define IMAGEFILE_H 1

class FWPixmap;
class Map;
class Iterator;

class ImageIO {
    private:
	static Map *map;

    public:
	ImageIO();
	virtual ~ImageIO();

	// Note: all data returned through pointer-to-pointer arguments
	// becomes the caller's responsibility and must be freed using
	// free().

	virtual const char *name() = 0;
	virtual bool can_read(const char *filename) = 0;
	virtual bool read(const char *filename, char **plugin_name,
			  void **plugin_data, int *plugin_data_length,
			  FWPixmap *pm, char **message) = 0;
	virtual bool write(const char *filename, const char *plugin_name,
			   const void *plugin_data, int plugin_data_length,
			   const FWPixmap *pm, char **message) = 0;

	static void regist(ImageIO *imgio);
	static Iterator *list();
	static bool sread(const char *filename, char **type,
			  char **plugin_name, void **plugin_data,
			  int *plugin_data_length, FWPixmap *pm,
			  char **message);
	static bool swrite(const char *filename, const char *type,
			   const char *plugin_name, const void *plugin_data,
			   int plugin_data_length, const FWPixmap *pm,
			   char **message);
};

#endif
