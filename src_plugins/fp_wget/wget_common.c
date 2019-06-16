/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017, 2018 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "wget_common.h"
#include "compat_misc.h"
#include "safe_fs.h"
#include "globalconst.h"

#include "../src_plugins/lib_wget/lib_wget.h"

int fp_wget_offline = 0;

enum {
	FCTX_INVALID = 0,
	FCTX_POPEN,
	FCTX_FOPEN,
	FCTX_NOP
};

static int mkdirp(const char *dir)
{
	int len;
	char buff[PCB_PATH_MAX+32];
	len = pcb_snprintf(buff, sizeof(buff), "mkdir -p '%s'", dir);
	if (len >= sizeof(buff)-1)
		return -1;
	return pcb_system(NULL, buff);
}

int fp_wget_open(const char *url, const char *cache_path, FILE **f, int *fctx, fp_get_mode mode)
{
	char *cmd;
	int ul = strlen(url), cl = strlen(cache_path);
	int update = (mode & FP_WGET_UPDATE);

	cmd = malloc(ul*2+cl+32);
	*fctx = FCTX_INVALID;

	if (cache_path == NULL) {
		if (f == NULL)
			goto error;
		if (!fp_wget_offline)
			*f = pcb_wget_popen(url, update);
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
			int res;
			sprintf(cmd, "%s/%s", cache_path, cdir);
			res = pcb_wget_disk(url, cmd, update);
/*			pcb_trace("------res=%d\n", res); */
			if ((res != 0) && (res != 768)) { /* some versions of wget will return error on -c if the file doesn't need update; try to guess whether it's really an error */
				/* when wget fails, a 0-long file might be left there - remove it so it won't block new downloads */
				pcb_remove(NULL, cmd);
			}
		}
		if (f != NULL) {
			sprintf(cmd, "%s/%s", cache_path, cdir);
			*f = pcb_fopen(NULL, cmd, "r");
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
		case FCTX_POPEN: pcb_pclose(*f); *f = NULL; return 0;
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
		f = pcb_fopen(NULL, last_fn, "w");
		fputs(md5_new, f);
		fclose(f);
		changed = 1;
	}
	if (md5_last != NULL)
		free(md5_last);
	free(md5_new);
	return changed;
}

int fp_wget_search_(char *out, int out_len, FILE *f, const char *fn)
{
	char *line, line_[8192];

	*out = '\0';

	if (f == NULL)
		return -1;

	while((line = fgets(line_, sizeof(line_), f)) != NULL) {
		char *sep;
		sep = strchr(line, '|');
		if (sep == NULL)
			continue;
		*sep = '\0';
		if ((strstr(line, fn) != NULL) && (strlen(line) < out_len)) {
			strcpy(out, line);
			return 0;
		}
	}
	return -1;
}


int fp_wget_search(char *out, int out_len, const char *name, int offline, const char *url, const char *cache)
{
	FILE *f_idx;
	int fctx;
	fp_get_mode mode = offline ? FP_WGET_OFFLINE : 0;

	if (fp_wget_open(url, cache, &f_idx, &fctx, mode) == 0) {
		if (fp_wget_search_(out, out_len, f_idx, name) != 0) {
			fp_wget_close(&f_idx, &fctx);
			return -1;
		}
	}
	else
		return -1;
	fp_wget_close(&f_idx, &fctx);
	return 0;
}
