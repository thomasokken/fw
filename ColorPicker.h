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
	Widget red, green, blue;
	XImage *colorwheel_image;
	XImage *colorslider_image;
	XImage *oldandnew_image;
	Widget colorwheel;
	Widget colorslider;
	Widget oldandnew;

    public:
	ColorPicker(Listener *listener, unsigned char r, unsigned char g,
		    unsigned char b);
	virtual ~ColorPicker();

    private:
	static void ok(Widget w, XtPointer ud, XtPointer cd);
	static void cancel(Widget w, XtPointer ud, XtPointer cd);
};
