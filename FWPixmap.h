#ifndef FWPIXMAP_H
#define FWPIXMAP_H 1

class FWColor;

struct FWPixmap {
    // NOTE: 'pixels' should be allocated using malloc() and freed using
    // free(); 'cmap' should be allocated using new FWColor[] and freed
    // using delete[].
    // Only depths 1, 8, and 24 are supported. When depth == 8, 'cmap' is
    // assumed to point to a colormap. Maybe FW should also support a mode
    // where depth == 8 and cmap == NULL, and treat that as 8 bit greyscale;
    // the rendering code in Viewer::paint() could be optimized for that
    // case.
    // If 'cmap' is not NULL, it should point to an array of 256 FWColors,
    // never fewer. If there are more than 256 elements in the array, the
    // excess is ignored.

    unsigned char *pixels;
    FWColor *cmap;
    int depth;
    int width, height, bytesperline;
};

#endif
