#ifndef MENU_H
#define MENU_H 1

#include <X11/Intrinsic.h>

class Menu {
    private:
	void (*commandCallback)(void *closure, const char *id);
	void *commandClosure;
	void (*toggleCallback)(void *closure, const char *id, bool value);
	void *toggleClosure;
	void (*radioCallback)(void *closure, const char *id, int value);
	void *radioClosure;

	enum { ITEM_MENU, ITEM_COMMAND, ITEM_TOGGLE, ITEM_RADIO };

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

    public:
	Menu();
	~Menu();
	void makeWidgets(Widget parent);
	void addItem(const char *name, const char *mnemonic,
		     const char *accelerator, const char *id);
	void addItem(const char *name, const char *mnemonic,
		     const char *accelerator, const char *id, Menu *menu);
	void addItem();
	void setCommandListener(void (*callback)(void *closure, const char *id),
			void *closure);
	void setToggleListener(void (*callback)(void *closure, const char *id,
			bool value), void *closure);
	void setRadioListener(void (*callback)(void *closure, const char *id,
			int value), void *closure);
	void setToggleValue(const char *id, bool value);
	void setRadioValue(const char *id, int value);
	bool getToggleValue(const char *id);
	int getRadioValue(const char *id);

    private:
	static void commandCB(Widget w, XtPointer ud, XtPointer cd);
};

#endif
