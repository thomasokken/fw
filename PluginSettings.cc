#include <stdlib.h>

#include "PluginSettings.h"
#include "util.h"

/* public */
PluginSettings::PluginSettings() {
    //
}

/* public virtual */
PluginSettings::~PluginSettings() {
    //
}

/* public virtual */ void
PluginSettings::setIntField(int, int) {
    crash();
}

/* public virtual */ void
PluginSettings::setDoubleField(int, double) {
    crash();
}

/* public virtual */ void
PluginSettings::setStringField(int, const char *) {
    crash();
}

/* public virtual */ int
PluginSettings::getIntField(int) {
    crash();
    return 0;
}

/* public virtual */ double
PluginSettings::getDoubleField(int) {
    crash();
    return 0;
}

/* public virtual */ char *
PluginSettings::getStringField(int) {
    crash();
    return NULL;
}

/* public virtual */ void
PluginSettings::fieldChanged(int) {
    //
}

/* public virtual */ int
PluginSettings::addField(int, const char *) {
    crash();
    return 0;
}
