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
	return pcb_system(buff);
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
			*f = pcb_popen(cmd, "r");
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
			sprintf(cmd, "%s -O '%s/%s' %s '%s'", wget_cmd, cache_path, cdir, upds, url);
			res = pcb_system(cmd);
/*			pcb_trace("------res=%d\n", res); */
			if ((res != 0) && (res != 768)) { /* some versions of wget will return error on -c if the file doesn't need update; try to guess whether it's really an error */
				/* when wget fails, a 0-long file might be left there - remove it so it won't block new downloads */
				sprintf(cmd, "%s/%s", cache_path, cdir);
				pcb_remove(cmd);
			}
		}
		if (f != NULL) {
			sprintf(cmd, "%s/%s", cache_path, cdir);
			*f = pcb_fopen(cmd, "r");
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
		f = pcb_fopen(last_fn, "w");
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
