#ifndef CLOSECONFIRMDIALOG_H
#define CLOSECONFIRMDIALOG_H 1

#include "Frame.h"

class CloseConfirmDialog : public Frame {
    public:
	class Listener {
	    public:
		Listener();
		virtual ~Listener();
		virtual void yes() = 0;
		virtual void no() = 0;
		virtual void cancel() = 0;
	};
	    
    private:
	Listener *listener;

    public:
	CloseConfirmDialog(Frame *parent, const char *message,
			   Listener *listener);
	virtual ~CloseConfirmDialog();
	virtual void close();

    private:
	static void yesCB(Widget w, XtPointer cd, XtPointer ud);
	static void noCB(Widget w, XtPointer cd, XtPointer ud);
	static void cancelCB(Widget w, XtPointer cd, XtPointer ud);
};

#endif
