#ifndef BASICPLUGINSETTINGS_H
#define BASICPLUGINSETTINGS_H 1

#include "PluginSettings.h"

class BasicPluginSettings : public PluginSettings {
    private:
	int nfields;
	int *types;
	union Value {
	    int i;
	    double d;
	    char *s;
	};
	Value *values;
	char **labels;

    public:
	BasicPluginSettings();
	virtual ~BasicPluginSettings();
	virtual void setIntField(int index, int value);
	virtual void setDoubleField(int index, double value);
	virtual void setStringField(int index, const char *value);
	virtual int getIntField(int index);
	virtual double getDoubleField(int index);
	virtual char *getStringField(int index);
	virtual int getFieldCount();
	virtual void getFieldInfo(int index, int *type, const char **label);
	// New fields are initialized to 0 or NULL.
	virtual int addField(int type, const char *label);
};

#endif
