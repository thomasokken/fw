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
#include "util.h"

#include "ImageIO_GIF.h"
#include "ImageIO_JPEG.h"
#include "ImageIO_PNG.h"
#include "ImageIO_PNM.h"

#include "fw.xpm"

// Exported globals

XtAppContext g_appcontext;
Widget g_appshell;
Display *g_display;
Screen *g_screen;
int g_screennumber;
Window g_rootwindow;
Visual *g_visual;
Colormap g_colormap;
int g_depth;
GC g_gc;
unsigned long g_black, g_white;
Pixmap g_icon, g_iconmask;
XColor *g_colorcube = NULL, *g_grayramp = NULL;
int g_cubesize, g_rampsize;
Preferences *g_prefs;


struct req_code {
    const char *name;
    int value;
};

// From Xproto.h

static req_code req_code_list[] = {
    { "X_CreateWindow",                  1 },
    { "X_ChangeWindowAttributes",        2 },
    { "X_GetWindowAttributes",           3 },
    { "X_DestroyWindow",                 4 },
    { "X_DestroySubwindows",             5 },
    { "X_ChangeSaveSet",                 6 },
    { "X_ReparentWindow",                7 },
    { "X_MapWindow",                     8 },
    { "X_MapSubwindows",                 9 },
    { "X_UnmapWindow",                  10 },
    { "X_UnmapSubwindows",              11 },
    { "X_ConfigureWindow",              12 },
    { "X_CirculateWindow",              13 },
    { "X_GetGeometry",                  14 },
    { "X_QueryTree",                    15 },
    { "X_InternAtom",                   16 },
    { "X_GetAtomName",                  17 },
    { "X_ChangeProperty",               18 },
    { "X_DeleteProperty",               19 },
    { "X_GetProperty",                  20 },
    { "X_ListProperties",               21 },
    { "X_SetSelectionOwner",            22 },
    { "X_GetSelectionOwner",            23 },
    { "X_ConvertSelection",             24 },
    { "X_SendEvent",                    25 },
    { "X_GrabPointer",                  26 },
    { "X_UngrabPointer",                27 },
    { "X_GrabButton",                   28 },
    { "X_UngrabButton",                 29 },
    { "X_ChangeActivePointerGrab",      30 },
    { "X_GrabKeyboard",                 31 },
    { "X_UngrabKeyboard",               32 },
    { "X_GrabKey",                      33 },
    { "X_UngrabKey",                    34 },
    { "X_AllowEvents",                  35 },
    { "X_GrabServer",                   36 },
    { "X_UngrabServer",                 37 },
    { "X_QueryPointer",                 38 },
    { "X_GetMotionEvents",              39 },
    { "X_TranslateCoords",              40 },
    { "X_WarpPointer",                  41 },
    { "X_SetInputFocus",                42 },
    { "X_GetInputFocus",                43 },
    { "X_QueryKeymap",                  44 },
    { "X_OpenFont",                     45 },
    { "X_CloseFont",                    46 },
    { "X_QueryFont",                    47 },
    { "X_QueryTextExtents",             48 },
    { "X_ListFonts",                    49 },
    { "X_ListFontsWithInfo",            50 },
    { "X_SetFontPath",                  51 },
    { "X_GetFontPath",                  52 },
    { "X_CreatePixmap",                 53 },
    { "X_FreePixmap",                   54 },
    { "X_CreateGC",                     55 },
    { "X_ChangeGC",                     56 },
    { "X_CopyGC",                       57 },
    { "X_SetDashes",                    58 },
    { "X_SetClipRectangles",            59 },
    { "X_FreeGC",                       60 },
    { "X_ClearArea",                    61 },
    { "X_CopyArea",                     62 },
    { "X_CopyPlane",                    63 },
    { "X_PolyPoint",                    64 },
    { "X_PolyLine",                     65 },
    { "X_PolySegment",                  66 },
    { "X_PolyRectangle",                67 },
    { "X_PolyArc",                      68 },
    { "X_FillPoly",                     69 },
    { "X_PolyFillRectangle",            70 },
    { "X_PolyFillArc",                  71 },
    { "X_PutImage",                     72 },
    { "X_GetImage",                     73 },
    { "X_PolyText8",                    74 },
    { "X_PolyText16",                   75 },
    { "X_ImageText8",                   76 },
    { "X_ImageText16",                  77 },
    { "X_CreateColormap",               78 },
    { "X_FreeColormap",                 79 },
    { "X_CopyColormapAndFree",          80 },
    { "X_InstallColormap",              81 },
    { "X_UninstallColormap",            82 },
    { "X_ListInstalledColormaps",       83 },
    { "X_AllocColor",                   84 },
    { "X_AllocNamedColor",              85 },
    { "X_AllocColorCells",              86 },
    { "X_AllocColorPlanes",             87 },
    { "X_FreeColors",                   88 },
    { "X_StoreColors",                  89 },
    { "X_StoreNamedColor",              90 },
    { "X_QueryColors",                  91 },
    { "X_LookupColor",                  92 },
    { "X_CreateCursor",                 93 },
    { "X_CreateGlyphCursor",            94 },
    { "X_FreeCursor",                   95 },
    { "X_RecolorCursor",                96 },
    { "X_QueryBestSize",                97 },
    { "X_QueryExtension",               98 },
    { "X_ListExtensions",               99 },
    { "X_ChangeKeyboardMapping",       100 },
    { "X_GetKeyboardMapping",          101 },
    { "X_ChangeKeyboardControl",       102 },
    { "X_GetKeyboardControl",          103 },
    { "X_Bell",                        104 },
    { "X_ChangePointerControl",        105 },
    { "X_GetPointerControl",           106 },
    { "X_SetScreenSaver",              107 },
    { "X_GetScreenSaver",              108 },
    { "X_ChangeHosts",                 109 },
    { "X_ListHosts",                   110 },
    { "X_SetAccessControl",            111 },
    { "X_SetCloseDownMode",            112 },
    { "X_KillClient",                  113 },
    { "X_RotateProperties",            114 },
    { "X_ForceScreenSaver",            115 },
    { "X_SetPointerMapping",           116 },
    { "X_GetPointerMapping",           117 },
    { "X_SetModifierMapping",          118 },
    { "X_GetModifierMapping",          119 },
    { "X_NoOperation",                 127 },
    { NULL,                             -1 }
};


static int x_error_handler(Display *display, XErrorEvent *event) {
    char buf[1024];
    XGetErrorText(display, event->error_code, buf, 1024);
    const char *req_name = "???";
    for (int i = 0; req_code_list[i].value != -1; i++)
	if (req_code_list[i].value == event->request_code) {
	    req_name = req_code_list[i].name;
	    break;
	}

    fprintf(stderr, "X Error of failed request:  %s\n", buf);
    fprintf(stderr, "  Major opcode of failed request:  %u (%s)\n",
		    event->request_code, req_name);
    fprintf(stderr, "  Resource id in failed request:  0x%lx\n",
		    event->resourceid);
    fprintf(stderr, "  Serial number of failed request:  %lu\n", event->serial);

    // OK, I'm trying to mimic the output from the default error handler,
    // but this one has me stumped. How to you find the current serial number
    // in the X output stream? Is that even possible without access to Xlib
    // internals?
    fprintf(stderr, "  Current serial number in output stream:  ???\n");

    if (g_prefs->x_errors_coredump)
	crash();
    else
	fprintf(stderr, "\007\n");
    return 0;
}


int main(int argc, char **argv) {
    XtSetLanguageProc(NULL, NULL, NULL);
    g_appshell = XtVaAppInitialize(&g_appcontext,/* application context */
				 "FW",		/* class name */
				 NULL, 0,	/* cmd line option descr. */
				 &argc, argv,	/* cmd line args */
				 NULL,		/* fallback resources */
				 NULL);		/* end of varargs list */

    g_prefs = new Preferences(&argc, argv);


    // Seed random number generator so things like Kaos don't give you
    // the same pictures every time...

    srand(time(NULL));


    g_display = XtDisplay(g_appshell);
    g_screen = XtScreen(g_appshell);
    g_screennumber = 0;
    while (g_screen != ScreenOfDisplay(g_display, g_screennumber))
	g_screennumber++;
    g_rootwindow = RootWindowOfScreen(g_screen);

    // If the user has requested core dumps for X errors, turn on X
    // synchronization as well (like using the "-synchronize" Xt switch).
    // This is necessary if you want the core dump to actually lead you,
    // via the stack trace, to the context in which the offending call
    // was made.
    if (g_prefs->x_errors_coredump)
	XSynchronize(g_display, True);
    XSetErrorHandler(x_error_handler);

    g_visual = DefaultVisual(g_display, g_screennumber);
    g_colormap = DefaultColormap(g_display, g_screennumber);
    g_depth = DefaultDepth(g_display, g_screennumber);
    g_black = BlackPixel(g_display, g_screennumber);
    g_white = WhitePixel(g_display, g_screennumber);

    if (g_visual->c_class == StaticColor
		|| g_visual->c_class == PseudoColor) {
	// Try to allocate as large as possible a color cube
	g_cubesize = 1;
	while (g_cubesize * g_cubesize * g_cubesize <= (1 << g_depth))
	    g_cubesize++;
	g_cubesize--;
	while (g_colorcube == NULL && g_cubesize > 1) {
	    int n = 0;
	    g_colorcube = new XColor[g_cubesize * g_cubesize * g_cubesize];
	    for (int r = 0; r < g_cubesize; r++)
		for (int g = 0; g < g_cubesize; g++)
		    for (int b = 0; b < g_cubesize; b++) {
			g_colorcube[n].red = r * 65535 / (g_cubesize - 1);
			g_colorcube[n].green = g * 65535 / (g_cubesize - 1);
			g_colorcube[n].blue = b * 65535 / (g_cubesize - 1);
			int success = XAllocColor(g_display, g_colormap,
						  &g_colorcube[n]);
			if (!success) {
			    if (n > 0) {
				unsigned long *pixels = new unsigned long[n];
				for (int i = 0; i < n; i++)
				    pixels[i] = g_colorcube[i].pixel;
				XFreeColors(g_display, g_colormap, pixels, n, 0);
				delete[] pixels;
				delete[] g_colorcube;
				g_colorcube = NULL;
			    }
			    goto endloop;
			}
			n++;
		    }
	    endloop:
	    if (g_colorcube == NULL)
		g_cubesize--;
	}
    }

    if ((g_visual->c_class == StaticGray || g_visual->c_class == GrayScale)
	|| (g_visual->c_class == StaticColor || g_visual->c_class == PseudoColor)
	    && g_colorcube == NULL) {
	int realdepth = g_depth;
	if (realdepth > g_visual->bits_per_rgb);
	    realdepth = g_visual->bits_per_rgb;
	if (realdepth > 8)
	    realdepth = 8;

	if (g_visual->c_class == TrueColor || g_visual->c_class == DirectColor) {
	    // Seems pointless, but we need to handle this to make the
	    // -gray option work properly. Without this, you get a 64-level
	    // gray ramp on a 16-bit TrueColor display, even though with the
	    // usual 5-6-5 component sizes, the display is only capable of
	    // 32 pure shades of gray.
	    unsigned long redmask = g_visual->red_mask;
	    unsigned long greenmask = g_visual->green_mask;
	    unsigned long bluemask = g_visual->blue_mask;
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

	g_rampsize = 1 << realdepth;
	while (g_grayramp == NULL) {
	    g_grayramp = new XColor[g_rampsize];
	    if (g_rampsize == 2) {
		g_grayramp[0].pixel = g_black;
		g_grayramp[0].red = g_grayramp[0].green = g_grayramp[0].blue = 0;
		g_grayramp[1].pixel = g_white;
		g_grayramp[1].red = g_grayramp[1].green = g_grayramp[1].blue = 65535;
		break;
	    }
	    for (int n = 0; n < g_rampsize; n++) {
		g_grayramp[n].red = g_grayramp[n].green = g_grayramp[n].blue
			= n * 65535 / (g_rampsize - 1);
		int success = XAllocColor(g_display, g_colormap, &g_grayramp[n]);
		if (!success) {
		    if (n > 0) {
			unsigned long *pixels = new unsigned long[n];
			for (int i = 0; i < n; i++)
			    pixels[i] = g_grayramp[i].pixel;
			XFreeColors(g_display, g_colormap, pixels, n, 0);
			delete[] pixels;
			delete[] g_grayramp;
			g_grayramp = NULL;
		    }
		    break;
		}
	    }
	    if (g_grayramp == NULL)
		g_rampsize >>= 1;
	}
    }

    if (g_prefs->verbosity >= 1) {
	if (g_colorcube != NULL)
	    fprintf(stderr, "Allocated a %dx%dx%d color cube.\n", g_cubesize,
		    g_cubesize, g_cubesize);
	if (g_grayramp != NULL)
	    fprintf(stderr, "Allocated a %d-entry gray ramp.\n", g_rampsize);
    }

    XGCValues values;
    values.foreground = g_white;
    values.background = g_black;
    g_gc = XCreateGC(g_display,
		   g_rootwindow,
		   GCForeground | GCBackground,
		   &values);

    XpmCreatePixmapFromData(g_display,
			    g_rootwindow,
			    fw,
			    &g_icon,
			    &g_iconmask,
			    NULL);

    ImageIO::regist(new ImageIO_GIF());
    ImageIO::regist(new ImageIO_JPEG());
    ImageIO::regist(new ImageIO_PNG());
    ImageIO::regist(new ImageIO_PNM());

    bool nothingOpened = true;
    for (int i = 1; i < argc; i++) {
	char *type = NULL;
	char *plugin_name = NULL;
	void *plugin_data = NULL;
	int plugin_data_length;
	FWPixmap pm;
	char *message = NULL;
	if (ImageIO::sread(argv[i], &type, &plugin_name, &plugin_data,
			  &plugin_data_length, &pm, &message)) {
	    // Read successful; open viewer
	    Viewer *viewer = new Viewer(plugin_name, plugin_data,
				       plugin_data_length, &pm);
	    viewer->setFile(argv[i], type);
	    nothingOpened = false;
	    if (message != NULL)
		// TODO: nicer warning reporting
		fprintf(stderr, "Re: \"%s\": %s\n", argv[i], message);
	} else {
	    // TODO: nicer error reporting
	    fprintf(stderr, "Can't open \"%s\" (%s).\n", argv[i], message);
	}
	if (type != NULL)
	    free(type);
	if (plugin_name != NULL)
	    free(plugin_name);
	if (plugin_data != NULL)
	    free(plugin_data);
	if (message != NULL)
	    free(message);
    }
    if (nothingOpened)
	new Viewer("About");

    XtAppMainLoop(g_appcontext);

    return 0;
}
