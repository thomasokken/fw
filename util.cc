#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "util.h"
#include "FWColor.h"
#include "main.h"


extern int g_verbosity;


char *strclone(const char *src) {
    if (src == NULL)
	return NULL;
    int len = strlen(src);
    char *dst = new char[len + 1];
    strcpy(dst, src);
    return dst;
}

unsigned int crc32(const void *buf, int size) {
    static unsigned int crctab[256];
    static bool inited = false;
    
    if (!inited) {
	unsigned int crc = 0;
	for (int i = 0; i < 256; i++) {
	    crc = i << 24;
	    for (int j = 0; j < 8; j++) {
		if (crc & 0x80000000)
		    crc = (crc << 1) ^ 0x04c11db7;
		else
		    crc <<= 1;
	    }
	    crctab[i] = crc;
	}
	inited = true;
    }

    if (size < 4)
	return 0;
    
    unsigned int result = 0;
    const unsigned char *data = (const unsigned char *) buf;
    result = *data++ << 24;
    result |= *data++ << 16;
    result |= *data++ << 8;
    result |= *data++;
    result = ~result;
    size -= 4;

    for (int i = 0; i < size; i++)
	result = (result << 8 | *data++) ^ crctab[result >> 24];

    return result;
}

bool isDirectory(const char *name) {
    struct stat st;
    if (stat(name, &st) == -1)
	return false;
    return S_ISDIR(st.st_mode);
}

bool isFile(const char *name) {
    struct stat st;
    if (stat(name, &st) == -1)
	return false;
    return S_ISREG(st.st_mode);
}

bool is_grayscale(const FWColor *cmap) {
    for (int i = 0; i < 256; i++)
	if (cmap[i].r != i || cmap[i].g != i || cmap[i].b != i)
	    return false;
    return true;
}

unsigned long rgb2pixel(unsigned int r, unsigned int g, unsigned int b) {
    if (g_grayramp != NULL) {
	int p = ((306 * r + 601 * g + 117 * b)
		    * (g_rampsize - 1) + 130560) / 261120;
	return g_grayramp[p].pixel;
    } else if (g_colorcube != NULL) {
	int index =  (((r * (g_cubesize - 1) + 127) / 255)
		    * g_cubesize
		    + ((g * (g_cubesize - 1) + 127) / 255))
		    * g_cubesize
		    + ((b * (g_cubesize - 1) + 127) / 255);
	return g_colorcube[index].pixel;
    } else {
	static bool inited = false;
	static int rmax, rmult, bmax, bmult, gmax, gmult;
	if (!inited) {
	    rmax = g_visual->red_mask;
	    rmult = 0;
	    while ((rmax & 1) == 0) {
		rmax >>= 1;
		rmult++;
	    }
	    gmax = g_visual->green_mask;
	    gmult = 0;
	    while ((gmax & 1) == 0) {
		gmax >>= 1;
		gmult++;
	    }
	    bmax = g_visual->blue_mask;
	    bmult = 0;
	    while ((bmax & 1) == 0) {
		bmax >>= 1;
		bmult++;
	    }
	    inited = true;
	}
	return  (((r * rmax + 127) / 255) << rmult)
	      + (((g * gmax + 127) / 255) << gmult)
	      + (((b * bmax + 127) / 255) << bmult);
    }
}

int bool_alignment() {
    struct {
	char a;
	bool b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int char_alignment() {
    struct {
	char a;
	short b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int short_alignment() {
    struct {
	char a;
	short b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int int_alignment() {
    struct {
	char a;
	int b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int long_alignment() {
    struct {
	char a;
	long b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int long_long_alignment() {
    struct {
	char a;
	long long b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int float_alignment() {
    struct {
	char a;
	float b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int double_alignment() {
    struct {
	char a;
	double b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int long_double_alignment() {
    struct {
	char a;
	long double b;
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

int string_alignment() {
    struct {
	char a;
	char b[13];
    } foo;
    return ((long) &foo.b) - ((long) &foo.a);
}

void crash() {
    kill(0, SIGQUIT);
}


/* public */
Iterator::Iterator() {
    //
}

/* public virtual */
Iterator::~Iterator() {
    //
}

class ListIterator : public Iterator {
    private:
	void **array;
	int pos, length;
    public:
	ListIterator(void **array, int length) {
	    this->array = array;
	    this->length = length;
	    pos = 0;
	}
	virtual ~ListIterator() {
	    //
	}
	virtual bool hasNext() {
	    return pos < length;
	}
	virtual void *next() {
	    if (pos >= length)
		crash();
	    return array[pos++];
	}
};

/* public */
List::List() {
    array = NULL;
    capacity = 0;
    length = 0;
}

/* public */
List::~List() {
    if (array != NULL)
	free(array);
}

/* public */ void
List::clear() {
    length = 0;
}

/* public */ void
List::append(void *item) {
    if (length == capacity) {
	capacity++;
	array = (void **) realloc(array, capacity * sizeof(void *));
    }
    array[length++] = item;
}

/* public */ void
List::insert(int index, void *item) {
    if (index < 0 || index > length)
	crash();
    if (length == capacity) {
	capacity++;
	array = (void **) realloc(array, capacity * sizeof(void *));
    }
    memmove(array + index + 1, array + index,
	    (length - index) * sizeof(void *));
    array[index] = item;
    length++;
}

/* public */ void *
List::remove(int index) {
    if (index < 0 || index >= length)
	crash();
    void *result = array[index];
    memmove(array + index, array + index + 1,
	    (length - index - 1) * sizeof(void *));
    length--;
    return result;
}

/* public */ void
List::set(int index, void *item) {
    if (index < 0 || index >= length)
	crash();
    array[index] = item;
}

/* public */ void *
List::get(int index) {
    if (index < 0 || index >= length)
	crash();
    return array[index];
}

/* public */ int
List::size() {
    return length;
}

/* public */ Iterator *
List::iterator() {
    return new ListIterator(array, length);
}

class Entry {
    public:
	char *key;
	const void *value;
};

static int entry_compare(const void *a, const void *b) {
    Entry *A = (Entry *) a;
    Entry *B = (Entry *) b;
    return strcmp(A->key, B->key);
}

class MapIterator : public Iterator {
    private:
	bool doKeys;
	Entry *entries;
	int nentries;
	int pos;

    public:
	MapIterator(Entry *entries, int nentries, bool doKeys) {
	    this->doKeys = doKeys;
	    this->entries = entries;
	    this->nentries = nentries;
	    pos = 0;
	}
	virtual ~MapIterator() {
	    //
	}
	virtual bool hasNext() {
	    return pos < nentries;
	}
	virtual void *next() {
	    if (pos < nentries) {
		if (doKeys)
		    return entries[pos++].key;
		else
		    return (void *) entries[pos++].value;
	    } else {
		crash();
		return NULL;
	    }
	}
};

/* public */
Map::Map() {
    entries = NULL;
}

/* public */
Map::~Map() {
    if (entries != NULL) {
	for (int i = 0; i < nentries; i++)
	    free(entries[i].key);
	free(entries);
    }
}

/* public */ void
Map::put(const char *key, const void *value) {
    if (key == NULL || value == NULL) {
	crash();
	return;
    }

    if (entries == NULL) {
	entries = (Entry *) malloc(sizeof(Entry));
	entries[0].key = strclone(key);
	entries[0].value = value;
	nentries = 1;
	size = 1;
	if (g_verbosity >= 3)
	    dump();
	return;
    }

    int low = 0;
    int high = nentries - 1;
    while (low <= high) {
	int mid = (low + high) / 2;
	Entry *midVal = entries + mid;
	int res = strcmp(midVal->key, key);
	if (res < 0)
	    low = mid + 1;
	else if (res > 0)
	    high = mid - 1;
	else {
	    // Key found: replace value
	    midVal->value = value;
	    if (g_verbosity >= 3)
		dump();
	    return;
	}
    }
    // Key not found: insert at 'low'
    if (nentries == size) {
	// entries array full; grow it
	size++;
	entries = (Entry *) realloc(entries, size * sizeof(Entry));
    }
    memmove(entries + (low + 1), entries + low,
	    (nentries - low) * sizeof(Entry));
    entries[low].key = strclone(key);
    entries[low].value = value;
    nentries++;
    if (g_verbosity >= 3)
	dump();
}

/* public */ const void *
Map::get(const char *key) {
    if (key == NULL) {
	crash();
	return NULL;
    }

    if (entries == NULL)
	return NULL;

    Entry e;
    e.key = (char *) key;
    Entry *res = (Entry *) bsearch(&e, entries, nentries, sizeof(Entry),
				    entry_compare);
    return res == NULL ? NULL : res->value;
}

/* public */ void
Map::remove(const char *key) {
    if (key == NULL) {
	crash();
	return;
    }

    if (entries == NULL)
	return;

    Entry e;
    e.key = (char *) key;
    Entry *res = (Entry *) bsearch(&e, entries, nentries, sizeof(Entry),
				    entry_compare);
    if (res == NULL)
	// Not found
	return;
    free(res->key);
    int res_pos = res - entries;
    memmove(res, res + 1, (nentries - res_pos - 1) * sizeof(Entry));
    nentries--;
    if (g_verbosity >= 3)
	dump();
}

/* public */ Iterator *
Map::keys() {
    return new MapIterator(entries, nentries, true);
}

/* public */ Iterator *
Map::values() {
    return new MapIterator(entries, nentries, false);
}

/* public */ void
Map::dump() {
    fprintf(stderr, "Contents of Map instance %p (entries=%d, size=%d)\n",
	    this, nentries, size);
    for (int i = 0; i < nentries; i++)
	fprintf(stderr, "  \"%s\" => %p\n", entries[i].key, entries[i].value);
}
