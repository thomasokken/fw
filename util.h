#ifndef UTIL_H
#define UTIL_H 1

char *strclone(const char *src);
void core();

class Map {
    private:
	struct entry {
	    char *key;
	    const void *value;
	};
	entry *entries;
	int nentries;
	int size;

    public:
	Map();
	~Map();
	void put(const char *key, const void *value);
	const void *get(const char *key);
	void remove(const char *key);

    private:
	static int entry_compare(const void *a, const void *b);
	void dump();
};

#endif
