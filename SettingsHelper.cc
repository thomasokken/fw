#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SettingsHelper.h"
#include "Plugin.h"

/* public */
SettingsHelper::SettingsHelper(Plugin *plugin) {
    this->plugin = plugin;
}

/* public */
SettingsHelper::~SettingsHelper() {
}

/* public */ int
SettingsHelper::getFieldCount() {
    //
    return 0;
}

/* public */ const char *
SettingsHelper::getFieldLabel(int index) {
    //
    return NULL;
}

/* public */ char *
SettingsHelper::getFieldValue(int index) {
    //
    return NULL;
}

/* public */ void
SettingsHelper::setFieldValue(int index, const char *value) {
    //
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

    // Yada yada yada

    *buf_out = buf;
    *nbytes_out = bufpos;
}

/* public */ void
SettingsHelper::deserialize(const void *buf_in, int nbytes_in) {
    buf = (char *) buf_in;
    bufsize = nbytes_in;
    bufpos = 0;
    failed = false;
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
	    length += 256;
	    buf = (char *) realloc(buf, length);
	}
	buf[length++] = c;
    } while (c != 0);
    return buf;
}
