#ifndef COLORPICKER_H
#define COLORPICKER_H 1

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
	bool oldnew_image_initialized;
	Colormap xcube;
	unsigned long old_pixel;
	unsigned long new_pixel;
	unsigned char R, G, B, oldR, oldG, oldB;
	float H, S, L;
	char *rs, *gs, *bs;
	int cross_x, cross_y, slider_x;
	bool disable_rgbChanged;
	static GC gc;
	static int instances;

    public:
	ColorPicker(Frame *parent, Listener *listener, unsigned char r,
		    unsigned char g, unsigned char b, bool allow_private_cmap);
	virtual ~ColorPicker();

    private:
	void updateRGBTextFields(unsigned char r, unsigned char g, unsigned char b);
	void rgbChanged();
	void repaintOldNewImage();
	void repaintWheelImage();
	void repaintSliderImage();
	void drawCross();
	void removeCross();
	void drawThumb();
	void removeThumb();
	void mouseInOldNew(XEvent *event);
	void mouseInWheel(XEvent *event);
	void mouseInSlider(XEvent *event);
	static void ok(Widget w, XtPointer ud, XtPointer cd);
	static void cancel(Widget w, XtPointer ud, XtPointer cd);
	static void expose(Widget w, XtPointer ud, XtPointer cd);
	static void input(Widget w, XtPointer ud, XtPointer cd);
	static void modifyVerify(Widget w, XtPointer ud, XtPointer cd);
	static void valueChanged(Widget w, XtPointer ud, XtPointer cd);
};

#endif
