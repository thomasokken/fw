#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
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
FileDialog::FileDialog(Frame *parent, Listener *listener)
	    : Frame(parent, true, false, true, false) {
    // I'm using the protected frame-or-dialog constructor from Frame,
    // so the FileDialog will be a modal dialog if 'parent' is non-null,
    // and a plain TopLevelShell if 'parent' is null.
    fsb = XmCreateFileSelectionBox(getContainer(), "FileSelBox", NULL, 0);
    XtAddCallback(fsb, XmNokCallback, okOrCancel, (XtPointer) this);
    XtAddCallback(fsb, XmNcancelCallback, okOrCancel, (XtPointer) this);
    XtManageChild(fsb);
    directory = NULL;
    this->listener = listener;
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
	    This->listener->fileSelected(filename);
	    XtFree(filename);
	}
    } else if (cbs->reason == XmCR_CANCEL) {
	This->listener->cancelled();
    }
}
