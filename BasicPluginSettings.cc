#include <stdlib.h>

#include "BasicPluginSettings.h"
#include "util.h"


/* public */
BasicPluginSettings::BasicPluginSettings() {
    nfields = 0;
    types = NULL;
    values = NULL;
    labels = NULL;
}

/* public virtual */
BasicPluginSettings::~BasicPluginSettings() {
    for (int i = 0; i < nfields; i++) {
	if (types[i] == STRING)
	    free(values[i].s);
    }
    if (types != NULL)
	free(types);
    if (values != NULL)
	free(values);
    if (labels != NULL)
	free(labels);
}

/* public virtual */ void
BasicPluginSettings::setIntField(int index, int value) {
    if (index < 0 || index >= nfields || types[index] != INT)
	crash();
    values[index].i = value;
}

/* public virtual */ void
BasicPluginSettings::setDoubleField(int index, double value) {
    if (index < 0 || index >= nfields || types[index] != DOUBLE)
	crash();
    values[index].d = value;
}

/* public virtual */ void
BasicPluginSettings::setStringField(int index, const char *value) {
    if (index < 0 || index >= nfields || types[index] != STRING)
	crash();
    free(values[index].s);
    values[index].s = value == NULL ? strclone("") : strclone(value);
}

/* public virtual */ int
BasicPluginSettings::getIntField(int index) {
    if (index < 0 || index >= nfields || types[index] != INT)
	crash();
    return values[index].i;
}

/* public virtual */ double
BasicPluginSettings::getDoubleField(int index) {
    if (index < 0 || index >= nfields || types[index] != DOUBLE)
	crash();
    return values[index].d;
}

/* public virtual */ char *
BasicPluginSettings::getStringField(int index) {
    if (index < 0 || index >= nfields || types[index] != STRING)
	crash();
    return strclone(values[index].s);
}

/* public virtual */ int
BasicPluginSettings::getFieldCount() {
    return nfields;
}

/* public virtual */ void
BasicPluginSettings::getFieldInfo(int index, int *type, const char **label) {
    if (index < 0 || index >= nfields)
	crash();
    *type = types[index];
    if (label != NULL)
	*label = labels[index];
}

/* public virtual */ int
BasicPluginSettings::addField(int type, const char *label) {
    if ((type != INT && type != DOUBLE && type != STRING) || label == NULL)
	crash();
    if (nfields == 0) {
	types = (int *) malloc(sizeof(int));
	values = (Value *) malloc(sizeof(Value));
	labels = (char **) malloc(sizeof(char *));
    } else {
	types = (int *) realloc(types, (nfields + 1) * sizeof(int));
	values = (Value *) realloc(values, (nfields + 1) * sizeof(Value));
	labels = (char **) realloc(labels, (nfields + 1) * sizeof(char *));
    }
    types[nfields] = type;
    switch (type) {
	case INT:
	    values[nfields].i = 0;
	    break;
	case DOUBLE:
	    values[nfields].d = 0;
	    break;
	case STRING:
	    values[nfields].s = strclone("");
	    break;
    }
    labels[nfields] = strclone(label);
    return nfields++;
}
