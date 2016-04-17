#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <genvector/gds_char.h>
#include <genht/htsp.h>
#include <genht/hash.h>
#include "global.h"
#include "wget_common.h"
#include "gedasymbols.h"
#include "plugins.h"
#include "plug_footprint.h"


#define REQUIRE_PATH_PREFIX "wget@gedasymbols"

#define CGI_URL "http://www.gedasymbols.org/scripts/global_list.cgi"
#define FP_URL "http://www.gedasymbols.org/"
#define FP_DL "?dl"
static const char *url_idx_md5 = CGI_URL "?md5";
static const char *url_idx_list = CGI_URL;
static const char *gedasym_cache = "fp_wget_cache";
static const char *last_sum_fn = "fp_wget_cache/gedasymbols.last";

static char *load_md5_sum(FILE *f)
{
	char *s, sum[64];

	if (f == NULL)
		return NULL;

	*sum = '\0';
	fgets(sum, sizeof(sum), f);
	sum[sizeof(sum)-1] = '\0';

	for(s = sum;; s++) {
		if ((*s == '\0') || (isspace(*s))) {
			if ((s - sum) == 32) {
				*s = '\0';
				return strdup(sum);
			}
			else
				return NULL;
		}
		if (isdigit(*s))
			continue;
		if ((*s >= 'a') && (*s <= 'f'))
			continue;
		if ((*s >= 'A') && (*s <= 'F'))
			continue;
		return NULL;
	}
}

static int md5_cmp_free(const char *last_fn, char *md5_last, char *md5_new)
{
	int changed = 0;

	if ((md5_last == NULL) || (strcmp(md5_last, md5_new) != 0)) {
		FILE *f;
		f = fopen(last_fn, "w");
		fputs(md5_new, f);
		fclose(f);
		changed = 1;
	}
	if (md5_last != NULL)
		free(md5_last);
	free(md5_new);
	return changed;
}

int fp_gedasymbols_load_dir(plug_fp_t *ctx, const char *path)
{
	FILE *f;
	int fctx;
	char *md5_last, *md5_new;
	int tmp;
	char line[1024];
	fp_get_mode mode;
	gds_t vpath;
	int vpath_base_len;

	if (strncmp(path, REQUIRE_PATH_PREFIX, strlen(REQUIRE_PATH_PREFIX)) != 0)
		return -1;

	if (fp_wget_open(url_idx_md5, gedasym_cache, &f, &fctx, 0) != 0)
		return -1;
	md5_new = load_md5_sum(f);
	fp_wget_close(&f, &fctx);

	if (md5_new == NULL)
		return -1;

	f = fopen(last_sum_fn, "r");
	md5_last = load_md5_sum(f);
	if (f != NULL)
		fclose(f);

	printf("old='%s' new='%s'\n", md5_last, md5_new);

	if (!md5_cmp_free(last_sum_fn, md5_last, md5_new)) {
		printf("no chg.\n");
		mode = FP_WGET_OFFLINE; /* use the cache */
	}
	else
		mode = 0;

	if (fp_wget_open(url_idx_list, gedasym_cache, &f, &fctx, mode) != 0) {
		printf("failed to download the new list\n");
		remove(last_sum_fn); /* make sure it is downloaded next time */
		return -1;
	}

	gds_init(&vpath);
	gds_append_str(&vpath, REQUIRE_PATH_PREFIX);
	gds_append(&vpath, '/');
	vpath_base_len = vpath.used;

	while(fgets(line, sizeof(line), f) != NULL) {
		char *end, *fn;
		library_t *l, *parent;

		if (*line == '#')
			continue;
		end = strchr(line, '|');
		if (end == NULL)
			continue;
		*end = '\0';

		/* split path and fn; path stays in vpath.array, fn is a ptr to the file name */
		gds_truncate(&vpath, vpath_base_len);
		gds_append_str(&vpath, line);
		end = vpath.array + vpath.used - 1;
		while((end > vpath.array) && (*end != '/')) { end--; vpath.used--; }
		*end = '\0';
		vpath.used--;
		end++;
		fn = end;

		/* add to the database */
		l = fp_mkdir_p(vpath.array);
		fp_append_entry(l, vpath.array, fn, PCB_FP_FILE, NULL);
	}
	fp_wget_close(&f, &fctx);

	printf("update!\n");
	return 0;
}

#define FIELD_WGET_CTX 0

FILE *fp_gedasymbols_fopen(plug_fp_t *ctx, const char *path, const char *name, fp_fopen_ctx_t *fctx)
{
	gds_t s;
	gds_init(&s);
	FILE *f;

	if (strncmp(path, "gedasymbols/", 12) == 0)
		path+=12;
	if (*path == '/')
		path++;

	gds_append_str(&s, FP_URL);
	gds_append_str(&s, path);
	gds_append(&s, '/');
	gds_append_str(&s, name);
	gds_append_str(&s, FP_DL);

	fp_wget_open(s.array, gedasym_cache, &f, &(fctx->field[FIELD_WGET_CTX].i), 1);

	gds_uninit(&s);
	return f;
}

void fp_gedasymbols_fclose(plug_fp_t *ctx, FILE * f, fp_fopen_ctx_t *fctx)
{
	fp_wget_close(&f, &(fctx->field[FIELD_WGET_CTX].i));
}


static plug_fp_t fp_gedasymbols;

void fp_gedasymbols_init(void)
{
	fp_gedasymbols.plugin_data = NULL;
	fp_gedasymbols.load_dir = fp_gedasymbols_load_dir;
	fp_gedasymbols.fopen = fp_gedasymbols_fopen;
	fp_gedasymbols.fclose = fp_gedasymbols_fclose;

	HOOK_REGISTER(plug_fp_t, plug_fp_chain, &fp_gedasymbols);
}
