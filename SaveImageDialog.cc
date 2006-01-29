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

#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <stdlib.h>

#include "SaveImageDialog.h"
#include "ImageIO.h"
#include "Menu.h"
#include "util.h"


/* public::public */
SaveImageDialog::Listener::Listener() {
    //
}

/* public::public */
SaveImageDialog::Listener::~Listener() {
    //
}


/* public */
SaveImageDialog::SaveImageDialog(Frame *parent, const char *filename,
				 const char *filetype, Listener *listener)
	: FileDialog(parent, true, this) {

    this->listener = listener;

    Arg args[2];
    Widget pulldown = XmCreatePulldownMenu(fsb, "TypeList", NULL, 0);
    XmString label = XmStringCreateLocalized("File Type:");
    XtSetArg(args[0], XmNlabelString, label);
    XtSetArg(args[1], XmNsubMenuId, pulldown);

    Widget option = XmCreateOptionMenu(fsb, "Type", args, 2);
    XtManageChild(option);

    Widget cascadebutton = XtNameToWidget(option, "OptionButton");
    XtSetArg(args[0], XmNalignment, XmALIGNMENT_BEGINNING);
    XtSetValues(cascadebutton, args, 1);

    typeMenu = new Menu;
    typeMenu->setOptionMenu(option);
    Iterator *iter = ImageIO::list();
    type = NULL;
    while (iter->hasNext()) {
	const char *t = (const char *) iter->next();
	typeMenu->addCommand(t, NULL, NULL, t);
	if (type == NULL)
	    type = t;
    }
    delete iter;
    typeMenu->setCommandListener(typeMenuCB, this);
    typeMenu->makeWidgets(pulldown);

    if (filename != NULL) {
	char *dirname;
	if (isDirectory(filename))
	    dirname = (char *) filename;
	else {
	    dirname = strclone(filename);
	    char *lastslash = strrchr(dirname, '/');
	    if (lastslash != NULL)
		lastslash[1] = 0;
	}
	XmString d = XmStringCreateLocalized(dirname);
	XtSetArg(args[0], XmNdirectory, d);
	XtSetValues(fsb, args, 1);
	XmStringFree(d);
	if (dirname != filename)
	    free(dirname);

	Widget text = XtNameToWidget(fsb, "Text");
	XtSetArg(args[0], XmNvalue, filename);
	XtSetArg(args[1], XmNcursorPosition, strlen(filename));
	XtSetValues(text, args, 2);

	typeMenu->setSelected(type);
	this->type = type;
    }
}

/* public virtual */
SaveImageDialog::~SaveImageDialog() {
    delete typeMenu;
}

/* public virtual */ void
SaveImageDialog::close() {
    listener->cancel();
}

/* private static */ void
SaveImageDialog::typeMenuCB(void *closure, const char *id) {
    SaveImageDialog *This = (SaveImageDialog *) closure;
    This->type = id;
}

/* private */ void
SaveImageDialog::fileSelected(const char *filename) {
    listener->save(filename, type);
}

/* private */ void
SaveImageDialog::cancelled() {
    listener->cancel();
}
