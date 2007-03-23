///////////////////////////////////////////////////////////////////////////////
// Fractal Wizard -- a free fractal renderer for Linux
// Copyright (C) 1987-2005  Thomas Okken
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2,
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
///////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SettingsHelper.h"
#include "Plugin.h"
#include "util.h"


class Field {
    private:
	const char *label;
    protected:
	SettingsHelper *helper;
    public:
	const char *getLabel() {
	    return label;
	}
	virtual void setValue(const char *s) = 0;
	virtual char *getValue() = 0;
	virtual void serialize() = 0;
	virtual void deserialize() = 0;
	virtual ~Field() {}
    protected:
	Field(const char *label, SettingsHelper *helper) {
	    this->label = label;
	    this->helper = helper;
	}
	char *align(char *addr, int n) {
	    unsigned long a = (unsigned long) addr;
	    unsigned long r = a % (unsigned int) n;
	    if (r == 0)
		return addr;
	    else
		return addr + (n - r);
	}
};

class BoolField : public Field {
    private:
	bool *addr;
    public:
	BoolField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, bool_alignment());
	    this->addr = (bool *) *addr;
	    *addr += sizeof(bool);
	}
	virtual void setValue(const char *s) {
	    *addr = strcmp(s, "yes") == 0;
	}
	virtual char *getValue() {
	    return *addr ? strclone("yes") : strclone("no");
	}
	virtual void serialize() {
	    helper->appendbool(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readbool();
	}
};

class CharField : public Field {
    private:
	char *addr;
    public:
	CharField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, char_alignment());
	    this->addr = *addr;
	    *addr += sizeof(char);
	}
	virtual void setValue(const char *s) {
	    *addr = s[0];
	}
	virtual char *getValue() {
	    char ret[2];
	    ret[0] = *addr;
	    ret[1] = 0;
	    return strclone(ret);
	}
	virtual void serialize() {
	    helper->appendchar(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readchar();
	}
};

class ShortField : public Field {
    private:
	short *addr;
    public:
	ShortField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, short_alignment());
	    this->addr = (short *) *addr;
	    *addr += sizeof(short);
	}
	virtual void setValue(const char *s) {
	    sscanf(s, "%hd", addr);
	}
	virtual char *getValue() {
	    char buf[64];
	    sprintf(buf, "%d", *addr);
	    return strclone(buf);
	}
	virtual void serialize() {
	    helper->appendlonglong(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readlonglong();
	}
};

class IntField : public Field {
    private:
	int *addr;
	bool dont_serialize;
    public:
	IntField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, int_alignment());
	    this->addr = (int *) *addr;
	    *addr += sizeof(int);
	    dont_serialize = false;
	}
	IntField(const char *label, SettingsHelper *helper, int *addr)
		: Field(label, helper) {
	    this->addr = addr;
	    this->dont_serialize = true;
	}
	virtual void setValue(const char *s) {
	    sscanf(s, "%d", addr);
	}
	virtual char *getValue() {
	    char buf[64];
	    sprintf(buf, "%d", *addr);
	    return strclone(buf);
	}
	virtual void serialize() {
	    if (!dont_serialize)
		helper->appendlonglong(*addr);
	}
	virtual void deserialize() {
	    if (!dont_serialize)
		*addr = helper->readlonglong();
	}
};

class LongField : public Field {
    private:
	long *addr;
    public:
	LongField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, long_alignment());
	    this->addr = (long *) *addr;
	    *addr += sizeof(long);
	}
	virtual void setValue(const char *s) {
	    sscanf(s, "%ld", addr);
	}
	virtual char *getValue() {
	    char buf[64];
	    sprintf(buf, "%ld", *addr);
	    return strclone(buf);
	}
	virtual void serialize() {
	    helper->appendlonglong(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readlonglong();
	}
};

class LongLongField : public Field {
    private:
	long long *addr;
    public:
	LongLongField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, long_long_alignment());
	    this->addr = (long long *) *addr;
	    *addr += sizeof(long long);
	}
	virtual void setValue(const char *s) {
	    sscanf(s, "%lld", addr);
	}
	virtual char *getValue() {
	    char buf[64];
	    sprintf(buf, "%lld", *addr);
	    return strclone(buf);
	}
	virtual void serialize() {
	    helper->appendlonglong(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readlonglong();
	}
};

class FloatField : public Field {
    private:
	float *addr;
    public:
	FloatField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, float_alignment());
	    this->addr = (float *) *addr;
	    *addr += sizeof(float);
	}
	virtual void setValue(const char *s) {
	    sscanf(s, "%f", addr);
	}
	virtual char *getValue() {
	    char buf[64];
	    sprintf(buf, "%.8g", *addr);
	    return strclone(buf);
	}
	virtual void serialize() {
	    helper->appendfloat(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readfloat();
	}
};

class DoubleField : public Field {
    private:
	double *addr;
    public:
	DoubleField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, double_alignment());
	    this->addr = (double *) *addr;
	    *addr += sizeof(double);
	}
	virtual void setValue(const char *s) {
	    sscanf(s, "%lf", addr);
	}
	virtual char *getValue() {
	    char buf[64];
	    sprintf(buf, "%.16g", *addr);
	    return strclone(buf);
	}
	virtual void serialize() {
	    helper->appenddouble(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readdouble();
	}
};

class LongDoubleField : public Field {
    private:
	long double *addr;
    public:
	LongDoubleField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, long_double_alignment());
	    this->addr = (long double *) *addr;
	    *addr += sizeof(long double);
	}
	virtual void setValue(const char *s) {
	    sscanf(s, "%Lf", addr);
	}
	virtual char *getValue() {
	    char buf[64];
	    sprintf(buf, "%.20Lg", *addr);
	    return strclone(buf);
	}
	virtual void serialize() {
	    helper->appendlongdouble(*addr);
	}
	virtual void deserialize() {
	    *addr = helper->readlongdouble();
	}
};

class CharPointerField : public Field {
    private:
	char **addr;
    public:
	CharPointerField(const char *label, SettingsHelper *helper, char **addr)
		: Field(label, helper) {
	    *addr = align(*addr, char_pointer_alignment());
	    this->addr = (char **) *addr;
	    *addr += sizeof(char *);
	}
	virtual void setValue(const char *s) {
	    if (*addr != NULL)
		free(*addr);
	    *addr = strclone(s);
	}
	virtual char *getValue() {
	    return strclone(*addr);
	}
	virtual void serialize() {
	    if (*addr == NULL)
		helper->appendstring("");
	    else
		helper->appendstring(*addr);
	}
	virtual void deserialize() {
	    if (*addr != NULL)
		free(*addr);
	    *addr = helper->readstring();
	}
};

class CharArrayField : public Field {
    private:
	char *addr;
	int size;
    public:
	CharArrayField(const char *label, SettingsHelper *helper, char **addr, int size)
		: Field(label, helper) {
	    *addr = align(*addr, char_array_alignment());
	    this->addr = *addr;
	    *addr += size;
	    this->size = size;
	}
	virtual void setValue(const char *s) {
	    strncpy(addr, s, size);
	    addr[size - 1] = 0;
	}
	virtual char *getValue() {
	    return strclone(addr);
	}
	virtual void serialize() {
	    helper->appendstring(addr);
	}
	virtual void deserialize() {
	    char *s = helper->readstring();
	    strncpy(addr, s, size);
	    addr[size - 1] = 0;
	    free(s);
	}
};


/* public */
SettingsHelper::SettingsHelper(Plugin *plugin) {
    this->plugin = plugin;
    fields = NULL;
    nfields = 0;
    dlgfields = NULL;
    ndlgfields = 0;

    // REPEAT stack.
    // 100 levels should be plenty, no?
    int repcount[100];
    int repindex[100];
    int sp = -1;

    for (int i = 0; i < plugin->settings_count; i++) {
	char *addr = (char *) plugin->settings_base[i];
	const char **lines = plugin->settings_layout[i];
	int lineno = 0;

	while (lines[lineno] != NULL) {
	    char *line = strclone(lines[lineno]);
	    int length = strlen(line);
	    
	    // The first part of a line should be a type (bool, char, short,
	    // int, long, longlong, float, double, longdouble, char*, char[n]),
	    // or one of the special words WIDTH and HEIGHT (which are hooked
	    // to plugin->pm.width and plugin->pm.height, respectively, and
	    // which are skipped during serialization/deserialization, since
	    // the plugin->pm structure is populated by the pixmap reading
	    // process (the pm structure is owned by the viewer, not the
	    // plugin, strictly speaking)), or one of REPEAT or ENDREP.
	    
	    int pos = 0;
	    while (pos < length && !isspace(line[pos]))
		pos++;
	    if (pos == 0)
		// Empty line
		crash();
	    line[pos] = 0;
	    char *rest;
	    if (pos == length)
		rest = line + pos;
	    else {
		rest = line + pos + 1;
		while (*rest != 0 && isspace(*rest))
		    rest++;
	    }

	    // The remainder of the line is used as the field label (in single
	    // quotes), or in the case of the REPEAT keyword, as the repeat
	    // count (i.e. the number of times everything following the REPEAT
	    // keyword, up to the matching ENDREP keyword, should be repeated).
	    
	    if (strcmp(line, "REPEAT") == 0) {
		int count;
		if (sscanf(rest, "%d", &count) != 1)
		    crash();
		sp++;
		if (sp == 100)
		    crash();
		repindex[sp] = lineno;
		repcount[sp] = count;
	    } else if (strcmp(line, "ENDREP") == 0) {
		if (sp == -1)
		    crash();
		if (--repcount[sp] != 0)
		    lineno = repindex[sp];
		else
		    sp--;
	    } else {
		char *label = NULL;
		char *firstquote = strchr(rest, '\'');
		if (firstquote != NULL) {
		    char *lastquote = strrchr(rest, '\'');
		    if (firstquote == lastquote)
			crash();
		    label = firstquote + 1;
		    *lastquote = 0;
		}

		Field *field;
		if (strcmp(line, "WIDTH") == 0)
		    field = new IntField(label, this, &plugin->pm->width);
		else if (strcmp(line, "HEIGHT") == 0)
		    field = new IntField(label, this, &plugin->pm->height);
		else if (strcmp(line, "bool") == 0)
		    field = new BoolField(label, this, &addr);
		else if (strcmp(line, "char") == 0)
		    field = new CharField(label, this, &addr);
		else if (strcmp(line, "short") == 0)
		    field = new ShortField(label, this, &addr);
		else if (strcmp(line, "int") == 0)
		    field = new IntField(label, this, &addr);
		else if (strcmp(line, "long") == 0)
		    field = new LongField(label, this, &addr);
		else if (strcmp(line, "longlong") == 0)
		    field = new LongLongField(label, this, &addr);
		else if (strcmp(line, "float") == 0)
		    field = new FloatField(label, this, &addr);
		else if (strcmp(line, "double") == 0)
		    field = new DoubleField(label, this, &addr);
		else if (strcmp(line, "longdouble") == 0)
		    field = new LongDoubleField(label, this, &addr);
		else if (strcmp(line, "char*") == 0)
		    field = new CharPointerField(label, this, &addr);
		else if (strcmp(line, "char[") == 0) {
		    int size;
		    if (sscanf(line + 5, "%d", &size) != 1 || size <= 0)
			crash();
		    field = new CharArrayField(label, this, &addr, size);
		} else
		    crash();

		fields = (Field **) realloc(fields,
					(nfields + 1) * sizeof(Field *));
		fields[nfields++] = field;
		if (label != NULL) {
		    dlgfields = (Field **) realloc(dlgfields,
					(ndlgfields + 1) * sizeof(Field *));
		    dlgfields[ndlgfields++] = field;
		}
	    }

	    lineno++;
	}
    }
}

/* public */
SettingsHelper::~SettingsHelper() {
    for (int i = 0; i < nfields; i++)
	delete fields[i];
    if (fields != NULL)
	free(fields);
    if (dlgfields != NULL)
	free(dlgfields);
}

/* public */ int
SettingsHelper::getFieldCount() {
    return ndlgfields;
}

/* public */ const char *
SettingsHelper::getFieldLabel(int index) {
    if (index < 0 || index >= ndlgfields)
	crash();
    return dlgfields[index]->getLabel();
}

/* public */ char *
SettingsHelper::getFieldValue(int index) {
    if (index < 0 || index >= ndlgfields)
	crash();
    return dlgfields[index]->getValue();
}

/* public */ void
SettingsHelper::setFieldValue(int index, const char *value) {
    if (index < 0 || index >= ndlgfields)
	crash();
    dlgfields[index]->setValue(value);
}

/* public */ char *
SettingsHelper::dumpSettings() {
    char *buf  = NULL;
    int bufsize = 0;
    int bufpos = 0;
    for (int i = 0; i < ndlgfields; i++) {
	const char *label = getFieldLabel(i);
	char *value = getFieldValue(i);
	char b[512];
	snprintf(b, 512, "%s = %s\n", label, value);
	free(value);
	int bs = strlen(b);
	if (bufpos + bs + 1 > bufsize) {
	    bufsize += 1024;
	    buf = (char *) realloc(buf, bufsize);
	}
	memcpy(buf + bufpos, b, bs);
	bufpos += bs;
    }
    if (buf != NULL)
	buf[bufpos] = 0;
    return buf;
}

/* public */ bool
SettingsHelper::allowDepth(int depth) {
    return plugin->does_depth(depth);
}

/* public */ int
SettingsHelper::getDepth() {
    return plugin->pm->depth;
}

/* public */ void
SettingsHelper::setDepth(int depth) {
    plugin->pm->depth = depth;
}

/* public */ void
SettingsHelper::serialize(void **buf_out, int *nbytes_out) {
    buf = NULL;
    bufsize = 0;
    bufpos = 0;
    const char *pluginname = plugin->name();
    appendstring(pluginname == NULL ? "Null" : pluginname);

    for (int i = 0; i < nfields; i++)
	fields[i]->serialize();

    *buf_out = buf;
    *nbytes_out = bufpos;
}

/* public */ void
SettingsHelper::deserialize(const void *buf_in, int nbytes_in) {
    buf = (char *) buf_in;
    bufsize = nbytes_in;
    bufpos = 0;
    failed = false;

    for (int i = 0; i < nfields; i++)
	fields[i]->deserialize();
}

/* private */ void
SettingsHelper::append(const char *b, int n) {
    if (bufpos + n > bufsize) {
	// Grow the buffer by the number of bytes needed, add another
	// 1024 for good measure (if we're too stingy, we only end up
	// having to realloc() more often). Finally, round the new size
	// up to the nearest kilobyte.
	int newsize = bufpos + n + 1024;
	newsize = (newsize + 1023) & ~1023;
	buf = (char *) realloc(buf, newsize);
	bufsize = newsize;
    }
    memcpy(buf + bufpos, b, n);
    bufpos += n;
}

/* private */ void
SettingsHelper::appendbool(bool b) {
    appendchar(b ? 'T' : 'F');
}

/* private */ void
SettingsHelper::appendchar(int c) {
    char ch = c;
    append(&ch, 1);
}

/* private */ void
SettingsHelper::appendlonglong(long long ll) {
    char buf[64];
    snprintf(buf, 64, "%lld", ll);
    append(buf, strlen(buf) + 1);
}

/* private */ void
SettingsHelper::appendfloat(float f) {

    // TODO: determine the actual number of significant digits at run time
    // (e.g. by examining the number of correct digits in 1/3, dividing 1
    // by 10 over and over until you hit zero, etc). Then you can make sure
    // you print as many digits as needed so as not to lose information,
    // but not more.
    // Printing an *exact* representation can require many more digits than
    // are actually significant. For example, on the machine I'm using at
    // the moment, the exact decimal representation of the long double value
    // of 1/3 is
    // 0.33333333333333333334236835143737920361672877334058284759521484375
    // that's 65 decimal digits, but only the first 19 are correct, which
    // suggests that the long double data type uses a 64-bit mantissa --
    // and 10log(2^64) = 19.3, so there you are, you can round to 20 digits
    // and still capture all the information that is actually there.
    // (Note: that is how Java converts floating-point data types to decimal
    // strings. It's so sensible, I wish it was easier to do it in C.)
    // (Note: I'm using hard-coded precisions of 8 digits for float, 16 for
    // double, and 20 for long double, based on the mantissa size being 24,
    // 52, and 64 bits, respectively. I *think* those are the correct values
    // for IEEE floating point, so should be fine in x86 machines and most
    // other popular platforms as well, I expect.)
    // (Note: all this to make sure I don't write a few characters too many
    // when serializing a Plugin's state. Oh, well...)

    char buf[64];
    snprintf(buf, 64, "%.8g", f);
    append(buf, strlen(buf) + 1);
}

/* private */ void
SettingsHelper::appenddouble(double d) {
    // TODO: see the long comment in appendfloat()
    char buf[64];
    snprintf(buf, 64, "%.16g", d);
    append(buf, strlen(buf) + 1);
}

/* private */ void
SettingsHelper::appendlongdouble(long double ld) {
    // TODO: see the long comment in appendfloat()
    char buf[64];
    snprintf(buf, 64, "%.20Lg", ld);
    append(buf, strlen(buf) + 1);
}

/* private */ void
SettingsHelper::appendstring(const char *s) {
    append(s, strlen(s) + 1);
}

/* private */ bool
SettingsHelper::readbool() {
    return readchar() == 'T';
}

/* private */ int
SettingsHelper::readchar() {
    if (failed)
	return -1;
    if (bufpos >= bufsize)
	return -1;
    return buf[bufpos++];
}

/* private */ long long
SettingsHelper::readlonglong() {
    char buf[64];
    int p = 0, c;
    do {
	c = readchar();
	if (c == -1)
	    c = 0;
	buf[p++] = c;
    } while (p < 63 && c != 0);
    buf[p] = 0;
    long long ll = 0;
    sscanf(buf, "%lld", &ll);
    return ll;
}

/* private */ float
SettingsHelper::readfloat() {
    char buf[64];
    int p = 0, c;
    do {
	c = readchar();
	if (c == -1)
	    c = 0;
	buf[p++] = c;
    } while (p < 63 && c != 0);
    buf[p] = 0;
    float f = 0;
    sscanf(buf, "%f", &f);
    return f;
}

/* private */ double
SettingsHelper::readdouble() {
    char buf[64];
    int p = 0, c;
    do {
	c = readchar();
	if (c == -1)
	    c = 0;
	buf[p++] = c;
    } while (p < 63 && c != 0);
    buf[p] = 0;
    double d = 0;
    sscanf(buf, "%lf", &d);
    return d;
}

/* private */ long double
SettingsHelper::readlongdouble() {
    char buf[64];
    int p = 0, c;
    do {
	c = readchar();
	if (c == -1)
	    c = 0;
	buf[p++] = c;
    } while (p < 63 && c != 0);
    buf[p] = 0;
    long double ld = 0;
    sscanf(buf, "%Lf", &ld);
    return ld;
}

/* private */ char *
SettingsHelper::readstring() {
    char *buf = (char *) malloc(256);
    int size = 256;
    int length = 0;
    int c;
    do {
	c = readchar();
	if (c == -1)
	    c = 0;
	if (length == size) {
	    size += 256;
	    buf = (char *) realloc(buf, size);
	}
	buf[length++] = c;
    } while (c != 0);
    return buf;
}
