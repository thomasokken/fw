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

#ifndef COLORMAPEDITOR_H
#define COLORMAPEDITOR_H 1

#include <X11/Xlib.h>

#include "Frame.h"

class ColorPicker;
class FWColor;
class FWPixmap;
class UndoManager;
class UndoableAction;

class ColormapEditor : public Frame {
    public:
	class Listener {
	    public:
		Listener() {}
		virtual ~Listener() {}
		virtual void cmeClosed() = 0;
		virtual void colormapChanged() = 0;
		virtual void loadColors() = 0;
		virtual void saveColors() = 0;
		virtual ColormapEditor *getCME() = 0;
	};

    private:
	Listener *listener;
	FWPixmap *pm;
	FWColor *initial_cmap;
	int initial_undo_id;
	UndoManager *undomgr;
	Widget drawingarea;
	Colormap colormap;
	bool sel_in_progress;
	int sel_start, sel_end;
	Dimension imagesize;
	XImage *image;
	ColorPicker *colorpicker;
	static FWColor *color_clipboard;
	static int color_clipboard_size;

    public:
	ColormapEditor(Frame *parent, Listener *listener, FWPixmap *pm,
		       UndoManager *undomanager, Colormap colormap);
	virtual ~ColormapEditor();
	void colormapChanged(Colormap colormap);
	void colorsChanged(int startindex, int endindex);
	void selectColor(int c);
	void extendSelection(int c);
	void finishSelection();

    private:
	void doPick();
	void doBlend();
	void doSwap();
	void doMix();
	void doCopy();
	void doPaste();
	void doLoad();
	void doSave();
	void doUndo();
	void doRedo();
	void doOK();
	void doCancel();

	void update_image();
	void update_image_gridlines();
	void select_cell(int c);
	void update_cell(int c);
	void redraw_cells(int begin, int end);

	void addButton(Widget parent, const char *label, int id);

	static void expose(Widget w, XtPointer ud, XtPointer cd);
	static void input(Widget w, XtPointer ud, XtPointer cd);
	void input2(XEvent *event);
	static void activate(Widget w, XtPointer ud, XtPointer cd);

	friend class CPListener;
};

#endif
