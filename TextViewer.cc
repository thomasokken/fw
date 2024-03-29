///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>

#include "TextViewer.h"

/* public */
TextViewer::TextViewer(const char *text) : Frame(true, true, false) {
    setTitle("Text Viewer");
    setIconTitle("Text Viewer");

    Widget form = getContainer();
    Arg args[10];


    // Create "OK" button

    XmString label = XmStringCreateLocalized((char *) " OK ");
    XtSetArg(args[0], XmNlabelString, label);
    XtSetArg(args[1], XmNrightAttachment, XmATTACH_FORM);
    XtSetArg(args[2], XmNrightOffset, 5);
    XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNbottomOffset, 5);
    Widget okB = XtCreateManagedWidget(
            "OKButton",
            xmPushButtonWidgetClass,
            form,
            args, 5);
    XmStringFree(label);
    XtAddCallback(okB, XmNactivateCallback, ok, (XtPointer) this);
    XtOverrideTranslations(okB,
                XtParseTranslationTable("<Key>Return: ArmAndActivate()"));


    // Create text area

    XtSetArg(args[0], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[1], XmNtopOffset, 5);
    XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNleftOffset, 5);
    XtSetArg(args[4], XmNrightAttachment, XmATTACH_FORM);
    XtSetArg(args[5], XmNrightOffset, 5);
    XtSetArg(args[6], XmNbottomAttachment, XmATTACH_WIDGET);
    XtSetArg(args[7], XmNbottomWidget, okB);
    XtSetArg(args[8], XmNbottomOffset, 5);
    Widget scroll = XtCreateManagedWidget(
            "ScrolledWindow",
            xmScrolledWindowWidgetClass,
            form,
            args, 9);

    int rows, columns;
    getTextSize(text, &rows, &columns);
    if (rows < 5)
        rows = 5;
    if (columns < 64)
        columns = 64;
    else if (columns > 100)
        columns = 100;

    XtSetArg(args[0], XmNeditable, False);
    XtSetArg(args[1], XmNcursorPositionVisible, False);
    XtSetArg(args[2], XmNscrollHorizontal, False);
    XtSetArg(args[3], XmNscrollVertical, True);
    XtSetArg(args[4], XmNwordWrap, True);
    XtSetArg(args[5], XmNeditMode, XmMULTI_LINE_EDIT);
    XtSetArg(args[6], XmNrows, rows);
    XtSetArg(args[7], XmNcolumns, columns);
    XtSetArg(args[8], XmNtraversalOn, False);
    Widget textw = XtCreateManagedWidget(
            "Text",
            xmTextWidgetClass,
            scroll,
            args, 9);

    XmTextSetString(textw, (char *) text);
}

/* private static */ void
TextViewer::getTextSize(const char *text, int *lines, int *columns) {
    *lines = 1;
    *columns = 0;
    int pos = 0;

    char ch;
    do {
        ch = *text++;
        if (ch == 0 || ch == '\n') {
            if (*columns < pos)
                *columns = pos;
            pos = 0;
            if (ch != 0)
                (*lines)++;
        } else if (ch == '\t')
            pos = (pos + 8) & ~7;
        else
            pos++;
    } while (ch != 0);
}

/* private static */ void
TextViewer::ok(Widget w, XtPointer ud, XtPointer cd) {
    ((TextViewer *) ud)->close();
}
