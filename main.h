#ifndef MAIN_H
#define MAIN_H

extern XtAppContext appcontext;
extern Widget appshell;
extern Display *display;
extern Screen *screen;
extern int screennumber;
extern Window rootwindow;
extern Visual *visual;
extern int depth;
extern Colormap colormap;
extern GC gc;
extern Pixmap icon, iconmask;
extern XColor *colorcube, *grayramp;
extern int cubesize, rampsize;

#endif
