#include <X11/Xlib.h>

#include "Frame.h"

class ColorPicker;
class FWColor;
class FWPixmap;
class UndoManager;

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
	ColormapEditor(Listener *listener, FWPixmap *pm,
		       UndoManager *undomanager, Colormap colormap);
	virtual ~ColormapEditor();
	void colormapChanged(Colormap colormap);
	void colorsChanged(int startindex, int endindex);

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
