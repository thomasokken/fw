#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <stdlib.h>

#include "ColormapEditor.h"
#include "ColorPicker.h"
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


class PickAction : public UndoableAction {
    private:
	ColormapEditor::Listener *listener;
	FWPixmap *pm;
	int index;
	unsigned char old_r, old_g, old_b;
	unsigned char new_r, new_g, new_b;
    public:
	PickAction(ColormapEditor::Listener *listener, FWPixmap *pm, int index,
		   unsigned char new_r,
		   unsigned char new_g,
		   unsigned char new_b) {
	    this->listener = listener;
	    this->pm = pm;
	    this->index = index;
	    old_r = pm->cmap[index].r;
	    old_g = pm->cmap[index].g;
	    old_b = pm->cmap[index].b;
	    this->new_r = new_r;
	    this->new_g = new_g;
	    this->new_b = new_b;
	}
	virtual void undo() {
	    pm->cmap[index].r = old_r;
	    pm->cmap[index].g = old_g;
	    pm->cmap[index].b = old_b;
	    ColormapEditor *cme = listener->getCME();
	    if (cme != NULL)
		cme->colorsChanged(index, index);
	    listener->colormapChanged();
	}
	virtual void redo() {
	    pm->cmap[index].r = new_r;
	    pm->cmap[index].g = new_g;
	    pm->cmap[index].b = new_b;
	    ColormapEditor *cme = listener->getCME();
	    if (cme != NULL)
		cme->colorsChanged(index, index);
	    listener->colormapChanged();
	}
	virtual const char *getUndoTitle() {
	    return "Undo Pick Color";
	}
	virtual const char *getRedoTitle() {
	    return "Redo Pick Color";
	}
};

class ChangeRangeAction : public UndoableAction {
    private:
	ColormapEditor::Listener *listener;
	FWPixmap *pm;
	int startindex, endindex;
	FWColor *oldcolors;
    protected:
	FWColor *newcolors;
    public:
	ChangeRangeAction(ColormapEditor::Listener *listener, FWPixmap *pm,
			  int startindex,
			  int endindex) {
	    this->listener = listener;
	    this->pm = pm;
	    this->startindex = startindex;
	    this->endindex = endindex;
	    oldcolors = new FWColor[endindex - startindex + 1];
	    newcolors = new FWColor[endindex - startindex + 1];
	    for (int i = startindex; i <= endindex; i++)
		oldcolors[i - startindex] = pm->cmap[i];
	}
	virtual ~ChangeRangeAction() {
	    delete[] oldcolors;
	    delete[] newcolors;
	}
	virtual void undo() {
	    for (int i = startindex; i <= endindex; i++)
		pm->cmap[i] = oldcolors[i - startindex];
	    ColormapEditor *cme = listener->getCME();
	    if (cme != NULL)
		cme->colorsChanged(startindex, endindex);
	    listener->colormapChanged();
	}
	virtual void redo() {
	    for (int i = startindex; i <= endindex; i++)
		pm->cmap[i] = newcolors[i - startindex];
	    ColormapEditor *cme = listener->getCME();
	    if (cme != NULL)
		cme->colorsChanged(startindex, endindex);
	    listener->colormapChanged();
	}
};

class BlendAction : public ChangeRangeAction {
    public:
	BlendAction(ColormapEditor::Listener *listener, FWPixmap *pm,
		    int startindex, int endindex)
		: ChangeRangeAction(listener, pm, startindex, endindex) {
	    int rstart, gstart, bstart, rend, gend, bend;
	    rstart = pm->cmap[startindex].r;
	    gstart = pm->cmap[startindex].g;
	    bstart = pm->cmap[startindex].b;
	    rend = pm->cmap[endindex].r;
	    gend = pm->cmap[endindex].g;
	    bend = pm->cmap[endindex].b;
	    for (int i = startindex; i <= endindex; i++) {
		newcolors[i - startindex].r =
		    rstart + ((int) rend - (int) rstart) * (i - startindex)
				/ (endindex - startindex);
		newcolors[i - startindex].g =
		    gstart + ((int) gend - (int) gstart) * (i - startindex)
				/ (endindex - startindex);
		newcolors[i - startindex].b =
		    bstart + ((int) bend - (int) bstart) * (i - startindex)
				/ (endindex - startindex);
	    }
	}
	virtual const char *getUndoTitle() {
	    return "Undo Blend Colors";
	}
	virtual const char *getRedoTitle() {
	    return "Redo Blend Colors";
	}
};

class SwapAction : public ChangeRangeAction {
    public:
	SwapAction(ColormapEditor::Listener *listener, FWPixmap *pm,
		   int startindex, int endindex)
		: ChangeRangeAction(listener, pm, startindex, endindex) {
	    for (int i = startindex; i <= endindex; i++)
		newcolors[i - startindex] =
			pm->cmap[startindex + endindex - i];
	}
	virtual const char *getUndoTitle() {
	    return "Undo Swap Colors";
	}
	virtual const char *getRedoTitle() {
	    return "Redo Swap Colors";
	}
};

class MixAction : public ChangeRangeAction {
    public:
	MixAction(ColormapEditor::Listener *listener, FWPixmap *pm,
		  int startindex, int endindex)
		: ChangeRangeAction(listener, pm, startindex, endindex) {
	    int k = (endindex - startindex + 1) / 2;
	    for (int i = 0; i < k; i++) {
		newcolors[2 * i] =
			pm->cmap[startindex + k + i];
		newcolors[2 * i + 1] =
			pm->cmap[startindex + i];
	    }
	    if ((endindex - startindex) % 2 == 0)
		newcolors[endindex - startindex] = pm->cmap[endindex];
	}
	virtual const char *getUndoTitle() {
	    return "Undo Mix Colors";
	}
	virtual const char *getRedoTitle() {
	    return "Redo Mix Colors";
	}
};

class PasteAction : public ChangeRangeAction {
    public:
	PasteAction(ColormapEditor::Listener *listener, FWPixmap *pm,
		    int startindex, int endindex, int color_clipboard_size,
		    FWColor *color_clipboard)
		: ChangeRangeAction(listener, pm, startindex, endindex) {

	    if (color_clipboard_size == 1 || startindex == 255) {
		for (int i = startindex; i <= endindex; i++)
		    newcolors[i - startindex] = color_clipboard[0];
	    } else if (endindex - startindex + 1 == color_clipboard_size) {
		for (int i = 0; i <= endindex - startindex; i++)
		    newcolors[i] = color_clipboard[i];
	    } else if (endindex - startindex + 1 < color_clipboard_size) {
		for (int i = startindex; i <= endindex; i++)
		    newcolors[i - startindex] = color_clipboard[
				(i - startindex) * (color_clipboard_size - 1)
				    / (endindex - startindex)];
	    } else {
		for (int i = startindex; i < endindex; i++) {
		    float cn = (i - startindex) * (color_clipboard_size - 1)
				    / ((float) (endindex - startindex));
		    int c1 = (int) cn;
		    int c2 = c1 + 1;
		    float w2 = cn - c1;
		    float w1 = 1 - w2;
		    FWColor mc;
		    mc.r = (unsigned char) (w1 * color_clipboard[c1].r
					  + w2 * color_clipboard[c2].r);
		    mc.g = (unsigned char) (w1 * color_clipboard[c1].g
					  + w2 * color_clipboard[c2].g);
		    mc.b = (unsigned char) (w1 * color_clipboard[c1].b
					  + w2 * color_clipboard[c2].b);
		    newcolors[i - startindex] = mc;
		}
		newcolors[endindex - startindex] = color_clipboard[
						color_clipboard_size - 1];
	    }
	}
	virtual const char *getUndoTitle() {
	    return "Undo Paste Colors";
	}
	virtual const char *getRedoTitle() {
	    return "Redo Paste Colors";
	}
};
    
class CancelAction : public ChangeRangeAction {
    public:
	CancelAction(ColormapEditor::Listener *listener, FWPixmap *pm,
		     FWColor *restore_cmap)
		: ChangeRangeAction(listener, pm, 0, 255) {
	    for (int i = 0; i <= 255; i++)
		newcolors[i] = restore_cmap[i];
	}
	virtual const char *getUndoTitle() {
	    return "Undo Cancel Color Edits";
	}
	virtual const char *getRedoTitle() {
	    return "Redo Cancel Color Edits";
	}
};


static bool halftone_inited = false;
static unsigned char halftone_pattern[256][16][2];

static void halftone_initialize() {
    if (halftone_inited)
	return;

    for (int i = 0; i < 256; i++) {
	// Each halftone patten differs by its predecessor by exactly one bit.
	// First, copy the predecessor into the current slot:

	if (i == 0) {
	    for (int j = 0; j < 16; j++)
		for (int k = 0; k < 2; k++)
		    halftone_pattern[0][j][k] = 0;
	} else {
	    for (int j = 0; j < 16; j++)
		for (int k = 0; k < 2; k++)
		    halftone_pattern[i][j][k] = halftone_pattern[i - 1][j][k];
	}

	// Next, find the bit to set. This is a bit tricky. One possibility is
	// to paint a gradually growing clump, which leads to clustered-dot
	// dithering. Clustered-dot is appropriate mostly for printers, since
	// it offers a measure of immunity to pixel smear; on computer screens,
	// pattern dithering usually looks better, because the artefacts tend
	// to be smaller. I use a very simplistic approach to generating these
	// patterns, which seems to work very well (and generalizes nicely to
	// other pattern sizes, not that that's important right here).

	int v = i;
	int x = 0;
	int y = 0;

	for (int k = 0; k < 4; k++) {
	    y = (y << 1) | (v & 1); v >>= 1;
	    x = (x << 1) | (v & 1); v >>= 1;
	}
	x ^= y;

	halftone_pattern[i][y][x >> 3] |= 1 << (x & 7);
    }

    halftone_inited = true;
}

static int halftone(unsigned char value, int x, int y) {
    // A little hack: since halftone_pattern[0] has one pixel off (in a 16x16
    // pattern, you have 257 possible levels of brightness, and our table only
    // has 256 entries), we couldn't represent 'black' exactly. We fudge that
    // by handling black directly...

    if (value == 0)
	return 0;
    else if (value < 128)
	// This means halftone_pattern[127] (50% gray) is never used. Hopefully
	// no one will notice. It should be less apparent than an inaccuracy at
	// pure white or pure black!
	value--;

    halftone_initialize();
    x &= 15;
    y &= 15;
    unsigned char c = halftone_pattern[value][y][x >> 3];
    return (c >> (x & 7)) & 1;
}


/* private static */ FWColor *
ColormapEditor::color_clipboard = new FWColor[256];

/* private static */ int
ColormapEditor::color_clipboard_size = 0;


/* public */
ColormapEditor::ColormapEditor(Listener *listener, FWPixmap *pm,
			       UndoManager *undomanager, Colormap colormap)
				    : Frame(false, true, false) {
    setTitle("Colormap Editor");
    setIconTitle("Colormap Editor");

    this->listener = listener;
    this->pm = pm;

    undomgr = undomanager;

    initial_cmap = new FWColor[256];
    for (int i = 0; i < 256; i++)
	initial_cmap[i] = pm->cmap[i];
    initial_undo_id = undomgr->getCurrentId();

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

    this->colormap = colormap;
    if (colormap != g_colormap)
	setColormap(colormap);

    colorpicker = NULL;

    update_image();
}

/* public virtual */
ColormapEditor::~ColormapEditor() {
    listener->cmeClosed();
    delete[] initial_cmap;
    free(image->data);
    XFree(image);
    if (colorpicker != NULL)
	colorpicker->close();
}

/* public */ void
ColormapEditor::colormapChanged(Colormap colormap) {
    Colormap prevcolormap = this->colormap;
    this->colormap = colormap;
    if (colormap != prevcolormap)
	setColormap(colormap);
    if (colormap == g_colormap || colormap != prevcolormap) {
	update_image();
	XPutImage(g_display, XtWindow(drawingarea), g_gc, image,
		  0, 0, 0, 0, imagesize, imagesize);
    }

    // NOTE: we don't assume responsibility for this colormap. When our
    // owning Viewer is destroyed, it should make sure we are destroyed
    // also, and then free the private colormap.
}

/* public */ void
ColormapEditor::colorsChanged(int startindex, int endindex) {
    // This method is meant to be called by Actions that change the colormap.
    // It tells the CME to update the color grid.
    // Notification of the Viewer is *not* handled here; the action should do
    // that itself (remember, the action can depend on the Viewer existing;
    // it cannot depend on a Colormap Editor existing).

    // Fix color grid. Note that this is a no-op if we're using a private
    // colormap.
    if (colormap == g_colormap) {
	for (int i = startindex; i <= endindex; i++)
	    update_cell(i);

	// Repair selection, if appropriate
	if (sel_start != -1) {
	    int start, end;
	    if (sel_start < sel_end) {
		start = sel_start;
		end = sel_end;
	    } else {
		start = sel_end;
		end = sel_start;
	    }
	    for (int i = start; i <= end; i++)
		if (i >= startindex && i <= endindex)
		    select_cell(i);
	}
	redraw_cells(startindex, endindex);
    }
}

class CPListener : public ColorPicker::Listener {
    private:
	ColormapEditor *cme;
	int index;
    public:
	CPListener(ColormapEditor *cme, int index) {
	    this->cme = cme;
	    this->index = index;
	}
	virtual ~CPListener() {
	    cme->colorpicker = NULL;
	}
	virtual void colorPicked(unsigned char r,
				 unsigned char g,
				 unsigned char b) {
	    PickAction *action = new PickAction(cme->listener, cme->pm,
						index, r, g, b);
	    cme->undomgr->addAction(action);
	    action->redo();
	}
};

/* private */ void
ColormapEditor::doPick() {
    if (sel_start == -1 || sel_start != sel_end)
	beep();
    else {
	if (colorpicker != NULL)
	    colorpicker->close();
	colorpicker = new ColorPicker(
		    new CPListener(this, sel_start),
		    pm->cmap[sel_start].r,
		    pm->cmap[sel_start].g,
		    pm->cmap[sel_start].b,
		    colormap != g_colormap);
	colorpicker->raise();
    }
}

/* private */ void
ColormapEditor::doBlend() {
    if (sel_start == -1 || sel_start == sel_end)
	beep();
    else {
	int start, end;
	if (sel_start < sel_end) {
	    start = sel_start;
	    end = sel_end;
	} else {
	    start = sel_end;
	    end = sel_start;
	}
	BlendAction *action = new BlendAction(listener, pm, start, end);
	undomgr->addAction(action);
	action->redo();
    }
}

/* private */ void
ColormapEditor::doSwap() {
    if (sel_start == -1 || sel_start == sel_end)
	beep();
    else {
	int start, end;
	if (sel_start < sel_end) {
	    start = sel_start;
	    end = sel_end;
	} else {
	    start = sel_end;
	    end = sel_start;
	}
	SwapAction *action = new SwapAction(listener, pm, start, end);
	undomgr->addAction(action);
	action->redo();
    }
}

/* private */ void
ColormapEditor::doMix() {
    if (sel_start == -1 || sel_start == sel_end)
	beep();
    else {
	int start, end;
	if (sel_start < sel_end) {
	    start = sel_start;
	    end = sel_end;
	} else {
	    start = sel_end;
	    end = sel_start;
	}
	MixAction *action = new MixAction(listener, pm, start, end);
	undomgr->addAction(action);
	action->redo();
    }
}

/* private */ void
ColormapEditor::doCopy() {
    if (sel_start == -1)
	beep();
    else {
	for (int i = sel_start; i <= sel_end; i++)
	    color_clipboard[i - sel_start] = pm->cmap[i];
	color_clipboard_size = sel_end - sel_start + 1;
    }
}

/* private */ void
ColormapEditor::doPaste() {
    if (sel_start == -1 || color_clipboard_size == 0)
	beep();
    else {
	int start, end;
	if (sel_start < sel_end) {
	    start = sel_start;
	    end = sel_end;
	} else {
	    start = sel_end;
	    end = sel_start;
	}
	if (start == end) {
	    // If exactly one cell is selected, while the color clipboard
	    // contains more than one color, we paste the entire range from
	    // the color clipboard, starting at the selected cell, and
	    // continuing for as many colors as are in the color clipboard
	    // (or until index 255, whichever comes first). The idea is to
	    // mimic the behavior of an "insertion point" in a word processor,
	    // kind of. Only it's more like an "overwriting point" here since
	    // I don't move anything out of the way first.
	    end = start + color_clipboard_size - 1;
	    if (end > 255)
		end = 255;
	}
	PasteAction *action = new PasteAction(listener, pm, start, end,
					ColormapEditor::color_clipboard_size,
					ColormapEditor::color_clipboard);
	undomgr->addAction(action);
	action->redo();
    }
}

/* private */ void
ColormapEditor::doLoad() {
    listener->loadColors();
}

/* private */ void
ColormapEditor::doSave() {
    listener->saveColors();
}

/* private */ void
ColormapEditor::doUndo() {
    undomgr->undo();
}

/* private */ void
ColormapEditor::doRedo() {
    undomgr->redo();
}

/* private */ void
ColormapEditor::doOK() {
    close();
}

/* private */ void
ColormapEditor::doCancel() {
    if (undomgr->getCurrentId() != initial_undo_id) {
	CancelAction *action = new CancelAction(listener, pm, initial_cmap);
	undomgr->addAction(action);
	action->redo();
    }
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
    } else if (g_grayramp != NULL) {
	// Greyscale halftoning
	unsigned char r = pm->cmap[c].r;
	unsigned char g = pm->cmap[c].g;
	unsigned char b = pm->cmap[c].b;
	unsigned long pixels[2];
	rgb2nearestgrays(&r, &g, &b, pixels);
	for (int xx = x; xx < x + CELL_SIZE; xx++)
	    for (int yy = y; yy < y + CELL_SIZE; yy++) {
		int index = halftone(r, xx - x, yy - y);
		XPutPixel(image, xx, yy, pixels[index]);
	    }
    } else {
	// Color halftoning
	unsigned char r = pm->cmap[c].r;
	unsigned char g = pm->cmap[c].g;
	unsigned char b = pm->cmap[c].b;
	unsigned long pixels[8];
	rgb2nearestcolors(&r, &g, &b, pixels);
	for (int xx = x; xx < x + CELL_SIZE; xx++)
	    for (int yy = y; yy < y + CELL_SIZE; yy++) {
		int index = (halftone(r, xx - x, yy - y) << 2)
			    | (halftone(g, xx - x, yy - y) << 1)
			    | halftone(b, xx - x, yy - y);
		XPutPixel(image, xx, yy, pixels[index]);
	    }
    }
    if (sel_start != -1 && c >= sel_start && c <= sel_end)
	select_cell(c);
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
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
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
	selectColor(c);
    } else if (sel_in_progress) {
	extendSelection(c);
	if (event->type == ButtonRelease)
	    sel_in_progress = false;
    }
}

/* public */ void
ColormapEditor::selectColor(int c) {
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
}

/* public */ void
ColormapEditor::extendSelection(int c) {
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
}

/* public */ void
ColormapEditor::finishSelection() {
    sel_in_progress = false;
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
