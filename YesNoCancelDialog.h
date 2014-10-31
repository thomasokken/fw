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

#ifndef CLOSECONFIRMDIALOG_H
#define CLOSECONFIRMDIALOG_H 1

#include "Frame.h"

class YesNoCancelDialog : public Frame {
    public:
        class Listener {
            public:
                Listener();
                virtual ~Listener();
                virtual void yes() = 0;
                virtual void no() = 0;
                virtual void cancel() = 0;
        };
            
    private:
        Listener *listener;

    public:
        YesNoCancelDialog(Frame *parent, const char *message,
                           Listener *listener, bool showCancel = true);
        virtual ~YesNoCancelDialog();
        virtual void close();

    private:
        static void yesCB(Widget w, XtPointer cd, XtPointer ud);
        static void noCB(Widget w, XtPointer cd, XtPointer ud);
        static void cancelCB(Widget w, XtPointer cd, XtPointer ud);
};

#endif
