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
	    const char *name;
	    const char *mnemonic;
	    const char *accelerator;
	    const char *id;
	    int type;
	    Menu *menu;
	    Menu *owner;
	    ItemNode *next;
	};
	ItemNode *firstitem, *lastitem;
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
	static void commandCB(Widget w, XtPointer ud, XtPointer cd);
	static void toggleCB(Widget w, XtPointer ud, XtPointer cd);
	static void radioCB(Widget w, XtPointer ud, XtPointer cd);
};

#endif
