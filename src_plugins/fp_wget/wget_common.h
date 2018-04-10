#include <stdio.h>

typedef enum {
	FP_WGET_UPDATE = 1,  /* wget -c: update existing file, don't replace (a.k.a. cache) */
	FP_WGET_OFFLINE = 2  /* do not wget, open cached file or fail */
} fp_get_mode;

int fp_wget_open(const char *url, const char *cache_path, FILE **f, int *fctx, fp_get_mode mode);
int fp_wget_close(FILE **f, int *fctx);

char *load_md5_sum(FILE *f);
int md5_cmp_free(const char *last_fn, char *md5_last, char *md5_new);

/* search fn in an auto updated index file; return 0 on success and
   fill in out with the full path from the index. */
int fp_wget_search(char *out, int out_len, char *fn, int offline, const char *idx_url, const char *idx_cache);

/* search fn in an index file; return 0 on success and fill in out with the
   full path from the index. */
int fp_wget_search_(char *out, int out_len, FILE *f_idx, const char *fn);

