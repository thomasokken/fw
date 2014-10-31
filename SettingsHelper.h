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
        friend class CharPointerField;
        friend class CharArrayField;
};

#endif
