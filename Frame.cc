#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/RowColumn.h>
#include <X11/Xmu/Editres.h>

#include "Frame.h"
#include "Menu.h"
#include "main.h"


/* private static */ bool
Frame::decor_known = false;

/* private static */ int
Frame::decor_width = 0;

/* private static */ int
Frame::decor_height = 0;

/* private static */ bool
Frame::taskbar_known = false;

/* private static */ int
Frame::taskbar_width = 0;

/* private static */ int
Frame::taskbar_height = 0;


/* protected */
Frame::Frame(Frame *parent, bool modal,
	     bool resizable, bool centered, bool hasMenuBar) {
    if (parent == NULL)
	init(resizable, centered, hasMenuBar);
    else
	init(parent, modal);
}

/* public */
Frame::Frame(bool resizable, bool centered, bool hasMenuBar) {
    init(resizable, centered, hasMenuBar);
}

/* public */
Frame::Frame(Frame *parent, bool modal) {
    init(parent, modal);
}


/* private */ void
Frame::init(bool resizable, bool centered, bool hasMenuBar) {
    this->centered = centered;
    this->menu = NULL;
    is_dialog = false;

    Arg args[11];
    int nargs = 0;
    XtSetArg(args[nargs], XmNtitle, "Frame"); nargs++;
    XtSetArg(args[nargs], XmNmappedWhenManaged, False); nargs++;
    XtSetArg(args[nargs], XmNallowShellResize, True); nargs++;
    XtSetArg(args[nargs], XmNdeleteResponse, XmDO_NOTHING); nargs++;
    XtSetArg(args[nargs], XmNscreen, g_screen); nargs++;
    XtSetArg(args[nargs], XmNiconPixmap, g_icon); nargs++;
    XtSetArg(args[nargs], XmNiconMask, g_iconmask); nargs++;
    XtSetArg(args[nargs], XmNiconName, "Icon"); nargs++;
    if (!resizable) {
	XtSetArg(args[nargs], XmNmwmFunctions,
	    MWM_FUNC_ALL | MWM_FUNC_RESIZE | MWM_FUNC_MAXIMIZE); nargs++;
	XtSetArg(args[nargs], XmNmwmDecorations,
	    MWM_DECOR_ALL | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE); nargs++;
    }
    toplevel = XtAppCreateShell("fw", "FW", topLevelShellWidgetClass,
				g_display, args, nargs);

    XmAddWMProtocolCallback(toplevel,
			    XmInternAtom(g_display, "WM_DELETE_WINDOW", False),
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

/* public */ void
Frame::init(Frame *parent, bool modal) {
    this->centered = false;
    this->menu = NULL;
    is_dialog = true;

    Arg args[11];
    int nargs = 0;
    XtSetArg(args[nargs], XmNtitle, "Frame"); nargs++;
    if (modal) {
	XtSetArg(args[nargs], XmNdialogStyle,
		 XmDIALOG_FULL_APPLICATION_MODAL);
	nargs++;
    }
    XtSetArg(args[nargs], XmNdeleteResponse, XmDO_NOTHING); nargs++;

    container = XmCreateFormDialog(parent->toplevel, "Dialog", args, nargs);
    toplevel = XtParent(container);

    XmAddWMProtocolCallback(toplevel,
			    XmInternAtom(g_display, "WM_DELETE_WINDOW", False),
			    deleteWindow,
			    (XtPointer) this);

    XtAddEventHandler(toplevel, 0, True,
		      (XtEventHandler) _XEditResCheckMessages,
		      NULL);

    menubar = NULL;
}

/* public virtual */
Frame::~Frame() {
    if (menu != NULL)
	delete menu;
    XtDestroyWidget(toplevel);
}

/* public */ void
Frame::raise() {
    if (is_dialog) {
	XtManageChild(container);
	return;
    }

    if (!XtIsRealized(toplevel)) {
	if (menu != NULL)
	    menu->makeWidgets(menubar);
	XtRealizeWidget(toplevel);

	if (decor_known)
	    fitToScreen();
	
	if (centered) {
	    // Center the window on the screen
	    // We may get here before decor size is known, and therefore before
	    // our window size has been reduced to fit the screen... Hopefully
	    // this is never a problem, as centering is something you'll want
	    // to do for dialog boxes, and those should never be too large to
	    // fit the screen anyway.

	    Arg args[2];
	    Dimension width, height;
	    XtSetArg(args[0], XmNwidth, &width);
	    XtSetArg(args[1], XmNheight, &height);
	    XtGetValues(toplevel, args, 2);
	    int x = (XWidthOfScreen(g_screen) - width) / 2;
	    int y = (XHeightOfScreen(g_screen) - height) / 2;
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
	    XSetWMNormalHints(g_display, XtWindow(toplevel), &hints);
	}

	if (!decor_known) {
	    XtAddEventHandler(toplevel, StructureNotifyMask, False, config,
				(XtPointer) this);
	    // Meanwhile, just do the best we can and size the window as
	    // though there are no decorations.
	    fitToScreen();
	}

	XMapRaised(g_display, XtWindow(toplevel));
    } else {
	Window win = XtWindow(toplevel);

	// Grab server to avoid the race condition that would occur
	// if the window was iconified between the XGetWindowAttributes()
	// and XSetInputFocus() calls.

	XGrabServer(g_display);
	XWindowAttributes atts;
	XGetWindowAttributes(g_display, win, &atts);
	if (atts.map_state == IsViewable) {
	    XRaiseWindow(g_display, win);
	    XSetInputFocus(g_display, win, RevertToNone, CurrentTime);
	} else
	    XMapRaised(g_display, win);
	XUngrabServer(g_display);
    }
}

/* public */ void
Frame::hide() {
    if (is_dialog)
	XtUnmanageChild(container);
    else
	XWithdrawWindow(g_display, XtWindow(toplevel), g_screennumber);
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

/* public */ const char *
Frame::getTitle() {
    const char *title;
    XtVaGetValues(toplevel, XmNtitle, &title, NULL);
    return title;
}

/* public virtual */ void
Frame::close() {
    if (is_dialog)
	hide();
    else
	delete this;
}

/* protected */ Widget
Frame::getContainer() {
    return container;
}

/* protected */ void
Frame::setColormap(Colormap xcmap) {
    Arg arg;
    XtSetArg(arg, XmNcolormap, xcmap);
    XtSetValues(toplevel, &arg, 1);
}

/* protected */ void
Frame::getSize(int *width, int *height) {
    Dimension w, h;
    Arg args[2];
    XtSetArg(args[0], XmNwidth, &w);
    XtSetArg(args[1], XmNheight, &h);
    XtGetValues(toplevel, args, 2);
    *width = w;
    *height = h;
}

/* protected */ void
Frame::setSize(int width, int height) {
    XtWidgetGeometry wg;
    XtQueryGeometry(menubar, NULL, &wg);
    if (width < wg.width)
	width = wg.width;
    Arg args[2];
    XtSetArg(args[0], XmNwidth, width);
    XtSetArg(args[1], XmNheight, height);
    XtSetValues(toplevel, args, 2);
}

/* protected */ void
Frame::getDecorSize(int *width, int *height) {
    findDecorSize();
    *width = decor_width;
    *height = decor_height;
}

/* protected */ void
Frame::getTaskBarSize(int *width, int *height) {
    findTaskBarSize();
    *width = taskbar_width;
    *height = taskbar_height;
}

/* private static */ void
Frame::deleteWindow(Widget w, XtPointer ud, XtPointer cd) {
    ((Frame *) ud)->close();
}

/* private */ void
Frame::findDecorSize() {
    // This method must not be called before the TopLevelShell is mapped,
    // because otherwise we aren't reparented by the WM yet, and we'd have no
    // way of finding out about the WM's decorations.

    if (decor_known)
	return;

    Window w = getWindow();

    // w is the outermost window, i.e. the window owned by the window
    // manager, which is a child of the root window, and which contains all
    // the decorations (and our own top level window, of course).

    Window root;
    int x, y;
    unsigned int width, height;
    unsigned int border_width, depth;
    if (XGetGeometry(g_display, w, &root, &x, &y, &width, &height,
			&border_width, &depth) == 0)
	// Fatal error
	return;

    width += 2 * border_width;
    height += 2 * border_width;

    Arg args[2];
    Dimension inner_width, inner_height;
    XtSetArg(args[0], XmNwidth, &inner_width);
    XtSetArg(args[1], XmNheight, &inner_height);
    XtGetValues(toplevel, args, 2);

    decor_width = width - inner_width;
    decor_height = height - inner_height;
    decor_known = true;
}

/* private */ void
Frame::findTaskBarSize() {
    if (taskbar_known)
	return;

    Window root;
    Window parent;
    Window *children;
    unsigned int nchildren;
    if (XQueryTree(g_display, g_rootwindow, &root, &parent,
		&children, &nchildren) == 0) 
	return;

    unsigned int scrnwidth = XWidthOfScreen(g_screen);
    unsigned int scrnheight = XHeightOfScreen(g_screen);

    for (unsigned int n = 0; n < nchildren; n++) {
	Window root;
	int x, y;
	unsigned int width, height;
	unsigned int border_width, depth;
	if (XGetGeometry(g_display, children[n], &root, &x, &y,
			    &width, &height,
			    &border_width, &depth) == 0)
	    // Fatal error
	    continue;

	width += border_width * 2;
	height += border_width * 2;

	if (!(width == scrnwidth
		&& height < scrnheight / 4
		&& height > 8
		&& x == 0
		&& (y == 0 || y == (int) (scrnheight - height)))
	    && !(height == scrnheight
		&& width < scrnwidth / 4
		&& width > 8
		&& (x == 0 || x == (int) (scrnwidth - width))
		&& y == 0))
	    continue;

	// TODO: add some extra checks to make sure we've really found a
	// task bar. For example, the fvwm95 taskbar has WM_NAME "FvwmTaskBar".
	// (So, what's the portable way to find the client window given a child
	// of the root window?!?)
	// Even better would be to actively track the task bar (i.e., find one
	// at startup, receive ConfigureNotify events on it, resize zoomed
	// windows to take changing task bar geometry into account; if a task
	// bar disappears, listen to MapNotify events on the root window to try
	// and detect when a new one gets mapped). It's a lot of yuck but for
	// one thing, it would save the overhead of looking for a task bar
	// whenever we create a new window, and for another, this is the kind
	// of behavior that a slick desktop app really should implement anyway
	// (so it can go straight into my bag of tricks for advanced X11
	// development :-) ).

	if (width == scrnwidth)
	    taskbar_height = height;
	else
	    taskbar_width = width;

	// Just the one task bar, thanks ma'am.
	break;
    }

    taskbar_known = true;
}

/* protected */ void
Frame::fitToScreen() {
    unsigned int scrnwidth = XWidthOfScreen(g_screen);
    unsigned int scrnheight = XHeightOfScreen(g_screen);
    
    // We must not call findDecorSize() here, because that method must not be
    // called before the window is mapped. Specifically, we need the window
    // manager to reparent our window, and that happens only after we request
    // to be mapped. For this reason, raise() registers a callback so we get
    // notified at the appropriate time, *only* then do we try to find the
    // decoration size.
    // If we did call findDecorSize() here, it would first get called on the
    // very first invocation of Frame::raise(), before the WM has reparented
    // our window, and it would find decor_width = decor_size = 0, and then set
    // decor_known = true, and that is not good.

    // Calling findTaskBarSize() is fine at this time. Presumably the task bar
    // will always exist before we get called.
    // *Proper* task bar support is another story. You really want to track
    // task bar geometry dynamically, instead of assuming (as we do here) that
    // it never changes within a session. The way MS Windows resizes maximized
    // windows when the task bar geometry changes is a good idea of how I would
    // like FW (and every X11 client, for that matter!) to behave.

    findTaskBarSize();

    Arg args[2];
    Dimension width, height;
    XtSetArg(args[0], XmNwidth, &width);
    XtSetArg(args[1], XmNheight, &height);
    XtGetValues(toplevel, args, 2);
    int maxwidth = scrnwidth - decor_width - taskbar_width;
    int maxheight = scrnheight - decor_height - taskbar_height;
    int n = 0;
    if (width > maxwidth) {
	XtSetArg(args[n], XmNwidth, maxwidth);
	n++;
    }
    if (height > maxheight) {
	XtSetArg(args[n], XmNheight, maxheight);
	n++;
    }
    if (n > 0)
	XtSetValues(toplevel, args, n);
}

/* protected */ Window
Frame::getWindow() {
    Window w = XtWindow(toplevel);
    while (true) {
	Window root;
	Window parent;
	Window *children;
	unsigned int nchildren;
	if (XQueryTree(g_display, w, &root, &parent, &children, &nchildren) == 0)
	    // Fatal error
	    return None;
	if (children != NULL)
	    XFree(children);
	if (root == parent)
	    return w;
	else
	    w = parent;
    }
}

/* private static */ void
Frame::config(Widget w, XtPointer closure, XEvent *event, Boolean *cont) {
    *cont = True;
    if (event->type != ConfigureNotify)
	return;
    Frame *This = (Frame *) closure;
    if (!This->decor_known) {
	This->findDecorSize();
    }
    if (This->decor_known) {
	XtRemoveEventHandler(w, StructureNotifyMask, False, config, closure);
	This->fitToScreen();
    }
}
