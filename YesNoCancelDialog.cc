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

#include <Xm/BulletinB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>

#include "YesNoCancelDialog.h"

/* public::public */
YesNoCancelDialog::Listener::Listener() {
    //
}

/* public::public virtual */
YesNoCancelDialog::Listener::~Listener() {
    //
}


static void pref_size(Widget w, int *width, int *height) {
    XtWidgetGeometry wg;
    XtQueryGeometry(w, NULL, &wg);
    *width = wg.width;
    *height = wg.height;
}

static void set_geom(Widget w, int x, int y, int width, int height) {
    Arg args[4];
    XtSetArg(args[0], XmNx, x);
    XtSetArg(args[1], XmNy, y);
    XtSetArg(args[2], XmNwidth, width);
    XtSetArg(args[3], XmNheight, height);
    XtSetValues(w, args, 4);
}


/* public */
YesNoCancelDialog::YesNoCancelDialog(Frame *parent, const char *message,
                                     Listener *listener,
                                     bool showCancel /* = true */) 
        : Frame(parent, true) {
    
    this->listener = listener;
    setTitle("Yes, No, Cancel?");
    
    Widget form = getContainer();
    Widget bb = XtVaCreateManagedWidget(
            "BulletinBoard",
            xmBulletinBoardWidgetClass,
            form,
            XmNmarginHeight, 0,
            XmNmarginWidth, 0,
            //XmNresizePolicy, XmRESIZE_GROW,
            XmNtopAttachment, XmATTACH_FORM,
            XmNleftAttachment, XmATTACH_FORM,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
            NULL);

    XmString s = XmStringCreateLocalized((char *) message);
    Widget label = XtVaCreateManagedWidget(
            "Message",
            xmLabelWidgetClass,
            bb,
            XmNlabelString, s,
            NULL);
    XmStringFree(s);

    s = XmStringCreateLocalized((char *) "Yes");
    Widget yesB = XtVaCreateManagedWidget(
            "Yes",
            xmPushButtonWidgetClass,
            bb,
            XmNlabelString, s,
            XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
            NULL);
    XmStringFree(s);
    XtAddCallback(yesB, XmNactivateCallback, yesCB, (XtPointer) this);

    s = XmStringCreateLocalized((char *) "No");
    Widget noB = XtVaCreateManagedWidget(
            "No",
            xmPushButtonWidgetClass,
            bb,
            XmNlabelString, s,
            XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
            NULL);
    XmStringFree(s);
    XtAddCallback(noB, XmNactivateCallback, noCB, (XtPointer) this);

    Widget cancelB;
    if (showCancel) {
        s = XmStringCreateLocalized((char *) "Cancel");
        cancelB = XtVaCreateManagedWidget(
                "Cancel",
                xmPushButtonWidgetClass,
                bb,
                XmNlabelString, s,
                XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
                NULL);
        XmStringFree(s);
        XtAddCallback(cancelB, XmNactivateCallback, cancelCB, (XtPointer) this);
    }

    int message_w, message_h;
    pref_size(label, &message_w, &message_h);

    int yes_w, yes_h;
    pref_size(yesB, &yes_w, &yes_h);
    int no_w, no_h;
    pref_size(noB, &no_w, &no_h);

    int button_w, button_h;
    button_w = yes_w;
    if (button_w < no_w)
        button_w = no_w;
    button_h = yes_h;
    if (button_h < no_h)
        button_h = no_h;

    if (showCancel) {
        int cancel_w, cancel_h;
        pref_size(cancelB, &cancel_w, &cancel_h);

        if (button_w < cancel_w)
            button_w = cancel_w;
        if (button_h < cancel_h)
            button_h = cancel_h;
    }

    int outer_margin = 10;
    int margin1 = 5; // Between message & buttons
    int margin2 = 5; // Between buttons

    int total_w = 3 * button_w + 2 * margin2;
    if (total_w < message_w)
        total_w = message_w;
    total_w += 2 * outer_margin;

    int total_h = 2 * outer_margin + margin1 + message_h + button_h;

    set_geom(label, (total_w - message_w) / 2, outer_margin,
                        message_w, message_h);
    int x;
    if (showCancel)
        x = (total_w - 3 * button_w - 2 * margin2) / 2;
    else
        x = (total_w - 2 * button_w - margin2) / 2;
    set_geom(yesB, x, outer_margin + message_h + margin1,
                button_w, button_h);
    x += button_w + margin2;
    set_geom(noB, x, outer_margin + message_h + margin1,
                button_w, button_h);
    if (showCancel) {
        x += button_w + margin2;
        set_geom(cancelB, x, outer_margin + message_h + margin1,
                    button_w, button_h);
    }

    XtVaSetValues(bb, XmNwidth, total_w, XmNheight, total_h, NULL);
}

/* public virtual */
YesNoCancelDialog::~YesNoCancelDialog() {
    //
}

/* public virtual */ void
YesNoCancelDialog::close() {
    listener->cancel();
}

/* private static */ void
YesNoCancelDialog::yesCB(Widget w, XtPointer ud, XtPointer cd) {
    YesNoCancelDialog *This = (YesNoCancelDialog *) ud;
    This->listener->yes();
}

/* private static */ void
YesNoCancelDialog::noCB(Widget w, XtPointer ud, XtPointer cd) {
    YesNoCancelDialog *This = (YesNoCancelDialog *) ud;
    This->listener->no();
}

/* private static */ void
YesNoCancelDialog::cancelCB(Widget w, XtPointer ud, XtPointer cd) {
    YesNoCancelDialog *This = (YesNoCancelDialog *) ud;
    This->listener->cancel();
}
