#ifndef FILEDIALOG_H
#define FILEDIALOG_H 1

#include "Frame.h"

class FileDialog : public Frame {
    public:
	class Listener {
	    public:
		Listener();
		virtual ~Listener();
		virtual void fileSelected(const char *filename) = 0;
		virtual void cancelled() = 0;
	};

    private:
	char **directory;
	Listener *listener;

    protected:
	Widget fsb;

    public:
	FileDialog(Frame *parent, Listener *listener);
	virtual ~FileDialog();
	virtual void close();
	void setDirectory(char **dir);

    private:
	static void okOrCancel(Widget w, XtPointer cd, XtPointer ud);
};

#endif
