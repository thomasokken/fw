#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>

#include "util.h"

char *strclone(const char *src) {
    if (src == NULL)
	return NULL;
    int len = strlen(src);
    char *dst = new char[len + 1];
    strcpy(dst, src);
    return dst;
}

struct node {
    char *name;
    node *next;
};

static int cmp(const void *a, const void *b) {
    return strcmp(*(char **) a, *(char **) b);
}

char **getpluginnames() {
    node *head = NULL;

    char dirname[_POSIX_PATH_MAX];
    snprintf(dirname, _POSIX_PATH_MAX, "%s/.fw", getenv("HOME"));
    DIR *dir = opendir(dirname);
    if (dir == NULL)
	return NULL;

    struct dirent *dent;
    int numnames = 0;
    while ((dent = readdir(dir)) != NULL) {
	// According to readdir(2), dent->d_reclen should be the
	// length of dent->d_name, but that ain't true
	int len = strlen(dent->d_name);
	if (len >= 3 && strcmp(dent->d_name + (len - 3), ".so") == 0
		     && strcmp(dent->d_name, "About.so") != 0) {
	    node *n = new node;
	    n->name = strclone(dent->d_name);
	    n->name[len - 3] = 0;
	    n->next = head;
	    head = n;
	    numnames++;
	}
    }
    closedir(dir);

    if (numnames == 0)
	return NULL;

    char **namelist = new char *[numnames + 1];
    for (int i = 0; i < numnames; i++) {
	namelist[i] = head->name;
	node *next = head->next;
	delete head;
	head = next;
    }
    namelist[numnames] = NULL;

    qsort(namelist, numnames, sizeof(char *), cmp);
    return namelist;
}

void deletenamelist(char **list) {
    if (list == NULL)
	return;
    char **l = list;
    while (*l != NULL) {
	delete[] *l;
	l++;
    }
    delete[] list;
}
