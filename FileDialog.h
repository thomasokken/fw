#ifndef FILEDIALOG_H
#define FILEDIALOG_H 1

#include "Frame.h"
#include "YesNoCancelDialog.h"

class FileDialog : public Frame, private YesNoCancelDialog::Listener {
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
	bool confirmReplace;
	Listener *listener;
	YesNoCancelDialog *ync;
	char *filename;

    protected:
	Widget fsb;

    public:
	FileDialog(Frame *parent, bool confirmReplace, Listener *listener);
	virtual ~FileDialog();
	virtual void close();
	void setDirectory(char **dir);

    private:
	static void okOrCancel(Widget w, XtPointer cd, XtPointer ud);
	// YesNoCancelDialog::Listener methods
	virtual void yes();
	virtual void no();
	virtual void cancel();
};

#endif
