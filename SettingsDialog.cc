#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <stdio.h>
#include <stdlib.h>

#include "SettingsDialog.h"
#include "Plugin.h"
#include "SettingsHelper.h"
#include "TextViewer.h"
#include "main.h"
#include "util.h"

/* public */
SettingsDialog::SettingsDialog(Plugin *plugin, SettingsHelper *settings)
					: Frame(false, true, false) {
    this->plugin = plugin;
    this->settings = settings;

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
    XtAddCallback(radio1, XmNvalueChangedCallback, depth1, (XtPointer) settings);
    XmStringFree(xms);
    if (settings->allowDepth(1)) {
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
    XtAddCallback(radio8, XmNvalueChangedCallback, depth8, (XtPointer) settings);
    XmStringFree(xms);
    if (settings->allowDepth(8)) {
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
    XtAddCallback(radio24, XmNvalueChangedCallback, depth24, (XtPointer) settings);
    XmStringFree(xms);
    if (settings->allowDepth(24)) {
	if (!depth_set)
	    XmToggleButtonSetState(radio24, True, True);
    } else
	XtSetSensitive(radio24, False);

    int nfields = settings->getFieldCount();
    if (nfields > 20)
	crash();
    for (int t = 0; t < nfields; t++) {
	char *value = settings->getFieldValue(t);
	XmTextSetString(tf[t], value);
	free(value);
	const char *label = settings->getFieldLabel(t);
	xms = XmStringCreateLocalized((char *) label);
	XtVaSetValues(labels[t], XmNlabelString, xms, NULL);
	XmStringFree(xms);
    }

    for (int t = nfields; t < 20; t++) {
	XtSetMappedWhenManaged(tf[t], False);
	XtSetMappedWhenManaged(labels[t], False);
    }

    char buf[256];
    snprintf(buf, 256, "New %s", plugin->name());
    setTitle(buf);
    setIconTitle(buf);
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
    int nfields = settings->getFieldCount();
    if (nfields > 20)
	crash();
    for (int t = 0; t < nfields; t++) {
	char *v = XmTextGetString(tf[t]);
	settings->setFieldValue(t, v);
	XtFree(v);
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
    const char *helptext = plugin->help();
    if (helptext == NULL) {
	char buf[1024];
	snprintf(buf, 1024, "The plugin \"%s\" does not provide help.",
		 plugin->name());
	TextViewer *tv = new TextViewer(buf);
	tv->raise();
    } else {
	TextViewer *tv = new TextViewer(helptext);
	tv->raise();
    }
}

/* private static */ void
SettingsDialog::depth1(Widget w, XtPointer ud, XtPointer cd) {
    if (XmToggleButtonGetState(w))
	((SettingsHelper *) ud)->setDepth(1);
}

/* private static */ void
SettingsDialog::depth8(Widget w, XtPointer ud, XtPointer cd) {
    if (XmToggleButtonGetState(w))
	((SettingsHelper *) ud)->setDepth(8);
}

/* private static */ void
SettingsDialog::depth24(Widget w, XtPointer ud, XtPointer cd) {
    if (XmToggleButtonGetState(w))
	((SettingsHelper *) ud)->setDepth(24);
}
