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
	const char *filetype, Listener *listener) : FileDialog(parent, this) {

    this->listener = listener;

    Widget form = XtCreateManagedWidget("TypeForm", xmFormWidgetClass,
					fsb, NULL, 0);

    Arg args[5];
    Widget pulldown = XmCreatePulldownMenu(form, "TypeList", NULL, 0);
    XmString label = XmStringCreateLocalized("File Tpe:");
    XtSetArg(args[0], XmNlabelString, label);
    XtSetArg(args[1], XmNsubMenuId, pulldown);
    XtSetArg(args[2], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNbottomAttachment, XmATTACH_FORM);

    Widget option = XmCreateOptionMenu(form, "Type", args, 5);
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
