#ifndef PLUGINSETTINGS_H
#define PLUGINSETTINGS_H 1

// This file defines an interface between the SettingsDialog and the Plugin.
// The PluginSettings class is used to communicate settings between
// SettingsDialog and Plugin, in both directions, that is, it is used by
// SettingsDialog to decide what input fields to display and what values to
// populate them with initially, and it is used by the Plugin to retrieve the
// values entered in the SettingsDialog by the user.
// Plugins can obtain an instance of a basic class implementing this interface
// by calling Plugin::getSettingsInstance(), or they can provide their own
// implementation. The latter makes it possible for the plugin to perform
// input validations and such, without having to wait for the SettingsDialog
// to be dismissed.

class PluginSettings {
	// NOTE: in all methods that take string arguments, the caller retains
	// responsibility for arguments passed into a function. If a function
	// needs to keep a string around, it should allocate a copy.
	// In all methods that return strings (whether as return values or
	// using char** arguments), the caller is responsible for the returned
	// string, and should dispose of it using free() when it is no longer
	// needed.
	// Exceptions are noted where applicable.

    public:
	// Field types
	enum { INT, DOUBLE, STRING };

	PluginSettings();
	virtual ~PluginSettings();


	// Methods to put stuff in and get stuff out. PluginSettings provides
	// default implementations (which dump core when called). You only have
	// to implement the ones that you actually need; if getFieldInfo()
	// never returns a 'type' of 'STRING', for instance, SettingsDialog
	// will never call getStringField() or setStringField(), so the default
	// (core dumping) implementations are fine in that case.

	virtual void setIntField(int index, int value);
	virtual void setDoubleField(int index, double value);
	virtual void setStringField(int index, const char *value);
	virtual int getIntField(int index);
	virtual double getDoubleField(int index);
	virtual char *getStringField(int index);


	// These methods are used by SettingsDialog to find out about the
	// number and types of fields to be displayed. In the getFieldInfo()
	// method, the 'label' string that is returned points to the
	// PluginSettings object's internal storage; if the caller wants to
	// hang on to it he needs to make a copy. It is OK to pass NULL for
	// 'label' if the caller is not interested in the field label.

	virtual int getFieldCount() = 0;
	virtual void getFieldInfo(int index, int *type, const char **label) = 0;


	// This method is called by SettingsDialog when the user has changed
	// a value. The default implementation in PluginSettings does nothing.

	virtual void fieldChanged(int index);


	// This method is implemented by BasicPluginSettings.
	// If a plugin defines its own PluginSettings class, it does not
	// have to implement this method; it is never called by FW itself.
	// To keep the linker happy, PluginSettings provides a default
	// implementation (which dumps core when invoked).
	// (NOTE: in a better world, this method would not even be in
	// PluginSettings, since it is specific to BasicPluginSettings. I put
	// it here anyway to keep the number of header files needed by plugins
	// as small as possible.)
	// The return value is the index of the field created.

	virtual int addField(int type, const char *label);
};

#endif
