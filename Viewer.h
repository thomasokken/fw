#ifndef VIEWER_H
#define VIEWER_H 1

#include <Xm/Xm.h>

#include "Frame.h"
#include "FWPixmap.h"

class ColormapEditor;
class Plugin;
class SaveImageDialog;

class Viewer : private Frame {
    private:
	char *filename;
	char *filetype;
	bool is_brand_new;
	SaveImageDialog *savedialog;
	ColormapEditor *cme;
	Plugin *plugin;
	Widget drawingarea;
	XImage *image;
	Colormap priv_cmap;
	int scale;
	FWPixmap pm;
	bool dithering;
	bool direct_copy;
	bool selection_visible;
	bool selection_in_progress;
	int sel_x1, sel_y1, sel_x2, sel_y2;
	Menu *optionsmenu;
	Menu *scalemenu;
	Widget clipwindow;
	static int instances;
	static GC gc;
	static char *file_directory;
	static char *colormap_directory;

    public:
	Viewer(const char *pluginname);
	Viewer(const char *pluginname, void *plugin_data,
	       int plugin_data_length, FWPixmap *pm);
	void finish_init();
	~Viewer();
	void deleteLater();
	void paint(int top, int left, int bottom, int right);
	void colormapChanged();
	void setFile(const char *filename, const char *filetype);

    private:
	void init(const char *pluginname, void *plugin_data,
		  int plugin_data_length, FWPixmap *pm);
	virtual void close();
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
	static void doOpen2(const char *fn, void *cl);
	static void doSaveAs2(const char *fn, const char *type, void *cl);
	static void doSaveAsCancelled(void *cl);
	static void doLoadColors2(const char *fn, void *cl);
	void doLoadColors3(const char *fn);
	static void doSaveColors2(const char *fn, void *cl);
	void doSaveColors3(const char *fn);

	static void doBeep();
	void doNew(const char *plugin);
	void doOpen();
	void doClose();
	void doSave();
	void doSaveAs();
	void doGetInfo();
	void doPrint();
	void doQuit();
	void doUndo();
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
	void doScale(const char *scale);
	void doGeneral();
	void doHelp(const char *plugin);

	friend class CMEProxy;
};

#endif
