class List;

class UndoableAction {
    public:
	UndoableAction() {}
	virtual ~UndoableAction() {}
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual const char *undoTitle() { return NULL; }
	virtual const char *redoTitle() { return NULL; }
};

class UndoManager {
    private:
	List *stack;
	int sp;
    public:
	UndoManager();
	~UndoManager();
	void clear();
	bool isEmpty();
	void addAction(UndoableAction *action);
	void undo();
	void redo();
	const char *undoTitle();
	const char *redoTitle();
};
