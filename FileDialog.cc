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

#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
#include <stdio.h>
#include <stdlib.h>

#include "FileDialog.h"
#include "main.h"
#include "util.h"


/* public::public */
FileDialog::Listener::Listener() {
    //
}

/* public::public */
FileDialog::Listener::~Listener() {
    //
}


/* public */
FileDialog::FileDialog(Frame *parent, bool confirmReplace, Listener *listener)
            : Frame(parent, true, false, true, false) {
    // I'm using the protected frame-or-dialog constructor from Frame,
    // so the FileDialog will be a modal dialog if 'parent' is non-null,
    // and a plain TopLevelShell if 'parent' is null.
    fsb = XmCreateFileSelectionBox(getContainer(), (char *) "FileSelBox", NULL, 0);
    XtAddCallback(fsb, XmNokCallback, okOrCancel, (XtPointer) this);
    XtAddCallback(fsb, XmNcancelCallback, okOrCancel, (XtPointer) this);
    XtManageChild(fsb);
    directory = NULL;
    this->confirmReplace = confirmReplace;
    this->listener = listener;
    ync = NULL;
}

/* public virtual */
FileDialog::~FileDialog() {
    //
}

/* public virtual */ void
FileDialog::close() {
    listener->cancelled();
}

/* public */ void
FileDialog::setDirectory(char **directory) {
    this->directory = directory;
    if (directory != NULL && *directory != NULL) {
        XmString s = XmStringCreateLocalized(*directory);
        Arg arg;
        XtSetArg(arg, XmNdirectory, s);
        XtSetValues(fsb, &arg, 1);
        XmStringFree(s);
    }
}

/* private static */ void
FileDialog::okOrCancel(Widget w, XtPointer ud, XtPointer cd) {
    XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) cd;
    FileDialog *This = (FileDialog *) ud;
    if (cbs->reason == XmCR_OK) {
        if (This->directory != NULL) {
            XmString s;
            Arg arg;
            XtSetArg(arg, XmNdirectory, &s);
            XtGetValues(w, &arg, 1);
            char *dir;
            if (XmStringGetLtoR(s, XmFONTLIST_DEFAULT_TAG, &dir)) {
                if (*This->directory != NULL)
                    free(*This->directory);
                *This->directory = strclone(dir);
                XtFree(dir);
            }
            XmStringFree(s);
        }
        char *filename;
        if (XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &filename)) {
            if (This->confirmReplace && isFile(filename)) {
                This->filename = filename;
                char buf[1024];
                snprintf(buf, 1024, "Replace existing \"%s\"?", filename);
                This->ync = new YesNoCancelDialog(This, buf, This, false);
                This->ync->setTitle("Replace?");
                This->ync->raise();
                beep();
            } else {
                This->listener->fileSelected(filename);
                XtFree(filename);
            }
        } else
            This->listener->cancelled();
    } else if (cbs->reason == XmCR_CANCEL) {
        This->listener->cancelled();
    }
}

/* private */ void
FileDialog::yes() {
    delete ync;
    ync = NULL;
    listener->fileSelected(filename);
    XtFree(filename);
}

/* private */ void
FileDialog::no() {
    delete ync;
    ync = NULL;
    XtFree(filename);
}

/* private */ void
FileDialog::cancel() {
    // We're suppressing the cancel button, but of course the user can
    // still use the WM to close the window, and that causes cancel()
    // to be called.
    delete ync;
    ync = NULL;
    XtFree(filename);
}
