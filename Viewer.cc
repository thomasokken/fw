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
    priv_cmap = None;
    pixels = NULL;
    cmap = NULL;

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
    drawmenu->addCommand("Update Now", NULL, "Ctrl+U", "Draw.UpdateNow");
    topmenu->addMenu("Draw", NULL, NULL, "Draw", drawmenu);

    optionsmenu = new Menu;
    optionsmenu->addToggle("Private Colormap", NULL, "Ctrl+M", "Options.PrivateColormap");
    optionsmenu->addToggle("Dither", NULL, NULL, "Options.Dither");
    optionsmenu->addSeparator();
    optionsmenu->addToggle("Notify When Ready", NULL, NULL, "Options.Notify");
    topmenu->addMenu("Options", NULL, NULL, "Options", optionsmenu);
    
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
    windowsmenu->addCommand("Enlarge", NULL, "Ctrl+0", "Windows.Enlarge");
    windowsmenu->addCommand("Reduce", NULL, "Ctrl+9", "Windows.Reduce");
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

    clipwindow = XtNameToWidget(scroll, "ClipWindow");

    scale = 1; // get from preferences!
    int image_width, image_height;
    if (scale > 0) {
	image_width = scale * width;
	image_height = scale * height;
    } else {
	int s = -scale;
	image_width = (width + s - 1) / s;
	image_height = (height + s - 1) / s;
    }

    drawingarea = XtVaCreateManagedWidget("DrawingArea",
					  xmDrawingAreaWidgetClass,
					  scroll,
					  XmNwidth, (Dimension) image_width,
					  XmNheight, (Dimension) image_height,
					  NULL);

    Arg args[7];

    XtTranslations translations = XtParseTranslationTable(
		"<KeyDown>: DrawingAreaInput()\n"
		"<KeyUp>: DrawingAreaInput()\n"
		"<BtnDown>: DrawingAreaInput()\n"
		"<BtnUp>: DrawingAreaInput()\n"
		"<Motion>: DrawingAreaInput()\n");
    XtSetArg(args[0], XmNtranslations, translations);
    XtSetValues(drawingarea, args, 1);


    Widget hsb, vsb;
    Dimension marginwidth, marginheight, spacing, borderWidth, shadowThickness;
    XtSetArg(args[0], XmNhorizontalScrollBar, &hsb);
    XtSetArg(args[1], XmNverticalScrollBar, &vsb);
    XtSetArg(args[2], XmNscrolledWindowMarginWidth, &marginwidth);
    XtSetArg(args[3], XmNscrolledWindowMarginHeight, &marginheight);
    XtSetArg(args[4], XmNspacing, &spacing);
    XtSetArg(args[5], XmNborderWidth, &borderWidth);
    XtSetArg(args[6], XmNshadowThickness, &shadowThickness);
    XtGetValues(scroll, args, 7);

    Dimension hsbheight, hsbborder;
    XtSetArg(args[0], XmNheight, &hsbheight);
    XtSetArg(args[1], XmNborderWidth, &hsbborder);
    XtGetValues(hsb, args, 2);

    Dimension vsbwidth, vsbborder;
    XtSetArg(args[0], XmNwidth, &vsbwidth);
    XtSetArg(args[1], XmNborderWidth, &vsbborder);
    XtGetValues(vsb, args, 2);

    // I'm trying to make the initial window size such that the image fits
    // the clip window exactly, so you can see it all without having to scroll
    // and yet there's no screen area wasted.
    // What I would LIKE to be able to do is to somehow just tell the scrolled-
    // window to size itself to fit the drawingarea, or maybe size the clip
    // window (viewport, whatever, the thing that's scrolled by the scrolled-
    // window and which serves as the container for my drawingarea), and then
    // hope that the scrolledwindow will take the hint.
    // Wishful thinking, I fear, and so I solve the problem by doing the math
    // about how big the scrolledwindow has to be, myself.
    // The scrolledwindow is laid out as follows: it is surrounded by a border
    // of width 'borderWidth' (the usual X window border, usually 0); inside
    // that there's a margin of width 'scrolledWindowMarginWidth' and height
    // 'scrolledWindowMarginHeight' (both usually 0); inside the margin the
    // remaining area is divided in four areas: at the top left there's the
    // image area, at the top right there's the vertical scroll bar, at the
    // bottom left there's the horizontal scroll bar, and at the bottom right
    // there's nothing.
    // The areas for the scroll bars are sized to fit the dimensions returned
    // by the scroll bars themselves; my code assumes the usual X situation
    // where the total width is 'width' + 2 * 'borderWidth', and likewise for
    // the height (although it seems that 'borderWidth' is always 0 and can't
    // be changed on scroll bars, but that could be a quirk of Open Motif 2.2.2
    // for all I know).
    // The image area has a margin along its right and bottom edges, separating
    // it from the scroll bar; its width is 'spacing' (usually 4). Then there
    // is an additional margin around all four edges, which looks to be a
    // hard-coded 2 pixels wide; inside that margin there's a shadow of width
    // 'shadowThickness' (usually 2), and inside the shadow, there's an
    // XmClipWindow instance which serves as the container for whatever needs
    // to be scrolled around.
    // The margin right around the shadow around the ClipWindow, with its
    // hard-coded 2 pixel width, is a bit of a mystery to me. It looks like the
    // only part of the XmScrolledWindow for which there is no resource to
    // control it. In fact, it may not even exist in other Motif
    // implementations (the one I'm using as I write this is Open Motif 2.2.2,
    // from the Red Hat Linux 7.3 distribution). I suspect it is a hack to
    // account for the way the scroll bars lie about their geometry (they also
    // leave a 2 pixel margin around themselves) so that the shadow around the
    // clipwindow lines up nicely with the shadows around the scroll bars.
    // The value mysteryBorder = 2 works with Open Motif 2.2.2, as I just said;
    // it also works with Lesstif (unfortunately I don't remember which version
    // of Lesstif I was using when I came to that conclusion; haven't tried it
    // in a while). On Solaris 7 and 8 with the CDE libXm, mysteryBorder needs
    // to be set to 0 for the layout to come out right (I guess the Solaris
    // scroll bars don't leave a margin around themselves so the Solaris
    // scrolledwindow doesn't need a kludge to make things look nice).

    Dimension mysteryBorder = 2; // see above...

    Dimension sw = 2 * (borderWidth + marginwidth + mysteryBorder
	    + shadowThickness + vsbborder) + vsbwidth + spacing + image_width;
    Dimension sh = 2 * (borderWidth + marginheight + mysteryBorder
	    + shadowThickness + hsbborder) + hsbheight + spacing + image_height;
    XtSetArg(args[0], XmNwidth, sw);
    XtSetArg(args[1], XmNheight, sh);
    XtSetValues(scroll, args, 2);
    
    XtAddCallback(drawingarea, XmNresizeCallback, resize, (XtPointer) this);
    XtAddCallback(drawingarea, XmNexposeCallback, expose, (XtPointer) this);
    XtAddCallback(drawingarea, XmNinputCallback, input, (XtPointer) this);
    raise();


    // TODO: better preferences handling, including persistence and
    // command-line option handling!
    // For now, enable dithering when fewer than 5 bits per component are
    // available; scale always starts at 1.


    if (g_visual->c_class == PseudoColor || g_visual->c_class == StaticColor)
	dithering = true;
    else
	dithering = g_visual->bits_per_rgb < 5;
    optionsmenu->setToggleValue("Options.Dither", dithering, false);
    char buf[3];
    sprintf(buf, "%d", scale);
    scalemenu->setRadioValue("Windows.Scale", buf, false);

    selection_active = false;

    bool bitmap_ok = depth == 1 && scale >= 1;
    image = XCreateImage(g_display,
			 g_visual,
			 bitmap_ok ? 1 : g_depth,
			 bitmap_ok ? XYBitmap : ZPixmap,
			 0,
			 NULL,
			 image_width,
			 image_height,
			 bitmap_ok ? 8 : 32,
			 0);

    bool want_priv_cmap = false; // get from preferences!
    optionsmenu->setToggleValue("Options.PrivateColormap", want_priv_cmap, false);
    if (want_priv_cmap
	    && g_depth == 8
	    && g_visual->c_class == PseudoColor
	    && (depth == 8 || depth == 24 || (depth == 1 && scale <= -1))) {
	priv_cmap = XCreateColormap(g_display, g_rootwindow, g_visual, AllocAll);
	setColormap(priv_cmap);
	colormapChanged();
    }

    direct_copy = canIDoDirectCopy();
    if (direct_copy)
	image->data = (char *) pixels;
    else
	image->data = (char *) malloc(image->bytes_per_line * image->height);
    if (g_verbosity >= 1)
	if (direct_copy)
	    fprintf(stderr, "Using direct copy.\n");
	else
	    fprintf(stderr, "Not using direct copy.\n");

    plugin->run();
}

/* public */
Viewer::~Viewer() {
    if (image != NULL) {
	if (!direct_copy)
	    free(image->data);
	XFree(image);
    }
    if (plugin != NULL)
	Plugin::release(plugin);
    if (pixels != NULL)
	free(pixels);
    if (cmap != NULL)
	delete[] cmap;
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

    XtAppAddWorkProc(g_appcontext, deleteLater2, (XtPointer) this);
}

/* private static */ Boolean
Viewer::deleteLater2(XtPointer ud) {
    Viewer *This = (Viewer *) ud;
    delete This;
    return True;
}

/* public */ void
Viewer::paint(int top, int left, int bottom, int right) {
    if (direct_copy)
	paint_direct(top, left, bottom, right);
    else if (scale == 1)
	paint_unscaled(top, left, bottom, right);
    else if (scale > 1)
	paint_enlarged(top, left, bottom, right);
    else
	paint_reduced(top, left, bottom, right);
}

/* private */ void
Viewer::paint_direct(int top, int left, int bottom, int right) {
    XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
	      left, top, left, top,
	      right - left, bottom - top);
}

/* private */ void
Viewer::paint_unscaled(int top, int left, int bottom, int right) {
    if (depth == 1) {
	// Black and white are always available, so we never have to
	// do anything fancy to render 1-bit images, so we use this simple
	// special-case code that does not do the error diffusion thing.
	unsigned long black = BlackPixel(g_display, g_screennumber);
	unsigned long white = WhitePixel(g_display, g_screennumber);
	for (int y = top; y < bottom; y++)
	    for (int x = left; x < right; x++)
		XPutPixel(image, x, y,
			(pixels[y * bytesperline + (x >> 3)] & 1 << (x & 7))
			    == 0 ? black : white);
    } else if (depth == 8 && priv_cmap != None) {
	// As in the 1-bit case, in this case we know all our colors are
	// available, so no dithering is required and all we do is copy pixels.
	for (int y = top; y < bottom; y++)
	    for (int x = left; x < right; x++)
		XPutPixel(image, x, y, pixels[y * bytesperline + x]);
    } else if (!dithering && (g_grayramp == NULL || priv_cmap != None)) {
	// Color display, not dithered
	for (int y = top; y < bottom; y++) {
	    for (int x = left; x < right; x++) {
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
		}
		unsigned long pixel;
		if (priv_cmap != None) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int rerr = (r + 25) % 51 - 25;
		    int gerr = (g + 25) % 51 - 25;
		    int berr = (b + 25) % 51 - 25;
		    int color_err = rerr * rerr + gerr * gerr + berr * berr;
		    int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		    rerr = r - k;
		    gerr = g - k;
		    berr = b - k;
		    int gray_err = rerr * rerr + gerr * gerr + berr * berr;
		    if (color_err < gray_err) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
		    } else {
			int kk = (int) (k * 3 / 17.0 + 0.5);
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
		    }
		} else if (g_colorcube != NULL) {
		    int index = ((int) (r * (g_cubesize - 1) / 255.0 + 0.5)
			    * g_cubesize
			    + (int) (g * (g_cubesize - 1) / 255.0 + 0.5))
			    * g_cubesize
			    + (int) (b * (g_cubesize - 1) / 255.0 + 0.5);
		    pixel = g_colorcube[index].pixel;
		} else {
		    static bool inited = false;
		    static int rmax, rmult, bmax, bmult, gmax, gmult;
		    if (!inited) {
			rmax = g_visual->red_mask;
			rmult = 0;
			while ((rmax & 1) == 0) {
			    rmax >>= 1;
			    rmult++;
			}
			gmax = g_visual->green_mask;
			gmult = 0;
			while ((gmax & 1) == 0) {
			    gmax >>= 1;
			    gmult++;
			}
			bmax = g_visual->blue_mask;
			bmult = 0;
			while ((bmax & 1) == 0) {
			    bmax >>= 1;
			    bmult++;
			}
			inited = true;
		    }

		    pixel = ((r * rmax / 255) << rmult)
			    + ((g * gmax / 255) << gmult)
			    + ((b * bmax / 255) << bmult);
		}
		XPutPixel(image, x, y, pixel);
	    }
	}
    } else if (dithering && (g_grayramp == NULL || priv_cmap != None)) {
	// Color display, dithered
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
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
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
		if (priv_cmap != None) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int dr1 = (r + 25) % 51 - 25;
		    int dg1 = (g + 25) % 51 - 25;
		    int db1 = (b + 25) % 51 - 25;
		    int color_err = dr1 * dr1 + dg1 * dg1 + db1 * db1;
		    int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		    int dr2 = r - k;
		    int dg2 = g - k;
		    int db2 = b - k;
		    int gray_err = dr2 * dr2 + dg2 * dg2 + db2 * db2;
		    if (color_err < gray_err) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
			dR = dr1;
			dG = dg1;
			dB = db1;
		    } else {
			int kk = (int) (k * 3 / 17.0 + 0.5);
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
			dR = dr2;
			dG = dg2;
			dB = db2;
		    }
		} else if (g_colorcube != NULL) {
		    int index = ((int) (r * (g_cubesize - 1) / 255.0 + 0.5)
			    * g_cubesize
			    + (int) (g * (g_cubesize - 1) / 255.0 + 0.5))
			    * g_cubesize
			    + (int) (b * (g_cubesize - 1) / 255.0 + 0.5);
		    dR = r - (g_colorcube[index].red >> 8);
		    dG = g - (g_colorcube[index].green >> 8);
		    dB = b - (g_colorcube[index].blue >> 8);
		    pixel = g_colorcube[index].pixel;
		} else {
		    static bool inited = false;
		    static int rmax, rmult, bmax, bmult, gmax, gmult;
		    if (!inited) {
			rmax = g_visual->red_mask;
			rmult = 0;
			while ((rmax & 1) == 0) {
			    rmax >>= 1;
			    rmult++;
			}
			gmax = g_visual->green_mask;
			gmult = 0;
			while ((gmax & 1) == 0) {
			    gmax >>= 1;
			    gmult++;
			}
			bmax = g_visual->blue_mask;
			bmult = 0;
			while ((bmax & 1) == 0) {
			    bmax >>= 1;
			    bmult++;
			}
			inited = true;
		    }

		    pixel = ((r * rmax / 255) << rmult)
			    + ((g * gmax / 255) << gmult)
			    + ((b * bmax / 255) << bmult);
		    dR = r - (int) (r * rmax / 255.0 + 0.5) * 255 / rmax;
		    dG = g - (int) (g * gmax / 255.0 + 0.5) * 255 / gmax;
		    dB = b - (int) (b * bmax / 255.0 + 0.5) * 255 / bmax;
		}
		XPutPixel(image, x, y, pixel);
		int prevx = x - dir;
		int nextx = x + dir;
		if (prevx >= left && prevx < right) {
		    nextdr[prevx - left] += dR * 3;
		    nextdg[prevx - left] += dG * 3;
		    nextdb[prevx - left] += dB * 3;
		}
		nextdr[x - left] += dR * 5;
		nextdg[x - left] += dG * 5;
		nextdb[x - left] += dB * 5;
		if (nextx >= left && nextx < right) {
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
    } else if (!dithering) {
	// Grayscale display, not dithered
	for (int y = top; y < bottom; y++) {
	    for (int x = left; x < right; x++) {
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
		}
		int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		int graylevel = 
		    (int) (k * (g_rampsize - 1) / 255.0 + 0.5);
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		unsigned long pixel = g_grayramp[graylevel].pixel;
		XPutPixel(image, x, y, pixel);
	    }
	}
    } else {
	// Grayscale display, dithered
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
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
		}
		int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		k += (dk[x - left] + dK) >> 4;
		if (k < 0) k = 0; else if (k > 255) k = 255;
		dk[x - left] = 0;
		int graylevel = 
		    (int) (k * (g_rampsize - 1) / 255.0 + 0.5);
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		dK = k - (g_grayramp[graylevel].red >> 8);
		unsigned long pixel = g_grayramp[graylevel].pixel;
		XPutPixel(image, x, y, pixel);
		int prevx = x - dir;
		int nextx = x + dir;
		if (prevx >= left && prevx < right)
		    nextdk[prevx - left] += dK * 3;
		nextdk[x - left] += dK * 5;
		if (nextx >= left && nextx < right)
		    nextdk[nextx - left] += dK;
		dK *= 7;
	    }
	}
	delete[] dk;
	delete[] nextdk;
    }
    XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
	      left, top, left, top,
	      right - left, bottom - top);
}

/* private */ void
Viewer::paint_enlarged(int top, int left, int bottom, int right) {
    int TOP = top * scale;
    int BOTTOM = bottom * scale;
    int LEFT = left * scale;
    int RIGHT = right * scale;

    if (depth == 1) {
	// Black and white are always available, so we never have to
	// do anything fancy to render 1-bit images, so we use this simple
	// special-case code that does not do the error diffusion thing.
	unsigned long black = BlackPixel(g_display, g_screennumber);
	unsigned long white = WhitePixel(g_display, g_screennumber);
	for (int y = top; y < bottom; y++) {
	    int Y = y * scale;
	    for (int x = left; x < right; x++) {
		int X = x * scale;
		unsigned long pixel =
			(pixels[y * bytesperline + (x >> 3)] & 1 << (x & 7))
			    == 0 ? black : white;
		for (int YY = Y; YY < Y + scale; YY++)
		    for (int XX = X; XX < X + scale; XX++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else if (depth == 8 && priv_cmap != None) {
	// As in the 1-bit case, in this case we know all our colors are
	// available, so no dithering is required and all we do is copy pixels.
	for (int y = top; y < bottom; y++) {
	    int Y = y * scale;
	    for (int x = left; x < right; x++) {
		int X = x * scale;
		unsigned long pixel = pixels[y * bytesperline + x];
		for (int YY = Y; YY < Y + scale; YY++)
		    for (int XX = X; XX < X + scale; XX++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else if (!dithering && (g_grayramp == NULL || priv_cmap != None)) {
	// Color display, not dithered
	for (int y = top; y < bottom; y++) {
	    int Y = y * scale;
	    for (int x = left; x < right; x++) {
		int X = x * scale;
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
		}
		unsigned long pixel;
		if (priv_cmap != None) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int rerr = (r + 25) % 51 - 25;
		    int gerr = (g + 25) % 51 - 25;
		    int berr = (b + 25) % 51 - 25;
		    int color_err = rerr * rerr + gerr * gerr + berr * berr;
		    int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		    rerr = r - k;
		    gerr = g - k;
		    berr = b - k;
		    int gray_err = rerr * rerr + gerr * gerr + berr * berr;
		    if (color_err < gray_err) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
		    } else {
			int kk = (int) (k * 3 / 17.0 + 0.5);
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
		    }
		} else if (g_colorcube != NULL) {
		    int index = ((int) (r * (g_cubesize - 1) / 255.0 + 0.5)
			    * g_cubesize
			    + (int) (g * (g_cubesize - 1) / 255.0 + 0.5))
			    * g_cubesize
			    + (int) (b * (g_cubesize - 1) / 255.0 + 0.5);
		    pixel = g_colorcube[index].pixel;
		} else {
		    static bool inited = false;
		    static int rmax, rmult, bmax, bmult, gmax, gmult;
		    if (!inited) {
			rmax = g_visual->red_mask;
			rmult = 0;
			while ((rmax & 1) == 0) {
			    rmax >>= 1;
			    rmult++;
			}
			gmax = g_visual->green_mask;
			gmult = 0;
			while ((gmax & 1) == 0) {
			    gmax >>= 1;
			    gmult++;
			}
			bmax = g_visual->blue_mask;
			bmult = 0;
			while ((bmax & 1) == 0) {
			    bmax >>= 1;
			    bmult++;
			}
			inited = true;
		    }

		    pixel = ((r * rmax / 255) << rmult)
			    + ((g * gmax / 255) << gmult)
			    + ((b * bmax / 255) << bmult);
		}
		for (int XX = X; XX < X + scale; XX++)
		    for (int YY = Y; YY < Y + scale; YY++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else if (dithering && (g_grayramp == NULL || priv_cmap != None)) {
	// Color display, dithered
	int *dr = new int[RIGHT - LEFT];
	int *dg = new int[RIGHT - LEFT];
	int *db = new int[RIGHT - LEFT];
	int *nextdr = new int[RIGHT - LEFT];
	int *nextdg = new int[RIGHT - LEFT];
	int *nextdb = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dr[i] = dg[i] = db[i] = nextdr[i] = nextdg[i] = nextdb[i] = 0;
	int dR = 0, dG = 0, dB = 0;
	for (int Y = TOP; Y < BOTTOM; Y++) {
	    int y = Y / scale;
	    int dir = ((Y & 1) << 1) - 1;
	    int START, END;
	    if (dir == 1) {
		START = LEFT;
		END = RIGHT;
	    } else {
		START = RIGHT - 1;
		END = LEFT - 1;
	    }
	    int *temp;
	    temp = nextdr; nextdr = dr; dr = temp;
	    temp = nextdg; nextdg = dg; dg = temp;
	    temp = nextdb; nextdb = db; db = temp;
	    dR = dG = dB = 0;
	    for (int X = START; X != END; X += dir) {
		int x = X / scale;
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
		}
		r += (dr[X - LEFT] + dR) >> 4;
		if (r < 0) r = 0; else if (r > 255) r = 255;
		dr[X - LEFT] = 0;
		g += (dg[X - LEFT] + dG) >> 4;
		if (g < 0) g = 0; else if (g > 255) g = 255;
		dg[X - LEFT] = 0;
		b += (db[X - LEFT] + dB) >> 4;
		if (b < 0) b = 0; else if (b > 255) b = 255;
		db[X - LEFT] = 0;
		unsigned long pixel;
		if (priv_cmap != None) {
		    // 256-entry color map, with first 216 entries containing a
		    // 6x6x6 color cube, and the remaining 40 entries containing
		    // 40 shades of gray (which, together with the 6 shades of
		    // gray in the color cube, make up a 46-entry gray ramp
		    // We look for a color match and a graylevel match, and pick
		    // the best fit.
		    int dr1 = (r + 25) % 51 - 25;
		    int dg1 = (g + 25) % 51 - 25;
		    int db1 = (b + 25) % 51 - 25;
		    int color_err = dr1 * dr1 + dg1 * dg1 + db1 * db1;
		    int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		    int dr2 = r - k;
		    int dg2 = g - k;
		    int db2 = b - k;
		    int gray_err = dr2 * dr2 + dg2 * dg2 + db2 * db2;
		    if (color_err < gray_err) {
			int rr = (r + 25) / 51;
			int gg = (g + 25) / 51;
			int bb = (b + 25) / 51;
			pixel = 36 * rr + 6 * gg + bb;
			dR = dr1;
			dG = dg1;
			dB = db1;
		    } else {
			int kk = (int) (k * 3 / 17.0 + 0.5);
			if (kk % 9 == 0)
			    pixel = kk * 43 / 9;
			else
			    pixel = (kk / 9) * 8 + (kk % 9) + 215;
			dR = dr2;
			dG = dg2;
			dB = db2;
		    }
		} else if (g_colorcube != NULL) {
		    int index = ((int) (r * (g_cubesize - 1) / 255.0 + 0.5)
			    * g_cubesize
			    + (int) (g * (g_cubesize - 1) / 255.0 + 0.5))
			    * g_cubesize
			    + (int) (b * (g_cubesize - 1) / 255.0 + 0.5);
		    dR = r - (g_colorcube[index].red >> 8);
		    dG = g - (g_colorcube[index].green >> 8);
		    dB = b - (g_colorcube[index].blue >> 8);
		    pixel = g_colorcube[index].pixel;
		} else {
		    static bool inited = false;
		    static int rmax, rmult, bmax, bmult, gmax, gmult;
		    if (!inited) {
			rmax = g_visual->red_mask;
			rmult = 0;
			while ((rmax & 1) == 0) {
			    rmax >>= 1;
			    rmult++;
			}
			gmax = g_visual->green_mask;
			gmult = 0;
			while ((gmax & 1) == 0) {
			    gmax >>= 1;
			    gmult++;
			}
			bmax = g_visual->blue_mask;
			bmult = 0;
			while ((bmax & 1) == 0) {
			    bmax >>= 1;
			    bmult++;
			}
			inited = true;
		    }

		    pixel = ((r * rmax / 255) << rmult)
			    + ((g * gmax / 255) << gmult)
			    + ((b * bmax / 255) << bmult);
		    dR = r - (int) (r * rmax / 255.0 + 0.5) * 255 / rmax;
		    dG = g - (int) (g * gmax / 255.0 + 0.5) * 255 / gmax;
		    dB = b - (int) (b * bmax / 255.0 + 0.5) * 255 / bmax;
		}
		XPutPixel(image, X, Y, pixel);
		int PREVX = X - dir;
		int NEXTX = X + dir;
		if (PREVX >= LEFT && PREVX < RIGHT) {
		    nextdr[PREVX - LEFT] += dR * 3;
		    nextdg[PREVX - LEFT] += dG * 3;
		    nextdb[PREVX - LEFT] += dB * 3;
		}
		nextdr[X - LEFT] += dR * 5;
		nextdg[X - LEFT] += dG * 5;
		nextdb[X - LEFT] += dB * 5;
		if (NEXTX >= LEFT && NEXTX < RIGHT) {
		    nextdr[NEXTX - LEFT] += dR;
		    nextdg[NEXTX - LEFT] += dG;
		    nextdb[NEXTX - LEFT] += dB;
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
    } else if (!dithering) {
	// Grayscale display, not dithered
	for (int y = top; y < bottom; y++) {
	    int Y = y * scale;
	    for (int x = left; x < right; x++) {
		int X = x * scale;
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
		}
		int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		int graylevel = 
		    (int) (k * (g_rampsize - 1) / 255.0 + 0.5);
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		unsigned long pixel = g_grayramp[graylevel].pixel;
		for (int XX = X; XX < X + scale; XX++)
		    for (int YY = Y; YY < Y + scale; YY++)
			XPutPixel(image, XX, YY, pixel);
	    }
	}
    } else {
	// Grayscale display, dithered
	int *dk = new int[RIGHT - LEFT];
	int *nextdk = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dk[i] = nextdk[i] = 0;
	int dK = 0;
	for (int Y = TOP; Y < BOTTOM; Y++) {
	    int y = Y / scale;
	    int dir = ((Y & 1) << 1) - 1;
	    int START, END;
	    if (dir == 1) {
		START = LEFT;
		END = RIGHT;
	    } else {
		START = RIGHT - 1;
		END = LEFT - 1;
	    }
	    int *temp;
	    temp = nextdk; nextdk = dk; dk = temp;
	    dK = 0;
	    for (int X = START; X != END; X += dir) {
		int x = X / scale;
		int r, g, b;
		if (depth == 8) {
		    unsigned char index = pixels[y * bytesperline + x];
		    r = cmap[index].r;
		    g = cmap[index].g;
		    b = cmap[index].b;
		} else /* depth == 24 */ {
		    r = pixels[y * bytesperline + (x << 2) + 1];
		    g = pixels[y * bytesperline + (x << 2) + 2];
		    b = pixels[y * bytesperline + (x << 2) + 3];
		}
		int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
		k += (dk[X - LEFT] + dK) >> 4;
		if (k < 0) k = 0; else if (k > 255) k = 255;
		dk[X - LEFT] = 0;
		int graylevel = 
		    (int) (k * (g_rampsize - 1) / 255.0 + 0.5);
		if (graylevel >= g_rampsize)
		    graylevel = g_rampsize - 1;
		dK = k - (g_grayramp[graylevel].red >> 8);
		unsigned long pixel = g_grayramp[graylevel].pixel;
		XPutPixel(image, X, Y, pixel);
		int PREVX = X - dir;
		int NEXTX = X + dir;
		if (PREVX >= LEFT && PREVX < RIGHT)
		    nextdk[PREVX - LEFT] += dK * 3;
		nextdk[X - LEFT] += dK * 5;
		if (NEXTX >= LEFT && NEXTX < RIGHT)
		    nextdk[NEXTX - LEFT] += dK;
		dK *= 7;
	    }
	}
	delete[] dk;
	delete[] nextdk;
    }
    XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
	      LEFT, TOP, LEFT, TOP,
	      RIGHT - LEFT, BOTTOM - TOP);
}

/* private */ void
Viewer::paint_reduced(int top, int left, int bottom, int right) {
    int s = -scale;
    int TOP = top / s;
    int BOTTOM = (bottom + s - 1) / s;
    if (BOTTOM > image->height)
	BOTTOM = image->height;
    int LEFT = left / s;
    int RIGHT = (right + s - 1) / s;
    if (RIGHT > image->width)
	RIGHT = image->width;

    // Normalize source coordinates
    top = TOP * s;
    bottom = BOTTOM * s;
    if (bottom > height)
	bottom = height;
    left = LEFT * s;
    right = RIGHT * s;
    if (right > width)
	right = width;

    int R[RIGHT - LEFT];
    int G[RIGHT - LEFT];
    int B[RIGHT - LEFT];
    int W[RIGHT - LEFT];
    for (int i = 0; i < RIGHT - LEFT; i++)
	R[i] = G[i] = B[i] = W[i] = 0;


    if (!dithering || (depth == 1 && priv_cmap != None)) {
	// Color or Gray, no dithering
	int Y = TOP;
	int YY = 0;
	for (int y = top; y < bottom; y++) {
	    int X = 0;
	    int XX = 0;
	    for (int x = left; x < right; x++) {
		if (depth == 1) {
		    int p = ((pixels[y * bytesperline + (x >> 3)] >> (x & 7)) & 1)
				* 255;
		    R[X] += p;
		    G[X] += p;
		    B[X] += p;
		} else if (depth == 8) {
		    int p = pixels[y * bytesperline + x];
		    R[X] += cmap[p].r;
		    G[X] += cmap[p].g;
		    B[X] += cmap[p].b;
		} else {
		    unsigned char *p = pixels + (y * bytesperline + x * 4 + 1);
		    R[X] += *p++;
		    G[X] += *p++;
		    B[X] += *p;
		}
		W[X]++;
		if (++XX == s) {
		    XX = 0;
		    X++;
		}
	    }

	    if (YY == s - 1 || Y == BOTTOM - 1) {
		for (X = 0; X < RIGHT - LEFT; X++) {
		    int w = W[X];
		    int r = (R[X] + w / 2) / w; if (r > 255) r = 255;
		    int g = (G[X] + w / 2) / w; if (g > 255) g = 255;
		    int b = (B[X] + w / 2) / w; if (b > 255) b = 255;
		    R[X] = G[X] = B[X] = W[X] = 0;

		    if (priv_cmap != None) {
			// Find the closest match to (R, G, B) in the colormap...
			if (depth == 1) {
			    int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
			    if (k > 255)
				k = 255;
			    XPutPixel(image, X + LEFT, Y, k);
			} else if (depth == 24) {
			    // 256-entry color map, with first 216 entries
			    // containing a 6x6x6 color cube, and the remaining
			    // 40 entries containing 40 shades of gray (which,
			    // together with the 6 shades of gray in the color
			    // cube, make up a 46-entry gray ramp We look for a
			    // color match and a graylevel match, and pick the
			    // best fit.
			    int rerr = (r + 25) % 51 - 25;
			    int gerr = (g + 25) % 51 - 25;
			    int berr = (b + 25) % 51 - 25;
			    int color_err = rerr * rerr + gerr * gerr + berr * berr;
			    int k = (int) (r * 0.299 + g * 0.587 + b * 0.114); 
			    rerr = r - k; 
			    gerr = g - k;
			    berr = b - k;
			    int gray_err = rerr * rerr + gerr * gerr + berr * berr;
			    unsigned long pixel;
			    if (color_err < gray_err) {
				int rr = (r + 25) / 51;
				int gg = (g + 25) / 51;
				int bb = (b + 25) / 51;
				pixel = 36 * rr + 6 * gg + bb;
			    } else {
				int kk = (int) (k * 3 / 17.0 + 0.5);
				if (kk % 9 == 0)
				    pixel = kk * 43 / 9;
				else
				    pixel = (kk / 9) * 8 + (kk % 9) + 215;
			    }
			    XPutPixel(image, X + LEFT, Y, pixel);
			} else {
			    // TODO -- this is pathetic, of course
			    unsigned long best_pixel;
			    int best_error = 1000000;
			    for (int i = 0; i < 256; i++) {
				int dr = r - cmap[i].r;
				int dg = g - cmap[i].g;
				int db = b - cmap[i].b;
				int err = dr * dr + dg * dg + db * db;
				if (err < best_error) {
				    best_pixel = i;
				    best_error = err;
				}
			    }
			    XPutPixel(image, X + LEFT, Y, best_pixel);
			}
		    } else if (g_grayramp == NULL) {
			unsigned long pixel;
			if (g_colorcube != NULL) {
			    int index = ((int) (r * (g_cubesize - 1) / 255.0 + 0.5)
				    * g_cubesize
				    + (int) (g * (g_cubesize - 1) / 255.0 + 0.5))
				    * g_cubesize
				    + (int) (b * (g_cubesize - 1) / 255.0 + 0.5);
			    pixel = g_colorcube[index].pixel;
			} else {
			    static bool inited = false;
			    static int rmax, rmult, bmax, bmult, gmax, gmult;
			    if (!inited) {
				rmax = g_visual->red_mask;
				rmult = 0;
				while ((rmax & 1) == 0) {
				    rmax >>= 1;
				    rmult++;
				}
				gmax = g_visual->green_mask;
				gmult = 0;
				while ((gmax & 1) == 0) {
				    gmax >>= 1;
				    gmult++;
				}
				bmax = g_visual->blue_mask;
				bmult = 0;
				while ((bmax & 1) == 0) {
				    bmax >>= 1;
				    bmult++;
				}
				inited = true;
			    }

			    pixel = ((r * rmax / 255) << rmult)
				    + ((g * gmax / 255) << gmult)
				    + ((b * bmax / 255) << bmult);
			}
			XPutPixel(image, X + LEFT, Y, pixel);
		    } else {
			int k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
			int graylevel = 
			    (int) (k * (g_rampsize - 1) / 255.0 + 0.5);
			if (graylevel >= g_rampsize)
			    graylevel = g_rampsize - 1;
			unsigned long pixel = g_grayramp[graylevel].pixel;
			XPutPixel(image, X + LEFT, Y, pixel);
		    }
		}
	    }

	    if (++YY == s) {
		YY = 0;
		Y++;
	    }
	}
    } else if (g_grayramp == NULL || priv_cmap != None) {
	// Color dithering
	int *dr = new int[RIGHT - LEFT];
	int *dg = new int[RIGHT - LEFT];
	int *db = new int[RIGHT - LEFT];
	int *nextdr = new int[RIGHT - LEFT];
	int *nextdg = new int[RIGHT - LEFT];
	int *nextdb = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dr[i] = dg[i] = db[i] = nextdr[i] = nextdg[i] = nextdb[i] = 0;
	int dR = 0, dG = 0, dB = 0;

	int Y = TOP;
	int YY = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((Y & 1) << 1) - 1;
	    int start, end, START, END;
	    if (dir == 1) {
		start = left;
		end = right;
		START = 0;
		END = RIGHT - LEFT;
	    } else {
		start = right - 1;
		end = left - 1;
		START = RIGHT - LEFT - 1;
		END = -1;
	    }

	    int X = dir == 1 ? 0 : RIGHT - LEFT - 1;
	    int XX = dir == 1 ? 0 : (right - 1) % s + 1;
	    for (int x = start; x != end; x += dir) {
		if (depth == 1) {
		    int p = ((pixels[y * bytesperline + (x >> 3)]
				>> (x & 7)) & 1) * 255;
		    R[X] += p;
		    G[X] += p;
		    B[X] += p;
		} else if (depth == 8) {
		    int p = pixels[y * bytesperline + x];
		    R[X] += cmap[p].r;
		    G[X] += cmap[p].g;
		    B[X] += cmap[p].b;
		} else {
		    unsigned char *p = pixels + (y * bytesperline + x * 4 + 1);
		    R[X] += *p++;
		    G[X] += *p++;
		    B[X] += *p;
		}
		W[X]++;
		if (dir == 1) {
		    if (++XX == s) {
			XX = 0;
			X++;
		    }
		} else {
		    if (--XX == 0) {
			XX = s;
			X--;
		    }
		}
	    }

	    if (YY == s - 1 || y == bottom - 1) {
		int *temp;
		temp = nextdr; nextdr = dr; dr = temp;
		temp = nextdg; nextdg = dg; dg = temp;
		temp = nextdb; nextdb = db; db = temp;
		dR = dG = dB = 0;

		for (X = START; X != END; X += dir) {
		    int w = W[X];
		    int r = (R[X] + w / 2) / w; if (r > 255) r = 255;
		    int g = (G[X] + w / 2) / w; if (g > 255) g = 255;
		    int b = (B[X] + w / 2) / w; if (b > 255) b = 255;
		    R[X] = G[X] = B[X] = W[X] = 0;
		    
		    r += (dr[X] + dR) >> 4;
		    if (r < 0) r = 0; else if (r > 255) r = 255;
		    dr[X] = 0;
		    g += (dg[X] + dG) >> 4;
		    if (g < 0) g = 0; else if (g > 255) g = 255;
		    dg[X] = 0;
		    b += (db[X] + dB) >> 4;
		    if (b < 0) b = 0; else if (b > 255) b = 255;
		    db[X] = 0;

		    if (priv_cmap != None && depth == 24) {
			// 256-entry color map, with first 216 entries
			// containing a 6x6x6 color cube, and the remaining 40
			// entries containing 40 shades of gray (which,
			// together with the 6 shades of gray in the color
			// cube, make up a 46-entry gray ramp We look for a
			// color match and a graylevel match, and pick the best
			// fit.
			int dr1 = (r + 25) % 51 - 25;
			int dg1 = (g + 25) % 51 - 25;
			int db1 = (b + 25) % 51 - 25;
			int color_err = dr1 * dr1 + dg1 * dg1 + db1 * db1;
			int k = (int) (r * 0.299 + g * 0.587 + b * 0.114); 
			int dr2 = r - k; 
			int dg2 = g - k;
			int db2 = b - k;
			int gray_err = dr2 * dr2 + dg2 * dg2 + db2 * db2;
			unsigned long pixel;
			if (color_err < gray_err) {
			    int rr = (r + 25) / 51;
			    int gg = (g + 25) / 51;
			    int bb = (b + 25) / 51;
			    pixel = 36 * rr + 6 * gg + bb;
			    dR = dr1;
			    dG = dg1;
			    dB = db1;
			} else {
			    int kk = (int) (k * 3 / 17.0 + 0.5);
			    if (kk % 9 == 0)
				pixel = kk * 43 / 9;
			    else
				pixel = (kk / 9) * 8 + (kk % 9) + 215;
			    dR = dr2;
			    dG = dg2;
			    dB = db2;
			}
			XPutPixel(image, X + LEFT, Y, pixel);
		    } else if (priv_cmap != None) {
			// Find the closest match to (R, G, B) in the colormap...
			// TODO -- this is pathetic, of course! Ssslllooowww...
			int best_pixel;
			int best_error = 1000000;
			for (int i = 0; i < 256; i++) {
			    int rerr = r - cmap[i].r;
			    int gerr = g - cmap[i].g;
			    int berr = b - cmap[i].b;
			    int err = rerr * rerr + gerr * gerr + berr * berr;
			    if (err < best_error) {
				best_pixel = i;
				best_error = err;
			    }
			}
			XPutPixel(image, X + LEFT, Y, best_pixel);
			dR = r - cmap[best_pixel].r;
			dG = g - cmap[best_pixel].g;
			dB = b - cmap[best_pixel].b;
		    } else {
			unsigned long pixel;
			if (g_colorcube != NULL) {
			    int index = ((int) (r * (g_cubesize - 1) / 255.0 + 0.5)
				    * g_cubesize
				    + (int) (g * (g_cubesize - 1) / 255.0 + 0.5))
				    * g_cubesize
				    + (int) (b * (g_cubesize - 1) / 255.0 + 0.5);
			    pixel = g_colorcube[index].pixel;
			    dR = r - (g_colorcube[index].red >> 8);
			    dG = g - (g_colorcube[index].green >> 8);
			    dB = b - (g_colorcube[index].blue >> 8);
			} else {
			    static bool inited = false;
			    static int rmax, rmult, bmax, bmult, gmax, gmult;
			    if (!inited) {
				rmax = g_visual->red_mask;
				rmult = 0;
				while ((rmax & 1) == 0) {
				    rmax >>= 1;
				    rmult++;
				}
				gmax = g_visual->green_mask;
				gmult = 0;
				while ((gmax & 1) == 0) {
				    gmax >>= 1;
				    gmult++;
				}
				bmax = g_visual->blue_mask;
				bmult = 0;
				while ((bmax & 1) == 0) {
				    bmax >>= 1;
				    bmult++;
				}
				inited = true;
			    }

			    pixel = ((r * rmax / 255) << rmult)
				    + ((g * gmax / 255) << gmult)
				    + ((b * bmax / 255) << bmult);
			    dR = r - (int) (r * rmax / 255.0 + 0.5) * 255 / rmax;
			    dG = g - (int) (g * gmax / 255.0 + 0.5) * 255 / gmax;
			    dB = b - (int) (b * bmax / 255.0 + 0.5) * 255 / bmax;
			}
			XPutPixel(image, X + LEFT, Y, pixel);
		    }
		    
		    int PREVX = X - dir;
		    int NEXTX = X + dir;
		    if (PREVX >= 0 && PREVX < RIGHT - LEFT) {
			nextdr[PREVX] += dR * 3;
			nextdg[PREVX] += dG * 3;
			nextdb[PREVX] += dB * 3;
		    }
		    nextdr[X] += dR * 5;
		    nextdg[X] += dG * 5;
		    nextdb[X] += dB * 5;
		    if (NEXTX >= 0 && NEXTX < RIGHT - LEFT) {
			nextdr[NEXTX] += dR;
			nextdg[NEXTX] += dG;
			nextdb[NEXTX] += dB;
		    }
		    dR *= 7;
		    dG *= 7;
		    dB *= 7;
		}
	    }

	    if (++YY == s) {
		YY = 0;
		Y++;
	    }
	}
    } else {
	// Grayscale dithering
	int *dk = new int[RIGHT - LEFT];
	int *nextdk = new int[RIGHT - LEFT];
	for (int i = 0; i < RIGHT - LEFT; i++)
	    dk[i] = nextdk[i] = 0;
	int dK = 0;

	int Y = TOP;
	int YY = 0;
	for (int y = top; y < bottom; y++) {
	    int dir = ((Y & 1) << 1) - 1;
	    int start, end, START, END;
	    if (dir == 1) {
		start = left;
		end = right;
		START = 0;
		END = RIGHT - LEFT;
	    } else {
		start = right - 1;
		end = left - 1;
		START = RIGHT - LEFT - 1;
		END = -1;
	    }

	    int X = dir == 1 ? 0 : RIGHT - LEFT - 1;
	    int XX = dir == 1 ? 0 : (right - 1) % s + 1;
	    for (int x = start; x != end; x += dir) {
		if (depth == 1) {
		    int p = ((pixels[y * bytesperline + (x >> 3)]
				>> (x & 7)) & 1) * 255;
		    R[X] += p;
		} else if (depth == 8) {
		    int p = pixels[y * bytesperline + x];
		    R[X] += cmap[p].r;
		    G[X] += cmap[p].g;
		    B[X] += cmap[p].b;
		} else {
		    unsigned char *p = pixels + (y * bytesperline + x * 4 + 1);
		    R[X] += *p++;
		    G[X] += *p++;
		    B[X] += *p;
		}
		W[X]++;
		if (dir == 1) {
		    if (++XX == s) {
			XX = 0;
			X++;
		    }
		} else {
		    if (--XX == 0) {
			XX = s;
			X--;
		    }
		}
	    }

	    if (YY == s - 1 || y == bottom - 1) {
		int *temp;
		temp = nextdk; nextdk = dk; dk = temp;
		dK = 0;

		for (X = START; X != END; X += dir) {
		    int w = W[X];
		    int k;
		    if (depth == 1) {
			k = (R[X] + w / 2) / w;
			if (k > 255)
			    k = 255;
			R[X] = W[X] = 0;
		    } else {
			int r = (R[X] + w / 2) / w; if (r > 255) r = 255;
			int g = (G[X] + w / 2) / w; if (g > 255) g = 255;
			int b = (B[X] + w / 2) / w; if (b > 255) b = 255;
			k = (int) (r * 0.299 + g * 0.587 + b * 0.114);
			if (k > 255)
			    k = 255;
			R[X] = G[X] = B[X] = W[X] = 0;
		    }

		    k += (dk[X] + dK) >> 4;
		    if (k < 0) k = 0; else if (k > 255) k = 255;
		    dk[X] = 0;

		    int graylevel = 
			(int) (k * (g_rampsize - 1) / 255.0 + 0.5);
		    if (graylevel >= g_rampsize)
			graylevel = g_rampsize - 1;
		    unsigned long pixel = g_grayramp[graylevel].pixel;
		    XPutPixel(image, X + LEFT, Y, pixel);
		    dK = k - (g_grayramp[graylevel].red >> 8);
		    int PREVX = X - dir;
		    int NEXTX = X + dir;
		    if (PREVX >= 0 && PREVX < RIGHT - LEFT)
			nextdk[PREVX] += dK * 3;
		    nextdk[X] += dK * 5;
		    if (NEXTX >= 0 && NEXTX < RIGHT - LEFT)
			nextdk[NEXTX] += dK;
		    dK *= 7;
		}
	    }

	    if (++YY == s) {
		YY = 0;
		Y++;
	    }
	}
	delete[] dk;
	delete[] nextdk;
    }
    XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
	      LEFT, TOP, LEFT, TOP,
	      RIGHT - LEFT, BOTTOM - TOP);
}

/* public */ void
Viewer::colormapChanged() {
    if (priv_cmap == None) {
	paint(0, 0, height, width);
    } else {
	XColor colors[256];
	if (depth == 1) {
	    for (int i = 0; i < 256; i++) {
		colors[i].pixel = i;
		int k = 257 * i;
		colors[i].red = k;
		colors[i].green = k;
		colors[i].blue = k;
		colors[i].flags = DoRed | DoGreen | DoBlue;
	    }
	} else if (depth == 8) {
	    for (int i = 0; i < 256; i++) {
		colors[i].pixel = i;
		colors[i].red = 257 * cmap[i].r;
		colors[i].green = 257 * cmap[i].g;
		colors[i].blue = 257 * cmap[i].b;
		colors[i].flags = DoRed | DoGreen | DoBlue;
	    }
	} else {
	    int i = 0;
	    for (int r = 0; r < 6; r++)
		for (int g = 0; g < 6; g++)
		    for (int b = 0; b < 6; b++) {
			colors[i].pixel = i;
			colors[i].red = 13107 * r;
			colors[i].green = 13107 * g;
			colors[i].blue = 13107 * b;
			colors[i].flags = DoRed | DoGreen | DoBlue;
			i++;
		    }
	    for (int m = 0; m < 5; m++)
		for (int n = 1; n < 9; n++) {
		    int p = 13107 * m + 1456 * n;
		    colors[i].pixel = i;
		    colors[i].red = p;
		    colors[i].green = p;
		    colors[i].blue = p;
		    colors[i].flags = DoRed | DoGreen | DoBlue;
		    i++;
		}
	}
	XStoreColors(g_display, priv_cmap, colors, 256);
    }
}

/* private virtual */ void
Viewer::close() {
    delete this;
}

/* private */ bool
Viewer::canIDoDirectCopy() {
    if (scale != 1)
	return false;
    if (depth == 1)
	return image->bytes_per_line == bytesperline
		&& image->byte_order == LSBFirst
		&& image->bitmap_bit_order == LSBFirst;
    if (depth == 8)
	return priv_cmap != None // implies 8-bit PseudoColor visual
		&& image->bytes_per_line == bytesperline
		&& image->byte_order == LSBFirst;
    // depth == 24
    return image->depth == 32
		&& image->bytes_per_line == bytesperline
		&& ((image->byte_order == LSBFirst
			&& image->red_mask == 0x00FF0000
			&& image->green_mask == 0x0000FF00
			&& image->blue_mask == 0x000000FF)
		    || (image->byte_order == MSBFirst
			&& image->red_mask == 0x0000FF00
			&& image->green_mask == 0x00FF0000
			&& image->blue_mask == 0xFF000000));
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
    XPutImage(g_display, XtWindow(drawingarea), g_gc, image, x, y, x, y, w, h);
}

/* private static */ void
Viewer::input(Widget w, XtPointer ud, XtPointer cd) {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    ((Viewer *) ud)->input2(cbs->event);
}

/* private */ void
Viewer::input2(XEvent *event) {
    switch (event->type) {
	case 2:
	    printf("KeyPress: state=%d keycode=%d\n", event->xkey.state,
						      event->xkey.keycode);
	    break;
	case 3:
	    printf("KeyRelease: state=%d keycode=%d\n", event->xkey.state,
							event->xkey.keycode);
	    break;
	case 4:
	    printf("ButtonPress: pos=(%d, %d) state=%d button=%d\n",
						    event->xbutton.x,
						    event->xbutton.y,
						    event->xbutton.state,
						    event->xbutton.button);
	    break;
	case 5:
	    printf("ButtonPress: pos=(%d, %d) state=%d button=%d\n",
						    event->xbutton.x,
						    event->xbutton.y,
						    event->xbutton.state,
						    event->xbutton.button);
	    break;
	case 6:
	    printf("MotionNotify: pos=(%d, %d)\n", event->xmotion.x,
						   event->xmotion.y);
	    break;
	case 7:
	    printf("Unexpected event!\n");
	    fflush(stdout);
	    core();
    }
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
    else if (strcmp(id, "Windows.Enlarge") == 0)
	doEnlarge();
    else if (strcmp(id, "Windows.Reduce") == 0)
	doReduce();
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
    if (g_verbosity >= 1)
	fprintf(stderr, "Toggle \"%s\" set to '%s'.\n", id, value ? "true" : "false");
    if (strcmp(id, "Options.PrivateColormap") == 0)
	doPrivateColormap(value);
    else if (strcmp(id, "Options.Dither") == 0)
	doDither(value);
    else if (strcmp(id, "Options.Notify") == 0)
	doNotify(value);
}

/* private static */ void
Viewer::radiocallback(void *closure, const char *id, const char *value) {
    ((Viewer *) closure)->radiocallback2(id, value);
}

/* private */ void
Viewer::radiocallback2(const char *id, const char *value) {
    if (g_verbosity >= 1)
	fprintf(stderr, "Radio \"%s\" set to '%s'.\n", id, value);
    if (strcmp(id, "Windows.Scale") == 0)
	doScale(value);
}

/* private */ void
Viewer::doBeep() {
    XBell(g_display, 100);
}

/* private */ void
Viewer::doNew(const char *plugin) {
    new Viewer(plugin);
}

/* private */ void
Viewer::doOpen() {
    Arg arg;
    XtSetArg(arg, XmNdirectory, curr_path_name);
    // Using g_appshell as the parent, instead of our own toplevel shell,
    // because I don't want the file selection dialog to inherit our colormap.
    // The drawback is that the dialog does not get positioned very nicely.
    // TODO...
    Widget fsb = XmCreateFileSelectionDialog(g_appshell, "Open", &arg, 1);
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
		XBell(g_display, 100);
	    XtFree(filename);
	}
    }
    XtUnmanageChild(w);
}

/* public static */ bool
Viewer::openFile(const char *filename) {
    if (g_verbosity >= 1)
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
    paint(0, 0, height, width);
}

/* private */ void
Viewer::doPrivateColormap(bool value) {
    if (!(g_depth == 8
		&& g_visual->c_class == PseudoColor
		&& (depth == 8 || depth == 24 || (depth == 1 && scale <= -1))))
	return;

    if (value) {
	priv_cmap = XCreateColormap(g_display, g_rootwindow, g_visual, AllocAll);
	setColormap(priv_cmap);
	if (canIDoDirectCopy()) {
	    free(image->data);
	    image->data = (char *) pixels;
	    direct_copy = true;
	    if (g_verbosity >= 1)
		fprintf(stderr, "Using direct copy.\n");
	}
	colormapChanged();
	paint(0, 0, height, width);
    } else {
	XFreeColormap(g_display, priv_cmap);
	priv_cmap = None;
	setColormap(g_colormap);
	if (direct_copy) {
	    image->data = (char *) malloc(image->bytes_per_line * image->height);
	    if (g_verbosity >= 1 && direct_copy)
		fprintf(stderr, "Not using direct copy.\n");
	    direct_copy = false;
	}
	colormapChanged();
    }
}

/* private */ void
Viewer::doDither(bool value) {
    dithering = value;
    paint(0, 0, height, width);
}

/* private */ void
Viewer::doNotify(bool) {
    doBeep();
}

/* private */ void
Viewer::doEnlarge() {
    if (scale < 8) {
	scale++;
	if (scale == -1)
	    scale = 1;
	char buf[3];
	sprintf(buf, "%d", scale);
	scalemenu->setRadioValue("Windows.Scale", buf, true);
    } else
	doBeep();
}

/* private */ void
Viewer::doReduce() {
    if (scale > -8) {
	scale--;
	if (scale == 0)
	    scale = -2;
	char buf[3];
	sprintf(buf, "%d", scale);
	scalemenu->setRadioValue("Windows.Scale", buf, true);
    } else
	doBeep();
}

/* private */ void
Viewer::doScale(const char *value) {
    sscanf(value, "%d", &scale);

    // Find out the difference in size between the ScrolledWindow's ClipWindow
    // and the overall window size
    Dimension clipwidth, clipheight;
    Arg args[2];
    XtSetArg(args[0], XmNwidth, &clipwidth);
    XtSetArg(args[1], XmNheight, &clipheight);
    XtGetValues(clipwindow, args, 2);
    Dimension windowwidth, windowheight;
    getSize(&windowwidth, &windowheight);
    Dimension extra_h = windowwidth - clipwidth;
    Dimension extra_v = windowheight - clipheight;
    
    // Compute scaled image size
    int w, h;
    if (scale > 0) {
	w = width * scale;
	h = height * scale;
    } else {
	int s = -scale;
	w = (width + s - 1) / s;
	h = (height + s - 1) / s;
    }

    // Resize the DrawingArea
    XtSetArg(args[0], XmNwidth, w);
    XtSetArg(args[1], XmNheight, h);
    XtSetValues(drawingarea, args, 2);

    // Resize top-level window to fit
    setSize(w + extra_h, h + extra_v);
    fitToScreen();

    if (!direct_copy)
	free(image->data);
    XFree(image);
    bool bitmap_ok = depth == 1 && scale >= 1;
    image = XCreateImage(g_display,
			 g_visual,
			 bitmap_ok ? 1 : g_depth,
			 bitmap_ok ? XYBitmap : ZPixmap,
			 0,
			 NULL,
			 w,
			 h,
			 bitmap_ok ? 8 : 32,
			 0);

    if (depth == 1) {
	bool want_priv_cmap = scale <= -1
		&& optionsmenu->getToggleValue("Options.PrivateColormap");
	bool have_priv_cmap = priv_cmap != None;
	
	if (want_priv_cmap && !have_priv_cmap) {
	    priv_cmap = XCreateColormap(g_display, g_rootwindow, g_visual, AllocAll);
	    setColormap(priv_cmap);
	    colormapChanged();
	} else if (!want_priv_cmap && have_priv_cmap) {
	    XFreeColormap(g_display, priv_cmap);
	    priv_cmap = None;
	    setColormap(g_colormap);
	}
    }
    
    bool old_direct_copy = direct_copy;
    direct_copy = canIDoDirectCopy();
    if (direct_copy)
	image->data = (char *) pixels;
    else
	image->data = (char *) malloc(image->bytes_per_line * image->height);
    if (g_verbosity >= 1 && old_direct_copy != direct_copy)
	if (direct_copy)
	    fprintf(stderr, "Using direct copy.\n");
	else
	    fprintf(stderr, "Not using direct copy.\n");

    paint(0, 0, height, width);
}

/* private */ void
Viewer::doGeneral() {
    doBeep();
}

/* private */ void
Viewer::doHelp(const char *id) {
    doBeep();
}
