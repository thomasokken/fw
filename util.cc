#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "util.h"

extern int verbosity;

char *strclone(const char *src) {
    if (src == NULL)
	return NULL;
    int len = strlen(src);
    char *dst = new char[len + 1];
    strcpy(dst, src);
    return dst;
}

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
Map::put(const char *key, void *value) {
    if (key == NULL || value == NULL) {
	// dump core
	kill(0, SIGQUIT);
	return;
    }

    if (entries == NULL) {
	entries = (entry *) malloc(sizeof(entry));
	entries[0].key = strclone(key);
	entries[0].value = value;
	nentries = 1;
	size = 1;
	return;
    }

    int low = 0;
    int high = nentries - 1;
    while (low <= high) {
	int mid = (low + high) / 2;
	entry *midVal = entries + mid;
	int res = strcmp(midVal->key, key);
	if (res < 0)
	    low = mid + 1;
	else if (res > 0)
	    high = mid - 1;
	else {
	    // Key found: replace value
	    midVal->value = value;
	    return;
	}
    }
    // Key not found: insert at 'low'
    if (nentries == size) {
	// entries array full; grow it
	size++;
	entries = (entry *) realloc(entries, size * sizeof(entry));
    }
    memmove(entries + (low + 1), entries + low,
	    (nentries - low) * sizeof(entry));
    entries[low].key = strclone(key);
    entries[low].value = value;
    nentries++;
    if (verbosity >= 2)
	dump();
}

/* public */ void *
Map::get(const char *key) {
    if (key == NULL) {
	// dump core
	kill(0, SIGQUIT);
	return NULL;
    }

    entry e;
    e.key = (char *) key;
    entry *res = (entry *) bsearch(&e, entries, nentries, sizeof(entry),
				    entry_compare);
    return res == NULL ? NULL : res->value;
}

/* public */ void
Map::remove(const char *key) {
    if (key == NULL) {
	// dump core
	kill(0, SIGQUIT);
	return;
    }

    entry e;
    e.key = (char *) key;
    entry *res = (entry *) bsearch(&e, entries, nentries, sizeof(entry),
				    entry_compare);
    if (res == NULL)
	// Not found
	return;
    free(res->key);
    int res_pos = res - entries;
    memmove(res, res + 1, (nentries - res_pos - 1) * sizeof(entry));
    nentries--;
    if (verbosity >= 2)
	dump();
}

/* private static */ int
Map::entry_compare(const void *a, const void *b) {
    entry *A = (entry *) a;
    entry *B = (entry *) b;
    return strcmp(A->key, B->key);
}

/* private */ void
Map::dump() {
    fprintf(stderr, "Contents of Map instance %p (entries=%d, size=%d)\n",
	    this, nentries, size);
    for (int i = 0; i < nentries; i++)
	fprintf(stderr, "  \"%s\" => %p\n", entries[i].key, entries[i].value);
}
