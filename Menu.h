#ifndef MENU_H
#define MENU_H 1

#include <X11/Intrinsic.h>

class Menu {
    private:
	void (*callback)(void *closure, const char *id);
	void *closure;
	struct ItemNode {
	    ItemNode(const char *name, const char *mnemonic, const char
		     *accelerator, const char *id, Menu *owner);
	    ItemNode(const char *name, const char *mnemonic, const char
		     *accelerator, const char *id, Menu *menu, Menu *owner);
	    ItemNode();
	    ~ItemNode();
	    const char *name;
	    const char *mnemonic;
	    const char *accelerator;
	    const char *id;
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
	void setListener(void (*callback)(void *closure, const char *id),
			 void *closure);

    private:
	static void widgetCallback(Widget w, XtPointer ud, XtPointer cd);
};

#endif
