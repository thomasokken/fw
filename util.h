#ifndef UTIL_H
#define UTIL_H 1

class FWColor;

char *strclone(const char *src);
unsigned int crc32(const void *buf, int size);
bool isDirectory(const char *name);
bool isFile(const char *name);
bool is_grayscale(const FWColor *cmap);

int bool_alignment();
int char_alignment();
int short_alignment();
int int_alignment();
int long_alignment();
int long_long_alignment();
int float_alignment();
int double_alignment();
int long_double_alignment();
int string_alignment();

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
	void dump();
};

#endif
