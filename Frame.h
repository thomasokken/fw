///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

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
	bool is_modal;
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
	Window getWindow();

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
