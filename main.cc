#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <X11/xpm.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "main.h"
#include "Viewer.h"
#include "fw.xpm"

// Exported globals

XtAppContext appcontext;
Widget appshell;
Display *display;
Screen *screen;
int screennumber;
Window rootwindow;
Visual *visual;
Colormap colormap;
int depth;
GC gc;
Pixmap icon, iconmask;
XColor *colorcube = NULL, *grayramp = NULL;
int cubesize, rampsize;
int verbosity = 0;
bool x_errors_coredump = false;


static int x_error_handler(Display *display, XErrorEvent *event) {
    char buf[1024];
    XGetErrorText(display, event->error_code, buf, 1024);
    fprintf(stderr, "X Error: type=%d, serial=%lu, request=%u, minor=%u "
	    "rsrc=0x%lx\n", event->type, event->serial, event->request_code,
	    event->minor_code, event->resourceid);
    fprintf(stderr, "         code=%d: %s\n", event->error_code, buf);
    if (x_errors_coredump)
	kill(0, SIGQUIT);
    else
	fprintf(stderr, "\007\n");
    return 0;
}


int main(int argc, char **argv) {
    XtSetLanguageProc(NULL, NULL, NULL);
    appshell = XtVaAppInitialize(&appcontext,	/* application context */
				 "FW",		/* class name */
				 NULL, 0,	/* cmd line option descr. */
				 &argc, argv,	/* cmd line args */
				 NULL,		/* fallback resources */
				 NULL);		/* end of varargs list */

    for (int i = 1; i < argc; i++) {
	int remove = 0;
	if (strncmp(argv[i], "-v", 2) == 0) {
	    char *p = argv[i] + 1;
	    while (*p++ == 'v')
		verbosity++;
	    remove = 1;
	} else if (strcmp(argv[i], "-xdump") == 0) {
	    x_errors_coredump = true;
	    remove = 1;
	} else if (strcmp(argv[i], "-h") == 0
		|| strcmp(argv[i], "-help") == 0
		|| strcmp(argv[i], "--help") == 0) {
	    fprintf(stderr, "Usage: fw [options] [files...]\n");
	    fprintf(stderr, "    available options:\n");
	    fprintf(stderr, "    X Toolkit options (see \"man X\")\n");
	    fprintf(stderr, "    -v : verbose (-vv, -vvv, etc: more verbosity)\n");
	    fprintf(stderr, "    -xdump : dump core on X errors (implies -synchronous)\n");
	    fprintf(stderr, "    -h , -help , --help : print usage information & exit\n");
	    exit(0);
	    
	} else if (argv[i][0] == '-') {
	    fprintf(stderr, "Unrecognized option \"%s\" (see \"fw -h\" for help)\n", argv[i]);
	    exit(1);
	}

	if (remove > 0) {
	    for (int j = i; j <= argc - remove; j++)
		argv[j] = argv[j + remove];
	    argc -= remove;
	    argv[argc] = NULL;
	    i--;
	}
    }

    if (getenv("FW_XDUMP") != NULL)
	x_errors_coredump = true;

    if (verbosity >= 1) {
	fprintf(stderr, "Verbosity level set to %d.\n", verbosity);
	if (x_errors_coredump)
	    fprintf(stderr, "Dumping core on X errors.\n");
    }

    // Seed random number generator so things like Kaos don't give you
    // the same pictures every time...

    srand(time(NULL));


    display = XtDisplay(appshell);
    screen = XtScreen(appshell);
    screennumber = 0;
    while (screen != ScreenOfDisplay(display, screennumber))
	screennumber++;
    rootwindow = RootWindowOfScreen(screen);

    // If the user has requested core dumps for X errors, turn on X
    // synchronization as well (like using the "-synchronize" Xt switch).
    // This is necessary if you want the core dump to actually lead you,
    // via the stack trace, to the context in which the offending call
    // was made.
    if (x_errors_coredump)
	XSynchronize(display, True);
    XSetErrorHandler(x_error_handler);

    visual = DefaultVisual(display, screennumber);
    colormap = DefaultColormap(display, screennumber);
    depth = DefaultDepth(display, screennumber);

    if (visual->c_class == StaticColor
		|| visual->c_class == PseudoColor) {
	// Try to allocate as large as possible a color cube
	cubesize = 1;
	while (cubesize * cubesize * cubesize <= (1 << depth))
	    cubesize++;
	cubesize--;
	while (colorcube == NULL && cubesize > 1) {
	    int n = 0;
	    colorcube = new XColor[cubesize * cubesize * cubesize];
	    for (int r = 0; r < cubesize; r++)
		for (int g = 0; g < cubesize; g++)
		    for (int b = 0; b < cubesize; b++) {
			colorcube[n].red = r * 65535 / (cubesize - 1);
			colorcube[n].green = g * 65535 / (cubesize - 1);
			colorcube[n].blue = b * 65535 / (cubesize - 1);
			int success = XAllocColor(display, colormap,
						  &colorcube[n]);
			if (!success) {
			    if (n > 0) {
				unsigned long *pixels = new unsigned long[n];
				for (int i = 0; i < n; i++)
				    pixels[i] = colorcube[i].pixel;
				XFreeColors(display, colormap, pixels, n, 0);
				delete[] pixels;
				delete[] colorcube;
				colorcube = NULL;
			    }
			    goto endloop;
			}
			n++;
		    }
	    endloop:
	    if (colorcube == NULL)
		cubesize--;
	}
    }

    if ((visual->c_class == StaticGray || visual->c_class == GrayScale)
	|| (visual->c_class == StaticColor || visual->c_class == PseudoColor)
	    && colorcube == NULL) {
	int realdepth = depth;
	if (realdepth > visual->bits_per_rgb);
	    realdepth = visual->bits_per_rgb;
	if (realdepth > 8)
	    realdepth = 8;

	if (visual->c_class == TrueColor || visual->c_class == DirectColor) {
	    // Seems pointless, but we need to handle this to make the
	    // -gray option work properly. Without this, you get a 64-level
	    // gray ramp on a 16-bit TrueColor display, even though with the
	    // usual 5-6-5 component sizes, the display is only capable of
	    // 32 pure shades of gray.
	    unsigned long redmask = visual->red_mask;
	    unsigned long greenmask = visual->green_mask;
	    unsigned long bluemask = visual->blue_mask;
	    int redbits = 0, greenbits = 0, bluebits = 0;
	    while (redmask != 0 || greenmask != 0 || bluemask != 0) {
		redbits += redmask & 1;
		redmask >>= 1;
		greenbits += greenmask & 1;
		greenmask >>= 1;
		bluebits += bluemask & 1;
		bluemask >>= 1;
	    }
	    if (realdepth > redbits)
		realdepth = redbits;
	    if (realdepth > greenbits)
		realdepth = greenbits;
	    if (realdepth > bluebits)
		realdepth = bluebits;
	}

	rampsize = 1 << realdepth;
	while (grayramp == NULL) {
	    grayramp = new XColor[rampsize];
	    if (rampsize == 2) {
		grayramp[0].pixel = BlackPixel(display, screennumber);
		grayramp[0].red = grayramp[0].green = grayramp[0].blue = 0;
		grayramp[1].pixel = WhitePixel(display, screennumber);
		grayramp[1].red = grayramp[1].green = grayramp[1].blue = 65535;
		break;
	    }
	    for (int n = 0; n < rampsize; n++) {
		grayramp[n].red = grayramp[n].green = grayramp[n].blue
			= n * 65535 / (rampsize - 1);
		int success = XAllocColor(display, colormap, &grayramp[n]);
		if (!success) {
		    if (n > 0) {
			unsigned long *pixels = new unsigned long[n];
			for (int i = 0; i < n; i++)
			    pixels[i] = grayramp[i].pixel;
			XFreeColors(display, colormap, pixels, n, 0);
			delete[] pixels;
			delete[] grayramp;
			grayramp = NULL;
		    }
		    break;
		}
	    }
	    if (grayramp == NULL)
		rampsize >>= 1;
	}
    }

    if (verbosity >= 1) {
	if (colorcube != NULL)
	    fprintf(stderr, "Allocated a %dx%dx%d color cube.\n", cubesize,
		    cubesize, cubesize);
	if (grayramp != NULL)
	    fprintf(stderr, "Allocated a %d-entry gray ramp.\n", rampsize);
    }

    XGCValues values;
    values.foreground = WhitePixel(display, screennumber);
    values.background = BlackPixel(display, screennumber);
    gc = XCreateGC(display,
		   rootwindow,
		   GCForeground | GCBackground,
		   &values);

    XpmCreatePixmapFromData(display,
			    rootwindow,
			    fw,
			    &icon,
			    &iconmask,
			    NULL);

    bool nothingOpened = true;
    for (int i = 1; i < argc; i++) {
	if (Viewer::openFile(argv[i]))
	    nothingOpened = false;
	else if (verbosity >= 1)
	    fprintf(stderr, "Can't open \"%s\".\n", argv[i]);
    }
    if (nothingOpened)
	new Viewer("AboutViewer");

    XtAppMainLoop(appcontext);

    return 0;
}
