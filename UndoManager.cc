#include <stdlib.h>

#include "UndoManager.h"
#include "util.h"

/* public */
UndoManager::UndoManager() {
    sp = -1;
    stack = new List();
}

/* public */
UndoManager::~UndoManager() {
    clear();
    delete stack;
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
}

/* public */ void
UndoManager::undo() {
    if (sp >= 0)
	((UndoableAction *) stack->get(sp--))->undo();
    else
	crash();
}

/* public */ void
UndoManager::redo() {
    if (sp < stack->size())
	((UndoableAction *) stack->get(++sp))->redo();
    else
	crash();
}

/* public */ const char *
UndoManager::undoTitle() {
    if (sp > -1)
	return ((UndoableAction *) stack->get(sp))->undoTitle();
    else {
	crash();
	return NULL;
    }
}

/* public */ const char *
UndoManager::redoTitle() {
    if (sp < stack->size() - 1)
	return ((UndoableAction *) stack->get(sp + 1))->redoTitle();
    else {
	crash();
	return NULL;
    }
}
