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

#ifndef MENU_H
#define MENU_H 1

#include <X11/Intrinsic.h>

class Map;

class Menu {
    private:
	void (*commandCallback)(void *closure, const char *id);
	void *commandClosure;
	void (*toggleCallback)(void *closure, const char *id, bool value);
	void *toggleClosure;
	void (*radioCallback)(void *closure, const char *id, const char *value);
	void *radioClosure;

	Widget optionmenu;

	// Menu item types
	enum {
	    ITEM_SEPARATOR,
	    ITEM_MENU,
	    ITEM_COMMAND,
	    ITEM_TOGGLE,
	    ITEM_RADIO
	};

	struct ItemNode {
	    ItemNode(const char *name, const char *mnemonic, const char
		     *accelerator, const char *id, int type, Menu *owner);
	    ItemNode(const char *name, const char *mnemonic, const char
		     *accelerator, const char *id, Menu *menu, Menu *owner);
	    ItemNode();
	    ~ItemNode();
	    char *name;
	    char *mnemonic;
	    char *accelerator;
	    char *id;
	    int type;
	    Menu *menu;
	    Menu *owner;
	    ItemNode *next;
	    Widget widget;
	};
	ItemNode *firstitem, *lastitem;
	Widget parent;
	Map *commandIdToWidgetMap;
	Map *toggleIdToWidgetMap;
	Map *radioIdToWidgetMap;
	Map *radioGroupToSelectedWidgetMap;
	Map *radioGroupToSelectedIdMap;

    public:
	Menu();
	~Menu();
	void makeWidgets(Widget parent);
	void addCommand(const char *name, const char *mnemonic,
		     const char *accelerator, const char *id);
	void addToggle(const char *name, const char *mnemonic,
		     const char *accelerator, const char *id);
	void addRadio(const char *name, const char *mnemonic,
		     const char *accelerator, const char *id);
	void addMenu(const char *name, const char *mnemonic,
		     const char *accelerator, const char *id, Menu *menu);
	void addSeparator();
	void remove(const char *id);
	void changeLabel(const char *id, const char *label);
	void setCommandListener(void (*callback)(void *closure, const char *id),
			void *closure);
	void setToggleListener(void (*callback)(void *closure, const char *id,
			bool value), void *closure);
	void setRadioListener(void (*callback)(void *closure, const char *id,
			const char *value), void *closure);
	void setToggleValue(const char *id, bool value, bool doCallbacks);
	bool getToggleValue(const char *id);
	void setRadioValue(const char *id, const char *value, bool doCallbacks);
	const char *getRadioValue(const char *id);

	// OptionMenu support
	void setOptionMenu(Widget w);
	void setSelected(const char *id);

    private:
	void makeItem(Widget parent, ItemNode *item);
	static void commandCB(Widget w, XtPointer ud, XtPointer cd);
	static void toggleCB(Widget w, XtPointer ud, XtPointer cd);
	static void radioCB(Widget w, XtPointer ud, XtPointer cd);
};

#endif
