#ifndef FWPIXMAP_H
#define FWPIXMAP_H 1

class Color;

struct FWPixmap {
    unsigned char *pixels;
    Color *cmap;
    int depth;
    int width, height, bytesperline;
};

#endif
