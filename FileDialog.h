#ifndef FILEDIALOG_H
#define FILEDIALOG_H 1

#include "Frame.h"

class FileDialog : public Frame {
    private:
	Widget fsb;
	char **directory;
	void (*fileSelectedCB)(const char *filename, void *closure);
	void *fileSelectedClosure;

    public:
	FileDialog();
	virtual ~FileDialog();
	void setDirectory(char **dir);
	void setFileSelectedCB(void (*fileSelectedCB)(const char *fn, void *cl),
			       void *fileSelectedClosure);

    private:
	static void okOrCancel(Widget w, XtPointer cd, XtPointer ud);
};

#endif
