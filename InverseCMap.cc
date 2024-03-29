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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "InverseCMap.h"
#include "FWColor.h"
#include "main.h"


struct foo {
    union {
        struct {
            unsigned char filler;
            unsigned char r;
            unsigned char g;
            unsigned char b;
        } color;
        int pixel;
    };
    int index;
};

static int foo_compar(const void *a, const void *b) {
    foo *A = (foo *) a;
    foo *B = (foo *) b;
    return A->pixel - B->pixel;
}

/* public */
InverseCMap::InverseCMap(FWColor *origcmap) {
    // Common initializations
    cmap = new FWColor[256];
    bool color = false;
    for (int i = 0; i < 256; i++) {
        cmap[i] = origcmap[i];
        if (cmap[i].r != cmap[i].g || cmap[i].r != cmap[i].b)
            color = true;
    }
    nsearches = 0;
    ncomparisons = 0;

    if (color) {

        // Color
        
        // Since the performance of the algorithm used here can be spoiled
        // big time by duplicated colors (and the colormaps generated by
        // 'xv' can contain LOTS of those), eliminate those first:
        
        foo f[256];
        for (int i = 0; i < 256; i++) {
            f[i].color.filler = 0;
            f[i].color.r = cmap[i].r;
            f[i].color.g = cmap[i].g;
            f[i].color.b = cmap[i].b;
            f[i].index = i;
        }
        qsort(f, 256, sizeof(foo), foo_compar);
        bool boring[256];
        for (int i = 0; i < 256; i++)
            boring[i] = false;
        int ndups = 0;
        for (int i = 1; i < 256; i++)
            if (f[i].pixel == f[i - 1].pixel) {
                boring[f[i].index] = true;
                ndups++;
            }

        // OK, done finding duplicates. The 'boring' array flags dups, and
        // 'ndups' says how many dups there are in total.


        nnearest = (int *) malloc(512 * sizeof(int));
        nearest = (unsigned char **) malloc(512 * sizeof(unsigned char *));

        int *distance[512];
        for (int i = 0; i < 512; i++) {
            distance[i] = (int *) malloc(256 * sizeof(int));
            nearest[i] = (unsigned char *) malloc(256);
        }
        
        int maxcolor = -1;
        for (int i = 0; i < 256; i++) {
            if (boring[i])
                continue;
            maxcolor++;

            FWColor c = cmap[i];
            int dist = (c.r - 16) * (c.r - 16)
                    + (c.g - 16) * (c.g - 16)
                    + (c.b - 16) * (c.b - 16);
            int cube = 0;
            int R = 16, G = 16, B = 16;
            while (true) {
                int low = 0;
                int high = maxcolor - 1;
                while (low <= high) {
                    int mid = (low + high) / 2;
                    int d = distance[cube][mid];
                    if (d < dist)
                        low = mid + 1;
                    else if (d > dist)
                        high = mid - 1;
                    else {
                        low = mid;
                        break;
                    }
                }
                memmove(nearest[cube] + low + 1, nearest[cube] + low,
                        (maxcolor - low));
                memmove(distance[cube] + low + 1, distance[cube] + low,
                        (maxcolor - low) * sizeof(int));
                nearest[cube][low] = i;
                distance[cube][low] = dist;
                cube++;
                if (B != 240) {
                    dist += 64 * (B - c.b) + 1024;
                    B += 32;
                } else {
                    dist += (-448) * (B - c.b) + 50176;
                    B = 16;
                    if (G != 240) {
                        dist += 64 * (G - c.g) + 1024;
                        G += 32;
                    } else {
                        dist += (-448) * (G - c.g) + 50176;
                        G = 16;
                        if (R != 240) {
                            dist += 64 * (R - c.r) + 1024;
                            R += 32;
                        } else
                            break;
                    }
                }
            }
        }

        for (int i = 0; i < 512; i++) {
            int dist = distance[i][0];
            int maxdist = dist + ((int) sqrt(3072 * dist)) + 768;
            int j = 1;
            while (j <= maxcolor && distance[i][j] <= maxdist)
                j++;
            unsigned char *nea = (unsigned char *) malloc(j);
            memcpy(nea, nearest[i], j);
            free(nearest[i]);
            nearest[i] = nea;
            nnearest[i] = j;
            free(distance[i]);
        }

        // Unused grayscale stuff
        invmap = NULL;

        if (g_verbosity >= 3) {
            fprintf(stderr, "InverseCMap allocated an 8x8x8 color preselection cube,\nwith %d colors (%d duplicates eliminated):\n", 256 - ndups, ndups);
            fprintf(stderr, "Preselection counts:               Where are the colors:\n");
            int where[512];
            for (int i = 0; i < 512; i++)
                where[i] = 0;
            for (int i = 0; i < 256; i++)
                if (!boring[i])
                    where[((cmap[i].r & 0xE0) << 1) | ((cmap[i].g & 0xE0) >> 2) | ((cmap[i].b & 0xE0) >> 5)]++;
            for (int row = 0; row < 64; row++) {
                for (int col = 0; col < 8; col++)
                    fprintf(stderr, "%3d ", nnearest[row * 8 + col]);
                fprintf(stderr, "   ");
                for (int col = 0; col < 8; col++)
                    fprintf(stderr, "%3d ", where[row * 8 + col]);
                fprintf(stderr, "\n");
                if ((row & 7) == 7)
                    fprintf(stderr, "\n");
            }
        }

    } else {

        // Grayscale

        int invmap1[256];
        int dist[256];
        for (int i = 0; i < 256; i++)
            invmap1[i] = -1;
        for (int i = 0; i < 256; i++)
            invmap1[cmap[i].r] = i;

        int last = -1;
        int lastpos;
        for (int i = 0; i < 256; i++)
            if (invmap1[i] == -1) {
                invmap1[i] = last;
                if (last != -1)
                    dist[i] = i - lastpos;
                else
                    dist[i] = -1;
            } else {
                last = invmap1[i];
                lastpos = i;
                dist[i] = 0;
            }

        last = -1;
        for (int i = 255; i >= 0; i--) {
            if (dist[i] != 0) {
                if (dist[i] == -1 || last != -1 && lastpos - i < dist[i])
                    invmap1[i] = last;
            } else {
                last = invmap1[i];
                lastpos = i;
            }
        }

        invmap = (unsigned char *) malloc(256);
        for (int i = 0; i < 256; i++)
            invmap[i] = invmap1[i];

        // Unused color stuff
        nnearest = NULL;
        nearest = NULL;

        if (g_verbosity >= 3) {
            fprintf(stderr, "InverseCMap allocated an inverse grayscale map:\n");
            for (int i = 0; i < 256; i++)
                fprintf(stderr, "%3d%c", invmap[i], (i & 15) == 15 ? '\n' : ' ');
            fprintf(stderr, "This is the forward graymap:\n");
            for (int i = 0; i < 256; i++)
                fprintf(stderr, "%3d%c", cmap[i].r, (i & 15) == 15 ? '\n' : ' ');
            fprintf(stderr, "\n");
        }
    }
}

/* public */
InverseCMap::~InverseCMap() {
    if (nnearest != NULL)
        free(nnearest);
    if (nearest != NULL) {
        for (int i = 0; i < 512; i++)
            free(nearest[i]);
        free(nearest);
    }
    if (invmap != NULL)
        free(invmap);
    delete[] cmap;
    if (g_verbosity >= 2) {
        if (invmap != NULL)
            fprintf(stderr, "InverseCMap: grayscale -- no searches.\n");
        else if (nsearches == 0)
            fprintf(stderr, "InverseCMap: 0 searches.\n");
        else
            fprintf(stderr, "InverseCMap: %lld searches, %lld comparisons: %g comp/srch.\n", nsearches, ncomparisons, ((float) ncomparisons) / nsearches);
    }
}

/* public */ unsigned char
InverseCMap::rgb2index(unsigned char r, unsigned char g,
                                    unsigned char b) {
    if (invmap == NULL) {
        // Color
        int cube = ((r & 0xE0) << 1) | ((g & 0xE0) >> 2) | ((b & 0xE0) >> 5);
        int n = nnearest[cube];
        nsearches++;
        ncomparisons += n;
        unsigned char *nea = nearest[cube];
        int mindist = 196608;
        int minindex;
        for (int i = 0; i < n; i++) {
            int index = nea[i];
            FWColor c = cmap[index];
            int dr = r - c.r;
            int dg = g - c.g;
            int db = b - c.b;
            int dist = dr * dr + dg * dg + db * db;
            if (dist < mindist) {
                mindist = dist;
                minindex = index;
            }
        }
        return minindex;
    } else {
        // Grayscale
        int k = (r * 306 + g * 601 + b * 117) / 1024;
        if (k > 255)
            k = 255;
        return invmap[k];
    }
}
