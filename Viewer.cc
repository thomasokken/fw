#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include <stdio.h>
#include <stdlib.h>

#include "Viewer.h"
#include "Menu.h"
#include "Plugin.h"
#include "main.h"
#include "util.h"

static XmString curr_path_name = NULL;

/* private static */ int
Viewer::instances = 0;

/* public */
Viewer::Viewer(const char *pluginname)
    : Frame(true, false, true) {
    init(pluginname, NULL, NULL);
}

/* public */
Viewer::Viewer(const char *pluginname, const Viewer *src)
    : Frame(true, false, true) {
    init(pluginname, src, NULL);
}

/* public */
Viewer::Viewer(const char *pluginname, const char *filename)
    : Frame(true, false, true) {
    init(pluginname, NULL, filename);
}

/* private */ void
Viewer::init(const char *pluginname, const Viewer *src, const char *filename) {
    instances++;
    image = NULL;
    plugin = Plugin::get(pluginname);
    if (plugin == NULL) {
	delete this;
	return;
    }
    plugin->setviewer(this);
    if (plugin != NULL) {
	if (src != NULL) {
	    if (plugin->init_clone(src->plugin))
		finish_init();
	    else
		delete this;
	} else if (filename != NULL) {
	    if (plugin->init_load(filename))
		finish_init();
	    else
		delete this;
	} else {
	    plugin->init_new();
	    // the plugin must call finish_init or delete the Viewer.
	    // If it deletes the viewer, it should do so using
	    // Viewer::deleteLater(), NOT using Viewer::~Viewer().
	}
    }
}

/* public */ void
Viewer::finish_init() {
    char title[256];
    snprintf(title, 256, "Fractal Wizard - %s", plugin->name());
    setTitle(title);
    setIconTitle(title);

    Menu *topmenu = new Menu;

    Menu *pluginmenu = new Menu;
    char **plugins = Plugin::list();
    if (plugins == NULL)
	pluginmenu->addCommand("No Plugins", NULL, NULL, "File.Beep");
    else {
	for (char **p = plugins; *p != NULL; p++) {
	    int len = strlen(*p);
	    if (len < 6 || strcmp(*p + (len - 6), "Viewer") != 0) {
		char id[100];
		snprintf(id, 100, "File.New.%s", *p);
		pluginmenu->addCommand(*p, NULL, NULL, id);
	    }
	}
    }
	
    Menu *filemenu = new Menu;
    filemenu->addMenu("New", NULL, NULL, "File.New", pluginmenu);
    filemenu->addCommand("Open...", NULL, NULL, "File.Open");
    filemenu->addSeparator();
    filemenu->addCommand("Close", NULL, NULL, "File.Close");
    filemenu->addCommand("Save", NULL, NULL, "File.Save");
    filemenu->addCommand("Save As...", NULL, NULL, "File.SaveAs");
    filemenu->addCommand("Get Info...", NULL, NULL, "File.GetInfo");
    filemenu->addSeparator();
    filemenu->addCommand("Print...", NULL, NULL, "File.Print");
    filemenu->addSeparator();
    filemenu->addCommand("Quit", "Q", "Ctrl+Q", "File.Quit");
    topmenu->addMenu("File", "F", NULL, "File", filemenu);

    Menu *editmenu = new Menu;
    editmenu->addCommand("Undo", NULL, NULL, "Edit.Undo");
    editmenu->addSeparator();
    editmenu->addCommand("Cut", NULL, NULL, "Edit.Cut");
    editmenu->addCommand("Copy", NULL, NULL, "Edit.Copy");
    editmenu->addCommand("Paste", NULL, NULL, "Edit.Paste");
    editmenu->addCommand("Clear", NULL, NULL, "Edit.Clear");
    topmenu->addMenu("Edit", NULL, NULL, "Edit", editmenu);

    Menu *colormenu = new Menu;
    colormenu->addCommand("Load Colors...", NULL, NULL, "Color.LoadColors");
    colormenu->addCommand("Save Colors...", NULL, NULL, "Color.SaveColors");
    colormenu->addSeparator();
    colormenu->addCommand("Edit Colors...", NULL, NULL, "Color.EditColors");
    topmenu->addMenu("Color", NULL, NULL, "Color", colormenu);

    Menu *drawmenu = new Menu;
    drawmenu->addCommand("Stop Others", NULL, NULL, "Draw.StopOthers");
    drawmenu->addCommand("Stop", NULL, NULL, "Draw.Stop");
    drawmenu->addCommand("Stop All", NULL, NULL, "Draw.StopAll");
    drawmenu->addCommand("Continue", NULL, NULL, "Draw.Continue");
    drawmenu->addCommand("Continue All", NULL, NULL, "Draw.ContinueAll");
    drawmenu->addSeparator();
    drawmenu->addCommand("Update Now", NULL, NULL, "Draw.UpdateNow");
    topmenu->addMenu("Draw", NULL, NULL, "Draw", drawmenu);

    //Options menu is a member var -- we need it to read the toggles
    //Menu *optionsmenu = new Menu;
    optionsmenu = new Menu;
    optionsmenu->addToggle("Private Colormap", NULL, NULL, "Options.PrivateColormap");
    optionsmenu->addToggle("Dither", NULL, NULL, "Options.Dither");
    optionsmenu->addSeparator();
    optionsmenu->addToggle("Notify When Ready", NULL, NULL, "Options.Notify");
    topmenu->addMenu("Options", NULL, NULL, "Options", optionsmenu);
    
    //Scale menu is a member var -- we need it to read the radios
    //Menu *scalemenu = new Menu;
    scalemenu = new Menu;
    scalemenu->addRadio(" 12%", NULL, NULL, "Windows.Scale@-8");
    scalemenu->addRadio(" 14%", NULL, NULL, "Windows.Scale@-7");
    scalemenu->addRadio(" 16%", NULL, NULL, "Windows.Scale@-6");
    scalemenu->addRadio(" 20%", NULL, NULL, "Windows.Scale@-5");
    scalemenu->addRadio(" 25%", NULL, NULL, "Windows.Scale@-4");
    scalemenu->addRadio(" 33%", NULL, NULL, "Windows.Scale@-3");
    scalemenu->addRadio(" 50%", NULL, NULL, "Windows.Scale@-2");
    scalemenu->addRadio("100%", NULL, NULL, "Windows.Scale@1");
    scalemenu->addRadio("200%", NULL, NULL, "Windows.Scale@2");
    scalemenu->addRadio("300%", NULL, NULL, "Windows.Scale@3");
    scalemenu->addRadio("400%", NULL, NULL, "Windows.Scale@4");
    scalemenu->addRadio("500%", NULL, NULL, "Windows.Scale@5");
    scalemenu->addRadio("600%", NULL, NULL, "Windows.Scale@6");
    scalemenu->addRadio("700%", NULL, NULL, "Windows.Scale@7");
    scalemenu->addRadio("800%", NULL, NULL, "Windows.Scale@8");

    Menu *windowsmenu = new Menu;
    windowsmenu->addCommand("Enlarge", NULL, NULL, "Windows.Enlarge");
    windowsmenu->addCommand("Reduce", NULL, NULL, "Windows.Reduce");
    windowsmenu->addMenu("Scale", NULL, NULL, "Scale", scalemenu);
    windowsmenu->addSeparator();
    topmenu->addMenu("Windows", NULL, NULL, "Windows", windowsmenu);

    Menu *helpmenu = new Menu;
    helpmenu->addCommand("About Fractal Wizard", NULL, NULL, "File.New.AboutViewer");
    helpmenu->addCommand("General", NULL, NULL, "Help.General");
    if (plugins != NULL) {
	helpmenu->addSeparator();
	for (char **p = plugins; *p != NULL; p++) {
	    int len = strlen(*p);
	    if (len < 6 || strcmp(*p + (len - 6), "Viewer") != 0) {
		char id[100];
		snprintf(id, 100, "Help.X.%s", *p);
		helpmenu->addCommand(*p, NULL, NULL, id);
	    }
	    free(*p);
	}
	free(plugins);
    }
    topmenu->addMenu("Help", NULL, NULL, "Help", helpmenu);

    topmenu->setCommandListener(menucallback, this);
    topmenu->setToggleListener(togglecallback, this);
    topmenu->setRadioListener(radiocallback, this);
    setMenu(topmenu);

    unsigned int W, H;
    plugin->getsize(&W, &H);
    width = W;
    height = H;

    Widget scroll = XtVaCreateManagedWidget("ScrolledWindow",
					    xmScrolledWindowWidgetClass,
					    getContainer(),
					    XmNscrollingPolicy, XmAUTOMATIC,
					    XmNscrollBarDisplayPolicy, XmSTATIC,
					    XmNtopAttachment, XmATTACH_FORM,
					    XmNtopOffset, 0,
					    XmNleftAttachment, XmATTACH_FORM,
					    XmNleftOffset, 0,
					    XmNrightAttachment, XmATTACH_FORM,
					    XmNrightOffset, 0,
					    XmNbottomAttachment, XmATTACH_FORM,
					    XmNbottomOffset, 0,
					    NULL);

    Widget draw = XtVaCreateManagedWidget("DrawingArea",
					  xmDrawingAreaWidgetClass,
					  scroll,
					  XmNwidth, width,
					  XmNheight, height,
					  NULL);

    Widget hsb, vsb;
    Dimension marginwidth, marginheight, spacing, borderWidth, shadowThickness;
    Arg args[7];
    XtSetArg(args[0], XmNhorizontalScrollBar, &hsb);
    XtSetArg(args[1], XmNverticalScrollBar, &vsb);
    XtSetArg(args[2], XmNscrolledWindowMarginWidth, &marginwidth);
    XtSetArg(args[3], XmNscrolledWindowMarginHeight, &marginheight);
    XtSetArg(args[4], XmNspacing, &spacing);
    XtSetArg(args[5], XmNborderWidth, &borderWidth);
    XtSetArg(args[6], XmNshadowThickness, &shadowThickness);
    XtGetValues(scroll, args, 7);

    Dimension hsbheight;
    XtSetArg(args[0], XmNheight, &hsbheight);
    XtGetValues(hsb, args, 1);

    Dimension vsbwidth;
    XtSetArg(args[0], XmNwidth, &vsbwidth);
    XtGetValues(vsb, args, 1);

    // FIXME Under Solaris + CDE, this code makes the window 2 pixels
    // too tall and too wide. (The original Motif XML GUI code that this
    // is based on, which has one less factor of 2 on shadowThickness
    // and adds a hard-coded 2 pixels to the total width and height, does
    // yield the right size on Solaris, but makes the window 4 pixels
    // too small on Linux + Lesstif.
    // Am I using the right shadowThickness? Or, consider this: isn't
    // it possible to avoid this ugliness of second-guessing the
    // XmScrolledWindow altogether, and set the size on the viewport
    // widget instead?
    Dimension sw = 2 * (marginwidth + borderWidth + 2 * shadowThickness)
         + vsbwidth + spacing + width;
    Dimension sh = 2 * (marginheight + borderWidth + 2 * shadowThickness)
         + hsbheight + spacing + height;
    XtSetArg(args[0], XmNwidth, sw);
    XtSetArg(args[1], XmNheight, sh);
    XtSetValues(scroll, args, 2);
    
    XtAddCallback(draw, XmNresizeCallback, resize, (XtPointer) this);
    XtAddCallback(draw, XmNexposeCallback, expose, (XtPointer) this);
    raise();
    drawwindow = XtWindow(draw);

    image = XCreateImage(display,
			 visual,
			 depth,
			 ZPixmap,
			 0,
			 NULL,
			 width,
			 height,
			 32,
			 0);
    image->data = (char *) malloc(image->bytes_per_line * height);

    plugin->run();
}

/* public */
Viewer::~Viewer() {
    if (image != NULL)
	XFree(image);
    if (plugin != NULL)
	Plugin::release(plugin);
    if (--instances == 0)
	exit(0);
}

/* public */ void
Viewer::deleteLater() {
    // This method is meant to be called in contexts where simply using the
    // destructor directly is not safe, specifically: a Plugin cannot use
    // ~Viewer() to delete its viewer because ~Viewer() will unload the plugin
    // if it is the only instance, and then execution would return from
    // ~Viewer() to a shared library that is no longer present => crash!
    // By deferring the destructor invocation until the next pass through the
    // main event loop, we make sure that the plugin is no longer on the stack
    // by the time its viewer is deleted.

    // It may take a while before deletion actually happens; make sure the user
    // can't mess things up in the meantime.
    //hide();

    XtAppAddWorkProc(appcontext, deleteLater2, (XtPointer) this);
}

/* private static */ Boolean
Viewer::deleteLater2(XtPointer ud) {
    Viewer *This = (Viewer *) ud;
    delete This;
    return True;
}

/* public */ void
Viewer::paint(const char *pixels, Color *cmap,
	      int depth, int width,
	      int height, int bytesperline,
	      int top, int left,
	      int bottom, int right) {
    // FIXME do something to take advantage of those cases where it is not
    // even necessary to copy pixels to an XImage, i.e. the cases where
    // depth = 1, or (depth = 8 and idepth = 8 and priv), or
    // (depth = 24 and idepth =24)... May not be practical, but it's a
    // thought... Consider stuff like Life, Kaos, or that totally cool
    // Xlock mode (ifs?) running full blast inside FW...
    if (depth == 1) {
	// Black and white are always available, so we never have to
	// do anything fancy to render 1-bit images, so we use this simple
	// special-case code that does not do the error diffusion thing.
	unsigned long black = BlackPixel(display, screennumber);
	unsigned long white = WhitePixel(display, screennumber);
	for (int y = top; y < bottom; y++)
	    for (int x = left; x < right; x++)
		XPutPixel(image, x, y,
			(pixels[y * bytesperline + (x >> 3)] & 1 << (x & 7))
			    == 0 ? black : white);
    } else if (grayramp == NULL) {
	// FIXME handle special cases that do not require dithering:
	// depth = 24, and also depth = 8 && idepth = 8 && using priv cmap
	int *dr = new int[right - left];
	int *dg = new int[right - left];
	int *db = new int[right - left];
	int *nextdr = new int[right - left];
	int *nextdg = new int[right - left];
	int *nextdb = new int[right - left];
	for (int i = 0; i < right - left; i++)
	    dr[i] = dg[i] = db[i] = nextdr[i] = nextdg[i] = nextdb[i] = 0;
	int dR = 0, dG = 0, dB = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((y & 1) << 1) - 1;
	    int start, end;
	    if (dir == 1) {
		start = left;
		end = right;
	    } else {
		start = right - 1;
		end = left - 1;
	    }
	    int *temp;
	    temp = nextdr; nextdr = dr; dr = temp;
	    temp = nextdg; nextdg = dg; dg = temp;
	    temp = nextdb; nextdb = db; db = temp;
	    dR = dG = dB = 0;
	    for (int x = start; x != end; x += dir) {
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1] & 255;
		    g = pixels[y * bytesperline + (x << 2) + 2] & 255;
		    b = pixels[y * bytesperline + (x << 2) + 3] & 255;
		}
		r += (dr[x - left] + dR) >> 4;
		if (r < 0) r = 0; else if (r > 255) r = 255;
		dr[x - left] = 0;
		g += (dg[x - left] + dG) >> 4;
		if (g < 0) g = 0; else if (g > 255) g = 255;
		dg[x - left] = 0;
		b += (db[x - left] + dB) >> 4;
		if (b < 0) b = 0; else if (b > 255) b = 255;
		db[x - left] = 0;
		unsigned long pixel;
		if (colorcube != NULL) {
		    int index = ((int) (r * (cubesize - 1) / 255.0 + 0.5)
			    * cubesize
			    + (int) (g * (cubesize - 1) / 255.0 + 0.5))
			    * cubesize
			    + (int) (b * (cubesize - 1) / 255.0 + 0.5);
		    dR = r - (colorcube[index].red >> 8);
		    dG = g - (colorcube[index].green >> 8);
		    dB = b - (colorcube[index].blue >> 8);
		    pixel = colorcube[index].pixel;
		} else {
		    static bool inited = false;
		    static int rmax, rmult, bmax, bmult,
							gmax, gmult;
		    if (!inited) {
			rmax = visual->red_mask;
			rmult = 1;
			while ((rmax & 1) == 0) {
			    rmax >>= 1;
			    rmult <<= 1;
			}
			gmax = visual->green_mask;
			gmult = 1;
			while ((gmax & 1) == 0) {
			    gmax >>= 1;
			    gmult <<= 1;
			}
			bmax = visual->blue_mask;
			bmult = 1;
			while ((bmax & 1) == 0) {
			    bmax >>= 1;
			    bmult <<= 1;
			}
			inited = true;
		    }

		    pixel = (r * rmax / 255) * rmult
			    + (g * gmax / 255) * gmult
			    + (b * bmax / 255) * bmult;
		    dR = r - (int) (r * rmax / 255.0 + 0.5) * 255 / rmax;
		    dG = g - (int) (g * gmax / 255.0 + 0.5) * 255 / gmax;
		    dB = b - (int) (b * bmax / 255.0 + 0.5) * 255 / bmax;
		}
		XPutPixel(image, x, y, pixel);

		int prevx = x - dir;
		int nextx = x + dir;
		if (prevx >= (int) left && prevx < (int) right) {
		    nextdr[prevx - left] += dR * 3;
		    nextdg[prevx - left] += dG * 3;
		    nextdb[prevx - left] += dB * 3;
		}
		nextdr[x - left] += dR * 5;
		nextdg[x - left] += dG * 5;
		nextdb[x - left] += dB * 5;
		if (nextx >= (int) left && nextx < (int) right) {
		    nextdr[nextx - left] += dR;
		    nextdg[nextx - left] += dG;
		    nextdb[nextx - left] += dB;
		}
		dR *= 7;
		dG *= 7;
		dB *= 7;
	    }
	}
	delete[] dr;
	delete[] dg;
	delete[] db;
	delete[] nextdr;
	delete[] nextdg;
	delete[] nextdb;
    } else {
	// FIXME handle special cases that do not require dithering:
	// depth = 8 && idepth = 8 && using priv cmap
	int *dk = new int[right - left];
	int *nextdk = new int[right - left];
	for (int i = 0; i < right - left; i++)
	    dk[i] = nextdk[i] = 0;
	int dK = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((y & 1) << 1) - 1;
	    int start, end;
	    if (dir == 1) {
		start = left;
		end = right;
	    } else {
		start = right - 1;
		end = left - 1;
	    }
	    int *temp;
	    temp = nextdk; nextdk = dk; dk = temp;
	    dK = 0;
	    for (int x = start; x != end; x += dir) {
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1] & 255;
		    g = pixels[y * bytesperline + (x << 2) + 2] & 255;
		    b = pixels[y * bytesperline + (x << 2) + 3] & 255;
		}
		int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		k += (dk[x - left] + dK) >> 4;
		if (k < 0) k = 0; else if (k > 255) k = 255;
		dk[x - left] = 0;
		int graylevel = 
		    (int) (k * (rampsize - 1) / 255.0 + 0.5);
		if (graylevel >= rampsize)
		    graylevel = rampsize - 1;
		dK = k - (grayramp[graylevel].red >> 8);
		unsigned long pixel = grayramp[graylevel].pixel;
		XPutPixel(image, x, y, pixel);

		int prevx = x - dir;
		int nextx = x + dir;
		if (prevx >= (int) left && prevx < (int) right)
		    nextdk[prevx - left] += dK * 3;
		nextdk[x - left] += dK * 5;
		if (nextx >= (int) left && nextx < (int) right)
		    nextdk[nextx - left] += dK;
		dK *= 7;
	    }
	}
	delete[] dk;
	delete[] nextdk;
    }
    XPutImage(display, drawwindow, gc, image,
	      left, top, left, top,
	      right - left, bottom - top);
}

/* private static */ void
Viewer::resize(Widget w, XtPointer ud, XtPointer cd) {
    ((Viewer *) ud)->resize2();
}

/* private */ void
Viewer::resize2() {
    /*
    Arg args[2];
    XtSetArg(args[0], XmNwidth, &width);
    XtSetArg(args[1], XmNheight, &height);
    XtGetValues(w, args, 2);
    */
}

/* private static */ void
Viewer::expose(Widget w, XtPointer ud, XtPointer cd) {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    XExposeEvent ev = cbs->event->xexpose;
    ((Viewer *) ud)->expose2(ev.x, ev.y, ev.width, ev.height);
}

/* private */ void
Viewer::expose2(int x, int y, int w, int h) {
    XPutImage(display, drawwindow, gc, image, x, y, x, y, w, h);
}

/* private virtual */ void
Viewer::close() {
    delete this;
}

/* private static */ void
Viewer::menucallback(void *closure, const char *id) {
    ((Viewer *) closure)->menucallback2(id);
}

/* private */ void
Viewer::menucallback2(const char *id) {
    if (strcmp(id, "File.Beep") == 0)
	doBeep();
    if (strncmp(id, "File.New.", 9) == 0)
	doNew(id + 9);
    else if (strcmp(id, "File.Open") == 0)
	doOpen();
    else if (strcmp(id, "File.Close") == 0)
	doClose();
    else if (strcmp(id, "File.Save") == 0)
	doSave();
    else if (strcmp(id, "File.SaveAs") == 0)
	doSaveAs();
    else if (strcmp(id, "File.GetInfo") == 0)
	doGetInfo();
    else if (strcmp(id, "File.Print") == 0)
	doPrint();
    else if (strcmp(id, "File.Quit") == 0)
	doQuit();
    else if (strcmp(id, "Edit.Undo") == 0)
	doUndo();
    else if (strcmp(id, "Edit.Cut") == 0)
	doCut();
    else if (strcmp(id, "Edit.Copy") == 0)
	doCopy();
    else if (strcmp(id, "Edit.Paste") == 0)
	doPaste();
    else if (strcmp(id, "Edit.Clear") == 0)
	doClear();
    else if (strcmp(id, "Color.LoadColors") == 0)
	doLoadColors();
    else if (strcmp(id, "Color.SaveColors") == 0)
	doSaveColors();
    else if (strcmp(id, "Color.EditColors") == 0)
	doEditColors();
    else if (strcmp(id, "Draw.StopOthers") == 0)
	doStopOthers();
    else if (strcmp(id, "Draw.Stop") == 0)
	doStop();
    else if (strcmp(id, "Draw.StopAll") == 0)
	doStopAll();
    else if (strcmp(id, "Draw.Continue") == 0)
	doContinue();
    else if (strcmp(id, "Draw.ContinueAll") == 0)
	doContinueAll();
    else if (strcmp(id, "Draw.UpdateNow") == 0)
	doUpdateNow();
    else if (strcmp(id, "Options.PrivateColormap") == 0)
	doPrivateColormap();
    else if (strcmp(id, "Options.Dither") == 0)
	doDither();
    else if (strcmp(id, "Options.Notify") == 0)
	doNotify();
    else if (strcmp(id, "Windows.Enlarge") == 0)
	doEnlarge();
    else if (strcmp(id, "Windows.Reduce") == 0)
	doReduce();
    else if (strncmp(id, "Windows.Scale.", 14) == 0)
	doScale(id + 14);
    else if (strcmp(id, "Help.General") == 0)
	doGeneral();
    else if (strncmp(id, "Help.X.", 7) == 0)
	doHelp(id + 7);
}

/* private static */ void
Viewer::togglecallback(void *closure, const char *id, bool value) {
    ((Viewer *) closure)->togglecallback2(id, value);
}

/* private */ void
Viewer::togglecallback2(const char *id, bool value) {
    fprintf(stderr, "Toggle \"%s\" set to '%s'.\n", id, value ? "true" : "false");
}

/* private static */ void
Viewer::radiocallback(void *closure, const char *id, const char *value) {
    ((Viewer *) closure)->radiocallback2(id, value);
}

/* private */ void
Viewer::radiocallback2(const char *id, const char *value) {
    fprintf(stderr, "Radio \"%s\" set to '%s'.\n", id, value);
}

/* private */ void
Viewer::doBeep() {
    XBell(display, 100);
}

/* private */ void
Viewer::doNew(const char *plugin) {
    new Viewer(plugin);
}

/* private */ void
Viewer::doOpen() {
    Arg arg;
    XtSetArg(arg, XmNdirectory, curr_path_name);
    Widget fsb = XmCreateFileSelectionDialog(getContainer(), "Open", &arg, 1);
    XtAddCallback(fsb, XmNokCallback, doOpen2, (XtPointer) this);
    XtAddCallback(fsb, XmNcancelCallback, doOpen2, (XtPointer) this);
    XtManageChild(fsb);
}

/* private static */ void
Viewer::doOpen2(Widget w, XtPointer ud, XtPointer cd) {
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) cd;
    if (cbs->reason == XmCR_OK) {
	if (curr_path_name != NULL)
	    XmStringFree(curr_path_name);
	Arg arg;
	XtSetArg(arg, XmNdirectory, &curr_path_name);
	XtGetValues(w, &arg, 1);
	char *filename;
	if (XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename)) {
	    if (!Viewer::openFile(filename))
		XBell(display, 100);
	    XtFree(filename);
	}
    }
    XtUnmanageChild(w);
}

/* public static */ bool
Viewer::openFile(const char *filename) {
    if (verbosity >= 1)
	fprintf(stderr, "Opening \"%s\"...\n", filename);
    char **names = Plugin::list();
    if (names != NULL) {
	int suitability = 0;
	char *pluginname;
	Plugin *plugin;
	for (char **n = names; *n != NULL; n++) {
	    Plugin *p= Plugin::get(*n);
	    if (p == NULL) {
		free(*n);
		continue;
	    }
	    int s = p->can_open(filename);
	    if (s > suitability) {
		if (suitability > 0) {
		    free(pluginname);
		    Plugin::release(plugin);
		}
		pluginname = *n;
		plugin = p;
		suitability = s;
	    } else {
		free(*n);
		Plugin::release(p);
	    }
	}
	free(names);
	if (suitability > 0) {
	    // TODO: shouldn't use a constructor here!
	    // What if opening the file fails?!?
	    new Viewer(pluginname, filename);
	    free(pluginname);
	    Plugin::release(plugin);
	    // NOTE: we only release the plugin *after* instantiating
	    // a Viewer, because we want to avoid loading the .so more
	    // often than necessary. If the instance openend for the
	    // can_open() call was the only one, and we'd close it
	    // between the can_open() call and the Viewer::Viewer()
	    // call, we'd be loading the shared library twice.
	    return true;
	} else
	    // No matching plugin found
	    return false;
    } else
	// No plugins found at all
	return false;
}

/* private */ void
Viewer::doClose() {
    close();
}

/* private */ void
Viewer::doSave() {
    doBeep();
}

/* private */ void
Viewer::doSaveAs() {
    doBeep();
}

/* private */ void
Viewer::doGetInfo() {
    doBeep();
}

/* private */ void
Viewer::doPrint() {
    doBeep();
}

/* private */ void
Viewer::doQuit() {
    // FIXME
    exit(0);
}

/* private */ void
Viewer::doUndo() {
    doBeep();
}

/* private */ void
Viewer::doCut() {
    doBeep();
}

/* private */ void
Viewer::doCopy() {
    doBeep();
}

/* private */ void
Viewer::doPaste() {
    doBeep();
}

/* private */ void
Viewer::doClear() {
    doBeep();
}

/* private */ void
Viewer::doLoadColors() {
    doBeep();
}

/* private */ void
Viewer::doSaveColors() {
    doBeep();
}

/* private */ void
Viewer::doEditColors() {
    doBeep();
}

/* private */ void
Viewer::doStopOthers() {
    doBeep();
}

/* private */ void
Viewer::doStop() {
    plugin->stop();
}

/* private */ void
Viewer::doStopAll() {
    doBeep();
}

/* private */ void
Viewer::doContinue() {
    plugin->restart();
}

/* private */ void
Viewer::doContinueAll() {
    doBeep();
}

/* private */ void
Viewer::doUpdateNow() {
    plugin->paint();
}

/* private */ void
Viewer::doPrivateColormap() {
    doBeep();
}

/* private */ void
Viewer::doDither() {
    doBeep();
}

/* private */ void
Viewer::doNotify() {
    doBeep();
}

/* private */ void
Viewer::doEnlarge() {
    doBeep();
}

/* private */ void
Viewer::doReduce() {
    doBeep();
}

/* private */ void
Viewer::doScale(const char *scale) {
    doBeep();
}

/* private */ void
Viewer::doGeneral() {
    doBeep();
}

/* private */ void
Viewer::doHelp(const char *id) {
    doBeep();
}
