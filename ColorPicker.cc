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
#define CROSS_SIZE 16
#define SLIDER_HEIGHT 16
#define THUMB_OUTER_HEIGHT 24
#define THUMB_INNER_HEIGHT 20
#define THUMB_OUTER_WIDTH 10
#define THUMB_INNER_WIDTH 6

#define WHEEL_W (WHEEL_DIAMETER + CROSS_SIZE)
#define WHEEL_H (WHEEL_DIAMETER + CROSS_SIZE)
#define SLIDER_W WHEEL_W
#define SLIDER_H THUMB_OUTER_HEIGHT
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


/* public */
ColorPicker::ColorPicker(Listener *listener, unsigned char r,
	unsigned char g, unsigned char b) : Frame(false, true, false) {
    this->listener = listener;

    setTitle("Color Picker");
    setIconTitle("Color Picker");

    private_colormap = false;
    private_colorcells = false;
    disable_rgbChanged = false;

    R = oldR = r;
    G = oldG = g;
    B = oldB = b;
    rgb2hsl(R, G, B, &H, &S, &L);

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

    Widget bb = XtVaCreateManagedWidget(
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

    wheel = XtVaCreateManagedWidget(
	    "ColorWheel",
	    xmDrawingAreaWidgetClass,
	    bb,
	    NULL);

    slider = XtVaCreateManagedWidget(
	    "Slider",
	    xmDrawingAreaWidgetClass,
	    bb,
	    NULL);

    oldnew = XtVaCreateManagedWidget(
	    "OldAndNew",
	    xmDrawingAreaWidgetClass,
	    bb,
	    NULL);

    XtAddCallback(wheel, XmNexposeCallback, expose, (XtPointer) this);
    XtAddCallback(slider, XmNexposeCallback, expose, (XtPointer) this);
    XtAddCallback(oldnew, XmNexposeCallback, expose, (XtPointer) this);


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
	if (g_colorcube != NULL && g_cubesize == 6) {
	    unsigned long pixels[2];
	    if (XAllocColorCells(g_display, g_colormap, False, NULL, 0,
				 pixels, 2) != 0) {
		// It worked! Goody.
		private_colorcells = true;
		old_pixel = pixels[0];
		new_pixel = pixels[1];
		goto color_alloc_done;
	    }
	}

	// main.cc did not succeed in creating a 6x6x6 read-only color cube,
	// or we did not succeed in allocating two read-write color cells for
	// the "old" and "new" color display. So, now we play hardball and
	// allocate a private colormap.
	
	private_6x6x6_cube = XCreateColormap(g_display, g_rootwindow,
						 g_visual, AllocAll);
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

	XStoreColors(g_display, private_6x6x6_cube, colors, 216);
	setColormap(private_6x6x6_cube);

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
    if (private_colormap)
	XFreeColormap(g_display, private_6x6x6_cube);
    if (private_colorcells) {
	unsigned long pixels[2];
	pixels[0] = old_pixel;
	pixels[1] = new_pixel;
	XFreeColors(g_display, g_colormap, pixels, 2, 0);
    }
}

/* private */ void
ColorPicker::rgbChanged() {
    if (disable_rgbChanged)
	return;

    // Repaint the XImages

    // The "old/new" area requires special treatment in the case we are using
    // a private colormap or private colorcells. I'll just write the image data
    // manually in that case...

    if (private_colormap || private_colorcells) {
	for (int x = 1; x < OLDNEW_W - 1; x++) {
	    XPutPixel(oldnew_image, x, 0, 0);
	    XPutPixel(oldnew_image, x, OLDNEW_H / 2, 0);
	    XPutPixel(oldnew_image, x, OLDNEW_H - 1, 0);
	}
	for (int y = 0; y < OLDNEW_H; y++) {
	    XPutPixel(oldnew_image, 0, y, 0);
	    XPutPixel(oldnew_image, OLDNEW_W - 1, y, 0);
	}
	for (int y = 1; y < OLDNEW_H / 2; y++)
	    for (int x = 1; x < OLDNEW_W - 1; x++)
		XPutPixel(oldnew_image, x, y, old_pixel);
	for (int y = OLDNEW_H / 2 + 1; y < OLDNEW_H - 1; y++)
	    for (int x = 1; x < OLDNEW_W - 1; x++)

		XPutPixel(oldnew_image, x, y, new_pixel);
    } else {
	FWPixmap pm;
	pm.width = OLDNEW_W;
	pm.height = OLDNEW_H;
	pm.depth = 24;
	pm.bytesperline = pm.width * 4;
	pm.pixels = (unsigned char *) malloc(pm.height * pm.bytesperline);
	for (int x = 1; x < OLDNEW_W - 1; x++) {
	    pm.put_pixel(x, 0, 0);
	    pm.put_pixel(x, OLDNEW_H / 2, 0);
	    pm.put_pixel(x, OLDNEW_H - 1, 0);
	}
	for (int y = 0; y < OLDNEW_H; y++) {
	    pm.put_pixel(0, y, 0);
	    pm.put_pixel(OLDNEW_W - 1, y, 0);
	}
	unsigned long oldp = (oldR << 16) | (oldG << 8) | oldB;
	for (int y = 1; y < OLDNEW_H / 2; y++)
	    for (int x = 1; x < OLDNEW_W - 1; x++)
		pm.put_pixel(x, y, oldp);
	unsigned long newp = (R << 16) | (G << 8) | B;
	for (int y = OLDNEW_H / 2 + 1; y < OLDNEW_H - 1; y++)
	    for (int x = 1; x < OLDNEW_W - 1; x++)
		pm.put_pixel(x, y, newp);
	CopyBits::copy_unscaled(&pm, oldnew_image, false, true,
				0, 0, OLDNEW_H, OLDNEW_W);
	free(pm.pixels);
    }


    // The color wheel

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
    XtGetValues(wheel, &arg, 1);
    XQueryColor(g_display, private_colormap ? private_6x6x6_cube :
		g_colormap, &xc);
    unsigned long background = ((xc.red / 257) << 16)
				| ((xc.green / 257) << 8) | (xc.blue / 257);
    for (int y = 0; y < WHEEL_H; y++)
	for (int x = 0; x < WHEEL_W; x++)
	    pm.put_pixel(x, y, background);

    // The wheel itself
    int r = WHEEL_DIAMETER / 2;
    for (int y = -r; y <= r; y++) {
	int w = (int) (sqrt((double) (r * r - y * y)) + 0.5);
	for (int x = -w; x <= w; x++)
	    pm.put_pixel(x + r + CROSS_SIZE / 2,
			 y + r + CROSS_SIZE / 2,
			 0xFF0000);
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

    CopyBits::copy_unscaled(&pm, wheel_image, false, true,
			    0, 0, WHEEL_H, WHEEL_W);
    free(pm.pixels);

    if (XtIsRealized(oldnew))
	XPutImage(g_display, XtWindow(oldnew), g_gc, oldnew_image,
		  0, 0, 0, 0, OLDNEW_W, OLDNEW_H);
    if (XtIsRealized(wheel))
	XPutImage(g_display, XtWindow(wheel), g_gc, wheel_image,
		  0, 0, 0, 0, WHEEL_W, WHEEL_H);
    if (XtIsRealized(slider))
	XPutImage(g_display, XtWindow(slider), g_gc, slider_image,
		  0, 0, 0, 0, SLIDER_W, SLIDER_H);
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
	XBell(g_display, 100);
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
    XPutImage(g_display, XtWindow(w), g_gc, image,
	      ev.x, ev.y, ev.x, ev.y, ev.width, ev.height);
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
