#include <stdio.h>

typedef enum {
	FP_WGET_UPDATE = 1,  /* wget -c: update existing file, don't replace (a.k.a. cache) */
	FP_WGET_OFFLINE = 2  /* do not wget, open cached file or fail */
} fp_get_mode;

int fp_wget_open(const char *url, const char *cache_path, FILE **f, int *fctx, fp_get_mode mode);
int fp_wget_close(FILE **f, int *fctx);
