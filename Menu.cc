#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>

#include "Menu.h"
#include "util.h"

/* public */
Menu::Menu() {
    firstitem = NULL;
    commandCallback = NULL;
    toggleCallback = NULL;
    radioCallback = NULL;
    toggleMap = new Map;
}

/* public */
Menu::~Menu() {
    while (firstitem != NULL) {
	ItemNode *next = firstitem->next;
	delete firstitem;
	firstitem = next;
    }
    delete toggleMap;
}

/* public */ void
Menu::addCommand(const char *name, const char *mnemonic,
	      const char *accelerator, const char *id) {
    ItemNode *item = new ItemNode(name, mnemonic, accelerator,
					    id, ITEM_COMMAND, this);
    if (firstitem == NULL)
	firstitem = lastitem = item;
    else {
	lastitem->next = item;
	lastitem = item;
    }
    item->next = NULL;
}

/* public */ void
Menu::addToggle(const char *name, const char *mnemonic,
	      const char *accelerator, const char *id) {
    ItemNode *item = new ItemNode(name, mnemonic, accelerator,
					    id, ITEM_TOGGLE, this);
    if (firstitem == NULL)
	firstitem = lastitem = item;
    else {
	lastitem->next = item;
	lastitem = item;
    }
    item->next = NULL;
}

/* public */ void
Menu::addMenu(const char *name, const char *mnemonic,
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
Menu::addSeparator() {
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
	if (item->type == ITEM_SEPARATOR) {
	    XtCreateManagedWidget("Separator",
				  xmSeparatorWidgetClass,
				  parent,
				  NULL, 0);
	} else if (item->type == ITEM_COMMAND) {
	    // Regular menu item
	    Widget w = XtCreateManagedWidget("Button",
					     xmPushButtonWidgetClass,
					     parent,
					     args, nargs);
	    XtAddCallback(w, XmNactivateCallback,
			  commandCB, (XtPointer) item);
	} else if (item->type == ITEM_TOGGLE) {
	    // Regular menu item
	    Widget w = XtCreateManagedWidget("Button",
					     xmToggleButtonWidgetClass,
					     parent,
					     args, nargs);
	    XtAddCallback(w, XmNvalueChangedCallback,
			  toggleCB, (XtPointer) item);
	    toggleMap->put(item->id, w);
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
Menu::setCommandListener(void (*callback)(void *closure, const char *id),
		  void *closure) {
    this->commandCallback = callback;
    this->commandClosure = closure;
    ItemNode *item = firstitem;
    while (item != NULL) {
	if (item->menu != NULL)
	    item->menu->setCommandListener(callback, closure);
	item = item->next;
    }
}

/* public */ void
Menu::setToggleListener(void (*callback)(void *closure, const char *id,
		    bool value), void *closure) {
    this->toggleCallback = callback;
    this->toggleClosure = closure;
    ItemNode *item = firstitem;
    while (item != NULL) {
	if (item->menu != NULL)
	    item->menu->setToggleListener(callback, closure);
	item = item->next;
    }
}

/* public */ void
Menu::setRadioListener(void (*callback)(void *closure, const char *id,
		    int value), void *closure) {
    this->radioCallback = callback;
    this->radioClosure = closure;
    ItemNode *item = firstitem;
    while (item != NULL) {
	if (item->menu != NULL)
	    item->menu->setRadioListener(callback, closure);
	item = item->next;
    }
}

/* public */ bool
Menu::getToggleValue(const char *id) {
    Widget w = (Widget) toggleMap->get(id);
    if (w == NULL)
	return false;
    return XmToggleButtonGetState(w) == True;
}

/* public */ void
Menu::setToggleValue(const char *id, bool value) {
    Widget w = (Widget) toggleMap->get(id);
    if (w != NULL)
	// arg 3 being False means no callbacks are invoked.
	XmToggleButtonSetState(w, value, False);
}

/* private static */ void
Menu::commandCB(Widget w, XtPointer ud, XtPointer cd) {
    ItemNode *item = (ItemNode *) ud;
    Menu *menu = item->owner;
    if (menu->commandCallback != NULL)
	menu->commandCallback(menu->commandClosure, item->id);
}

/* private static */ void
Menu::toggleCB(Widget w, XtPointer ud, XtPointer cd) {
    ItemNode *item = (ItemNode *) ud;
    Menu *menu = item->owner;
    bool value = XmToggleButtonGetState(w) == True;
    if (menu->toggleCallback != NULL)
	menu->toggleCallback(menu->toggleClosure, item->id, value);
}

/* private::public */
Menu::ItemNode::ItemNode(const char *name, const char *mnemonic,
			 const char *accelerator, const char *id,
			 int type, Menu *owner) {
    this->name = strclone(name);
    this->mnemonic = strclone(mnemonic);
    this->accelerator = strclone(accelerator);
    this->id = strclone(id);
    this->type = type;
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
    this->type = ITEM_MENU;
    this->menu = menu;
    this->owner = owner;
}

/* private::public */
Menu::ItemNode::ItemNode() {
    this->name = NULL;
    this->mnemonic = NULL;
    this->accelerator = NULL;
    this->id = NULL;
    this->type = ITEM_SEPARATOR;
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
