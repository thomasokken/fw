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

#ifndef MAIN_H
#define MAIN_H 1

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

extern XtAppContext g_appcontext;
extern Widget g_appshell;
extern Display *g_display;
extern Screen *g_screen;
extern int g_screennumber;
extern Window g_rootwindow;
extern Visual *g_visual;
extern int g_depth;
extern Colormap g_colormap;
extern GC g_gc;
extern unsigned long g_black, g_white;
extern Pixmap g_icon, g_iconmask;
extern XColor *g_colorcube, *g_grayramp;
extern int g_cubesize, g_rampsize;
extern int g_verbosity;

#endif
