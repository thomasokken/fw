#ifndef INVERSECMAP_H
#define INVERSECMAP_H 1

/* InverseCMap is a helper class that finds the closest match to a
 * given RGB value in a 256-entry colormap. It speeds up the search by
 * breaking the RGB space into 512 cubes and finding the candidates for
 * "closest match" within each cube. So, given an RGB value, it looks
 * at the top 3 bits of each component, and given those, selects one of
 * the 512 cubes, and only searches the colors that are candidates
 * within that cube. For natural color images, the speed-up compared to
 * a brute force search of the entire colormap can be in the twenties.
 * If the colormap entries are not nicely distributed, the speedup will
 * be worse; for greyscale images I have observed speedups of about 8.
 *
 * If the given colormap is pure grayscale, that is, if all its entries
 * satisfy r == g == b, InverseCMap can generate an actual inverse
 * mapping table, and no color comparisons are required at all by
 * rgb2index(). (Good thing, too, because the 512-cubes algorithm works
 * best if the colormap entries are nicely spread through the color space,
 * whereas a grayscale colormap has all its entries on the RGB cube's
 * main diagonal -- which leads to long searches, because for large
 * numbers of color subcubes, the list of candidates is much longer than
 * it would be with a more balanced colormap. Because of this, performance
 * is easily three times worse than for a nice "color" colormap!)
 *
 * Run FW with a verbosity level of 2 (-vv) to see statistics on the
 * average number of color comparisons per rgb2index() call.
 * Note: the InverseCMap constructor is rather expensive, once you
 * create one, try to hang onto it as long as possible.
 */

class FWColor;

class InverseCMap {
    private:

	// Inverse color mapping
	int *nnearest;
	unsigned char **nearest;

	// Inverse grayscale mapping
	unsigned char *invmap;

	// Common stuff
	FWColor *cmap;
	long long nsearches;
	long long ncomparisons;

    public:
	InverseCMap(FWColor *cmap);
	~InverseCMap();
	unsigned char rgb2index(unsigned char r, unsigned char g,
				unsigned char b);
};

#endif
