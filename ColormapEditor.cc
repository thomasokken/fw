#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <stdlib.h>

#include "ColormapEditor.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "UndoManager.h"
#include "main.h"
#include "util.h"

#define CELL_SIZE 16
#define CELL_SPACING 2
#define CELL_HIGHLIGHT 1

#define BTN_PICK    1
#define BTN_BLEND   2
#define BTN_SWAP    3
#define BTN_MIX     4
#define BTN_COPY    5
#define BTN_PASTE   6
#define BTN_LOAD    7
#define BTN_SAVE    8
#define BTN_UNDO    9
#define BTN_REDO   10
#define BTN_OK     11
#define BTN_CANCEL 12


/* public */
ColormapEditor::ColormapEditor(Owner *owner, FWPixmap *pm)
				    : Frame(false, true, false) {
    setTitle("Colormap Editor");
    setIconTitle("CME");

    this->owner = owner;
    this->pm = pm;

    undomgr = new UndoManager;

    backup_cmap = new FWColor[256];
    for (int i = 0; i < 256; i++)
	backup_cmap[i] = pm->cmap[i];

    Widget form = getContainer();

    imagesize = 16 * CELL_SIZE + 17 * CELL_SPACING;

    XtTranslations translations = XtParseTranslationTable(
		"<KeyDown>: DrawingAreaInput()\n"
		"<KeyUp>: DrawingAreaInput()\n"
		"<BtnDown>: DrawingAreaInput()\n"
		"<BtnUp>: DrawingAreaInput()\n"
		"<Motion>: DrawingAreaInput()\n");

    drawingarea = XtVaCreateManagedWidget(
	    "DrawingArea",
	    xmDrawingAreaWidgetClass,
	    form,
	    XmNwidth, imagesize,
	    XmNheight, imagesize,
	    XmNtranslations, translations,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);

    XtAddCallback(drawingarea, XmNexposeCallback, expose, (XtPointer) this);
    XtAddCallback(drawingarea, XmNinputCallback, input, (XtPointer) this);

    Widget rowcolumn = XtVaCreateManagedWidget(
	    "ButtonGrid",
	    xmRowColumnWidgetClass,
	    form,
	    XmNnumColumns, 2,
	    XmNpacking, XmPACK_COLUMN,
	    XmNentryAlignment, XmALIGNMENT_CENTER,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, drawingarea,
	    NULL);

    addButton(rowcolumn, "Pick", BTN_PICK);
    addButton(rowcolumn, "Swap", BTN_SWAP);
    addButton(rowcolumn, "Copy", BTN_COPY);
    addButton(rowcolumn, "Load", BTN_LOAD);
    addButton(rowcolumn, "Undo", BTN_UNDO);
    addButton(rowcolumn, "OK", BTN_OK);

    addButton(rowcolumn, "Blend", BTN_BLEND);
    addButton(rowcolumn, "Mix", BTN_MIX);
    addButton(rowcolumn, "Paste", BTN_PASTE);
    addButton(rowcolumn, "Save", BTN_SAVE);
    addButton(rowcolumn, "Redo", BTN_REDO);
    addButton(rowcolumn, "Cancel", BTN_CANCEL);

    sel_in_progress = false;
    sel_start = sel_end = -1;

    XGCValues values;
    values.foreground = g_white;
    values.background = g_black;

    image = XCreateImage(g_display, g_visual, g_depth, ZPixmap, 0, NULL,
			 imagesize, imagesize, 32, 0);
    image->data = (char *) malloc(image->bytes_per_line * image->height);

    colormap = g_colormap;

    update_image();
}

/* public virtual */
ColormapEditor::~ColormapEditor() {
    // Of course we're not deleting the owner itself, we're deleting
    // its proxy; this proxy should have a destructor that takes care
    // of nulling out any references the owner might have to us, thus
    // severing the bonds in both directions.
    delete owner;
    delete[] backup_cmap;
    free(image->data);
    XFree(image);
    delete undomgr;
}

/* public virtual */ void
ColormapEditor::colormapChanged(Colormap colormap) {
    Colormap prevcolormap = this->colormap;
    this->colormap = colormap;
    if (colormap != prevcolormap) {
	setColormap(colormap == None ? g_colormap : colormap);
    }
    if (colormap == None || colormap != prevcolormap) {
	update_image();
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  0, 0, 0, 0, imagesize, imagesize);
    }

    // NOTE: we don't assume responsibility for this colormap. When our
    // owning Viewer is destroyed, it should make sure we are destroyed
    // also, and then free the private colormap.
}

/* private */ void
ColormapEditor::doPick() {
    //
}

/* private */ void
ColormapEditor::doBlend() {
    //
}

/* private */ void
ColormapEditor::doSwap() {
    //
}

/* private */ void
ColormapEditor::doMix() {
    //
}

/* private */ void
ColormapEditor::doCopy() {
    //
}

/* private */ void
ColormapEditor::doPaste() {
    //
}

/* private */ void
ColormapEditor::doLoad() {
    owner->loadColors();
}

/* private */ void
ColormapEditor::doSave() {
    owner->saveColors();
}

/* private */ void
ColormapEditor::doUndo() {
    //
}

/* private */ void
ColormapEditor::doRedo() {
    //
}

/* private */ void
ColormapEditor::doOK() {
    close();
}

/* private */ void
ColormapEditor::doCancel() {
    for (int i = 0; i < 256; i++)
	pm->cmap[i] = backup_cmap[i];
    owner->colormapChanged();
    close();
}

/* private */ void
ColormapEditor::update_image() {
    update_image_gridlines();
    for (int i = 0; i < 256; i++)
	update_cell(i);
}

/* private */ void
ColormapEditor::update_image_gridlines() {
    Arg arg;
    unsigned long background;
    XtSetArg(arg, XmNbackground, &background);
    XtGetValues(drawingarea, &arg, 1);
    for (int i = 0; i < imagesize; i += CELL_SIZE + CELL_SPACING)
	for (int x = i; x < i + CELL_SPACING; x++)
	    for (int y = 0; y < imagesize; y++) {
		XPutPixel(image, x, y, background);
		XPutPixel(image, y, x, background);
	    }
}

/* private */ void
ColormapEditor::select_cell(int c) {
    int x = (c & 15) * (CELL_SIZE + CELL_SPACING) + CELL_SPACING;
    int y = (c >> 4) * (CELL_SIZE + CELL_SPACING) + CELL_SPACING;
    for (int i = 0; i < CELL_HIGHLIGHT; i++) {
	int xo = x + i;
	int yo = y + i;
	int so = CELL_SIZE - 2 * i;
	for (int xx = xo; xx < xo + so; xx++) {
	    XPutPixel(image, xx, yo, g_black);
	    XPutPixel(image, xx, yo + so - 1, g_black);
	}
	for (int yy = yo + 1; yy < yo + so - 1; yy++) {
	    XPutPixel(image, xo, yy, g_black);
	    XPutPixel(image, xo + so - 1, yy, g_black);
	}
	int xi = x + i + CELL_HIGHLIGHT;
	int yi = y + i + CELL_HIGHLIGHT;
	int si = CELL_SIZE - 2 * (i + CELL_HIGHLIGHT);
	for (int xx = xi; xx < xi + si; xx++) {
	    XPutPixel(image, xx, yi, g_white);
	    XPutPixel(image, xx, yi + si - 1, g_white);
	}
	for (int yy = yi + 1; yy < yi + si - 1; yy++) {
	    XPutPixel(image, xi, yy, g_white);
	    XPutPixel(image, xi + si - 1, yy, g_white);
	}
    }
}

/* private */ void
ColormapEditor::update_cell(int c) {
    int x = (c & 15) * (CELL_SIZE + CELL_SPACING) + CELL_SPACING;
    int y = (c >> 4) * (CELL_SIZE + CELL_SPACING) + CELL_SPACING;
    if (colormap != g_colormap) {
	for (int xx = x; xx < x + CELL_SIZE; xx++)
	    for (int yy = y; yy < y + CELL_SIZE; yy++)
		XPutPixel(image, xx, yy, c);
    } else {
	// TODO: Generate halftone patterns (1 for grayscale, 3 for RGB,
	// yada yada yada...)
	// Just picking closest match for now.
	unsigned long pixel = rgb2pixel(pm->cmap[c].r,
					pm->cmap[c].g,
					pm->cmap[c].b);
	for (int xx = x; xx < x + CELL_SIZE; xx++)
	    for (int yy = y; yy < y + CELL_SIZE; yy++)
		XPutPixel(image, xx, yy, pixel);
    }
}

/* private */ void
ColormapEditor::addButton(Widget parent, const char *label, int id) {
    XmString s = XmStringCreateLocalized((char *) label);
    Widget button = XtVaCreateManagedWidget(
	    label,
	    xmPushButtonWidgetClass,
	    parent,
	    XmNlabelString, s,
	    XmNuserData, (XtPointer) id,
	    NULL);
    XmStringFree(s);
    XtAddCallback(button, XmNactivateCallback, activate, (XtPointer) this);
}

/* private static */ void
ColormapEditor::expose(Widget w, XtPointer ud, XtPointer cd) {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    XExposeEvent ev = cbs->event->xexpose;
    ColormapEditor *This = (ColormapEditor *) ud;
    XPutImage(g_display, XtWindow(This->drawingarea), g_gc, This->image,
	      ev.x, ev.y, ev.x, ev.y, ev.width, ev.height);
}

/* private static */ void
ColormapEditor::input(Widget w, XtPointer ud, XtPointer cd) {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    XEvent *event = cbs->event;
    ColormapEditor *This = (ColormapEditor *) ud;
    This->input2(event);
}

/* private */ void
ColormapEditor::input2(XEvent *event) {
    if (event->type != ButtonPress
	    && event->type != ButtonRelease
	    && event->type != MotionNotify)
	return;

    int x = (event->xbutton.x - CELL_SPACING / 2) / (CELL_SIZE + CELL_SPACING);
    if (x < 0)
	x = 0;
    else if (x > 15)
	x = 15;
    int y = (event->xbutton.y - CELL_SPACING / 2) / (CELL_SIZE + CELL_SPACING);
    if (y < 0)
	y = 0;
    else if (y > 15)
	y = 15;

    int c = x + 16 * y;

    if (event->type == ButtonPress && !sel_in_progress) {
	if (sel_start != -1) {
	    // A selection existed already; it needs to be erased
	    int start, end;
	    if (sel_start < sel_end) {
		start = sel_start;
		end = sel_end;
	    } else {
		start = sel_end;
		end = sel_start;
	    }
	    sel_start = -1;
	    for (int i = start; i <= end; i++)
		update_cell(i);
	    redraw_cells(start, end);
	}
	sel_in_progress = true;
	sel_start = sel_end = c;
	select_cell(c);
	redraw_cells(c, c);
    } else if (sel_in_progress) {
	// Selection in progress; first let's take care of painting it
	if (c != sel_end) {
	    int old_sel_end = sel_end;
	    sel_end = c;
	    if (old_sel_end >= sel_start) {
		if (sel_end > old_sel_end) {
		    // Selection grows
		    for (int i = old_sel_end + 1; i <= sel_end; i++)
			select_cell(i);
		    redraw_cells(old_sel_end + 1, sel_end);
		} else if (sel_end >= sel_start) {
		    // Selection shrinks
		    for (int i = sel_end + 1; i <= old_sel_end; i++)
			update_cell(i);
		    redraw_cells(sel_end + 1, old_sel_end);
		} else {
		    // Selection crossed over its anchor
		    for (int i = sel_end; i < sel_start; i++)
			select_cell(i);
		    for (int i = sel_start + 1; i <= old_sel_end; i++)
			update_cell(i);
		    redraw_cells(sel_end, old_sel_end);
		}
	    } else {
		if (sel_end < old_sel_end) {
		    // Selection grows
		    for (int i = sel_end; i < old_sel_end; i++)
			select_cell(i);
		    redraw_cells(sel_end, old_sel_end - 1);
		} else if (sel_end <= sel_start) {
		    // Selection shrinks
		    for (int i = old_sel_end; i < sel_end; i++)
			update_cell(i);
		    redraw_cells(old_sel_end, sel_end - 1);
		} else {
		    // Selection crossed over its anchor
		    for (int i = sel_start + 1; i <= sel_end; i++)
			select_cell(i);
		    for (int i = old_sel_end; i < sel_start; i++)
			update_cell(i);
		    redraw_cells(old_sel_end, sel_end);
		}
	    }
	}
	// OK, that took care of keeping the on-screen representation in sync
	// with our internal one. Now, what exactly was that event?
	if (event->type == ButtonRelease)
	    sel_in_progress = false;
    }
}

/* private */ void
ColormapEditor::redraw_cells(int begin, int end) {
    int xb = begin & 15;
    int yb = begin >> 4;
    int xe = end & 15;
    int ye = end >> 4;
    if (ye > yb) {
	xb = 0;
	xe = 15;
    }
    int x = xb * (CELL_SIZE + CELL_SPACING) + CELL_SPACING;
    int y = yb * (CELL_SIZE + CELL_SPACING) + CELL_SPACING;
    int w = (xe - xb + 1) * (CELL_SIZE + CELL_SPACING) - CELL_SPACING;
    int h = (ye - yb + 1) * (CELL_SIZE + CELL_SPACING) - CELL_SPACING;
    XPutImage(g_display, XtWindow(drawingarea), g_gc, image, x, y, x, y, w, h);
}

/* private static */ void
ColormapEditor::activate(Widget w, XtPointer ud, XtPointer cd) {
    Arg arg;
    XtPointer userData;
    XtSetArg(arg, XmNuserData, &userData);
    XtGetValues(w, &arg, 1);
    int button = (int) userData;

    ColormapEditor *This = (ColormapEditor *) ud;
    switch (button) {
	case BTN_PICK:   This->doPick();   break;
	case BTN_BLEND:  This->doBlend();  break;
	case BTN_SWAP:   This->doSwap();   break;
	case BTN_MIX:    This->doMix();    break;
	case BTN_COPY:   This->doCopy();   break;
	case BTN_PASTE:  This->doPaste();  break;
	case BTN_LOAD:   This->doLoad();   break;
	case BTN_SAVE:   This->doSave();   break;
	case BTN_UNDO:   This->doUndo();   break;
	case BTN_REDO:   This->doRedo();   break;
	case BTN_OK:     This->doOK();     break;
	case BTN_CANCEL: This->doCancel(); break;
    }
}
