#include <Xm/Xm.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
#include <stdlib.h>

#include "FileDialog.h"
#include "main.h"
#include "util.h"

/* public */
FileDialog::FileDialog(Frame *parent)
	    : Frame(parent, true, false, true, false) {
    // I'm using the protected frame-or-dialog constructor from Frame,
    // so the FileDialog will be a modal dialog if 'parent' is non-null,
    // and a plain TopLevelShell if 'parent' is null.
    fsb = XmCreateFileSelectionBox(getContainer(), "FileSelBox", NULL, 0);
    XtAddCallback(fsb, XmNokCallback, okOrCancel, (XtPointer) this);
    XtAddCallback(fsb, XmNcancelCallback, okOrCancel, (XtPointer) this);
    XtManageChild(fsb);
    directory = NULL;
    fileSelectedCB = NULL;
    fileSelectedClosure = NULL;
    cancelledCB = NULL;
    cancelledClosure = NULL;
}

/* public virtual */
FileDialog::~FileDialog() {
    //
}

/* public virtual */ void
FileDialog::close() {
    if (cancelledCB != NULL)
	cancelledCB(cancelledClosure);
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

/* public */ void
FileDialog::setFileSelectedCB(void (*fileSelectedCB)(const char *fn, void *cl),
			      void *fileSelectedClosure) {
    this->fileSelectedCB = fileSelectedCB;
    this->fileSelectedClosure = fileSelectedClosure;
}

/* public */ void
FileDialog::setCancelledCB(void (*cancelledCB)(void *cl),
			   void *cancelledClosure) {
    this->cancelledCB = cancelledCB;
    this->cancelledClosure = cancelledClosure;
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
	    if (This->fileSelectedCB != NULL)
		This->fileSelectedCB(filename, This->fileSelectedClosure);
	    XtFree(filename);
	}
    } else if (cbs->reason == XmCR_CANCEL) {
	if (This->cancelledCB != NULL)
	    This->cancelledCB(This->cancelledClosure);
    }
}
