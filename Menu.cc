#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>

#include "Menu.h"
#include "util.h"

/* public */
Menu::Menu() {
    firstitem = NULL;
    callback = NULL;
}

/* public */
Menu::~Menu() {
    while (firstitem != NULL) {
	ItemNode *next = firstitem->next;
	delete firstitem;
	firstitem = next;
    }
}

/* public */ void
Menu::addItem(const char *name, const char *mnemonic,
	      const char *accelerator, const char *id) {
    ItemNode *item = new ItemNode(name, mnemonic, accelerator, id, this);
    if (firstitem == NULL)
	firstitem = lastitem = item;
    else {
	lastitem->next = item;
	lastitem = item;
    }
    item->next = NULL;
}

/* public */ void
Menu::addItem(const char *name, const char *mnemonic,
	      const char *accelerator, const char *id, Menu *menu) {
    ItemNode *item = new ItemNode(name, mnemonic, accelerator, id, menu, this);
    if (firstitem == NULL)
	firstitem = lastitem = item;
    else {
	lastitem->next = item;
	lastitem = item;
    }
    item->next = NULL;
}

/* public */ void
Menu::addItem() {
    ItemNode *item = new ItemNode;
    if (firstitem == NULL)
	firstitem = lastitem = item;
    else {
	lastitem->next = item;
	lastitem = item;
    }
    item->next = NULL;
}

/* public */ void
Menu::makeWidgets(Widget parent) {
    ItemNode *item = firstitem;
    Arg args[10];
    int nargs;
    while (item != NULL) {
	nargs = 0;
	XmString label = XmStringCreateLocalized((char *) item->name);
	XtSetArg(args[nargs], XmNlabelString, label); nargs++;
	KeySym ks = item->mnemonic != NULL ?
		    XStringToKeysym(item->mnemonic) : NoSymbol;
	XtSetArg(args[nargs], XmNmnemonic, ks); nargs++;
	XmString acc;
	if (item->accelerator != NULL) {
	    char *accKey = new char[strlen(item->accelerator) + 5];
	    char *plus = strchr(item->accelerator, '+');
	    if (plus == NULL)
		strcpy(accKey, item->accelerator);
	    else {
		memcpy(accKey, item->accelerator, plus - item->accelerator);
		accKey[plus - item->accelerator] = 0;
		strcat(accKey, "<Key>");
		strcat(accKey, plus + 1);
	    }
	    XtSetArg(args[nargs], XmNaccelerator, accKey); nargs++;
	    acc = XmStringCreateLocalized((char *) item->accelerator);
	    XtSetArg(args[nargs], XmNacceleratorText, acc); nargs++;
	}
	if (item->id == NULL) {
	    XtCreateManagedWidget("Separator",
				  xmSeparatorWidgetClass,
				  parent,
				  NULL, 0);
	} else if (item->menu == NULL) {
	    // Regular menu item
	    Widget w = XtCreateManagedWidget("Button",
					     xmPushButtonWidgetClass,
					     parent,
					     args, nargs);
	    XtAddCallback(w, XmNactivateCallback,
			  widgetCallback, (XtPointer) item);
	} else {
	    // Submenu
	    Widget submenu = XmCreatePulldownMenu(parent, "Menu", NULL, 0);
	    XtSetArg(args[nargs], XmNsubMenuId, submenu); nargs++;
	    Widget w = XtCreateManagedWidget("Cascade",
					     xmCascadeButtonWidgetClass,
					     parent,
					     args, nargs);
	    if (strcmp(item->name, "Help") == 0)
		XtVaSetValues(parent, XmNmenuHelpWidget, w, NULL);
	    item->menu->makeWidgets(submenu);
	}
	XmStringFree(label);
	if (item->accelerator != NULL)
	    XmStringFree(acc);
	item = item->next;
    }
}

/* public */ void
Menu::setListener(void (*callback)(void *closure, const char *id),
		  void *closure) {
    this->callback = callback;
    this->closure = closure;
    ItemNode *item = firstitem;
    while (item != NULL) {
	if (item->menu != NULL)
	    item->menu->setListener(callback, closure);
	item = item->next;
    }
}

/* private static */ void
Menu::widgetCallback(Widget w, XtPointer ud, XtPointer cd) {
    ItemNode *item = (ItemNode *) ud;
    Menu *menu = item->owner;
    if (menu->callback != NULL)
	menu->callback(menu->closure, item->id);
}

/* private::public */
Menu::ItemNode::ItemNode(const char *name, const char *mnemonic,
			 const char *accelerator, const char *id,
			 Menu *owner) {
    this->name = strclone(name);
    this->mnemonic = strclone(mnemonic);
    this->accelerator = strclone(accelerator);
    this->id = strclone(id);
    this->menu = NULL;
    this->owner = owner;
}

/* private::public */
Menu::ItemNode::ItemNode(const char *name, const char *mnemonic,
			 const char *accelerator, const char *id,
			 Menu *menu, Menu *owner) {
    this->name = strclone(name);
    this->mnemonic = strclone(mnemonic);
    this->accelerator = strclone(accelerator);
    this->id = strclone(id);
    this->menu = menu;
    this->owner = owner;
}

/* private::public */
Menu::ItemNode::ItemNode() {
    this->name = NULL;
    this->mnemonic = NULL;
    this->accelerator = NULL;
    this->id = NULL;
    this->menu = NULL;
    this->owner = NULL;
}

/* private::public */
Menu::ItemNode::~ItemNode() {
    if (name != NULL)
	delete[] name;
    if (mnemonic != NULL)
	delete[] mnemonic;
    if (accelerator != NULL)
	delete[] accelerator;
    if (id != NULL)
	delete[] id;
    if (menu != NULL)
	delete menu;
}
