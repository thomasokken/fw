#include "Frame.h"

class ColorPicker : public Frame {
    public:
	class Listener {
	    public:
		Listener() {}
		virtual ~Listener() {}
		virtual void colorPicked(unsigned short r,
					 unsigned short g,
					 unsigned short b) = 0;
	};

    private:
	Listener *listener;
	Widget red, green, blue;

    public:
	ColorPicker(Listener *listener, unsigned short r, unsigned short g,
		    unsigned short b);
	virtual ~ColorPicker();

    private:
	static void ok(Widget w, XtPointer ud, XtPointer cd);
	static void cancel(Widget w, XtPointer ud, XtPointer cd);
};
