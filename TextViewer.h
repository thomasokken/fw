#ifndef TEXTVIEWER_H
#define TEXTVIEWER_H 1

#include "Frame.h"

class TextViewer : public Frame {
    public:
	TextViewer(const char *text);
    private:
	static void getTextSize(const char *text, int *lines, int *columns);
	static void ok(Widget w, XtPointer ud, XtPointer cd);
};

#endif
