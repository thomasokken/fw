#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <stdlib.h>

#include "Menu.h"
#include "util.h"

/* public */
Menu::Menu() {
    firstitem = NULL;
    commandCallback = NULL;
    toggleCallback = NULL;
    radioCallback = NULL;
    commandIdToWidgetMap = new Map;
    toggleIdToWidgetMap = new Map;
    radioIdToWidgetMap = new Map;
    radioGroupToSelectedWidgetMap = new Map;
    radioGroupToSelectedIdMap = new Map;
    optionmenu = NULL;
}

/* public */
Menu::~Menu() {
    while (firstitem != NULL) {
	ItemNode *next = firstitem->next;
	delete firstitem;
	firstitem = next;
    }
    delete commandIdToWidgetMap;
    delete toggleIdToWidgetMap;
    delete radioIdToWidgetMap;
    delete radioGroupToSelectedWidgetMap;
    delete radioGroupToSelectedIdMap;
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
Menu::addRadio(const char *name, const char *mnemonic,
	      const char *accelerator, const char *id) {
    ItemNode *item = new ItemNode(name, mnemonic, accelerator,
					    id, ITEM_RADIO, this);
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
	    Widget w = XtCreateManagedWidget("Command",
					     xmPushButtonWidgetClass,
					     parent,
					     args, nargs);
	    commandIdToWidgetMap->put(item->id, w);
	    XtAddCallback(w, XmNactivateCallback,
			  commandCB, (XtPointer) item);
	} else if (item->type == ITEM_TOGGLE) {
	    // Toggle menu item
	    Widget w = XtCreateManagedWidget("Toggle",
					     xmToggleButtonWidgetClass,
					     parent,
					     args, nargs);
	    toggleIdToWidgetMap->put(item->id, w);
	    XtAddCallback(w, XmNvalueChangedCallback,
			  toggleCB, (XtPointer) item);
	} else if (item->type == ITEM_RADIO) {
	    // Radio menu item
	    if (strchr(item->id, '@') == NULL) {
		fprintf(stderr, "Lame attempt to add radio w/o '@' in id.\n");
	    } else {
		XtSetArg(args[nargs], XmNindicatorType, XmONE_OF_MANY); nargs++;
		Widget w = XtCreateManagedWidget("Radio",
						 xmToggleButtonWidgetClass,
						 parent,
						 args, nargs);
		radioIdToWidgetMap->put(item->id, w);
		XtAddCallback(w, XmNvalueChangedCallback,
			    radioCB, (XtPointer) item);
	    }
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
		    const char *value), void *closure) {
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
    Widget w = (Widget) toggleIdToWidgetMap->get(id);
    if (w == NULL)
	return false;
    return XmToggleButtonGetState(w) == True;
}

/* public */ void
Menu::setToggleValue(const char *id, bool value, bool doCallbacks) {
    Widget w = (Widget) toggleIdToWidgetMap->get(id);
    if (w != NULL)
	XmToggleButtonSetState(w, value, doCallbacks);
}

/* public */ const char *
Menu::getRadioValue(const char *id) {
    return (const char *) radioGroupToSelectedIdMap->get(id);
}

/* public */ void
Menu::setRadioValue(const char *id, const char *value, bool doCallbacks) {
    char *buf = (char *) malloc(strlen(id) + strlen(value) + 2);
    strcpy(buf, id);
    strcat(buf, "@");
    strcat(buf, value);
    Widget new_w = (Widget) radioIdToWidgetMap->get(buf);
    free(buf);
    if (new_w == NULL) {
	fprintf(stderr, "Attempt to set nonexistent radio \"%s@%s\".\n", id, value);
	return;
    }
    Widget old_w = (Widget) radioGroupToSelectedWidgetMap->get(id);
    if (old_w != NULL)
	XmToggleButtonSetState(old_w, False, False);

    radioGroupToSelectedWidgetMap->put(id, new_w);
    radioGroupToSelectedIdMap->put(id, value);
    XmToggleButtonSetState(new_w, True, False);

    if (doCallbacks && radioCallback != NULL)
	radioCallback(radioClosure, id, value);
}

/* public */ void
Menu::setOptionMenu(Widget w) {
    optionmenu = w;
}

/* public */ void
Menu::setSelected(const char *id) {
    // Should be used when the menu is used with an OptionMenu
    if (optionmenu == NULL)
	crash();
    Widget sel_w = (Widget) commandIdToWidgetMap->get(id);
    if (sel_w != NULL) {
	Arg arg;
	XtSetArg(arg, XmNmenuHistory, sel_w);
	XtSetValues(optionmenu, &arg, 1);
    }
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

/* private static */ void
Menu::radioCB(Widget w, XtPointer ud, XtPointer cd) {
    ItemNode *item = (ItemNode *) ud;
    Menu *menu = item->owner;
    bool state = XmToggleButtonGetState(w) == True;
    if (!state) {
	XmToggleButtonSetState(w, True, False);
	return;
    }
    char *groupid = strclone(item->id);
    *strchr(groupid, '@') = 0;
    char *value = strchr(item->id, '@') + 1;
    Widget selected = (Widget) menu->radioGroupToSelectedWidgetMap->get(groupid);
    if (selected != NULL)
	XmToggleButtonSetState(selected, False, False);
    menu->radioGroupToSelectedWidgetMap->put(groupid, w);
    menu->radioGroupToSelectedIdMap->put(groupid, value);
	
    if (menu->radioCallback != NULL)
	menu->radioCallback(menu->radioClosure, groupid, value);
    free(groupid);
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
