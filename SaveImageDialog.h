#ifndef SAVEIMAGEDIALOG_H
#define SAVEIMAGEDIALOG_H 1

#include "FileDialog.h"

class Menu;

class SaveImageDialog : public FileDialog {
    public:
	class Listener {
	    public:
		Listener();
		virtual ~Listener();
		virtual void save(const char *fn, const char *type) = 0;
		virtual void cancel() = 0;
	};

    private:
	Listener *listener;
	const char *type;
	Menu *typeMenu;

    public:
	SaveImageDialog(Frame *parent, const char *file, const char *type,
			Listener *listener);
	virtual ~SaveImageDialog();

    private:
	static void typeMenuCB(void *closure, const char *id);
	static void privateFileSelectedCallback(const char *fn, void *cl);
	static void privateCancelledCallback(void *cl);
};

#endif
