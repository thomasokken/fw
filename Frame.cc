#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <Xm/Form.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/RowColumn.h>
#include <X11/Xmu/Editres.h>

#include "Frame.h"
#include "Menu.h"
#include "main.h"

/* public */
Frame::Frame(bool resizable, bool centered, bool hasMenuBar) {
    this->centered = centered;
    this->menu = NULL;

    Arg args[12];
    int nargs = 0;
    XtSetArg(args[nargs], XmNtitle, "Frame"); nargs++;
    XtSetArg(args[nargs], XmNmappedWhenManaged, False); nargs++;
    XtSetArg(args[nargs], XmNallowShellResize, True); nargs++;
    XtSetArg(args[nargs], XmNmaxWidth, 32767); nargs++;
    XtSetArg(args[nargs], XmNmaxHeight, 32767); nargs++;
    XtSetArg(args[nargs], XmNdeleteResponse, XmDO_NOTHING); nargs++;
    XtSetArg(args[nargs], XmNscreen, screen); nargs++;
    XtSetArg(args[nargs], XmNiconPixmap, icon); nargs++;
    XtSetArg(args[nargs], XmNiconMask, iconmask); nargs++;
    XtSetArg(args[nargs], XmNiconName, "Icon"); nargs++;
    if (!resizable) {
	XtSetArg(args[nargs], XmNmwmFunctions,
	    MWM_FUNC_ALL | MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE); nargs++;
	XtSetArg(args[nargs], XmNmwmDecorations,
	    MWM_DECOR_ALL | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE); nargs++;
    }
    toplevel = XtAppCreateShell("fw", "FW", topLevelShellWidgetClass,
				display, args, nargs);

    XmAddWMProtocolCallback(toplevel,
			    XmInternAtom(display, "WM_DELETE_WINDOW", False),
			    deleteWindow,
			    (XtPointer) this);

    XtAddEventHandler(toplevel, 0, True,
		      (XtEventHandler) _XEditResCheckMessages,
		      NULL);

    Widget form = XtVaCreateManagedWidget("Form",
					  xmFormWidgetClass,
					  toplevel,
					  XmNautoUnmanage, False,
					  XmNmarginHeight, 0,
					  XmNmarginWidth, 0,
					  NULL);

    if (hasMenuBar) {
	XtSetArg(args[0], XmNtopAttachment, XmATTACH_FORM);
	XtSetArg(args[1], XmNtopOffset, 0);
	XtSetArg(args[2], XmNleftAttachment, XmATTACH_FORM);
	XtSetArg(args[3], XmNleftOffset, 0);
	XtSetArg(args[4], XmNrightAttachment, XmATTACH_FORM);
	XtSetArg(args[5], XmNrightOffset, 0);
	menubar = XmCreateMenuBar(form, "MenuBar", args, 6);
	XtManageChild(menubar);
    } else
	menubar = NULL;

    nargs = 0;
    XtSetArg(args[nargs], XmNmarginHeight, 0); nargs++;
    XtSetArg(args[nargs], XmNmarginWidth, 0); nargs++;
    if (hasMenuBar) {
	XtSetArg(args[nargs], XmNtopAttachment, XmATTACH_WIDGET); nargs++;
	XtSetArg(args[nargs], XmNtopWidget, menubar); nargs++;
    } else {
	XtSetArg(args[nargs], XmNtopAttachment, XmATTACH_FORM); nargs++;
    }
    XtSetArg(args[nargs], XmNtopOffset, 0); nargs++;
    XtSetArg(args[nargs], XmNleftAttachment, XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs], XmNleftOffset, 0); nargs++;
    XtSetArg(args[nargs], XmNrightAttachment, XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs], XmNrightOffset, 0); nargs++;
    XtSetArg(args[nargs], XmNbottomAttachment, XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs], XmNbottomOffset, 0); nargs++;
    container = XtCreateManagedWidget("Container",
				      xmFormWidgetClass,
				      form,
				      args, nargs);
}

/* public virtual */
Frame::~Frame() {
    if (menu != NULL)
	delete menu;
    XtDestroyWidget(toplevel);
}

/* public */ void
Frame::raise() {
    if (!XtIsRealized(toplevel)) {
	if (menu != NULL)
	    menu->makeWidgets(menubar);
	XtRealizeWidget(toplevel);
	if (centered) {
	    // Center the window on the screen
	    Arg args[2];
	    Dimension width, height;
	    XtSetArg(args[0], XmNwidth, &width);
	    XtSetArg(args[1], XmNheight, &height);
	    XtGetValues(toplevel, args, 2);
	    int x = (XWidthOfScreen(screen) - width) / 2;
	    int y = (XHeightOfScreen(screen) - height) / 2;
	    XtSetArg(args[0], XmNx, x);
	    XtSetArg(args[1], XmNy, y);
	    XtSetValues(toplevel, args, 2);

	    // This may seem redundant - it does to me, anyway - but it is
	    // needed to make window positioning work on NCR X terminals.
	    // Note that *both* this code, *and* the XtSetValues above,
	    // are necessary!
	    XSizeHints hints;
	    hints.flags = USPosition;
	    hints.x = x;
	    hints.y = y;
	    XSetWMNormalHints(display, XtWindow(toplevel), &hints);
	}
	XMapRaised(display, XtWindow(toplevel));
    } else {
	Window win = XtWindow(toplevel);

	// Grab server to avoid the race condition that would occur
	// if the window was iconified between the XGetWindowAttributes()
	// and XSetInputFocus() calls.

	XGrabServer(display);
	XWindowAttributes atts;
	XGetWindowAttributes(display, win, &atts);
	if (atts.map_state == IsViewable) {
	    XRaiseWindow(display, win);
	    XSetInputFocus(display, win, RevertToNone, CurrentTime);
	} else
	    XMapRaised(display, win);
	XUngrabServer(display);
    }
}

/* public */ void
Frame::hide() {
    XWithdrawWindow(display, XtWindow(toplevel), screennumber);
}

/* public */ void
Frame::setMenu(Menu *menu) {
    this->menu = menu;
}

/* public */ void
Frame::setTitle(const char *title) {
    XtVaSetValues(toplevel, XmNtitle, title, NULL);
}

/* public */ void
Frame::setIconTitle(const char *title) {
    XtVaSetValues(toplevel, XmNiconName, title, NULL);
}

/* public virtual */ void
Frame::close() {
    delete this;
}

/* protected */ Widget
Frame::getContainer() {
    return container;
}

/* private static */ void
Frame::deleteWindow(Widget w, XtPointer ud, XtPointer cd) {
    ((Frame *) ud)->close();
}
