#include <stdio.h>

int fp_wget_open(const char *url, const char *cache_path, FILE **f, int *fctx, int update);
int fp_wget_close(FILE **f, int *fctx);
