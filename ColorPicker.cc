#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>

#include "ColorPicker.h"
#include "main.h"

/* public */
ColorPicker::ColorPicker(Listener *listener, unsigned char r,
	unsigned char g, unsigned char b) : Frame(false, true, false) {
    this->listener = listener;

    Widget form = getContainer();

    Widget fields = XtVaCreateManagedWidget(
	    "RGBGrid",
	    xmRowColumnWidgetClass,
	    form,
	    XmNorientation, XmVERTICAL,
	    XmNentryAlignment, XmALIGNMENT_CENTER,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);

    Widget subform = XtVaCreateManagedWidget(
	    "RedForm", xmFormWidgetClass,
	    fields,
	    XmNautoUnmanage, False,
	    XmNmarginHeight, 0,
	    XmNmarginWidth, 0,
	    NULL);

    red = XtVaCreateManagedWidget(
	    "RedText",
	    xmTextWidgetClass,
	    subform,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    XmNcolumns, 5,
	    XmNmarginWidth, 0,
	    XmNmarginHeight, 0,
	    XmNeditMode, XmSINGLE_LINE_EDIT,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);

    XmString s = XmStringCreateLocalized("Red:");
    XtVaCreateManagedWidget(
	    "RedLabel",
	    xmLabelWidgetClass,
	    subform,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, red,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
    XmStringFree(s);


    subform = XtVaCreateManagedWidget(
	    "GreenForm", xmFormWidgetClass,
	    fields,
	    XmNautoUnmanage, False,
	    XmNmarginHeight, 0,
	    XmNmarginWidth, 0,
	    NULL);

    green = XtVaCreateManagedWidget(
	    "GreenText",
	    xmTextWidgetClass,
	    subform,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    XmNcolumns, 5,
	    XmNmarginWidth, 0,
	    XmNmarginHeight, 0,
	    XmNeditMode, XmSINGLE_LINE_EDIT,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);

    s = XmStringCreateLocalized("Green:");
    XtVaCreateManagedWidget(
	    "GreenLabel",
	    xmLabelWidgetClass,
	    subform,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, green,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
    XmStringFree(s);


    subform = XtVaCreateManagedWidget(
	    "BlueForm", xmFormWidgetClass,
	    fields,
	    XmNautoUnmanage, False,
	    XmNmarginHeight, 0,
	    XmNmarginWidth, 0,
	    NULL);

    blue = XtVaCreateManagedWidget(
	    "BlueText",
	    xmTextWidgetClass,
	    subform,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    XmNcolumns, 5,
	    XmNmarginWidth, 0,
	    XmNmarginHeight, 0,
	    XmNeditMode, XmSINGLE_LINE_EDIT,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);

    s = XmStringCreateLocalized("Blue:");
    XtVaCreateManagedWidget(
	    "BlueLabel",
	    xmLabelWidgetClass,
	    subform,
	    XmNalignment, XmALIGNMENT_END,
	    XmNlabelString, s,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, blue,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
    XmStringFree(s);


    Widget buttons = XtVaCreateManagedWidget(
	    "Buttons",
	    xmRowColumnWidgetClass,
	    form,
	    XmNorientation, XmVERTICAL,
	    XmNentryAlignment, XmALIGNMENT_CENTER,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, fields,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);

    s = XmStringCreateLocalized("OK");
    Widget okB = XtVaCreateManagedWidget(
	    "OK",
	    xmPushButtonWidgetClass,
	    buttons,
	    XmNlabelString, s,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    NULL);
    XtAddCallback(okB, XmNactivateCallback, ok, (XtPointer) this);
    XmStringFree(s);

    s = XmStringCreateLocalized("Cancel");
    Widget cancelB = XtVaCreateManagedWidget(
	    "Cancel",
	    xmPushButtonWidgetClass,
	    buttons,
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
