#include <X11/Xlib.h>

#include "Frame.h"

class FWColor;
class FWPixmap;
class UndoManager;

class ColormapEditor : public Frame {
    public:
	class Owner {
	    public:
		Owner() {}
		virtual ~Owner() {}
		virtual void colormapChanged() = 0;
		virtual void loadColors() = 0;
		virtual void saveColors() = 0;
	};

    private:
	Owner *owner;
	FWPixmap *pm;
	FWColor *backup_cmap;
	UndoManager *undomgr;
	Widget drawingarea;
	Colormap colormap;
	bool sel_in_progress;
	int sel_start, sel_end;
	Dimension imagesize;
	XImage *image;

    public:
	ColormapEditor(Owner *owner, FWPixmap *pm);
	virtual ~ColormapEditor();
	void colormapChanged(Colormap colormap);

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
	friend class PickAction;
	friend class ChangeRangeAction;
	friend class BlendAction;
};
