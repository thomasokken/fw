#ifndef FILEDIALOG_H
#define FILEDIALOG_H 1

#include "Frame.h"

class FileDialog : public Frame {
    private:
	char **directory;
	void (*fileSelectedCB)(const char *filename, void *closure);
	void *fileSelectedClosure;
	void (*cancelledCB)(void *closure);
	void *cancelledClosure;

    protected:
	Widget fsb;

    public:
	FileDialog(Frame *parent);
	virtual ~FileDialog();
	virtual void close();
	void setDirectory(char **dir);
	void setFileSelectedCB(void (*fileSelectedCB)(const char *fn, void *cl),
			       void *fileSelectedClosure);
	void setCancelledCB(void (*cancelledCB)(void *cl),
			    void *cancelledClosure);

    private:
	static void okOrCancel(Widget w, XtPointer cd, XtPointer ud);
};

#endif
