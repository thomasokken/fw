#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "util.h"


extern int g_verbosity;


char *strclone(const char *src) {
    if (src == NULL)
	return NULL;
    int len = strlen(src);
    char *dst = new char[len + 1];
    strcpy(dst, src);
    return dst;
}

int crc32(void *buf, int size) {
    // TODO -- actual CRCs!
    // Just XORing for now.
    int res = 0;
    int w = 0;
    for (int i = 0; i < size + 3; i++) {
	if (i >= size)
	    w <<= 8;
	else
	    w = (w << 8) + *(((unsigned char *) buf) + i);
	if ((i & 3) == 3) {
	    res ^= w;
	    w = 0;
	}
    }
    return res;
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
	    } else
		return NULL;
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
