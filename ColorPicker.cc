///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

#include <Xm/BulletinB.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ColorPicker.h"
#include "CopyBits.h"
#include "FWColor.h"
#include "FWPixmap.h"
#include "main.h"
#include "util.h"

#define WHEEL_DIAMETER 150
#define CROSS_SIZE 10
#define SLIDER_HEIGHT 12
#define THUMB_HEIGHT 20
#define THUMB_WIDTH 9

#define WHEEL_W (WHEEL_DIAMETER + CROSS_SIZE)
#define WHEEL_H (WHEEL_DIAMETER + CROSS_SIZE)
#define SLIDER_W WHEEL_W
#define SLIDER_H THUMB_HEIGHT
#define OLDNEW_W 80
#define OLDNEW_H 64

#define MARGINS 4


static void pref_size(Widget w, int *width, int *height) {
    XtWidgetGeometry wg;
    XtQueryGeometry(w, NULL, &wg);
    *width = wg.width;
    *height = wg.height;
}

static void set_geom(Widget w, int x, int y, int width, int height) {
    Arg args[4];
    XtSetArg(args[0], XmNx, x);
    XtSetArg(args[1], XmNy, y);
    XtSetArg(args[2], XmNwidth, width);
    XtSetArg(args[3], XmNheight, height);
    XtSetValues(w, args, 4);
}


/* private static */ GC
ColorPicker::gc;

/* private static */ int
ColorPicker::instances = 0;


/* public */
ColorPicker::ColorPicker(Frame *parent, Listener *listener, unsigned char r,
	unsigned char g, unsigned char b, bool allow_private_cmap)
				: Frame(parent, false) {
    this->listener = listener;

    setTitle("Color Picker");
    setIconTitle("Color Picker");

    private_colormap = false;
    private_colorcells = false;
    oldnew_image_initialized = false;
    disable_rgbChanged = false;

    R = oldR = r;
    G = oldG = g;
    B = oldB = b;
    CopyBits::rgb2hsl(R, G, B, &H, &S, &L);

    cross_x = -1;
    cross_y = -1;
    slider_x = -1;

    if (instances++ == 0)
	gc = XCreateGC(g_display, g_rootwindow, 0, NULL);

    wheel_image = XCreateImage(g_display, g_visual, g_depth, ZPixmap,
			       0, NULL, WHEEL_W, WHEEL_H, 32, 0);
    wheel_image->data = (char *) malloc(wheel_image->bytes_per_line
					* wheel_image->height);
    slider_image = XCreateImage(g_display, g_visual, g_depth, ZPixmap,
				0, NULL, SLIDER_W, SLIDER_H, 32, 0);
    slider_image->data = (char *) malloc(slider_image->bytes_per_line
					 * slider_image->height);
    oldnew_image = XCreateImage(g_display, g_visual, g_depth, ZPixmap,
				0, NULL, OLDNEW_W, OLDNEW_H, 32, 0);
    oldnew_image->data = (char *) malloc(oldnew_image->bytes_per_line
					 * oldnew_image->height);


    Widget form = getContainer();

    bb = XtVaCreateManagedWidget(
	    "BulletinBoard",
	    xmBulletinBoardWidgetClass,
	    form,
	    XmNmarginHeight, 0,
	    XmNmarginWidth, 0,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);

    XmString s = XmStringCreateLocalized("Pick a color:");
    Widget main_label = XtVaCreateManagedWidget(
	    "MainLabel",
	    xmLabelWidgetClass,
	    bb,
	    XmNalignment, XmALIGNMENT_BEGINNING,
	    XmNlabelString, s,
	    NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("Original:");
    Widget original_label = XtVaCreateManagedWidget(
	    "OriginalLabel",
	    xmLabelWidgetClass,
	    bb,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("New:");
    Widget new_label = XtVaCreateManagedWidget(
	    "NewLabel",
	    xmLabelWidgetClass,
	    bb,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("Red:");
    Widget red_label = XtVaCreateManagedWidget(
	    "RedLabel",
	    xmLabelWidgetClass,
	    bb,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("Green:");
    Widget green_label = XtVaCreateManagedWidget(
	    "GreenLabel",
	    xmLabelWidgetClass,
	    bb,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized("Blue:");
    Widget blue_label = XtVaCreateManagedWidget(
	    "BlueLabel",
	    xmLabelWidgetClass,
	    bb,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    NULL);
    XmStringFree(s);

    red = XtVaCreateManagedWidget(
	    "RedText",
	    xmTextWidgetClass,
	    bb,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    XmNcolumns, 5,
	    XmNmarginWidth, 0,
	    XmNmarginHeight, 0,
	    XmNeditMode, XmSINGLE_LINE_EDIT,
	    NULL);
    XtAddCallback(red, XmNmodifyVerifyCallback, modifyVerify, (XtPointer) this);
    XtAddCallback(red, XmNvalueChangedCallback, valueChanged, (XtPointer) this);

    green = XtVaCreateManagedWidget(
	    "GreenText",
	    xmTextWidgetClass,
	    bb,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    XmNcolumns, 5,
	    XmNmarginWidth, 0,
	    XmNmarginHeight, 0,
	    XmNeditMode, XmSINGLE_LINE_EDIT,
	    NULL);
    XtAddCallback(green, XmNmodifyVerifyCallback, modifyVerify, (XtPointer) this);
    XtAddCallback(green, XmNvalueChangedCallback, valueChanged, (XtPointer) this);

    blue = XtVaCreateManagedWidget(
	    "BlueText",
	    xmTextWidgetClass,
	    bb,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    XmNcolumns, 5,
	    XmNmarginWidth, 0,
	    XmNmarginHeight, 0,
	    XmNeditMode, XmSINGLE_LINE_EDIT,
	    NULL);
    XtAddCallback(blue, XmNmodifyVerifyCallback, modifyVerify, (XtPointer) this);
    XtAddCallback(blue, XmNvalueChangedCallback, valueChanged, (XtPointer) this);

    s = XmStringCreateLocalized("OK");
    Widget okB = XtVaCreateManagedWidget(
	    "OK",
	    xmPushButtonWidgetClass,
	    bb,
	    XmNlabelString, s,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    NULL);
    XtAddCallback(okB, XmNactivateCallback, ok, (XtPointer) this);
    XmStringFree(s);

    s = XmStringCreateLocalized("Cancel");
    Widget cancelB = XtVaCreateManagedWidget(
	    "Cancel",
	    xmPushButtonWidgetClass,
	    bb,
	    XmNlabelString, s,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    NULL);
    XtAddCallback(cancelB, XmNactivateCallback, cancel, (XtPointer) this);
    XmStringFree(s);

    disable_rgbChanged = true;
    char buf[64];
    rs = gs = bs = "";
    snprintf(buf, 64, "%d", R);
    XmTextSetString(red, buf);
    rs = XmTextGetString(red);
    snprintf(buf, 64, "%d", G);
    XmTextSetString(green, buf);
    gs = XmTextGetString(green);
    snprintf(buf, 64, "%d", B);
    XmTextSetString(blue, buf);
    bs = XmTextGetString(blue);
    disable_rgbChanged = false;

    XtTranslations translations = XtParseTranslationTable(
	    "<KeyDown>: DrawingAreaInput()\n"
	    "<KeyUp>: DrawingAreaInput()\n"
	    "<BtnDown>: DrawingAreaInput()\n"
	    "<BtnUp>: DrawingAreaInput()\n"
	    "<Motion>: DrawingAreaInput()\n");

    wheel = XtVaCreateManagedWidget(
	    "ColorWheel",
	    xmDrawingAreaWidgetClass,
	    bb,
	    XmNtranslations, translations,
	    XmNtraversalOn, False,
	    NULL);

    slider = XtVaCreateManagedWidget(
	    "Slider",
	    xmDrawingAreaWidgetClass,
	    bb,
	    XmNtranslations, translations,
	    NULL);

    oldnew = XtVaCreateManagedWidget(
	    "OldAndNew",
	    xmDrawingAreaWidgetClass,
	    bb,
	    XmNtranslations, translations,
	    NULL);

    XtAddCallback(wheel, XmNexposeCallback, expose, (XtPointer) this);
    XtAddCallback(wheel, XmNinputCallback, input, (XtPointer) this);
    XtAddCallback(slider, XmNexposeCallback, expose, (XtPointer) this);
    XtAddCallback(slider, XmNinputCallback, input, (XtPointer) this);
    XtAddCallback(oldnew, XmNexposeCallback, expose, (XtPointer) this);
    XtAddCallback(oldnew, XmNinputCallback, input, (XtPointer) this);


    // Now, lay 'em all out...

    int mainlabel_x = MARGINS;
    int mainlabel_y = MARGINS;
    int mainlabel_w, mainlabel_h;
    pref_size(main_label, &mainlabel_w, &mainlabel_h);

    int wheel_x = MARGINS;
    int wheel_y = mainlabel_y + mainlabel_h + MARGINS;
    int wheel_w = WHEEL_W;
    int wheel_h = WHEEL_H;

    int slider_x = MARGINS;
    int slider_y = wheel_y + wheel_h + MARGINS;
    int slider_w = SLIDER_W;
    int slider_h = SLIDER_H;

    int left_h = slider_y + slider_h + MARGINS;
    int left_w = mainlabel_w;
    if (left_w < wheel_w)
	left_w = wheel_w;
    if (left_w < slider_w)
	left_w = slider_w;
    left_w += 2 * MARGINS;
    
    int orig_w, orig_h;
    pref_size(original_label, &orig_w, &orig_h);
    int new_w, new_h;
    pref_size(new_label, &new_w, &new_h);
    int redlabel_w, redlabel_h;
    pref_size(red_label, &redlabel_w, &redlabel_h);
    int greenlabel_w, greenlabel_h;
    pref_size(green_label, &greenlabel_w, &greenlabel_h);
    int bluelabel_w, bluelabel_h;
    pref_size(blue_label, &bluelabel_w, &bluelabel_h);

    int middle_w = orig_w;
    if (middle_w < new_w)
	middle_w = new_w;
    if (middle_w < redlabel_w)
	middle_w = redlabel_w;
    if (middle_w < greenlabel_w)
	middle_w = greenlabel_w;
    if (middle_w < bluelabel_w)
	middle_w = bluelabel_w;

    int orignew_y = MARGINS;
    int orignew_w = OLDNEW_W;
    int orignew_h = OLDNEW_H;

    int red_y = orignew_y + orignew_h + MARGINS;
    int red_w, red_h;
    pref_size(red, &red_w, &red_h);

    int green_y = red_y + red_h;
    int green_w, green_h;
    pref_size(green, &green_w, &green_h);

    int blue_y = green_y + green_h;
    int blue_w, blue_h;
    pref_size(blue, &blue_w, &blue_h);

    int ok_w, ok_h;
    pref_size(okB, &ok_w, &ok_h);
    int cancel_w, cancel_h;
    pref_size(cancelB, &cancel_w, &cancel_h);
    if (ok_w < cancel_w)
	ok_w = cancel_w;
    else
	cancel_w = ok_w;

    int ok_cancel_h = ok_h;
    if (ok_cancel_h < cancel_h)
	ok_cancel_h = cancel_h;
    int ok_cancel_w = ok_w + cancel_w + MARGINS;

    int right_h = green_y + green_h + MARGINS + ok_cancel_h + MARGINS;
    int total_h = left_h > right_h ? left_h : right_h;
    int right_w = orignew_w;
    if (right_w < red_w)
	right_w = red_w;
    if (right_w < green_w)
	right_w = green_w;
    if (right_w < blue_w)
	right_w = blue_w;
    if (middle_w + right_w < ok_cancel_w)
	middle_w = ok_cancel_w - right_w;

    int total_w = left_w + middle_w + right_w + MARGINS;

    int orig_x = left_w + middle_w - orig_w;
    int orig_y = (orignew_h - orig_h - new_h) / 4 + MARGINS;
    int new_x = left_w + middle_w - new_w;
    int new_y = (orignew_h - orig_h -new_h) * 3 / 4 + orig_h + MARGINS;
    int redlabel_x = left_w + middle_w - redlabel_w;
    int redlabel_y = red_y + (red_h - redlabel_h) / 2;
    int greenlabel_x = left_w + middle_w - greenlabel_w;
    int greenlabel_y = green_y + (green_h - greenlabel_h) / 2;
    int bluelabel_x = left_w + middle_w - bluelabel_w;
    int bluelabel_y = blue_y + (blue_h - bluelabel_h) / 2;

    int orignew_x = left_w + middle_w;
    int red_x = left_w + middle_w;
    int green_x = left_w + middle_w;
    int blue_x = left_w + middle_w;

    int cancel_x = total_w - cancel_w - MARGINS;
    int cancel_y = total_h - cancel_h - MARGINS;
    int ok_x = cancel_x - ok_w - MARGINS;
    int ok_y = total_h - ok_h - MARGINS;

    set_geom(main_label, mainlabel_x, mainlabel_y, mainlabel_w, mainlabel_h);
    set_geom(wheel, wheel_x, wheel_y, wheel_w, wheel_h);
    set_geom(slider, slider_x, slider_y, slider_w, slider_h);
    set_geom(original_label, orig_x, orig_y, orig_w, orig_h);
    set_geom(new_label, new_x, new_y, new_w, new_h);
    set_geom(red_label, redlabel_x, redlabel_y, redlabel_w, redlabel_h);
    set_geom(green_label, greenlabel_x, greenlabel_y, greenlabel_w, greenlabel_h);
    set_geom(blue_label, bluelabel_x, bluelabel_y, bluelabel_w, bluelabel_h);
    set_geom(oldnew, orignew_x, orignew_y, orignew_w, orignew_h);
    set_geom(red, red_x, red_y, red_w, red_h);
    set_geom(green, green_x, green_y, green_w, green_h);
    set_geom(blue, blue_x, blue_y, blue_w, blue_h);
    set_geom(okB, ok_x, ok_y, ok_w, ok_h);
    set_geom(cancelB, cancel_x, cancel_y, cancel_w, cancel_h);
    XtVaSetValues(bb, XmNwidth, total_w, XmNheight, total_h, NULL);

    
    // We go an extra mile or two to make sure the user gets optimal feedback
    // from the color picker dialog... That means *accurate* colors!

    if (g_visual->c_class == PseudoColor && g_depth == 8) {
	// Ah, an 8-bit PseudoColor visual. Always the most challenging...
	// If we have already succeeded in allocating a 6x6x6 read-only
	// color cube in main.cc, we'll just use that. We will, however, try to
	// allocate two read-write color cells, so we can render the "old" and
	// "new" colors exactly. If allocating the read-write cells fails, we
	// or if we don't have a 6x6x6 color cube, we go brute force and
	// allocate a private colormap.
	
	int p = 0;
	if (!allow_private_cmap || (g_colorcube != NULL && g_cubesize == 6)) {
	    unsigned long pixels[2];
	    if (XAllocColorCells(g_display, g_colormap, False, NULL, 0,
				 pixels, 2) != 0) {
		// It worked! Goody.
		private_colorcells = true;
		old_pixel = pixels[0];
		new_pixel = pixels[1];
		// Store old_pixel; we'll take care of new_pixel later.
		XColor xc;
		xc.pixel = old_pixel;
		xc.red = R * 257;
		xc.green = G * 257;
		xc.blue = B * 257;
		xc.flags = DoRed | DoGreen | DoBlue;
		XStoreColors(g_display, g_colormap, &xc, 1);
		goto color_alloc_done;
	    }
	}

	// main.cc did not succeed in creating a 6x6x6 read-only color cube,
	// or we did not succeed in allocating two read-write color cells for
	// the "old" and "new" color display. So, now we play hardball and
	// allocate a private colormap.
	
	if (!allow_private_cmap)
	    // The user isn't using a private colormap on the window from where
	    // we were invoked; we take that as a hint that they don't like
	    // colormap flashing, and so we don't use one either.
	    goto color_alloc_done;

	xcube = XCreateColormap(g_display, g_rootwindow, g_visual, AllocAll);
	XColor colors[218];
	for (int r = 0; r < 6; r++)
	    for (int g = 0; g < 6; g++)
		for (int b = 0; b < 6; b++) {
		    colors[p].pixel = p;
		    colors[p].red = 13107 * r;
		    colors[p].green = 13107 * g;
		    colors[p].blue = 13107 * b;
		    colors[p].flags = DoRed | DoGreen | DoBlue;
		    p++;
		}

	old_pixel = p++;
	new_pixel = p++;
	colors[old_pixel].pixel = old_pixel;
	colors[old_pixel].red = R;
	colors[old_pixel].red = G;
	colors[old_pixel].red = B;
	colors[old_pixel].flags = DoRed | DoGreen | DoBlue;
	colors[new_pixel].pixel = new_pixel;
	colors[new_pixel].red = R;
	colors[new_pixel].red = G;
	colors[new_pixel].red = B;
	colors[new_pixel].flags = DoRed | DoGreen | DoBlue;

	XStoreColors(g_display, xcube, colors, p);
	setColormap(xcube);
	private_colormap = true;

	color_alloc_done:;
    }

    rgbChanged();
}

/* public virtual */
ColorPicker::~ColorPicker() {
    delete listener;
    free(wheel_image->data);
    XFree(wheel_image);
    free(slider_image->data);
    XFree(slider_image);
    free(oldnew_image->data);
    XFree(oldnew_image);
    XtFree(rs);
    XtFree(gs);
    XtFree(bs);
    if (private_colormap) {
	XFreeColormap(g_display, xcube);
	setColormap(g_colormap);
    }
    if (private_colorcells) {
	unsigned long pixels[2];
	pixels[0] = old_pixel;
	pixels[1] = new_pixel;
	XFreeColors(g_display, g_colormap, pixels, 2, 0);
    }
    if (--instances == 0)
	XFreeGC(g_display, gc);
}

/* private */ void
ColorPicker::updateRGBTextFields(unsigned char r, unsigned char g, unsigned char b) {
    disable_rgbChanged = true;
    char buf[64];
    snprintf(buf, 64, "%d", r);
    XmTextSetString(red, buf);
    snprintf(buf, 64, "%d", g);
    XmTextSetString(green, buf);
    snprintf(buf, 64, "%d", b);
    XmTextSetString(blue, buf);
    disable_rgbChanged = false;
}

/* private */ void
ColorPicker::rgbChanged() {
    if (!disable_rgbChanged) {
	CopyBits::rgb2hsl(R, G, B, &H, &S, &L);
	repaintOldNewImage();
	repaintWheelImage();
	repaintSliderImage();
	float th = 2 * 3.141592654 * H;
	int offset = (WHEEL_DIAMETER + CROSS_SIZE) / 2;
	cross_x = (int) (S * WHEEL_DIAMETER * cos(th) / 2) + offset;
	cross_y = (int) (-S * WHEEL_DIAMETER * sin(th) / 2) + offset;
	drawCross();
	slider_x = CROSS_SIZE / 2 + 1 + (int) (L * (WHEEL_DIAMETER - 2) + 0.5);
	drawThumb();
    }
}

/* private */ void
ColorPicker::repaintOldNewImage() {
    if (private_colormap || private_colorcells) {
	XColor xc;
	xc.pixel = new_pixel;
	xc.red = R * 257;
	xc.green = G * 257;
	xc.blue = B * 257;
	xc.flags = DoRed | DoGreen | DoBlue;
	XStoreColors(g_display, private_colormap ? xcube : g_colormap, &xc, 1);

	if (oldnew_image_initialized)
	    return;

	for (int x = 1; x < OLDNEW_W - 1; x++) {
	    XPutPixel(oldnew_image, x, 0, 0);
	    XPutPixel(oldnew_image, x, OLDNEW_H - 1, 0);
	}
	for (int y = 0; y < OLDNEW_H; y++) {
	    XPutPixel(oldnew_image, 0, y, 0);
	    XPutPixel(oldnew_image, OLDNEW_W - 1, y, 0);
	}
	for (int y = 1; y < OLDNEW_H / 2; y++)
	    for (int x = 1; x < OLDNEW_W - 1; x++)
		XPutPixel(oldnew_image, x, y, old_pixel);
	for (int y = OLDNEW_H / 2; y < OLDNEW_H - 1; y++)
	    for (int x = 1; x < OLDNEW_W - 1; x++)
		XPutPixel(oldnew_image, x, y, new_pixel);

	oldnew_image_initialized = true;
    } else {
	if (g_grayramp != NULL) {
	    // Greyscale halftoning
	    unsigned char r, g, b;
	    unsigned long pixels[2];
	    if (!oldnew_image_initialized) {
		r = oldR;
		g = oldG;
		b = oldB;
		CopyBits::rgb2nearestgrays(&r, &g, &b, pixels);
		for (int y = 1; y < OLDNEW_H / 2; y++)
		    for (int x = 1; x < OLDNEW_W - 1; x++) {
			int index = CopyBits::halftone(r, x - 1, y - 1);
			XPutPixel(oldnew_image, x, y, pixels[index]);
		    }
	    }
	    r = R;
	    g = G;
	    b = B;
	    CopyBits::rgb2nearestgrays(&r, &g, &b, pixels);
	    for (int y = OLDNEW_H / 2; y < OLDNEW_H - 1; y++)
		for (int x = 1; x < OLDNEW_W - 1; x++) {
		    int index = CopyBits::halftone(r, x - 1, y - 1);
		    XPutPixel(oldnew_image, x, y, pixels[index]);
		}
	} else {
	    // Color halftoning
	    unsigned char r, g, b;
	    unsigned long pixels[8];
	    if (!oldnew_image_initialized) {
		r = oldR;
		g = oldG;
		b = oldB;
		CopyBits::rgb2nearestcolors(&r, &g, &b, pixels);
		for (int y = 1; y < OLDNEW_H / 2; y++)
		    for (int x = 1; x < OLDNEW_W - 1; x++) {
			int index = (CopyBits::halftone(r, x - 1, y - 1) << 2)
			    | (CopyBits::halftone(g, x - 1, y - 1) << 1)
			    | CopyBits::halftone(b, x - 1, y - 1);
			XPutPixel(oldnew_image, x, y, pixels[index]);
		    }
	    }
	    r = R;
	    g = G;
	    b = B;
	    CopyBits::rgb2nearestcolors(&r, &g, &b, pixels);
	    for (int y = OLDNEW_H / 2; y < OLDNEW_H - 1; y++)
		for (int x = 1; x < OLDNEW_W - 1; x++) {
		    int index = (CopyBits::halftone(r, x - 1, y - 1) << 2)
			| (CopyBits::halftone(g, x - 1, y - 1) << 1)
			| CopyBits::halftone(b, x - 1, y - 1);
		    XPutPixel(oldnew_image, x, y, pixels[index]);
		}
	}
	if (!oldnew_image_initialized) {
	    for (int x = 1; x < OLDNEW_W - 1; x++) {
		XPutPixel(oldnew_image, x, 0, 0);
		XPutPixel(oldnew_image, x, OLDNEW_H - 1, 0);
	    }
	    for (int y = 0; y < OLDNEW_H; y++) {
		XPutPixel(oldnew_image, 0, y, 0);
		XPutPixel(oldnew_image, OLDNEW_W - 1, y, 0);
	    }
	    oldnew_image_initialized = true;
	}
	
//	I think pattern dithering looks better than error diffusion
//	in color preview areas -- that is, in the ColormapEditor and
//	in the old/new preview area in the ColorPicker.
//	I kept the old code that used error diffusion for the old/new
//	area around in case I ever change my mind. :-)
//
//	FWPixmap pm;
//	pm.width = OLDNEW_W;
//	pm.height = OLDNEW_H;
//	pm.depth = 24;
//	pm.bytesperline = pm.width * 4;
//	pm.pixels = (unsigned char *) malloc(pm.height * pm.bytesperline);
//	for (int x = 1; x < OLDNEW_W - 1; x++) {
//	    pm.put_pixel(x, 0, 0);
//	    pm.put_pixel(x, OLDNEW_H - 1, 0);
//	}
//	for (int y = 0; y < OLDNEW_H; y++) {
//	    pm.put_pixel(0, y, 0);
//	    pm.put_pixel(OLDNEW_W - 1, y, 0);
//	}
//	unsigned long oldp = (oldR << 16) | (oldG << 8) | oldB;
//	for (int y = 1; y < OLDNEW_H / 2; y++)
//	    for (int x = 1; x < OLDNEW_W - 1; x++)
//		pm.put_pixel(x, y, oldp);
//	unsigned long newp = (R << 16) | (G << 8) | B;
//	for (int y = OLDNEW_H / 2; y < OLDNEW_H - 1; y++)
//	    for (int x = 1; x < OLDNEW_W - 1; x++)
//		pm.put_pixel(x, y, newp);
//	CopyBits::copy_unscaled(&pm, oldnew_image, false, true, true,
//				0, 0, OLDNEW_H, OLDNEW_W);
//	free(pm.pixels);
    }

    // Paint it!
    if (XtIsRealized(oldnew))
	XPutImage(g_display, XtWindow(oldnew), gc, oldnew_image,
		  0, 0, 0, 0, OLDNEW_W, OLDNEW_H);
}

/* private */ void
ColorPicker::repaintWheelImage() {
    FWPixmap pm;
    pm.width = WHEEL_W;
    pm.height = WHEEL_H;
    pm.depth = 24;
    pm.bytesperline = pm.width * 4;
    pm.pixels = (unsigned char *) malloc(pm.height * pm.bytesperline);

    // Background
    XColor xc;
    Arg arg;
    XtSetArg(arg, XmNbackground, &xc.pixel);
    XtGetValues(bb, &arg, 1);
    XQueryColor(g_display, private_colormap ? xcube : g_colormap, &xc);
    unsigned long background = ((xc.red / 257) << 16)
				| ((xc.green / 257) << 8) | (xc.blue / 257);
    for (int y = 0; y < WHEEL_H; y++)
	for (int x = 0; x < WHEEL_W; x++)
	    pm.put_pixel(x, y, background);

    // The wheel itself
    int r = WHEEL_DIAMETER / 2;
    for (int y = -r; y <= r; y++) {
	int w = (int) (sqrt((double) (r * r - y * y)) + 0.5);
	for (int x = -w; x <= w; x++) {
	    float h = atan2((double) -y, (double) -x) / (2 * 3.141592654) + 0.5;
	    float s = sqrt(x * x + y * y) / r;
	    unsigned char rr, gg, bb;
	    CopyBits::hsl2rgb(h, s, L, &rr, &gg, &bb);
	    pm.put_pixel(x + r + CROSS_SIZE / 2,
			 y + r + CROSS_SIZE / 2,
			 (rr << 16) | (bb << 8) | gg);
	}
    }

    // The black circle around the wheel
    int x = r;
    int y = 0;
    int R2 = x * x;
    int r2 = r * r;
    int offset = CROSS_SIZE / 2 + r;
    do {
	pm.put_pixel(offset + x, offset + y, 0);
	pm.put_pixel(offset - x, offset + y, 0);
	pm.put_pixel(offset + x, offset - y, 0);
	pm.put_pixel(offset - x, offset - y, 0);
	pm.put_pixel(offset + y, offset + x, 0);
	pm.put_pixel(offset - y, offset + x, 0);
	pm.put_pixel(offset + y, offset - x, 0);
	pm.put_pixel(offset - y, offset - x, 0);
	R2 += 2 * y + 1;
	y++;
	int RR2 = R2 - 2 * x + 1;
	int dR = R2 - r2; if (dR < 0) dR = -dR;
	int dRR = RR2 - r2; if (dRR < 0) dRR = -dRR;
	if (dRR < dR) {
	    R2 = RR2;
	    x--;
	}
    } while (x >= y);

    CopyBits::copy_unscaled(&pm, wheel_image, private_colormap, true, true,
			    0, 0, WHEEL_H, WHEEL_W);
    free(pm.pixels);

    // Paint it!
    if (XtIsRealized(wheel))
	XPutImage(g_display, XtWindow(wheel), gc, wheel_image,
		  0, 0, 0, 0, WHEEL_W, WHEEL_H);
}

/* private */ void
ColorPicker::repaintSliderImage() {
    FWPixmap pm;
    pm.width = SLIDER_W;
    pm.height = SLIDER_H;
    pm.depth = 24;
    pm.bytesperline = pm.width * 4;
    pm.pixels = (unsigned char *) malloc(pm.height * pm.bytesperline);

    // Background
    XColor xc;
    Arg arg;
    XtSetArg(arg, XmNbackground, &xc.pixel);
    XtGetValues(bb, &arg, 1);
    XQueryColor(g_display, private_colormap ? xcube : g_colormap, &xc);
    unsigned long background = ((xc.red / 257) << 16)
				| ((xc.green / 257) << 8) | (xc.blue / 257);
    for (int y = 0; y < SLIDER_H; y++)
	for (int x = 0; x < SLIDER_W; x++)
	    pm.put_pixel(x, y, background);

    // The slider itself
    for (int x = 1; x <= WHEEL_DIAMETER - 1; x++) {
	float l = (x - 1.0) / (WHEEL_DIAMETER - 1);
	unsigned char rr, gg, bb;
	CopyBits::hsl2rgb(H, S, l, &rr, &gg, &bb);
	unsigned long pixel = (rr << 16) | (gg << 8) | bb;
	for (int y = (SLIDER_H - SLIDER_HEIGHT) / 2 + 1;
		 y < (SLIDER_H + SLIDER_HEIGHT) / 2 - 1; y++) {
	    pm.put_pixel(x + CROSS_SIZE / 2, y, pixel);
	}
    }

    // The slider's black outline
    for (int x = 1; x <= WHEEL_DIAMETER - 1; x++) {
	pm.put_pixel(x + CROSS_SIZE / 2, (SLIDER_H - SLIDER_HEIGHT) / 2, 0);
	pm.put_pixel(x + CROSS_SIZE / 2, (SLIDER_H + SLIDER_HEIGHT) / 2 - 1, 0);
    }
    for (int y = (SLIDER_H - SLIDER_HEIGHT) / 2 + 1;
	     y < (SLIDER_H + SLIDER_HEIGHT) / 2 - 1; y++) {
	pm.put_pixel(CROSS_SIZE / 2, y, 0);
	pm.put_pixel(CROSS_SIZE / 2 + WHEEL_DIAMETER, y, 0);
    }

    CopyBits::copy_unscaled(&pm, slider_image, private_colormap, true, true,
			    0, 0, SLIDER_H, SLIDER_W);
    free(pm.pixels);

    // Paint it!
    if (XtIsRealized(slider))
	XPutImage(g_display, XtWindow(slider), gc, slider_image,
		  0, 0, 0, 0, SLIDER_W, SLIDER_H);
}

/* private */ void
ColorPicker::drawCross() {
    if (cross_x == -1 || cross_y == -1)
	return;
    if (!XtIsRealized(wheel))
	return;
    int cw = CROSS_SIZE / 2;
    XSetForeground(g_display, gc, private_colormap ? 215 : g_black);
    XDrawLine(g_display, XtWindow(wheel), gc, cross_x + 1, cross_y - cw + 1,
	      cross_x + 1, cross_y + cw + 1);
    XDrawLine(g_display, XtWindow(wheel), gc, cross_x - cw + 1, cross_y + 1,
	      cross_x + cw + 1, cross_y + 1);
    XSetForeground(g_display, gc, private_colormap ? 0 : g_white);
    XDrawLine(g_display, XtWindow(wheel), gc, cross_x, cross_y - cw,
	      cross_x, cross_y + cw);
    XDrawLine(g_display, XtWindow(wheel), gc, cross_x - cw, cross_y,
	      cross_x + cw, cross_y);
}

/* private */ void
ColorPicker::removeCross() {
    if (cross_x == -1 || cross_y == -1)
	return;
    if (!XtIsRealized(wheel))
	return;
    int cw = CROSS_SIZE / 2;
    XPutImage(g_display, XtWindow(wheel), gc, wheel_image,
	      cross_x - cw, cross_y - cw,
	      cross_x - cw, cross_y - cw,
	      cross_x + cw + 2, cross_y + cw + 2);
}

/* private */ void
ColorPicker::drawThumb() {
    if (slider_x == -1)
	return;
    if (!XtIsRealized(slider))
	return;
    XSetForeground(g_display, gc, private_colormap ? 215 : g_white);
    XDrawRectangle(g_display, XtWindow(slider), gc,
		   slider_x - THUMB_WIDTH / 2 + 1,
		   (SLIDER_H - THUMB_HEIGHT) / 2 + 1,
		   THUMB_WIDTH - 3, THUMB_HEIGHT - 3);
    XSetForeground(g_display, gc, private_colormap ? 0 : g_black);
    XDrawRectangle(g_display, XtWindow(slider), gc,
		   slider_x - THUMB_WIDTH / 2,
		   (SLIDER_H - THUMB_HEIGHT) / 2,
		   THUMB_WIDTH - 1, THUMB_HEIGHT - 1);
    XDrawRectangle(g_display, XtWindow(slider), gc,
		   slider_x - THUMB_WIDTH / 2 + 2,
		   (SLIDER_H - THUMB_HEIGHT) / 2 + 2,
		   THUMB_WIDTH - 5, THUMB_HEIGHT - 5);
}

/* private */ void
ColorPicker::removeThumb() {
    if (slider_x == -1)
	return;
    if (!XtIsRealized(slider))
	return;
    XPutImage(g_display, XtWindow(slider), gc, slider_image,
	      slider_x - THUMB_WIDTH / 2, (SLIDER_H - THUMB_HEIGHT) / 2,
	      slider_x - THUMB_WIDTH / 2, (SLIDER_H - THUMB_HEIGHT) / 2,
	      THUMB_WIDTH, THUMB_HEIGHT);
}

/* private */ void
ColorPicker::mouseInOldNew(XEvent *event) {
    if (event->type == ButtonPress && event->xbutton.y < OLDNEW_H / 2) {
	updateRGBTextFields(oldR, oldG, oldB);
	rgbChanged();
    }
}

/* private */ void
ColorPicker::mouseInWheel(XEvent *event) {
    // TODO: where are these flags defined? It looks like 0x100 is
    // button1, 0x200 is button2, and 0x300 is button3, but where is
    // that documented?
    if (event->type != ButtonPress &&
	(event->type != MotionNotify || (event->xbutton.state & 0x700) == 0))
	return;
    float x = ((float) event->xbutton.x - (CROSS_SIZE + WHEEL_DIAMETER) / 2)
		    / (WHEEL_DIAMETER / 2);
    float y = ((float) event->xbutton.y - (CROSS_SIZE + WHEEL_DIAMETER) / 2)
		    / (WHEEL_DIAMETER / 2);
    float s = sqrt(x * x + y * y);
    if (s > 1 && event->type != MotionNotify)
	return;

    int newcx, newcy;
    float th = atan2(y, -x);
    H = th / (2 * 3.141592654) + 0.5;
    if (s <= 1) {
	newcx = event->xbutton.x;
	newcy = event->xbutton.y;
    } else {
	s = 1;
	newcx = (int) (((1 - cos(th)) * WHEEL_DIAMETER + CROSS_SIZE + 1) / 2);
	newcy = (int) (((1 + sin(th)) * WHEEL_DIAMETER + CROSS_SIZE + 1) / 2);
    }

    if (cross_x == newcx && cross_y == newcy)
	return;

    removeCross();
    cross_x = newcx;
    cross_y = newcy;
    S = s;
    CopyBits::hsl2rgb(H, S, L, &R, &G, &B);
    updateRGBTextFields(R, G, B);
    drawCross();

    repaintOldNewImage();
    repaintSliderImage();
    drawThumb();
}

/* private */ void
ColorPicker::mouseInSlider(XEvent *event) {
    // TODO: where are these flags defined? It looks like 0x100 is
    // button1, 0x200 is button2, and 0x300 is button3, but where is
    // that documented?
    if (event->type != ButtonPress &&
	(event->type != MotionNotify || (event->xbutton.state & 0x700) == 0))
	return;
    float l = ((float) event->xbutton.x - CROSS_SIZE / 2) / WHEEL_DIAMETER;
    if ((l < 0 || l > 1) && event->type != MotionNotify)
	return;

    int newsx;
    if (l < 0) {
	l = 0;
	newsx = CROSS_SIZE / 2;
    } else if (l > 1) {
	l = 1;
	newsx = WHEEL_DIAMETER + CROSS_SIZE / 2;
    } else
	newsx = event->xbutton.x;

    if (slider_x == newsx)
	return;

    removeThumb();
    slider_x = newsx;
    L = l;
    CopyBits::hsl2rgb(H, S, L, &R, &G, &B);
    updateRGBTextFields(R, G, B);
    drawThumb();

    repaintOldNewImage();
    repaintWheelImage();
    drawCross();
}

/* private static */ void
ColorPicker::ok(Widget w, XtPointer ud, XtPointer cd) {
    ColorPicker *This = (ColorPicker *) ud;
    char *redS = XmTextGetString(This->red);
    char *greenS = XmTextGetString(This->green);
    char *blueS = XmTextGetString(This->blue);
    int r, g, b, n = 0;
    n += sscanf(redS, "%d", &r);
    n += sscanf(greenS, "%d", &g);
    n += sscanf(blueS, "%d", &b);
    XtFree(redS);
    XtFree(greenS);
    XtFree(blueS);
    if (n == 3 && r >= 0 && r <= 255 && g >= 0
	       && g <= 255 && b >= 0 && b <= 255)
	This->listener->colorPicked(r, g, b);
    else
	beep();
    This->close();
}

/* private static */ void
ColorPicker::cancel(Widget w, XtPointer ud, XtPointer cd) {
    ColorPicker *This = (ColorPicker *) ud;
    This->close();
}

/* private static */ void
ColorPicker::expose(Widget w, XtPointer ud, XtPointer cd) {
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    XExposeEvent ev = cbs->event->xexpose;
    ColorPicker *This = (ColorPicker *) ud;
    XImage *image;
    if (w == This->wheel)
	image = This->wheel_image;
    else if (w == This->slider)
	image = This->slider_image;
    else if (w == This->oldnew)
	image = This->oldnew_image;
    else
	return;
    XPutImage(g_display, XtWindow(w), gc, image,
	      ev.x, ev.y, ev.x, ev.y, ev.width, ev.height);
    if (w == This->wheel)
	This->drawCross();
    if (w == This->slider)
	This->drawThumb();
}

/* private static */ void
ColorPicker::input(Widget w, XtPointer ud, XtPointer cd) {
    ColorPicker *This = (ColorPicker *) ud;
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *) cd;
    XEvent *event = cbs->event;
    if (event->type == KeyPress) {
	// Look away, sensitive spirits! This is the kind of ugly code I
	// write when I really want that feature but am too lazy to do it
	// cleanly.
	// I'm handling keystrokes by creating fake mouse events and sending
	// them to mouseInWheel() and mouseInSlider(). All I populate in those
	// events is stuff that I happen to know those methods care about.
	// This is evil to the max and should be fixed sometime. (TODO)

	// Cursor keys move the cross 16 pixels at a time; shift-cursor
	// moves the cross one pixel at a time; pageup/pagedown moves the
	// slider 16 pixels at a time; shift-pageup/pagedown moves the slider
	// one pixel at a time.

	bool shift = event->xkey.state & 1;
	int motion_amount = shift ? 1 : 16;
	unsigned int keycode = event->xkey.keycode;

	XEvent my_event;
	my_event.type = MotionNotify;
	// TODO: uh-oh, hard-coded stuff again! Also hard-coded keycodes below.
	my_event.xbutton.state = 0x100;

	switch (keycode) {
	    case 98: // Up
		my_event.xbutton.x = This->cross_x;
		my_event.xbutton.y = This->cross_y - motion_amount;
		This->mouseInWheel(&my_event);
		break;
	    case 100: // Left
		my_event.xbutton.x = This->cross_x - motion_amount;
		my_event.xbutton.y = This->cross_y;
		This->mouseInWheel(&my_event);
		break;
	    case 102: // Right
		my_event.xbutton.x = This->cross_x + motion_amount;
		my_event.xbutton.y = This->cross_y;
		This->mouseInWheel(&my_event);
		break;
	    case 104: // Down
		my_event.xbutton.x = This->cross_x;
		my_event.xbutton.y = This->cross_y + motion_amount;
		This->mouseInWheel(&my_event);
		break;
	    case 99: // PageUp
		my_event.xbutton.x = This->slider_x + motion_amount;
		This->mouseInSlider(&my_event);
		break;
	    case 105: // PageDown
		my_event.xbutton.x = This->slider_x - motion_amount;
		This->mouseInSlider(&my_event);
		break;
	}
	return;
    } else if (event->type == KeyRelease)
	return;
    if (w == This->oldnew)
	This->mouseInOldNew(event);
    else if (w == This->wheel)
	This->mouseInWheel(event);
    else if (w == This->slider)
	This->mouseInSlider(event);
}

/* private static */ void
ColorPicker::modifyVerify(Widget w, XtPointer ud, XtPointer cd) {
    ColorPicker *This = (ColorPicker *) ud;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *) cd;

    char *rgbs;
    if (w == This->red)
	rgbs = This->rs;
    else if (w == This->green)
	rgbs = This->gs;
    else if (w == This->blue)
	rgbs = This->bs;

    if (strlen(rgbs) - (cbs->endPos - cbs->startPos)
			    + cbs->text->length >= 64) {
	// Too much for the wimpy buffer I want to use to check
	// the after-modification text; if it's that long it's
	// not valid anyway. Hey, I'm looking for an unsigned byte...
	cbs->doit = False;
	return;
    }

    char buf[64];
    strncpy(buf, rgbs, cbs->startPos);
    buf[cbs->startPos] = 0;
    strcat(buf, cbs->text->ptr);
    strcat(buf, rgbs + cbs->endPos);

    int v;
    if (strlen(buf) == 0 || sscanf(buf, "%d", &v) == 1 && v >= 0 && v <= 255)
	cbs->doit = True;
    else
	cbs->doit = False;
}

/* private static */ void
ColorPicker::valueChanged(Widget w, XtPointer ud, XtPointer cd) {
    ColorPicker *This = (ColorPicker *) ud;
    char *s = XmTextGetString(w);
    int v;
    if (sscanf(s, "%d", &v) != 1 || v < 0)
	v = 0;
    else if (v > 255)
	v = 255;

    if (w == This->red) {
	This->R = v;
	XtFree(This->rs);
	This->rs = s;
    } else if (w == This->green) {
	This->G = v;
	XtFree(This->gs);
	This->gs = s;
    } else if (w == This->blue) {
	This->B = v;
	XtFree(This->bs);
	This->bs = s;
    } else {
	XtFree(s);
	return;
    }
    This->rgbChanged();
}
