#include <X11/Xlib.h>

class FWPixmap;

class CopyBits {
    public:
	static void copy_unscaled(FWPixmap *pm, XImage *image,
				  bool priv_cmap, bool no_grays, bool dither,
				  int top, int left, int bottom, int right);
	static void copy_enlarged(int factor,
				  FWPixmap *pm, XImage *image,
				  bool priv_cmap, bool no_grays, bool dither,
				  int top, int left, int bottom, int right);
	static void copy_reduced(int factor,
				 FWPixmap *pm, XImage *image,
				 bool priv_cmap, bool no_grays, bool dither,
				 int top, int left, int bottom, int right);
};
