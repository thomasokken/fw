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

    public:
	Viewer(const char *pluginname);
	Viewer(const char *pluginname, const Viewer *src);
	Viewer(const char *pluginname, const char *filename);
	void finish_init();
	~Viewer();
	void paint(const char *pixels, Color *cmap,
		   int depth, int width,
		   int height, int bytesperline,
		   int top, int left,
		   int bottom, int right);

    private:
	void init(const char *pluginname, const Viewer *src,
		  const char *filename);
	static void resize(Widget w, XtPointer cd, XtPointer ud);
	void resize2();
	static void expose(Widget w, XtPointer cd, XtPointer ud);
	void expose2();
	virtual void close();
	static void menucallback(void *closure, const char *id);
	void menucallback2(const char *id);

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
