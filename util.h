#ifndef UTIL_H
#define UTIL_H 1

class FWColor;

char *strclone(const char *src);

unsigned int crc32(const void *buf, int size);

bool isDirectory(const char *name);
bool isFile(const char *name);
char *basename(const char *fullname);
char *canonical_pathname(const char *fullname);

int bool_alignment();
int char_alignment();
int short_alignment();
int int_alignment();
int long_alignment();
int long_long_alignment();
int float_alignment();
int double_alignment();
int long_double_alignment();
int char_pointer_alignment();
int char_array_alignment();

void beep();

void crash();


class Iterator {
    public:
	Iterator();
	virtual ~Iterator();
	virtual bool hasNext() = 0;
	virtual void *next() = 0;
};

class List {
    private:
	void **array;
	int capacity;
	int length;
    public:
	List();
	~List();
	void clear();
	void append(void *item);
	void insert(int index, void *item);
	void *remove(int index);
	void remove(void *item);
	void set(int index, void *item);
	void *get(int index);
	int size();
	Iterator *iterator();
};

class Entry;

class Map {
    private:
	Entry *entries;
	int nentries;
	int capacity;

    public:
	Map();
	~Map();
	void put(const char *key, const void *value);
	const void *get(const char *key);
	void remove(const char *key);
	int size();
	Iterator *keys();
	Iterator *values();
	void dump();
};

#endif
