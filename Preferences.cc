#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Preferences.h"

/* public */
Preferences::Preferences(int *argc, char *argv[]) {
    verbosity = 0;
    x_errors_coredump = false;
    no_priv_cmap_in_cpicker = false;


    // Read command line args (and remove ones that we recognized)

    for (int i = 1; i < *argc; i++) {
	int remove = 0;
	if (strncmp(argv[i], "-v", 2) == 0) {
	    char *p = argv[i] + 1;
	    while (*p++ == 'v')
		verbosity++;
	    remove = 1;
	} else if (strcmp(argv[i], "-xdump") == 0) {
	    x_errors_coredump = true;
	    remove = 1;
	} else if (strcmp(argv[i], "-nopriv") == 0) {
	    no_priv_cmap_in_cpicker = true;
	} else if (strcmp(argv[i], "-h") == 0
		|| strcmp(argv[i], "-help") == 0
		|| strcmp(argv[i], "--help") == 0) {
	    fprintf(stderr, "Usage: fw [options] [files...]\n");
	    fprintf(stderr, "    available options:\n");
	    fprintf(stderr, "    X Toolkit options (see \"man X\")\n");
	    fprintf(stderr, "    -v : verbose (-vv, -vvv, etc: more verbosity)\n");
	    fprintf(stderr, "    -xdump : dump core on X errors (implies -synchronous)\n");
	    fprintf(stderr, "    -nopriv : never use private colormap in color picker\n");
	    fprintf(stderr, "    -h , -help , --help : print usage information & exit\n");
	    exit(0);

	} else if (argv[i][0] == '-') {
	    fprintf(stderr, "Unrecognized option \"%s\" (see \"fw -h\" for help)\n", argv[i]);
	    exit(1);
	}

	if (remove > 0) {
	    for (int j = i; j <= *argc - remove; j++)
		argv[j] = argv[j + remove];
	    *argc -= remove;
	    argv[*argc] = NULL;
	    i--;
	}
    }


    // Settings from environment variables

    if (getenv("FW_XDUMP") != NULL)
	x_errors_coredump = true;
    if (getenv("FW_NOPRIV") != NULL)
	no_priv_cmap_in_cpicker = true;

    // All done, report settings that are not at their defaults

    if (verbosity >= 1) {
	fprintf(stderr, "Verbosity level set to %d.\n", verbosity);
	if (x_errors_coredump)
	    fprintf(stderr, "Dumping core on X errors.\n");
	if (no_priv_cmap_in_cpicker)
	    fprintf(stderr, "Color picker will never use private colormap.\n");
    }
}

/* public */
Preferences::~Preferences() {
    //
}
