#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Viewer.h"
#include "YesNoCancelDialog.h"
#include "ColormapEditor.h"
#include "CopyBits.h"
#include "FileDialog.h"
#include "ImageIO.h"
#include "Menu.h"
#include "Plugin.h"
#include "SaveImageDialog.h"
#include "TextViewer.h"
#include "UndoManager.h"
#include "main.h"
#include "util.h"

#include "FW.help"


/* private static */ List *
Viewer::instances = new List;

/* private static */ int
Viewer::idcount = 0;

/* private static */ GC
Viewer::gc = None;

/* private static */ char *
Viewer::file_directory = NULL;

/* private static */ char *
Viewer::colormap_directory = NULL;

/* private static */ bool
Viewer::inner_decor_known = false;

/* private static */ int
Viewer::inner_decor_width = 0;

/* private static */ int
Viewer::inner_decor_height = 0;


/* public */
Viewer::Viewer(const char *pluginname)
    : Frame(true, false, true) {

    // This constructor is called in response to "File->New->(Plugin)"
    is_brand_new = true;
    filename = NULL;
    filetype = NULL;
    init(pluginname, NULL, NULL, 0, NULL);
}

/* public */
Viewer::Viewer(Plugin *clonee)
    : Frame(true, false, true) {

    // This constructor is called in response to "File->New->Clone"
    is_brand_new = true;
    filename = NULL;
    filetype = NULL;
    init(NULL, clonee, NULL, 0, NULL);
}

/* public */
Viewer::Viewer(const char *pluginname, void *plugin_data,
	               int plugin_data_length, FWPixmap *fpm,
		       const char *filename, const char *filetype)
    : Frame(true, false, true) {

    // This constructor is called in response to "File->Open"
    // and for files passed as command line arguments
    is_brand_new = false;
    this->filename = filename == NULL ? NULL : canonical_pathname(filename);
    this->filetype = filetype == NULL ? NULL : strclone(filetype);
    init(pluginname, NULL, plugin_data, plugin_data_length, fpm);
}

class UndoManagerListener : public UndoManager::Listener {
    private:
	Viewer *viewer;
    public:
	UndoManagerListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	virtual void titleChanged(const char *undo, const char *redo) {
	    viewer->editmenu->changeLabel("Edit.Undo", undo);
	    viewer->editmenu->changeLabel("Edit.Redo", redo);
	}
};

class ColormapEditorListener : public ColormapEditor::Listener {
    private:
	Viewer *viewer;
    public:
	ColormapEditorListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	virtual void cmeClosed() {
	    viewer->cme = NULL;
	}
	virtual void colormapChanged() {
	    viewer->colormapChanged();
	}
	virtual void loadColors() {
	    viewer->doLoadColors();
	}
	virtual void saveColors() {
	    viewer->doSaveColors();
	}
	virtual ColormapEditor *getCME() {
	    return viewer->cme;
	}
};

/* private */ void
Viewer::init(const char *pluginname, Plugin *clonee, void *plugin_data,
	     int plugin_data_length, FWPixmap *fpm) {
    id = idcount++;
    image = NULL;
    colormap = g_colormap;
    cme = NULL;
    finished = false;
    untitled = true;
    undomanager = new UndoManager;
    undomanager->addListener(new UndoManagerListener(this));
    saved_undo_id = undomanager->getCurrentId();
    cmelistener = NULL;

    // The 'dirty' flag is used to track changes made by the plugin. Viewer
    // sets this flag whenever it calls plugin->start() or plugin->restart()
    // and they return 'false' (if they return 'true', it is assumed that no
    // changes took place, in addition to 'finished' being set).
    // Also, SlaveDriver sets 'dirty' thenever it invokes 'work()' on our
    // plugin, regardless of the value returned by 'work()'.
    // The determination of whether or not to ask "save changes?" when the user
    // tries to close a Viewer or quit the application, is made using this
    // 'dirty' flag, and the state of the UndoManager (by comparing
    // saved_undo_id to undomanager->getCurrentId()).

    dirty = false;

    if (clonee != NULL)
	pluginname = clonee->name();

    bool nullplugin = pluginname == NULL;

    plugin = Plugin::get(pluginname);
    if (plugin == NULL) {
	if (fpm != NULL) {
	    // We're here because a file has been opened, and we're going to
	    // display it, whether we have a matching plugin or not.
	    // TODO: error reporting!
	    // Since the plugin was not found, we're falling back on the Null
	    // plugin.
	    plugin = Plugin::get("Null");
	    nullplugin = true;
	    finished = true;
	    plugin->setFinished();
	} else {
	    delete this;
	    return;
	}
    }
    plugin->setViewer(this);
    plugin->setPixmap(&pm);
    if (fpm != NULL) {
	pm = *fpm;
	if (!nullplugin)
	    plugin->deserialize(plugin_data, plugin_data_length);
	finish_init();
    } else if (clonee != NULL) {
	plugin->init_clone(clonee);
	// The plugin must call Plugin::init_proceed() (which calls
	// Viewer::finish_init()) or Plugin::init_abort() (which calls
	// Viewer::deleteLater()).
    } else {
	plugin->init_new();
	// The plugin must call Plugin::init_proceed() (which calls
	// Viewer::finish_init()) or Plugin::init_abort() (which calls
	// Viewer::deleteLater()).
    }
}

/* public */ void
Viewer::finish_init() {
    char *name;
    if (filename == NULL)
	name = "Untitled";
    else
	name = basename(filename);
    if (strcmp(plugin->name(), "Null") == 0) {
	setTitle(name);
	setIconTitle(name);
    } else {
	char buf[256];
	snprintf(buf, 256, "%s (%s)", name, plugin->name());
	setTitle(buf);
	setIconTitle(buf);
    }
    if (filename != NULL)
	free(name);

    Menu *topmenu = new Menu;

    Menu *pluginmenu = new Menu;
    pluginmenu->addCommand("Clone", NULL, "Ctrl+N", "File.Clone");
    pluginmenu->addSeparator();
    char **plugins = Plugin::list();
    if (plugins == NULL)
	pluginmenu->addCommand("No Plugins", NULL, NULL, "File.Beep");
    else {
	for (char **p = plugins; *p != NULL; p++) {
	    char id[100];
	    snprintf(id, 100, "File.New.%s", *p);
	    pluginmenu->addCommand(*p, NULL, NULL, id);
	}
    }
	
    Menu *filemenu = new Menu;
    filemenu->addMenu("New", NULL, NULL, "File.New", pluginmenu);
    filemenu->addCommand("Open...", NULL, "Ctrl+O", "File.Open");
    filemenu->addSeparator();
    filemenu->addCommand("Close", NULL, "Ctrl+W", "File.Close");
    filemenu->addCommand("Save", NULL, "Ctrl+S", "File.Save");
    filemenu->addCommand("Save As...", NULL, NULL, "File.SaveAs");
    filemenu->addCommand("Get Info...", NULL, "Ctrl+I", "File.GetInfo");
    filemenu->addSeparator();
    filemenu->addCommand("Print...", NULL, "Ctrl+P", "File.Print");
    filemenu->addSeparator();
    filemenu->addCommand("Quit", "Q", "Ctrl+Q", "File.Quit");
    topmenu->addMenu("File", "F", NULL, "File", filemenu);

    editmenu = new Menu;
    editmenu->addCommand("Undo", NULL, "Ctrl+Z", "Edit.Undo");
    editmenu->addCommand("Redo", NULL, "Ctrl+Y", "Edit.Redo");
    editmenu->addSeparator();
    editmenu->addCommand("Cut", NULL, "Ctrl+X", "Edit.Cut");
    editmenu->addCommand("Copy", NULL, "Ctrl+C", "Edit.Copy");
    editmenu->addCommand("Paste", NULL, "Ctrl+V", "Edit.Paste");
    editmenu->addCommand("Clear", NULL, NULL, "Edit.Clear");
    topmenu->addMenu("Edit", NULL, NULL, "Edit", editmenu);

    Menu *colormenu = new Menu;
    colormenu->addCommand("Load Colors...", NULL, NULL, "Color.LoadColors");
    colormenu->addCommand("Save Colors...", NULL, NULL, "Color.SaveColors");
    colormenu->addSeparator();
    colormenu->addCommand("Edit Colors...", NULL, "Ctrl+E", "Color.EditColors");
    topmenu->addMenu("Color", NULL, NULL, "Color", colormenu);

    Menu *drawmenu = new Menu;
    drawmenu->addCommand("Stop Others", NULL, NULL, "Draw.StopOthers");
    drawmenu->addCommand("Stop", NULL, "Ctrl+K", "Draw.Stop");
    drawmenu->addCommand("Stop All", NULL, NULL, "Draw.StopAll");
    drawmenu->addCommand("Continue", NULL, "Ctrl+L", "Draw.Continue");
    drawmenu->addCommand("Continue All", NULL, NULL, "Draw.ContinueAll");
    drawmenu->addSeparator();
    drawmenu->addCommand("Update Now", NULL, "Ctrl+U", "Draw.UpdateNow");
    topmenu->addMenu("Draw", NULL, NULL, "Draw", drawmenu);

    optionsmenu = new Menu;
    optionsmenu->addToggle("Private Colormap", NULL, "Ctrl+M", "Options.PrivateColormap");
    optionsmenu->addToggle("Dither", NULL, "Ctrl+D", "Options.Dither");
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
    scalemenu->addRadio("100%", NULL, "Ctrl+1", "Windows.Scale@1");
    scalemenu->addRadio("200%", NULL, NULL, "Windows.Scale@2");
    scalemenu->addRadio("300%", NULL, NULL, "Windows.Scale@3");
    scalemenu->addRadio("400%", NULL, NULL, "Windows.Scale@4");
    scalemenu->addRadio("500%", NULL, NULL, "Windows.Scale@5");
    scalemenu->addRadio("600%", NULL, NULL, "Windows.Scale@6");
    scalemenu->addRadio("700%", NULL, NULL, "Windows.Scale@7");
    scalemenu->addRadio("800%", NULL, NULL, "Windows.Scale@8");

    windowsmenu = new Menu;
    windowsmenu->addCommand("Enlarge", NULL, "Ctrl+0", "Windows.Enlarge");
    windowsmenu->addCommand("Reduce", NULL, "Ctrl+9", "Windows.Reduce");
    windowsmenu->addMenu("Scale", NULL, NULL, "Scale", scalemenu);
    windowsmenu->addSeparator();
    for (int i = 0; i < instances->size(); i++) {
	Viewer *vw = (Viewer *) instances->get(i);
	char buf[64];
	snprintf(buf, 256, "Windows.X.%d", vw->id);
	windowsmenu->addCommand(vw->getTitle(), NULL, NULL, buf);
    }
    topmenu->addMenu("Windows", NULL, NULL, "Windows", windowsmenu);

    Menu *helpmenu = new Menu;
    helpmenu->addCommand("About Fractal Wizard", NULL, NULL, "File.New.About");
    helpmenu->addCommand("General", NULL, NULL, "Help.General");
    if (plugins != NULL) {
	helpmenu->addSeparator();
	for (char **p = plugins; *p != NULL; p++) {
	    char id[100];
	    snprintf(id, 100, "Help.X.%s", *p);
	    helpmenu->addCommand(*p, NULL, NULL, id);
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
	image_width = scale * pm.width;
	image_height = scale * pm.height;
    } else {
	int s = -scale;
	image_width = (pm.width + s - 1) / s;
	image_height = (pm.height + s - 1) / s;
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


    if (!inner_decor_known) {
	// Find out the difference in size between the ScrolledWindow's
	// ClipWindow and the overall window size
	Dimension clipwidth, clipheight;
	Arg args[2];
	XtSetArg(args[0], XmNwidth, &clipwidth);
	XtSetArg(args[1], XmNheight, &clipheight);
	XtGetValues(clipwindow, args, 2);
	int windowwidth, windowheight;
	getSize(&windowwidth, &windowheight);
	inner_decor_width = windowwidth - clipwidth;
	inner_decor_height = windowheight - clipheight;
	inner_decor_known = true;
    }


    // TODO: better preferences handling, including persistence and
    // command-line option handling!
    // For now, enable dithering when fewer than 5 bits per component are
    // available; scale always starts at 1.


    if (g_visual->c_class == PseudoColor || g_visual->c_class == StaticColor)
	dithering = true;
    else if (g_visual->c_class == GrayScale || g_visual->c_class == StaticGray){
	int realdepth = g_depth;
	if (realdepth > g_visual->bits_per_rgb)
	    realdepth = g_visual->bits_per_rgb;
	dithering = realdepth < 5;
    } else {
	// Don't rely on g_visual->bits_per_rgb alone. Using XFree86 4.2.0
	// in TrueColor mode at 8 bits per pixel, I got bits_per_rgb = 8,
	// while in actual fact I had 3 bits each for red and green, and
	// two bits for blue. On that visual, dithering should be on by
	// default.
	// So, I do the same kind of math that I do in main.cc (when I try to
	// figure out how many shades of gray it makes sense to allocate) to
	// get a really accurate number of bits per component.
	// I turn dithering on automagically if there are fewer than 5 bits per
	// component. At 5 or more, e.g. at 16 bpp with 5 bits each for red and
	// blue, and 6 bits for green, I can't tell the difference between
	// dithering and no dithering. Maybe someone with better eyesight
	// could, but it's such a close call that I feel it's safe to opt for
	// speed in that case.
	int realdepth = g_depth;
	if (realdepth > g_visual->bits_per_rgb)
	    realdepth = g_visual->bits_per_rgb;
	unsigned long redmask = g_visual->red_mask;
	unsigned long greenmask = g_visual->green_mask;
	unsigned long bluemask = g_visual->blue_mask;
	int redbits = 0, greenbits = 0, bluebits = 0;
	while (redmask != 0 || greenmask != 0 || bluemask != 0) {
	    redbits += redmask & 1;
	    redmask >>= 1;
	    greenbits += greenmask & 1;
	    greenmask >>= 1;
	    bluebits += bluemask & 1;
	    bluemask >>= 1;
	}
	if (realdepth > redbits)
	    realdepth = redbits;
	if (realdepth > greenbits)
	    realdepth = greenbits;
	if (realdepth > bluebits)
	    realdepth = bluebits;
	dithering = realdepth < 5;
    }

    optionsmenu->setToggleValue("Options.Dither", dithering, false);
    char buf[3];
    sprintf(buf, "%d", scale);
    scalemenu->setRadioValue("Windows.Scale", buf, false);

    selection_in_progress = false;
    selection_visible = false;

    bool use_bitmap = pm.depth == 1 && scale >= 1;
    image = XCreateImage(g_display,
			 g_visual,
			 use_bitmap ? 1 : g_depth,
			 use_bitmap ? XYBitmap : ZPixmap,
			 0,
			 NULL,
			 image_width,
			 image_height,
			 32,
			 0);

    bool want_priv_cmap = false; // get from preferences!
    optionsmenu->setToggleValue("Options.PrivateColormap", want_priv_cmap, false);
    if (want_priv_cmap
	    && g_depth == 8
	    && g_visual->c_class == PseudoColor
	    && (pm.depth == 8 || pm.depth == 24 || (pm.depth == 1 && scale <= -1))) {
	colormap = XCreateColormap(g_display, g_rootwindow, g_visual, AllocAll);
	setColormap(colormap);
	colormapChanged();
    }

    direct_copy = canIDoDirectCopy();
    if (direct_copy)
	image->data = (char *) pm.pixels;
    else
	image->data = (char *) malloc(image->bytes_per_line * image->height);
    if (g_verbosity >= 1)
	if (direct_copy)
	    fprintf(stderr, "Using direct copy.\n");
	else
	    fprintf(stderr, "Not using direct copy.\n");

    if (is_brand_new) {
	if (plugin->start()) {
	    finished = true;
	    plugin->setFinished();
	} else
	    dirty = true;
    } else {
	paint(0, 0, pm.height, pm.width);
	if (!finished) {
	    if (plugin->restart()) {
		finished = true;
		plugin->setFinished();
	    } else {
		dirty = true;
		// The user may not be aware that the document they just opened
		// is an unfinished one, so in this case we turn on
		// notifications automagically.
		optionsmenu->setToggleValue("Options.Notify", true, false);
	    }
	}
    }

    addViewer(this);
}

/* public */
Viewer::~Viewer() {
    if (cme != NULL) {
	cme->close();
	cme = NULL;
    }
    if (plugin != NULL) {
	Plugin::release(plugin);
	plugin = NULL;
    }
    delete undomanager;
    if (cmelistener != NULL)
	delete cmelistener;
    if (colormap != g_colormap) {
	XFreeColormap(g_display, colormap);
	setColormap(g_colormap);
    }
    if (filename != NULL)
	free(filename);
    if (filetype != NULL)
	free(filetype);
    if (image != NULL) {
	if (!direct_copy)
	    free(image->data);
	XFree(image);
    }
    if (pm.pixels != NULL)
	free(pm.pixels);
    if (pm.cmap != NULL)
	delete[] pm.cmap;

    removeViewer(this);
    if (instances->size() == 0)
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

/* private static */ void
Viewer::addViewer(Viewer *viewer) {
    instances->append(viewer);
    char buf[64];
    snprintf(buf, 64, "Windows.X.%d", viewer->id);
    Iterator *iter = instances->iterator();
    while (iter->hasNext()) {
	Viewer *v = (Viewer *) iter->next();
	v->windowsmenu->addCommand(viewer->getTitle(), NULL, NULL, buf);
    }
    delete iter;
}

/* private static */ void
Viewer::removeViewer(Viewer *viewer) {
    char buf[64];
    snprintf(buf, 64, "Windows.X.%d", viewer->id);
    Iterator *iter = instances->iterator();
    while (iter->hasNext()) {
	Viewer *v = (Viewer *) iter->next();
	v->windowsmenu->remove(buf);
    }
    delete iter;
    instances->remove(viewer);
}

/* public */ void
Viewer::setDirty() {
    dirty = true;
}

/* public */ void
Viewer::pluginFinished(bool notify) {
    finished = true;
    // The 'notify' parameter is 'false' when we get called by
    // Plugin::deserialize(), and 'true' when we get called by SlaveDriver.
    // Even if it's 'true', we won't actually do anything to attract the user's
    // attention unless they have requested we do so.
    if (notify && optionsmenu->getToggleValue("Options.Notify")) {
	char buf[1024];
	snprintf(buf, 1024, "The plugin \"%s\" would like to inform you that work on the image \"%s\" has been completed.", plugin->name(), filename == NULL ? "(null)" : filename);
	TextViewer *tv = new TextViewer(buf);
	tv->raise();
	beep();
    }
}

/* public */ void
Viewer::paint(int top, int left, int bottom, int right) {
    if (direct_copy) {
	image->data = (char *) pm.pixels;
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  left, top, left, top,
		  right - left, bottom - top);
    } else if (scale == 1) {
	CopyBits::copy_unscaled(&pm, image, colormap != g_colormap, false, dithering,
				 top, left, bottom, right);
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  left, top, left, top, right - left, bottom - top);
    } else if (scale > 1) {
	CopyBits::copy_enlarged(scale, &pm, image, colormap != g_colormap, false, dithering,
				 top, left, bottom, right);
	int TOP = top * scale;
	int BOTTOM = bottom * scale;
	int LEFT = left * scale;
	int RIGHT = right * scale;
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  LEFT, TOP, LEFT, TOP,
		  RIGHT - LEFT, BOTTOM - TOP);
    } else {
	CopyBits::copy_reduced(-scale, &pm, image, colormap != g_colormap, false, dithering,
				top, left, bottom, right);
	int s = -scale;
	int TOP = top / s;
	int BOTTOM = (bottom + s - 1) / s;
	if (BOTTOM > image->height)
	    BOTTOM = image->height;
	int LEFT = left / s;
	int RIGHT = (right + s - 1) / s;
	if (RIGHT > image->width)
	    RIGHT = image->width;
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  LEFT, TOP, LEFT, TOP,
		  RIGHT - LEFT, BOTTOM - TOP);
    }

    if (selection_visible) {
	int s_top, s_left, s_bottom, s_right;
	if (sel_x1 > sel_x2) {
	    s_left = sel_x2;
	    s_right = sel_x1;
	} else {
	    s_left = sel_x1;
	    s_right = sel_x2;
	}
	if (sel_y1 > sel_y2) {
	    s_top = sel_y2;
	    s_bottom = sel_y1;
	} else {
	    s_top = sel_y1;
	    s_bottom = sel_y2;
	}
	if (top <= s_bottom && s_top <= bottom
		&& left <= s_right && s_left <= right)
	    draw_selection();
    }
}

/* public */ void
Viewer::get_recommended_size(int *width, int *height) {
    // Task bar
    int taskbar_width, taskbar_height;
    getTaskBarSize(&taskbar_width, &taskbar_height);

    // Outer decor (WM decorations)
    int outer_decor_width, outer_decor_height;
    getDecorSize(&outer_decor_width, &outer_decor_height);

    *width = XWidthOfScreen(g_screen) - taskbar_width - inner_decor_width
			    - outer_decor_width;
    *height = XHeightOfScreen(g_screen) - taskbar_height - inner_decor_height
			      - outer_decor_height;
}

/* public */ void
Viewer::get_screen_size(int *width, int *height) {
    *width = XWidthOfScreen(g_screen);
    *height = XHeightOfScreen(g_screen);
}

/* public */ void
Viewer::get_selection(int *x, int *y, int *width, int *height) {
    if (selection_visible) {
	if (sel_x1 < sel_x2) {
	    *x = sel_x1;
	    *width = sel_x2 - sel_x1;
	} else {
	    *x = sel_x2;
	    *width = sel_x1 - sel_x2;
	}
	if (sel_y1 < sel_y2) {
	    *y = sel_y1;
	    *height = sel_y2 - sel_y1;
	} else {
	    *y = sel_y2;
	    *height = sel_y1 - sel_y2;
	}
    } else {
	*x = *y = *width = *height = -1;
    }
}

/* public */ int
Viewer::get_scale() {
    return scale;
}

/* private */ void
Viewer::draw_selection() {
    if (gc == None)
	gc = XCreateGC(g_display, g_rootwindow, 0, NULL);
    int s_top, s_left, s_bottom, s_right;
    if (sel_x1 > sel_x2) {
	s_left = sel_x2;
	s_right = sel_x1;
    } else {
	s_left = sel_x1;
	s_right = sel_x2;
    }
    if (sel_y1 > sel_y2) {
	s_top = sel_y2;
	s_bottom = sel_y1;
    } else {
	s_top = sel_y1;
	s_bottom = sel_y2;
    }
    int x, y, w, h;
    if (scale <= 1) {
	if (scale == 1) {
	    x = s_left;
	    y = s_top;
	    w = s_right - s_left;
	    h = s_bottom - s_top;
	} else {
	    int s = -scale;
	    x = s_left / s;
	    y = s_top / s;
	    w = (s_right / s) - x;
	    h = (s_bottom / s) - y;
	}
	XSetLineAttributes(g_display, gc, 1, LineOnOffDash, CapButt, JoinMiter);
	XSetDashes(g_display, gc, 0, "\004\004", 2);
	XSetForeground(g_display, gc, g_white);
	XDrawRectangle(g_display, XtWindow(drawingarea), gc, x, y, w, h);
	XSetDashes(g_display, gc, 4, "\004\004", 2);
	XSetForeground(g_display, gc, g_black);
	XDrawRectangle(g_display, XtWindow(drawingarea), gc, x, y, w, h);
    } else {
	x = s_left * scale + scale / 2;
	y = s_top * scale + scale / 2;
	w = (s_right - s_left) * scale;
	h = (s_bottom - s_top) * scale;
	XSetLineAttributes(g_display, gc, scale, LineOnOffDash, CapButt, JoinMiter);
	char dashes[2];
	dashes[0] = dashes[1] = 4 * scale;
	XSetDashes(g_display, gc, 0, dashes, 2);
	XSetForeground(g_display, gc, g_white);
	XDrawRectangle(g_display, XtWindow(drawingarea), gc, x, y, w, h);
	XSetDashes(g_display, gc, 4 * scale, dashes, 2);
	XSetForeground(g_display, gc, g_black);
	XDrawRectangle(g_display, XtWindow(drawingarea), gc, x, y, w, h);
    }
}

/* private */ void
Viewer::erase_selection() {
    int s_top, s_left, s_bottom, s_right;
    if (sel_x1 > sel_x2) {
	s_left = sel_x2;
	s_right = sel_x1;
    } else {
	s_left = sel_x1;
	s_right = sel_x2;
    }
    if (sel_y1 > sel_y2) {
	s_top = sel_y2;
	s_bottom = sel_y1;
    } else {
	s_top = sel_y1;
	s_bottom = sel_y2;
    }
    int x, y, w, h;
    if (scale <= 1) {
	if (scale == 1) {
	    x = s_left;
	    y = s_top;
	    w = s_right - s_left + 1;
	    h = s_bottom - s_top + 1;
	} else {
	    int s = -scale;
	    x = s_left / s;
	    y = s_top / s;
	    w = (s_right / s) - x + 1;
	    h = (s_bottom / s) - y + 1;
	}
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x, y, x, y, w, 1);
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x + w - 1, y + 1, x + w - 1, y + 1, 1, h - 1);
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x, y + 1, x, y + 1, 1, h - 1);
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x + 1, y + h - 1, x + 1, y + h - 1, w - 2, 1);
    } else {
	x = s_left * scale;
	y = s_top * scale;
	w = (s_right - s_left + 1) * scale;
	h = (s_bottom - s_top + 1) * scale;
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x, y, x, y, w, scale);
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x + w - scale, y + scale, x + w - scale, y + scale, scale, h - scale);
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x, y + scale, x, y + scale, scale, h - scale);
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  x + scale, y + h - scale, x + scale, y + h - scale, w - 2, scale);
    }
}

/* private */ void
Viewer::screen2pixmap(int *x, int *y) {
    int X = *x;
    int Y = *y;
    if (scale <= -1) {
	X *= -scale;
	Y *= -scale;
    } else if (scale > 1) {
	X /= scale;
	Y /= scale;
    }
    if (X < 0)
	X = 0;
    else if (X >= pm.width)
	X = pm.width - 1;
    if (Y < 0)
	Y = 0;
    else if (Y >= pm.height)
	Y = pm.height - 1;
    *x = X;
    *y = Y;
}

/* private */ void
Viewer::pixmap2screen(int *x, int *y) {
    int X = *x;
    int Y = *y;
    if (scale <= -1) {
	X /= -scale;
	Y /= -scale;
    } else if (scale > 1) {
	X *= scale;
	Y *= scale;
    }
    *x = X;
    *y = Y;
}

/* public */ void
Viewer::colormapChanged() {
    if (colormap == g_colormap) {
	paint(0, 0, pm.height, pm.width);
    } else {
	XColor colors[256];
	if (pm.depth == 1) {
	    for (int i = 0; i < 256; i++) {
		colors[i].pixel = i;
		int k = 257 * i;
		colors[i].red = k;
		colors[i].green = k;
		colors[i].blue = k;
		colors[i].flags = DoRed | DoGreen | DoBlue;
	    }
	} else if (pm.depth == 8) {
	    for (int i = 0; i < 256; i++) {
		colors[i].pixel = i;
		colors[i].red = 257 * pm.cmap[i].r;
		colors[i].green = 257 * pm.cmap[i].g;
		colors[i].blue = 257 * pm.cmap[i].b;
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
		    int p = 13107 * m + (4369 * n) / 3;
		    colors[i].pixel = i;
		    colors[i].red = p;
		    colors[i].green = p;
		    colors[i].blue = p;
		    colors[i].flags = DoRed | DoGreen | DoBlue;
		    i++;
		}
	}
	XStoreColors(g_display, colormap, colors, 256);
    }
}

class SaveBeforeClosingListener : public SaveImageDialog::Listener {
    private:
	Viewer *viewer;
	SaveImageDialog *sid;
    public:
	SaveBeforeClosingListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setSID(SaveImageDialog *sid) {
	    this->sid = sid;
	}
	virtual void save(const char *filename, const char *filetype) {
	    delete sid;
	    if (viewer->save(filename, filetype)) {
		delete viewer;
	    }
	    delete this;
	}
	virtual void cancel() {
	    delete sid;
	    delete this;
	}
};
		
class YesNoCancelBeforeClosingListener : public YesNoCancelDialog::Listener {
    private:
	Viewer *viewer;
	YesNoCancelDialog *ync;
    public:
	YesNoCancelBeforeClosingListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setYNC(YesNoCancelDialog *ync) {
	    this->ync = ync;
	}
	virtual void yes() {
	    delete ync;
	    if (viewer->filename == NULL) {
		SaveBeforeClosingListener *sidlistener = new SaveBeforeClosingListener(viewer);
		SaveImageDialog *sid = new SaveImageDialog(
			viewer,
			viewer->filename,
			viewer->filetype,
			sidlistener);
		sidlistener->setSID(sid);
		sid->setTitle("Save File");
		sid->setDirectory(&viewer->file_directory);
		sid->raise();
	    } else {
		if (viewer->save(viewer->filename, viewer->filetype))
		    delete viewer;
	    }
	    delete this;
	}
	virtual void no() {
	    delete ync;
	    delete viewer;
	    delete this;
	}
	virtual void cancel() {
	    delete ync;
	    delete this;
	}
};

/* private virtual */ void
Viewer::close() {
    if (!dirty && undomanager->getCurrentId() == saved_undo_id) {
	delete this;
	return;
    }
    YesNoCancelBeforeClosingListener *ynclistener = new YesNoCancelBeforeClosingListener(this);
    YesNoCancelDialog *ync = new YesNoCancelDialog(
	    this,
	    "Save changes before closing?",
	    ynclistener);
    ynclistener->setYNC(ync);
    ync->setTitle("Save Changes?");
    ync->raise();
}

/* private */ bool
Viewer::canIDoDirectCopy() {
    if (scale != 1)
	return false;
    if (pm.depth == 1)
	return image->bytes_per_line == pm.bytesperline
		&& image->byte_order == LSBFirst
		&& image->bitmap_bit_order == LSBFirst;
    if (pm.depth == 8)
	return colormap != g_colormap // implies 8-bit PseudoColor visual
		&& image->bytes_per_line == pm.bytesperline
		&& image->byte_order == LSBFirst;
    // pm.depth == 24
    return image->depth == 32
		&& image->bytes_per_line == pm.bytesperline
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
    XtSetArg(args[0], XmNwidth, &pm.width);
    XtSetArg(args[1], XmNheight, &pm.height);
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
    if (selection_visible)
	draw_selection();
}

/* private static */ void
Viewer::input(Widget w, XtPointer ud, XtPointer cd) {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    ((Viewer *) ud)->input2(cbs->event);
}

/* private */ void
Viewer::input2(XEvent *event) {
    // TODO: xv-like use of cursor keys to move or resize the selection.
    // Also, establishing a selection by double-clicking; getting rid of a
    // selection by double-clicking inside it; moving a selection by dragging
    // inside it; changing a selection by dragging its edges... Ugh...
    switch (event->type) {
	case KeyPress:
	    // event->xkey.state, event->xkey.keycode
	    break;
	case KeyRelease:
	    // event->xkey.state, event->xkey.keycode
	    break;
	case ButtonPress: {
	    // event->xbutton.x, event->xbutton.y,
	    // event->xbutton.state, event->xbutton.button
	    if (selection_visible)
		erase_selection();
	    int x = event->xbutton.x;
	    int y = event->xbutton.y;
	    screen2pixmap(&x, &y);
	    sel_x1 = sel_x2 = x;
	    sel_y1 = sel_y2 = y;
	    draw_selection();
	    selection_visible = true;
	    selection_in_progress = true;
	    if (cme != NULL)
		cme->selectColor(pm.pixels[y * pm.bytesperline + x]);
	    break;
	}
	case ButtonRelease: {
	    // event->xbutton.x, event->xbutton.y,
	    // event->xbutton.state, event->xbutton.button
	    if (!selection_in_progress)
		return;
	    int x = event->xbutton.x;
	    int y = event->xbutton.y;
	    screen2pixmap(&x, &y);
	    if (x == sel_x1 || y == sel_y1) {
		erase_selection();
		selection_visible = false;
	    } else if (sel_x2 != x || sel_y2 != y) {
		erase_selection();
		sel_x2 = x;
		sel_y2 = y;
		draw_selection();
	    }
	    selection_in_progress = false;
	    if (cme != NULL) {
		cme->extendSelection(pm.pixels[y * pm.bytesperline + x]);
		cme->finishSelection();
	    }
	    break;
	}
	case MotionNotify: {
	    // event->xmotion.x, event->xmotion.y
	    if (!selection_in_progress)
		return;
	    int x = event->xbutton.x;
	    int y = event->xbutton.y;
	    screen2pixmap(&x, &y);
	    if (sel_x2 != x || sel_y2 != y) {
		erase_selection();
		sel_x2 = x;
		sel_y2 = y;
		draw_selection();
	    }
	    if (cme != NULL)
		cme->extendSelection(pm.pixels[y * pm.bytesperline + x]);
	    break;
	}
    }
}

/* private static */ void
Viewer::menucallback(void *closure, const char *id) {
    ((Viewer *) closure)->menucallback2(id);
}

/* private */ void
Viewer::menucallback2(const char *id) {
    if (strcmp(id, "File.Beep") == 0)
	beep();
    else if (strncmp(id, "File.New.", 9) == 0)
	doNew(id + 9);
    else if (strcmp(id, "File.Clone") == 0)
	doClone();
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
    else if (strcmp(id, "Edit.Redo") == 0)
	doRedo();
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
    else if (strncmp(id, "Windows.X.", 10) == 0)
	doWindows(id + 10);
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

/* private */ bool
Viewer::save(const char *name, const char *type) {
    const char *plugin_name = plugin->name();
    void *plugin_data;
    int plugin_data_length;
    if (strcmp(plugin_name, "Null") == 0) {
	plugin_data = NULL;
	plugin_data_length = 0;
    } else
	plugin->serialize(&plugin_data, &plugin_data_length);
    char *message = NULL;

    bool success;
    if (ImageIO::swrite(name, type, plugin_name, plugin_data,
			 plugin_data_length, &pm, &message)) {
	if (filename == NULL || strcmp(name, filename) != 0) {
	    char buf[1024];
	    snprintf(buf, 1024, "Windows.X.%d", id);
	    Iterator *iter = instances->iterator();
	    char *label = basename(name);
	    while (iter->hasNext()) {
		Viewer *v = (Viewer *) iter->next();
		v->windowsmenu->changeLabel(buf, label);
	    }
	    delete iter;
	    snprintf(buf, 1024, "%s (%s)", label, plugin->name());
	    setTitle(buf);
	    setIconTitle(buf);
	    free(label);
	}
	if (filename != NULL)
	    free(filename);
	filename = strclone(name);
	if (filetype != NULL)
	    free(filetype);
	filetype = strclone(type);
	success = true;
    } else {
	char buf[1024];
	snprintf(buf, 1024, "Saving \"%s\" failed (%s).\n", name, message);
	TextViewer *tv = new TextViewer(buf);
	tv->raise();
	success = false;
    }

    if (plugin_data != NULL)
	free(plugin_data);
    if (message != NULL)
	free(message);
    return success;
}

class LoadColorsAction : public UndoableAction {
    private:
	ColormapEditorListener *listener;
	FWPixmap *pm;
	FWColor oldcolors[256];
	FWColor newcolors[256];
    public:
	LoadColorsAction(ColormapEditorListener *listener, FWPixmap *pm, FWColor *nc) {
	    this->listener = listener;
	    this->pm = pm;
	    for (int i = 0; i < 256; i++) {
		oldcolors[i] = pm->cmap[i];
		newcolors[i] = nc[i];
	    }
	}
	virtual void undo() {
	    for (int i = 0; i < 256; i++)
		pm->cmap[i] = oldcolors[i];
	    ColormapEditor *cme = listener->getCME();
	    if (cme != NULL)
		cme->colorsChanged(0, 255);
	    listener->colormapChanged();
	}
	virtual void redo() {
	    for (int i = 0; i < 256; i++)
		pm->cmap[i] = newcolors[i];
	    ColormapEditor *cme = listener->getCME();
	    if (cme != NULL)
		cme->colorsChanged(0, 255);
	    listener->colormapChanged();
	}
	virtual const char *getUndoTitle() {
	    return "Undo Load Colors";
	}
	virtual const char *getRedoTitle() {
	    return "Redo Load Colors";
	}
};

/* private */ void
Viewer::doNew(const char *pluginname) {
    new Viewer(pluginname);
}

/* private */ void
Viewer::doClone() {
    new Viewer(plugin);
}

class OpenListener : public FileDialog::Listener {
    private:
	Viewer *viewer;
	FileDialog *dialog;
    public:
	OpenListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setDialog(FileDialog *dialog) {
	    this->dialog = dialog;
	}
	virtual void fileSelected(const char *filename) {
	    char *type = NULL;
	    char *plugin_name = NULL;
	    void *plugin_data = NULL;
	    int plugin_data_length;
	    FWPixmap pm;
	    char *message = NULL;
	    if (ImageIO::sread(filename, &type, &plugin_name, &plugin_data,
			    &plugin_data_length, &pm, &message)) {
		// Read successful; open viewer
		new Viewer(plugin_name, plugin_data,
			plugin_data_length, &pm,
			filename, type);
		if (message != NULL) {
		    TextViewer *tv = new TextViewer(message);
		    tv->raise();
		    beep();
		}
	    } else {
		// TODO: nicer error reporting
		char buf[1024];
		snprintf(buf, 1024, "Can't open \"%s\" (%s).\n", filename, message);
		TextViewer *tv = new TextViewer(buf);
		tv->raise();
		beep();
	    }
	    if (type != NULL)
		free(type);
	    if (plugin_name != NULL)
		free(plugin_name);
	    if (plugin_data != NULL)
		free(plugin_data);
	    if (message != NULL)
		free(message);
	    delete dialog;
	    delete this;
	}
	virtual void cancelled() {
	    delete dialog;
	    delete this;
	}
};

/* private */ void
Viewer::doOpen() {
    OpenListener *listener = new OpenListener(this);
    FileDialog *opendialog = new FileDialog(NULL, listener);
    listener->setDialog(opendialog);
    opendialog->setTitle("Open File");
    opendialog->setIconTitle("Open File");
    opendialog->setDirectory(&file_directory);
    opendialog->raise();
}

/* private */ void
Viewer::doClose() {
    close();
}

/* private */ void
Viewer::doSave() {
    if (filename == NULL)
	doSaveAs();
    else
	save(filename, filetype);
}

class SaveAsListener : public SaveImageDialog::Listener {
    private:
	Viewer *viewer;
	SaveImageDialog *sid;
    public:
	SaveAsListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setSID(SaveImageDialog *sid) {
	    this->sid = sid;
	}
	virtual void save(const char *filename, const char *filetype) {
	    delete sid;
	    viewer->save(filename, filetype);
	    delete this;
	}
	virtual void cancel() {
	    delete sid;
	    delete this;
	}
};


/* private */ void
Viewer::doSaveAs() {
    SaveAsListener *sidlistener = new SaveAsListener(this);
    SaveImageDialog *sid = new SaveImageDialog(this, filename,
					       filetype, sidlistener);
    sidlistener->setSID(sid);
    sid->setTitle("Save File");
    sid->setDirectory(&file_directory);
    sid->raise();
}

/* private */ void
Viewer::doGetInfo() {
    char *info = plugin->dumpSettings();
    if (info == NULL) {
	char buf[1024];
	snprintf(buf, 1024, "The plugin \"%s\" does not have public settings.", plugin->name());
	TextViewer *tv = new TextViewer(buf);
	tv->raise();
    } else {
	TextViewer *tv = new TextViewer(info);
	free(info);
	tv->raise();
    }
}

/* private */ void
Viewer::doPrint() {
    TextViewer *tv = new TextViewer("Printing is not yet implemented.\nAnd, until I feel like spending the money to buy a PostScript manual\n(and read it and all that!), it won't be, either.\nUnless YOU wish to donate some code? :-)");
    tv->raise();
}

/* private */ void
Viewer::doQuit() {
    doQuit2();
}

class SaveBeforeQuittingListener : public SaveImageDialog::Listener {
    private:
	Viewer *viewer;
	SaveImageDialog *sid;
    public:
	SaveBeforeQuittingListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setSID(SaveImageDialog *sid) {
	    this->sid = sid;
	}
	virtual void save(const char *filename, const char *filetype) {
	    delete sid;
	    if (viewer->save(filename, filetype)) {
		delete viewer;
		Viewer::doQuit2();
	    }
	    delete this;
	}
	virtual void cancel() {
	    delete sid;
	    delete this;
	}
};

		
class YesNoCancelBeforeQuittingListener : public YesNoCancelDialog::Listener {
    private:
	Viewer *viewer;
	YesNoCancelDialog *ync;
    public:
	YesNoCancelBeforeQuittingListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setYNC(YesNoCancelDialog *ync) {
	    this->ync = ync;
	}
	virtual void yes() {
	    delete ync;
	    if (viewer->filename == NULL) {
		SaveBeforeQuittingListener *sidlistener = new SaveBeforeQuittingListener(viewer);
		SaveImageDialog *sid = new SaveImageDialog(
			viewer,
			viewer->filename,
			viewer->filetype,
			sidlistener);
		sidlistener->setSID(sid);
		sid->setTitle("Save File");
		sid->setDirectory(&viewer->file_directory);
		viewer->raise();
		sid->raise();
		return;
	    } else {
		if (viewer->save(viewer->filename, viewer->filetype)) {
		    delete viewer;
		    Viewer::doQuit2();
		}
	    }
	    delete this;
	}
	virtual void no() {
	    delete ync;
	    delete viewer;
	    Viewer::doQuit2();
	    delete this;
	}
	virtual void cancel() {
	    delete ync;
	    delete this;
	}
};

/* private static */ void
Viewer::doQuit2() {
    while (true) {
	int sz = instances->size();
	if (sz == 0)
	    exit(0);

	// TODO -- it would be nicer to start with the one that's closest
	// to the top of the window stacking order, but that's too much of
	// a pain for me to code right now.

	Viewer *viewer = (Viewer *) instances->get(sz - 1);
	if (!viewer->dirty && viewer->undomanager->getCurrentId()
				== viewer->saved_undo_id) {
	    delete viewer;
	    continue;
	}

	char buf[1024];
	char *bn = viewer->filename == NULL ? (char *) "Untitled"
					    : basename(viewer->filename);
	snprintf(buf, 1024, "Save changes to \"%s\" before quitting?", bn);
	if (viewer->filename != NULL)
	    free(bn);

	YesNoCancelBeforeQuittingListener *ynclistener =
			    new YesNoCancelBeforeQuittingListener(viewer);
	YesNoCancelDialog *ync = new YesNoCancelDialog(viewer,
					    buf, ynclistener);
	ynclistener->setYNC(ync);
	ync->raise();
	return;
    }
}

/* private */ void
Viewer::doUndo() {
    undomanager->undo();
}

/* private */ void
Viewer::doRedo() {
    undomanager->redo();
}

/* private */ void
Viewer::doCut() {
    beep();
}

/* private */ void
Viewer::doCopy() {
    beep();
}

/* private */ void
Viewer::doPaste() {
    beep();
}

/* private */ void
Viewer::doClear() {
    beep();
}

class LoadColorsListener : public FileDialog::Listener {
    private:
	Viewer *viewer;
	FileDialog *dialog;
    public:
	LoadColorsListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setDialog(FileDialog *dialog) {
	    this->dialog = dialog;
	}
	void fileSelected(const char *filename) {
	    FILE *file = fopen(filename, "r");
	    if (file == NULL) {
		char buf[1024];
		snprintf(buf, 1024, "Can't open \"%s\" for reading (%s).\n",
				    filename, strerror(errno));
		TextViewer *tv = new TextViewer(buf);
		tv->raise();
		beep();
		delete dialog;
		delete this;
		return;
	    }

	    FWColor newcmap[256];
	    for (int i = 0; i < 256; i++) {
		int r, g, b;
		if (fscanf(file, "%d %d %d", &r, &g, &b) != 3
			|| r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
		    char buf[1024];
		    snprintf(buf, 1024, "The file \"%s\" is not formatted correctly.\nIt should be a plain text file, containing 256 lines, with\neach line containing three decimal numbers in the range 0..255,\nrepresenting red, green, and blue intensities.", filename);
		    TextViewer *tv = new TextViewer(buf);
		    tv->raise();
		    beep();
		    fclose(file);
		    delete dialog;
		    delete this;
		    return;
		}
		newcmap[i].r = r;
		newcmap[i].g = g;
		newcmap[i].b = b;
	    }

	    if (viewer->cmelistener == NULL)
		viewer->cmelistener = new ColormapEditorListener(viewer);
	    LoadColorsAction *action = new LoadColorsAction(
				    viewer->cmelistener, &viewer->pm, newcmap);
	    viewer->undomanager->addAction(action);
	    action->redo();

	    fclose(file);
	    delete dialog;
	    delete this;
	}
	void cancelled() {
	    delete dialog;
	    delete this;
	}
};

/* private */ void
Viewer::doLoadColors() {
    if (pm.cmap == NULL) {
	beep();
	return;
    }
    LoadColorsListener *listener = new LoadColorsListener(this);
    FileDialog *loaddialog = new FileDialog(NULL, listener);
    listener->setDialog(loaddialog);
    loaddialog->setTitle("Load Colormap");
    loaddialog->setIconTitle("Load Colormap");
    loaddialog->setDirectory(&colormap_directory);
    loaddialog->raise();
}

class SaveColorsListener : public FileDialog::Listener {
    private:
	Viewer *viewer;
	FileDialog *dialog;
    public:
	SaveColorsListener(Viewer *viewer) {
	    this->viewer = viewer;
	}
	void setDialog(FileDialog *dialog) {
	    this->dialog = dialog;
	}
	virtual void fileSelected(const char *filename) {
	    FILE *file = fopen(filename, "w");
	    if (file == NULL) {
		char buf[1024];
		snprintf(buf, 1024, "Can't open \"%s\" for writing (%s).\n",
				    filename, strerror(errno));
		TextViewer *tv = new TextViewer(buf);
		tv->raise();
		beep();
		delete dialog;
		delete this;
		return;
	    }
	    for (int i = 0; i < 256; i++)
		fprintf(file, "%d %d %d\n", viewer->pm.cmap[i].r,
			viewer->pm.cmap[i].g, viewer->pm.cmap[i].b);
	    delete dialog;
	    fclose(file);
	    delete this;
	}
	virtual void cancelled() {
	    delete dialog;
	    delete this;
	}
};

/* private */ void
Viewer::doSaveColors() {
    if (pm.cmap == NULL) {
	beep();
	return;
    }
    SaveColorsListener *listener = new SaveColorsListener(this);
    FileDialog *savedialog = new FileDialog(NULL, listener);
    listener->setDialog(savedialog);
    savedialog->setTitle("Save Colormap");
    savedialog->setIconTitle("Save Colormap");
    savedialog->setDirectory(&colormap_directory);
    savedialog->raise();
}

/* private */ void
Viewer::doEditColors() {
    if (pm.depth != 8) {
	beep();
	return;
    }
    if (cmelistener == NULL)
	cmelistener = new ColormapEditorListener(this);
    if (cme == NULL)
	cme = new ColormapEditor(cmelistener, &pm, undomanager, colormap);
    cme->raise();
}

/* private */ void
Viewer::doStopOthers() {
    Iterator *iter = instances->iterator();
    while (iter->hasNext()) {
	Viewer *viewer = (Viewer *) iter->next();
	if (viewer != this && !viewer->finished)
	    viewer->plugin->stop();
    }
    delete iter;
}

/* private */ void
Viewer::doStop() {
    if (finished)
	beep();
    else
	plugin->stop();
}

/* private */ void
Viewer::doStopAll() {
    Iterator *iter = instances->iterator();
    while (iter->hasNext()) {
	Viewer *viewer = (Viewer *) iter->next();
	if (!viewer->finished)
	    viewer->plugin->stop();
    }
    delete iter;
}

/* private */ void
Viewer::doContinue() {
    if (finished)
	beep();
    else {
	if (plugin->restart()) {
	    finished = true;
	    plugin->setFinished();
	} else
	    dirty = true;
    }
}

/* private */ void
Viewer::doContinueAll() {
    Iterator *iter = instances->iterator();
    while (iter->hasNext()) {
	Viewer *viewer = (Viewer *) iter->next();
	if (!viewer->finished) {
	    if (viewer->plugin->restart()) {
		viewer->finished = true;
		viewer->plugin->setFinished();
	    } else
		viewer->dirty = true;
	}
    }
    delete iter;
}

/* private */ void
Viewer::doUpdateNow() {
    paint(0, 0, pm.height, pm.width);
}

/* private */ void
Viewer::doPrivateColormap(bool value) {
    if (!(g_depth == 8
		&& g_visual->c_class == PseudoColor
		&& (pm.depth == 8 || pm.depth == 24 || (pm.depth == 1 && scale <= -1))))
	return;

    if (value) {
	colormap = XCreateColormap(g_display, g_rootwindow, g_visual, AllocAll);
	setColormap(colormap);
	if (canIDoDirectCopy()) {
	    free(image->data);
	    image->data = (char *) pm.pixels;
	    direct_copy = true;
	    if (g_verbosity >= 1)
		fprintf(stderr, "Using direct copy.\n");
	}
	colormapChanged();
	if (cme != NULL)
	    cme->colormapChanged(colormap);
	paint(0, 0, pm.height, pm.width);
    } else {
	XFreeColormap(g_display, colormap);
	colormap = g_colormap;
	setColormap(g_colormap);
	if (direct_copy) {
	    image->data = (char *) malloc(image->bytes_per_line * image->height);
	    if (g_verbosity >= 1 && direct_copy)
		fprintf(stderr, "Not using direct copy.\n");
	    direct_copy = false;
	}
	colormapChanged();
	if (cme != NULL)
	    cme->colormapChanged(colormap);
    }
}

/* private */ void
Viewer::doDither(bool value) {
    dithering = value;
    paint(0, 0, pm.height, pm.width);
}

/* private */ void
Viewer::doNotify(bool) {
    // There's actually nothing to do here. pluginFinished() checks the
    // "Options.Notify" toggle, but changing the state of the toggle does not
    // cause anything to happen immediately.
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
	beep();
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
	beep();
}

/* private */ void
Viewer::doWindows(const char *sid) {
    int nid;
    if (sscanf(sid, "%d", &nid) != 1)
	beep();
    else {
	Iterator *iter = instances->iterator();
	while (iter->hasNext()) {
	    Viewer *viewer = (Viewer *) iter->next();
	    if (viewer->id == nid) {
		delete iter;
		viewer->raise();
		return;
	    }
	}
	delete iter;
	beep();
    }
}

/* private */ void
Viewer::doScale(const char *value) {
    sscanf(value, "%d", &scale);

    // Compute scaled image size
    int w, h;
    if (scale > 0) {
	w = pm.width * scale;
	h = pm.height * scale;
    } else {
	int s = -scale;
	w = (pm.width + s - 1) / s;
	h = (pm.height + s - 1) / s;
    }

    // Resize the DrawingArea
    Arg args[2];
    XtSetArg(args[0], XmNwidth, w);
    XtSetArg(args[1], XmNheight, h);
    XtSetValues(drawingarea, args, 2);

    // Resize top-level window to fit
    setSize(w + inner_decor_width, h + inner_decor_height);
    fitToScreen();

    if (!direct_copy)
	free(image->data);
    XFree(image);
    bool use_bitmap = pm.depth == 1 && scale >= 1;
    image = XCreateImage(g_display,
			 g_visual,
			 use_bitmap ? 1 : g_depth,
			 use_bitmap ? XYBitmap : ZPixmap,
			 0,
			 NULL,
			 w,
			 h,
			 32,
			 0);

    if (pm.depth == 1) {
	bool want_priv_cmap = scale <= -1
		&& optionsmenu->getToggleValue("Options.PrivateColormap");
	bool have_priv_cmap = colormap != g_colormap;
	
	if (want_priv_cmap && !have_priv_cmap) {
	    colormap = XCreateColormap(g_display, g_rootwindow, g_visual, AllocAll);
	    setColormap(colormap);
	    colormapChanged();
	} else if (!want_priv_cmap && have_priv_cmap) {
	    XFreeColormap(g_display, colormap);
	    colormap = g_colormap;
	    setColormap(g_colormap);
	}
    }
    
    bool old_direct_copy = direct_copy;
    direct_copy = canIDoDirectCopy();
    if (direct_copy)
	image->data = (char *) pm.pixels;
    else
	image->data = (char *) malloc(image->bytes_per_line * image->height);
    if (g_verbosity >= 1 && old_direct_copy != direct_copy)
	if (direct_copy)
	    fprintf(stderr, "Using direct copy.\n");
	else
	    fprintf(stderr, "Not using direct copy.\n");

    paint(0, 0, pm.height, pm.width);
}

/* private */ void
Viewer::doGeneral() {
    TextViewer *tv = new TextViewer(helptext);
    tv->raise();
}

/* private */ void
Viewer::doHelp(const char *id) {
    Plugin *plugin = Plugin::get(id);
    if (plugin == NULL) {
	char buf[1024];
	snprintf(buf, 1024, "The plugin \"%s\" could not be loaded.\n"
			    "Please check $HOME/.fw to see if it is missing.",
			    id);
	TextViewer *tv = new TextViewer(buf);
	tv->raise();
    } else {
	const char *help = plugin->help();
	if (help == NULL) {
	    char buf[1024];
	    snprintf(buf, 1024, "The plugin \"%s\" does not provide help.", id);
	    TextViewer *tv = new TextViewer(buf);
	    tv->raise();
	} else {
	    TextViewer *tv = new TextViewer(help);
	    tv->raise();
	}
	Plugin::release(plugin);
    }
}
