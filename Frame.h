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
	bool is_dialog;
	static bool decor_known;
	static int decor_width, decor_height;
	static bool taskbar_known;
	static int taskbar_width, taskbar_height;

    public:
	Frame(bool resizable, bool centered, bool hasMenuBar);
	Frame(Frame *parent, bool modal);
	virtual ~Frame();
	void raise();
	void hide();
	void setMenu(Menu *menu);
	void setTitle(const char *title);
	void setIconTitle(const char *title);
	const char *getTitle();
	virtual void close();

    protected:
	Frame(Frame *parent, bool modal,
	      bool resizable, bool centered, bool hasMenuBar);
	Widget getContainer();
	void setColormap(Colormap xcmap);
	void getSize(int *width, int *height);
	void setSize(int width, int height);
	void getDecorSize(int *width, int *height);
	void getTaskBarSize(int *width, int *height);
	void fitToScreen();

    private:
	void init(bool resizable, bool centered, bool hasMenuBar);
	void init(Frame *parent, bool modal);
	static void deleteWindow(Widget w, XtPointer ud, XtPointer cd);
	void findDecorSize();
	void findTaskBarSize();
	static void config(Widget w, XtPointer closure,
				XEvent *event, Boolean *cont);
};

#endif
