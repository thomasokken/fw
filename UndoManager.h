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
