#ifndef FRAME_H
#define FRAME_H 1

#include <X11/Intrinsic.h>

class Menu;

class Frame {
    private:
	Widget toplevel;
	Widget menubar;
	Widget container;
	Menu *menu;
	bool centered;
	static bool decor_known;
	static int decor_width, decor_height;

    public:
	Frame(bool resizable, bool centered, bool hasMenuBar);
	virtual ~Frame();
	void raise();
	void hide();
	void setMenu(Menu *menu);
	void setTitle(const char *title);
	void setIconTitle(const char *title);
	virtual void close();

    protected:
	Widget getContainer();
	void setColormap(Colormap xcmap);

    private:
	static void deleteWindow(Widget w, XtPointer ud, XtPointer cd);
	void findDecorSize();
	void fitToScreen();
	static void config(Widget w, XtPointer closure,
				XEvent *event, Boolean *cont);
};

#endif
