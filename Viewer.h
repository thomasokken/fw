#ifndef VIEWER_H
#define VIEWER_H

#include <Xm/Xm.h>

#include "Frame.h"
class Color;
class Plugin;

class Viewer : private Frame {
    private:
	Plugin *plugin;
	Window drawwindow;
	XImage *image;
	Dimension width, height;
	static int instances;
	Menu *optionsmenu;

    public:
	Viewer(const char *pluginname);
	Viewer(const char *pluginname, const Viewer *src);
	Viewer(const char *pluginname, const char *filename);
	void finish_init();
	~Viewer();
	void deleteLater();
	void paint(const char *pixels, Color *cmap,
		   int depth, int width,
		   int height, int bytesperline,
		   int top, int left,
		   int bottom, int right);
	static bool openFile(const char *filename);

    private:
	void init(const char *pluginname, const Viewer *src,
		  const char *filename);
	static void resize(Widget w, XtPointer cd, XtPointer ud);
	void resize2();
	static void expose(Widget w, XtPointer cd, XtPointer ud);
	void expose2(int x, int y, int w, int h);
	virtual void close();
	static void menucallback(void *closure, const char *id);
	void menucallback2(const char *id);
	static void togglecallback(void *closure, const char *id, bool value);
	void togglecallback2(const char *id, bool value);

	static Boolean deleteLater2(XtPointer ud);
	static void doOpen2(Widget w, XtPointer cd, XtPointer ud);

	void doBeep();
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
	void doPrivateColormap();
	void doDither();
	void doNotify();
	void doEnlarge();
	void doReduce();
	void doScale(const char *scale);
	void doGeneral();
	void doHelp(const char *plugin);
};

#endif
