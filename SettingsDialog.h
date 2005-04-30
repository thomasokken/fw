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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H 1

#include "Frame.h"

class Plugin;
class SettingsHelper;

class SettingsDialog : private Frame {
    private:
	Plugin *plugin;
	SettingsHelper *settings;
	Widget tf[20];

    public:
	SettingsDialog(Plugin *plugin, SettingsHelper *settings);
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
