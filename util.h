#ifndef UTIL_H
#define UTIL_H 1

char *strclone(const char *src);
int crc32(void *buf, int size);
void crash();

class Iterator {
    public:
	Iterator();
	virtual ~Iterator();
	virtual bool hasNext() = 0;
	virtual void *next() = 0;
};

class Entry;

class Map {
    private:
	Entry *entries;
	int nentries;
	int size;

    public:
	Map();
	~Map();
	void put(const char *key, const void *value);
	const void *get(const char *key);
	void remove(const char *key);
	Iterator *keys();
	Iterator *values();

    private:
	void dump();
};

#endif
