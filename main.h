#ifndef MAIN_H
#define MAIN_H

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
