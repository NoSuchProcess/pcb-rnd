/* for popen() */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "wget_common.h"
#include "compat_misc.h"

enum {
	FCTX_INVALID = 0,
	FCTX_POPEN,
	FCTX_FOPEN,
	FCTX_NOP
};

const char *wget_cmd = "wget -U 'pcb-rnd-fp_wget'";
int fp_wget_offline = 0;

static int mkdirp(const char *dir)
{
/* TODO */
	char buff[8192];
	sprintf(buff, "mkdir -p '%s'", dir);
	return system(buff);
}

int fp_wget_open(const char *url, const char *cache_path, FILE **f, int *fctx, fp_get_mode mode)
{
	char *cmd;
	const char *upds;
	int wl = strlen(wget_cmd), ul = strlen(url), cl = strlen(cache_path);
	cmd = malloc(wl+ul*2+cl+32);

	*fctx = FCTX_INVALID;

	if (mode & FP_WGET_UPDATE)
		upds = "-c";
	else
		upds = "";

	if (cache_path == NULL) {
		sprintf(cmd, "%s -O - %s '%s'", wget_cmd, upds, url);
		if (f == NULL)
			goto error;
		if (!fp_wget_offline)
			*f = popen(cmd, "r");
		if (*f == NULL)
			goto error;
		*fctx = FCTX_POPEN;
	}
	else {
		char *cdir;
		cdir = strstr(url, "://");
		if (cdir == NULL)
			goto error;
		cdir += 3;

		{
			char *end;
			sprintf(cmd, "%s/%s", cache_path, cdir);
			end = strrchr(cmd, '/');
			if (end != NULL) {
				*end = '\0';
				if (mkdirp(cmd) != 0)
					goto error;
				*end = '/';
			}
		}

		if ((!fp_wget_offline) && !(mode & FP_WGET_OFFLINE)) {
			sprintf(cmd, "%s -O '%s/%s' %s '%s'", wget_cmd, cache_path, cdir, upds, url);
			system(cmd);
		}
		if (f != NULL) {
			sprintf(cmd, "%s/%s", cache_path, cdir);
			*f = fopen(cmd, "r");
			if (*f == NULL)
				goto error;
			*fctx = FCTX_FOPEN;
		}
		else
			*fctx = FCTX_NOP;
	}
	free(cmd);
	return 0;

	error:;
	free(cmd);
	return -1;
}

int fp_wget_close(FILE **f, int *fctx)
{
	if (*fctx == FCTX_NOP)
		return 0;

	if (*f == NULL)
		return -1;

	switch(*fctx) {
		case FCTX_POPEN:  pclose(*f); *f = NULL; return 0;
		case FCTX_FOPEN: fclose(*f); *f = NULL; return 0;
	}

	return -1;
}


char *load_md5_sum(FILE *f)
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
				return pcb_strdup(sum);
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

int md5_cmp_free(const char *last_fn, char *md5_last, char *md5_new)
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
