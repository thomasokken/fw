#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H 1

#include "Frame.h"
class Plugin;
class PluginSettings;

class SettingsDialog : private Frame {
    private:
	Plugin *plugin;
	PluginSettings *settings;
	Widget tf[20];

    public:
	SettingsDialog(Plugin *plugin, PluginSettings *settings);
	virtual ~SettingsDialog();
	virtual void close();

    private:
	static void ok(Widget w, XtPointer ud, XtPointer cd);
	void ok2();
	static void cancel(Widget w, XtPointer ud, XtPointer cd);
	void cancel2();
	static void help(Widget w, XtPointer ud, XtPointer cd);
	void help2();
	static void depth1(Widget w, XtPointer ud, XtPointer cd);
	static void depth8(Widget w, XtPointer ud, XtPointer cd);
	static void depth24(Widget w, XtPointer ud, XtPointer cd);
};

#endif
