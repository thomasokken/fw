#ifndef VIEWER_H
#define VIEWER_H 1

#include <Xm/Xm.h>

#include "Frame.h"
#include "FWPixmap.h"

class ColormapEditorListener;
class ColormapEditor;
class List;
class Plugin;
class UndoManager;

class Viewer : private Frame {
    private:
	int id;
	char *filename;
	char *filetype;
	bool is_brand_new;
	bool finished;
	int untitled;
	bool dirty;
	UndoManager *undomanager;
	int saved_undo_id;
	ColormapEditor *cme;
	ColormapEditorListener *cmelistener;
	Plugin *plugin;
	Widget drawingarea;
	XImage *image;
	Colormap colormap;
	int scale;
	FWPixmap pm;
	bool dithering;
	bool direct_copy;
	bool selection_visible;
	bool selection_in_progress;
	int sel_x1, sel_y1, sel_x2, sel_y2;
	Menu *editmenu;
	Menu *optionsmenu;
	Menu *windowsmenu;
	Menu *scalemenu;
	Widget clipwindow;
	static List *instances;
	static int idcount;
	static int untitledcount;
	static GC gc;
	static char *file_directory;
	static char *colormap_directory;
	static bool inner_decor_known;
	static int inner_decor_width, inner_decor_height;
	static bool disable_new_and_open;

    public:
	Viewer(const char *pluginname);
	Viewer(Plugin *clonee);
	Viewer(const char *pluginname, void *plugin_data,
	       int plugin_data_length, FWPixmap *pm,
	       const char *filename, const char *filetype);
	void finish_init();
	~Viewer();
	void deleteLater();
	void paint(int top, int left, int bottom, int right);
	void get_recommended_size(int *width, int *height);
	void get_screen_size(int *width, int *height);
	void get_selection(int *x, int *y, int *width, int *height);
	int get_scale();
	void colormapChanged();
	void setDirty();
	void pluginFinished(bool notify);

    private:
	void init(const char *pluginname, Plugin *clonee, void *plugin_data,
		  int plugin_data_length, FWPixmap *pm);
	virtual void close();
	static void addViewer(Viewer *viewer);
	static void removeViewer(Viewer *viewer);
	static Viewer *getTopmostViewer();
	bool canIDoDirectCopy();
	void draw_selection();
	void erase_selection();
	void screen2pixmap(int *x, int *y);
	void pixmap2screen(int *x, int *y);

	static void resize(Widget w, XtPointer cd, XtPointer ud);
	void resize2();
	static void expose(Widget w, XtPointer cd, XtPointer ud);
	void expose2(int x, int y, int w, int h);
	static void input(Widget w, XtPointer cd, XtPointer ud);
	void input2(XEvent *event);
	static void menucallback(void *cl, const char *id);
	void menucallback2(const char *id);
	static void togglecallback(void *cl, const char *id, bool value);
	void togglecallback2(const char *id, bool value);
	static void radiocallback(void *cl, const char *id, const char *value);
	void radiocallback2(const char *id, const char *value);

	static Boolean deleteLater2(XtPointer ud);
	bool save(const char *filename, const char *filetype);
	static void doQuit2();

	void doNew(const char *plugin);
	void doClone();
	void doOpen();
	void doClose();
	void doSave();
	void doSaveAs();
	void doGetInfo();
	void doPrint();
	void doQuit();
	void doUndo();
	void doRedo();
	void doCut();
	void doCopy();
	void doPaste();
	void doClear();
	void doLoadColors();
	void doSaveColors();
	void doEditColors();
	void doStopOthers();
	void doStop();
	void doStopAll();
	void doContinue();
	void doContinueAll();
	void doUpdateNow();
	void doPrivateColormap(bool value);
	void doDither(bool value);
	void doNotify(bool value);
	void doEnlarge();
	void doReduce();
	void doWindows(const char *sid);
	void doScale(const char *scale);
	void doGeneral();
	void doHelp(const char *plugin);

	friend class ColormapEditorListener;
	friend class UndoManagerListener;
	friend class OpenListener;
	friend class LoadColorsListener;
	friend class SaveColorsListener;
	friend class YesNoCancelBeforeClosingListener;
	friend class YesNoCancelBeforeQuittingListener;
	friend class SaveAsListener;
	friend class SaveBeforeClosingListener;
	friend class SaveBeforeQuittingListener;
};

#endif
