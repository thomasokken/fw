#include "Frame.h"

class ColorPicker : public Frame {
    public:
	class Listener {
	    public:
		Listener() {}
		virtual ~Listener() {}
		virtual void colorPicked(unsigned char r,
					 unsigned char g,
					 unsigned char b) = 0;
	};

    private:
	Listener *listener;
	Widget bb;
	Widget red, green, blue;
	XImage *wheel_image;
	XImage *slider_image;
	XImage *oldnew_image;
	Widget wheel;
	Widget slider;
	Widget oldnew;
	bool private_colormap;
	bool private_colorcells;
	Colormap private_6x6x6_cube;
	unsigned long old_pixel;
	unsigned long new_pixel;
	unsigned char R, G, B, oldR, oldG, oldB;
	float H, S, L;
	char *rs, *gs, *bs;
	bool disable_rgbChanged;

    public:
	ColorPicker(Listener *listener, unsigned char r, unsigned char g,
		    unsigned char b);
	virtual ~ColorPicker();

    private:
	void rgbChanged();
	void repaintOldNewImage();
	void repaintWheelImage();
	void repaintSliderImage();
	static void ok(Widget w, XtPointer ud, XtPointer cd);
	static void cancel(Widget w, XtPointer ud, XtPointer cd);
	static void expose(Widget w, XtPointer ud, XtPointer cd);
	static void modifyVerify(Widget w, XtPointer ud, XtPointer cd);
	static void valueChanged(Widget w, XtPointer ud, XtPointer cd);
};
