#include <X11/Xlib.h>
#include <stdlib.h>

#include "UndoManager.h"
#include "main.h"
#include "util.h"


/* private static */ int
UndoableAction::id_seq = 0;

/* public */
UndoableAction::UndoableAction() {
    id = id_seq++;
}

/* public virtual */
UndoableAction::~UndoableAction() {
    //
}

/* public */ int
UndoableAction::getId() {
    return id;
}


/* public::public */
UndoManager::Listener::Listener() {
    //
}

/* public::public virtual */
UndoManager::Listener::~Listener() {
    //
}

/* public */
UndoManager::UndoManager() {
    sp = -1;
    stack = new List;
    listeners = new List;
}

/* public */
UndoManager::~UndoManager() {
    clear();
    delete stack;
    Iterator *iter = listeners->iterator();
    while (iter->hasNext()) {
	Listener *listener = (Listener *) iter->next();
	delete listener;
    }
    delete iter;
    delete listeners;
}

/* public */ void
UndoManager::clear() {
    Iterator *iter = stack->iterator();
    while (iter->hasNext()) {
	UndoableAction *action = (UndoableAction *) iter->next();
	delete action;
    }
    delete iter;
    stack->clear();
}

/* public */ bool
UndoManager::isEmpty() {
    return sp == -1;
}

/* public */ void
UndoManager::addAction(UndoableAction *action) {
    for (int i = stack->size() - 1; i > sp; i--) {
	UndoableAction *action = (UndoableAction *) stack->remove(i);
	delete action;
    }
    stack->append(action);
    sp++;
    notifyListeners();
}

/* public */ void
UndoManager::undo() {
    if (sp >= 0) {
	UndoableAction *action = (UndoableAction *) stack->get(sp--);
	action->undo();
	notifyListeners();
    } else
	XBell(g_display, 100);
}

/* public */ void
UndoManager::redo() {
    if (sp < stack->size() - 1) {
	UndoableAction *action = (UndoableAction *) stack->get(++sp);
	action->redo();
	notifyListeners();
    } else
	XBell(g_display, 100);
}

/* public */ int
UndoManager::getCurrentId() {
    if (sp >= 0) {
	UndoableAction *action = (UndoableAction *) stack->get(sp);
	return action->getId();
    } else
	return -1;
}

/* public */ const char *
UndoManager::getUndoTitle() {
    if (sp > -1) {
	UndoableAction *action = (UndoableAction *) stack->get(sp);
	return action->getUndoTitle();
    } else
	return NULL;
}

/* public */ const char *
UndoManager::getRedoTitle() {
    if (sp < stack->size() - 1) {
	UndoableAction *action = (UndoableAction *) stack->get(sp + 1);
	return action->getRedoTitle();
    } else
	return NULL;
}

/* public */ void
UndoManager::addListener(Listener *listener) {
    listeners->append(listener);
}

/* public */ void
UndoManager::removeListener(Listener *listener) {
    listeners->remove(listener);
}

/* private */ void
UndoManager::notifyListeners() {
    const char *undoTitle = getUndoTitle();
    if (undoTitle == NULL)
	undoTitle = "Undo";
    const char *redoTitle = getRedoTitle();
    if (redoTitle == NULL)
	redoTitle = "Redo";
    Iterator *iter = listeners->iterator();
    while (iter->hasNext())
	((Listener *) iter->next())->titleChanged(undoTitle, redoTitle);
    delete iter;
}
