#include <Xm/BulletinB.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>

#include "ColorPicker.h"
#include "main.h"

#define WHEEL_DIAMETER 150
#define CROSS_SIZE 16
#define SLIDER_HEIGHT 16
#define THUMB_OUTER_HEIGHT 24
#define THUMB_INNER_HEIGHT 20
#define THUMB_OUTER_WIDTH 10
#define THUMB_INNER_WIDTH 6
#define PREVIEW_WIDTH 80
#define PREVIEW_HEIGHT 64
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

    Widget form = getContainer();

    Widget bb = XtVaCreateManagedWidget(
	    "BulletinBoard",
	    xmBulletinBoardWidgetClass,
	    form,
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

    char buf[64];
    snprintf(buf, 64, "%d", r);
    XmTextSetString(red, buf);
    snprintf(buf, 64, "%d", g);
    XmTextSetString(green, buf);
    snprintf(buf, 64, "%d", b);
    XmTextSetString(blue, buf);

    colorwheel = XtVaCreateManagedWidget(
	    "ColorWheel",
	    xmDrawingAreaWidgetClass,
	    bb,
	    NULL);

    colorslider = XtVaCreateManagedWidget(
	    "Slider",
	    xmDrawingAreaWidgetClass,
	    bb,
	    NULL);

    oldandnew = XtVaCreateManagedWidget(
	    "OldAndNew",
	    xmDrawingAreaWidgetClass,
	    bb,
	    NULL);


    // Now, lay 'em all out...

    int mainlabel_x = MARGINS;
    int mainlabel_y = MARGINS;
    int mainlabel_w, mainlabel_h;
    pref_size(main_label, &mainlabel_w, &mainlabel_h);

    int wheel_x = MARGINS;
    int wheel_y = mainlabel_y + mainlabel_h + MARGINS;
    int wheel_w = WHEEL_DIAMETER + CROSS_SIZE;
    int wheel_h = WHEEL_DIAMETER + CROSS_SIZE;

    int slider_x = MARGINS;
    int slider_y = wheel_y + wheel_h + MARGINS;
    int slider_w = wheel_w;
    int slider_h = THUMB_OUTER_HEIGHT;

    int left_h = slider_y + slider_h + MARGINS;
    int left_w = mainlabel_w;
    if (left_w < wheel_w)
	left_w = wheel_w;
    if (left_w < slider_w)
	left_w = slider_w;
    left_w += MARGINS;
    
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
    int orignew_w = PREVIEW_WIDTH;
    int orignew_h = PREVIEW_HEIGHT;

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
    set_geom(colorwheel, wheel_x, wheel_y, wheel_w, wheel_h);
    set_geom(colorslider, slider_x, slider_y, slider_w, slider_h);
    set_geom(original_label, orig_x, orig_y, orig_w, orig_h);
    set_geom(new_label, new_x, new_y, new_w, new_h);
    set_geom(red_label, redlabel_x, redlabel_y, redlabel_w, redlabel_h);
    set_geom(green_label, greenlabel_x, greenlabel_y, greenlabel_w, greenlabel_h);
    set_geom(blue_label, bluelabel_x, bluelabel_y, bluelabel_w, bluelabel_h);
    set_geom(oldandnew, orignew_x, orignew_y, orignew_w, orignew_h);
    set_geom(red, red_x, red_y, red_w, red_h);
    set_geom(green, green_x, green_y, green_w, green_h);
    set_geom(blue, blue_x, blue_y, blue_w, blue_h);
    set_geom(okB, ok_x, ok_y, ok_w, ok_h);
    set_geom(cancelB, cancel_x, cancel_y, cancel_w, cancel_h);
    XtVaSetValues(bb, XmNwidth, total_w, XmNheight, total_h, NULL);
}

/* public virtual */
ColorPicker::~ColorPicker() {
    delete listener;
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
