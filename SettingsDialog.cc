#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <stdio.h>

#include "SettingsDialog.h"
#include "Plugin.h"
#include "main.h"

/* public */
SettingsDialog::SettingsDialog(void *settings, char **settings_layout,
			       Plugin *plugin) : Frame(false, true, false) {
    this->settings = settings;
    this->settings_layout = settings_layout;
    this->plugin = plugin;
    Widget form = getContainer();

    Widget left = XtVaCreateManagedWidget(
	    "Left", xmRowColumnWidgetClass,
	    form,
	    XmNorientation, XmVERTICAL,
	    XmNrowColumnType, XmWORK_AREA,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 0,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, 0,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 0,
	    NULL);

    Widget right = XtVaCreateManagedWidget(
	    "Right", xmRowColumnWidgetClass,
	    form,
	    XmNorientation, XmVERTICAL,
	    XmNrowColumnType, XmWORK_AREA,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 0,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 0,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, left,
	    XmNleftOffset, 0,
	    NULL);

    Widget labels[20];
    XmString xms;

    for (int i = 0; i < 20; i++) {
	Widget parent = i < 10 ? left : right;
	Widget subform = XtVaCreateManagedWidget(
		"SubForm", xmFormWidgetClass,
		parent,
		XmNautoUnmanage, False,
		XmNmarginHeight, 0,
		XmNmarginWidth, 0,
		NULL);

	tf[i] = XtVaCreateManagedWidget(
		"Text", xmTextWidgetClass,
		subform,
		XmNnavigationType,XmEXCLUSIVE_TAB_GROUP,
		XmNcolumns, 20,
		XmNmarginWidth, 0,
		XmNmarginHeight, 0,
		XmNeditMode, XmSINGLE_LINE_EDIT,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 0,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 0,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 0,
		NULL);

	xms = XmStringCreateLocalized("Label");
	labels[i] = XtVaCreateManagedWidget(
		"Label", xmLabelWidgetClass,
		subform,
		XmNalignment, XmALIGNMENT_END,
		XmNlabelString, xms,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 0,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 0,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, tf[i],
		XmNrightOffset, 0,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 0,
		NULL);
	XmStringFree(xms);
    }

    Widget buttons = XtVaCreateManagedWidget(
	    "Buttons", xmRowColumnWidgetClass,
	    form,
	    XmNorientation, XmVERTICAL,
	    XmNentryAlignment, XmALIGNMENT_CENTER,
	    XmNrowColumnType, XmWORK_AREA,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, right,
	    XmNleftOffset, 0,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNrightOffset, 0,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, 0,
	    NULL);

    xms = XmStringCreateLocalized("OK");
    Widget okB = XtVaCreateManagedWidget(
	    "Button", xmPushButtonWidgetClass,
	    buttons,
	    XmNlabelString, xms,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    NULL);
    XtAddCallback(okB, XmNactivateCallback, ok, (XtPointer) this);
    XmStringFree(xms);

    xms = XmStringCreateLocalized("Cancel");
    Widget cancelB = XtVaCreateManagedWidget(
	    "Button", xmPushButtonWidgetClass,
	    buttons,
	    XmNlabelString, xms,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    NULL);
    XtAddCallback(cancelB, XmNactivateCallback, cancel, (XtPointer) this);
    XmStringFree(xms);

    xms = XmStringCreateLocalized("Help");
    Widget helpB = XtVaCreateManagedWidget(
	    "Button", xmPushButtonWidgetClass,
	    buttons,
	    XmNlabelString, xms,
	    XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
	    NULL);
    XtAddCallback(helpB, XmNactivateCallback, help, (XtPointer) this);
    XmStringFree(xms);

    Widget radios = XtVaCreateManagedWidget(
	    "Radios", xmRowColumnWidgetClass,
	    form,
	    XmNradioBehavior, True,
	    XmNorientation, XmVERTICAL,
	    XmNentryAlignment, XmALIGNMENT_CENTER,
	    XmNrowColumnType, XmWORK_AREA,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, right,
	    XmNleftOffset, 0,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNrightOffset, 0,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, buttons,
	    XmNbottomOffset, 0,
	    NULL);

    bool depth_set = false;

    xms = XmStringCreateLocalized(" 1 Bit ");
    Widget radio1 = XtVaCreateManagedWidget(
	    "Radio", xmToggleButtonWidgetClass,
	    radios,
	    XmNlabelString, xms,
	    NULL);
    XtAddCallback(radio1, XmNvalueChangedCallback, depth1, (XtPointer) this);
    XmStringFree(xms);
    if (plugin->does_depth(1)) {
	XmToggleButtonSetState(radio1, True, True);
	depth_set = true;
    } else
	XtSetSensitive(radio1, False);

    xms = XmStringCreateLocalized(" 8 Bits");
    Widget radio8 = XtVaCreateManagedWidget(
	    "Radio", xmToggleButtonWidgetClass,
	    radios,
	    XmNlabelString, xms,
	    NULL);
    XtAddCallback(radio8, XmNvalueChangedCallback, depth8, (XtPointer) this);
    XmStringFree(xms);
    if (plugin->does_depth(8)) {
	if (!depth_set) {
	    XmToggleButtonSetState(radio8, True, True);
	    depth_set = true;
	}
    } else
	XtSetSensitive(radio8, False);

    xms = XmStringCreateLocalized("24 Bits");
    Widget radio24 = XtVaCreateManagedWidget(
	    "Radio", xmToggleButtonWidgetClass,
	    radios,
	    XmNlabelString, xms,
	    NULL);
    XtAddCallback(radio24, XmNvalueChangedCallback, depth24, (XtPointer) this);
    XmStringFree(xms);
    if (plugin->does_depth(24)) {
	if (!depth_set)
	    XmToggleButtonSetState(radio24, True, True);
    } else
	XtSetSensitive(radio24, False);

    char **p = settings_layout;
    char *ptr = (char *) settings;
    int t = 0;
    char text[256];
    while (*p != NULL) {
	char *s = *p;
	switch (s[0]) {
	    case 'i':
		snprintf(text, 256, "%d", *(int *) ptr);
		ptr += sizeof(int);
		break;
	    case 'd':
		snprintf(text, 256, "%f", *(double *) ptr);
		ptr += sizeof(double);
		break;
	    case 's':
		snprintf(text, 256, "%s", (char *) ptr);
		ptr += 256;
		break;
	}
	XmTextSetString(tf[t], text);
	xms = XmStringCreateLocalized(s + 1);
	XtVaSetValues(labels[t], XmNlabelString, xms, NULL);
	XmStringFree(xms);
	t++;
	p++;
    }

    while (t < 20) {
	XtSetMappedWhenManaged(tf[t], False);
	XtSetMappedWhenManaged(labels[t], False);
	t++;
    }

    snprintf(text, 256, "New %s", plugin->name());
    setTitle(text);
    setIconTitle(text);
    raise();
    XmProcessTraversal(tf[0], XmTRAVERSE_CURRENT);
}

/* public virtual */
SettingsDialog::~SettingsDialog() {
    //
}

/* public virtual */ void
SettingsDialog::close() {
    plugin->get_settings_cancel();
    delete this;
}

/* private static */ void
SettingsDialog::ok(Widget w, XtPointer ud, XtPointer cd) {
    ((SettingsDialog *) ud)->ok2();
}

/* private */ void
SettingsDialog::ok2() {
    hide();
    char **p = settings_layout;
    char *ptr = (char *) settings;
    int t = 0;
    while (*p != NULL) {
	char *s = *p;
	char *v = XmTextGetString(tf[t++]);
	switch (s[0]) {
	    case 'i':
		sscanf(v, "%d", (int *) ptr);
		ptr += sizeof(int);
		break;
	    case 'd':
		sscanf(v, "%lf", (double *) ptr);
		ptr += sizeof(double);
		break;
	    case 's':
		snprintf((char *) ptr, 256, "%s", v);
		ptr += 256;
		break;
	}
	XtFree(v);
	p++;
    }

    plugin->get_settings_ok();
    delete this;
}

/* private static */ void
SettingsDialog::cancel(Widget w, XtPointer ud, XtPointer cd) {
    ((SettingsDialog *) ud)->cancel2();
}

/* private */ void
SettingsDialog::cancel2() {
    hide();
    plugin->get_settings_cancel();
    delete this;
}

/* private static */ void
SettingsDialog::help(Widget w, XtPointer ud, XtPointer cd) {
    ((SettingsDialog *) ud)->help2();
}

/* private */ void
SettingsDialog::help2() {
    XBell(display, 100);
}

/* private static */ void
SettingsDialog::depth1(Widget w, XtPointer ud, XtPointer cd) {
    if (XmToggleButtonGetState(w))
	((SettingsDialog *) ud)->plugin->set_depth(1);
}

/* private static */ void
SettingsDialog::depth8(Widget w, XtPointer ud, XtPointer cd) {
    if (XmToggleButtonGetState(w))
	((SettingsDialog *) ud)->plugin->set_depth(8);
}

/* private static */ void
SettingsDialog::depth24(Widget w, XtPointer ud, XtPointer cd) {
    if (XmToggleButtonGetState(w))
	((SettingsDialog *) ud)->plugin->set_depth(24);
}
