#include <Xm/Form.h>
#include <Xm/RowColumn.h>

#include "SaveImageDialog.h"
#include "ImageIO.h"
#include "Menu.h"
#include "util.h"


/* public */
SaveImageDialog::SaveImageDialog() {
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

    setFileSelectedCB(privateFileSelectedCallback, this);
}

/* public virtual */
SaveImageDialog::~SaveImageDialog() {
    delete typeMenu;
}

/* public */ void
SaveImageDialog::setImageSelectedCB(
	void (*imageSelectedCB)(const char *fn, const char *type, void *cl),
	void *imageSelectedClosure) {
    this->imageSelectedCB = imageSelectedCB;
    this->imageSelectedClosure = imageSelectedClosure;
}

/* private static */ void
SaveImageDialog::typeMenuCB(void *closure, const char *id) {
    SaveImageDialog *This = (SaveImageDialog *) closure;
    This->type = id;
}

/* private static */ void
SaveImageDialog::privateFileSelectedCallback(const char *fn, void *cl) {
    SaveImageDialog *This = (SaveImageDialog *) cl;
    This->imageSelectedCB(fn, This->type, This->imageSelectedClosure);
}
