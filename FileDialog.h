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

#ifndef FILEDIALOG_H
#define FILEDIALOG_H 1

#include "Frame.h"
#include "YesNoCancelDialog.h"

class FileDialog : public Frame, private YesNoCancelDialog::Listener {
    public:
	class Listener {
	    public:
		Listener();
		virtual ~Listener();
		virtual void fileSelected(const char *filename) = 0;
		virtual void cancelled() = 0;
	};

    private:
	char **directory;
	bool confirmReplace;
	Listener *listener;
	YesNoCancelDialog *ync;
	char *filename;

    protected:
	Widget fsb;

    public:
	FileDialog(Frame *parent, bool confirmReplace, Listener *listener);
	virtual ~FileDialog();
	virtual void close();
	void setDirectory(char **dir);

    private:
	static void okOrCancel(Widget w, XtPointer cd, XtPointer ud);
	// YesNoCancelDialog::Listener methods
	virtual void yes();
	virtual void no();
	virtual void cancel();
};

#endif
