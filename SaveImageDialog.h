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

#ifndef SAVEIMAGEDIALOG_H
#define SAVEIMAGEDIALOG_H 1

#include "FileDialog.h"

class Menu;

class SaveImageDialog : public FileDialog, private FileDialog::Listener {
    public:
        class Listener {
            public:
                Listener();
                virtual ~Listener();
                virtual void save(const char *fn, const char *type) = 0;
                virtual void cancel() = 0;
        };

    private:
        Listener *listener;
        const char *type;
        Menu *typeMenu;

    public:
        SaveImageDialog(Frame *parent, const char *file, const char *type,
                        Listener *listener);
        virtual ~SaveImageDialog();
        virtual void close();

    private:
        static void typeMenuCB(void *closure, const char *id);
        // FileDialog::Listener methods
        void fileSelected(const char *filename);
        void cancelled();
};

#endif
