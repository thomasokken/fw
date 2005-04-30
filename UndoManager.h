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

#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H 1

class List;

class UndoableAction {
    private:
	static int id_seq;
	int id;
    public:
	UndoableAction();
	virtual ~UndoableAction();
	int getId();
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual const char *getUndoTitle() = 0;
	virtual const char *getRedoTitle() = 0;
};

class UndoManager {
    private:
	List *stack;
	int sp;
	List *listeners;

    public:
	class Listener {
	    public:
		Listener();
		virtual ~Listener();
		virtual void titleChanged(const char *undo, const char *redo)=0;
	};

	UndoManager();
	~UndoManager();
	void clear();
	bool isEmpty();
	void addAction(UndoableAction *action);
	void undo();
	void redo();
	int getCurrentId();
	const char *getUndoTitle();
	const char *getRedoTitle();
	void addListener(Listener *listener);
	void removeListener(Listener *listener);

    private:
	void notifyListeners();
};

#endif
