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


char *strclone(const char *src) {
    if (src == NULL)
	return NULL;
    int len = strlen(src);
    char *dst = (char *) malloc(len + 1);
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

unsigned long rgb2pixel(unsigned char r, unsigned char g, unsigned char b) {
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

void rgb2hsl(unsigned char R, unsigned char G, unsigned char B,
	     float *H, float *S, float *L) {
    float var_R = R / 255.0;
    float var_G = G / 255.0;
    float var_B = B / 255.0;

    float var_Min = var_R < var_G ? var_R : var_G;
    if (var_B < var_Min)
	var_Min = var_B;
    float var_Max = var_R > var_G ? var_R : var_G;
    if (var_B > var_Max)
	var_Max = var_B;
    float del_Max = var_Max - var_Min;

    *L = (var_Max + var_Min) / 2;

    if (del_Max == 0) {
	*H = 0;
	*S = 0;
    } else {
	if (*L < 0.5)
	    *S = del_Max / (var_Max + var_Min);
	else
	    *S = del_Max / (2 - var_Max - var_Min);

	float del_R = ((var_Max - var_R) / 6 + del_Max / 2) / del_Max;
	float del_G = ((var_Max - var_G) / 6 + del_Max / 2) / del_Max;
	float del_B = ((var_Max - var_B) / 6 + del_Max / 2) / del_Max;

	if (var_R == var_Max)
	    *H = del_B - del_G;
	else if (var_G == var_Max)
	    *H = 1.0 / 3 + del_R - del_B;
	else if (var_B == var_Max)
	    *H = 2.0 / 3 + del_G - del_R;

	if (*H < 0) *H += 1;
	if (*H > 1) *H -= 1;
    }
}

static float hue2rgb(float v1, float v2, float vH) {
   if (vH < 0) vH += 1;
   if (vH > 1) vH -= 1;
   if (6 * vH < 1) return v1 + (v2 - v1) * 6 * vH;
   if (2 * vH < 1) return v2;
   if (3 * vH < 2) return v1 + (v2 - v1) * (2.0 / 3 - vH) * 6;
   return v1;
}

void hsl2rgb(float H, float S, float L,
	     unsigned char *R, unsigned char *G, unsigned char *B) {
    if (S == 0) {
	*R = (unsigned char) (L * 255);
	*G = (unsigned char) (L * 255);
	*B = (unsigned char) (L * 255);
    } else {
	float var_2;
	if (L < 0.5)
	    var_2 = L * (1 + S);
	else
	    var_2 = (L + S) - (S * L);

	float var_1 = 2 * L - var_2;

	*R = (unsigned char) (255 * hue2rgb(var_1, var_2, H + 1.0 / 3));
	*G = (unsigned char) (255 * hue2rgb(var_1, var_2, H));
	*B = (unsigned char) (255 * hue2rgb(var_1, var_2, H - 1.0 / 3));
    }
}

void rgb2hsv(unsigned char R, unsigned char G, unsigned char B,
	     float *H, float *S, float *V) {
    float var_R = R / 255.0;
    float var_G = G / 255.0;
    float var_B = B / 255.0;

    float var_Min = var_R < var_G ? var_R : var_G;
    if (var_B < var_Min)
	var_Min = var_B;
    float var_Max = var_R > var_G ? var_R : var_G;
    if (var_B > var_Max)
	var_Max = var_B;
    float del_Max = var_Max - var_Min;

    *V = var_Max;

    if (del_Max == 0) {
	*H = 0;
	*S = 0;
    } else {
	*S = del_Max / var_Max;

	float del_R = ((var_Max - var_R) / 6 + del_Max / 2) / del_Max;
	float del_G = ((var_Max - var_G) / 6 + del_Max / 2) / del_Max;
	float del_B = ((var_Max - var_B) / 6 + del_Max / 2) / del_Max;

	if (var_R == var_Max)
	    *H = del_B - del_G;
	else if (var_G == var_Max)
	    *H = (1.0 / 3) + del_R - del_B;
	else if (var_B == var_Max)
	    *H = ( 2 / 3 ) + del_G - del_R;

	if (*H < 0) *H += 1;
	if (*H > 1) *H -= 1;
    }
}

void hsv2rgb(float H, float S, float V,
	     unsigned char *R, unsigned char *G, unsigned char *B) {
    if (S == 0) {
	*R = (unsigned char) (V * 255);
	*G = (unsigned char) (V * 255);
	*B = (unsigned char) (V * 255);
    } else {
	float var_h = H * 6;
	int var_i = (int) var_h;
	float var_1 = V * (1 - S);
	float var_2 = V * (1 - S * (var_h - var_i));
	float var_3 = V * (1 - S * (1 - (var_h - var_i)));

	float var_r, var_g, var_b;
	if ( var_i == 0 ) {
	    var_r = V     ; var_g = var_3 ; var_b = var_1;
	} else if (var_i == 1) {
	    var_r = var_2 ; var_g = V     ; var_b = var_1;
	} else if (var_i == 2) {
	    var_r = var_1 ; var_g = V     ; var_b = var_3;
	} else if (var_i == 3) {
	    var_r = var_1 ; var_g = var_2 ; var_b = V;
	} else if (var_i == 4) {
	    var_r = var_3 ; var_g = var_1 ; var_b = V;
	} else {
	    var_r = V     ; var_g = var_1 ; var_b = var_2;
	}

	*R = (unsigned char) (var_r * 255);
	*G = (unsigned char) (var_g * 255);
	*B = (unsigned char) (var_b * 255);
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
List::remove(void *item) {
    for (int i = 0; i < length; i++)
	if (array[i] == item) {
	    remove(i);
	    return;
	}
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
	if (g_prefs->verbosity >= 3)
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
	    if (g_prefs->verbosity >= 3)
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
    if (g_prefs->verbosity >= 3)
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
    if (g_prefs->verbosity >= 3)
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
