#ifndef SAVEIMAGEDIALOG_H
#define SAVEIMAGEDIALOG_H 1

#include "FileDialog.h"

class Menu;

class SaveImageDialog : public FileDialog {
    private:
	void (*imageSelectedCB)(const char *fn, const char *type, void *cl);
	void *imageSelectedClosure;
	const char *type;
	Menu *typeMenu;

    public:
	SaveImageDialog();
	virtual ~SaveImageDialog();
	void setImageSelectedCB(
	    void (*imageSelectedCB)(const char *fn, const char *type, void *cl),
	    void *imageSelectedClosure);

    private:
	static void typeMenuCB(void *closure, const char *id);
	static void privateFileSelectedCallback(const char *fn, void *cl);
};

#endif
