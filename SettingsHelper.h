#ifndef SETTINGSHELPER_H
#define SETTINGSHELPER_H 1

class FWPixmap;
class Plugin;
class Field;

class SettingsHelper {
    private:
	Plugin *plugin;
	char *buf;
	int bufsize;
	int bufpos;
	bool failed;
	Field **fields;
	int nfields;
	Field **dlgfields;
	int ndlgfields;

    public:
	SettingsHelper(Plugin *plugin);
	~SettingsHelper();

	// SettingsDialog support
	int getFieldCount();
	const char *getFieldLabel(int index);
	char *getFieldValue(int index);
	void setFieldValue(int index, const char *value);
	char *dumpSettings();
	bool allowDepth(int depth);
	int getDepth();
	void setDepth(int depth);

	// Serialization support
	void serialize(void **buf, int *nbytes);
	void deserialize(const void *buf, int nbytes);

    private:
	// Serialization/deserialization
	void append(const char *b, int n);
	void appendbool(bool b);
	void appendchar(int c);
	void appendlonglong(long long i);
	void appendfloat(float f);
	void appenddouble(double d);
	void appendlongdouble(long double ld);
	void appendstring(const char *s);
	bool readbool();
	int readchar();
	long long readlonglong();
	float readfloat();
	double readdouble();
	long double readlongdouble();
	char *readstring();

	friend class Field;
	friend class BoolField;
	friend class CharField;
	friend class ShortField;
	friend class IntField;
	friend class LongField;
	friend class LongLongField;
	friend class FloatField;
	friend class DoubleField;
	friend class LongDoubleField;
	friend class StringField;
};

#endif
