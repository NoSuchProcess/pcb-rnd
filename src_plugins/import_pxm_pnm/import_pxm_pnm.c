/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  pixmap loader for pnm
 *  pcb-rnd Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <librnd/core/plugins.h>
#include <librnd/core/pixmap.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/error.h>
#include <librnd/core/compat_misc.h>

static const char *import_pxm_pnm_cookie = "import_pxm_pnm";

#define ADDPX(pxm, r_, g_, b_, not_transparent) \
do { \
	int r = r_, g = g_, b = b_; \
	if ((r < 0) || (g < 0) || (b < 0)) \
		goto error; \
	if (not_transparent && (r == pxm->tr) && (g == pxm->tg) && (b == pxm->tb)) \
		b--; \
	*o++ = r; \
	*o++ = g; \
	*o++ = b; \
} while(0)

static void decode_comment(rnd_pixmap_t *pxm, char *comment)
{
	while(isspace(*comment)) comment++;
	if (rnd_strncasecmp(comment, "transparent pixel:", 18) == 0) {
		int r, g, b;
		if (sscanf(comment+18, "%d %d %d", &r, &g, &b) == 3) {
			if ((r >= 0) && (r <= 255) && (g >= 0) && (g <= 255) && (b >= 0) && (b <= 255)) {
				pxm->tr = r;
				pxm->tg = g;
				pxm->tb = b;
				pxm->has_transp = 1;
			}
			else
				rnd_message(RND_MSG_ERROR, "pnm_load(): ignoring invalid transparent pixel: value out of range (%d %d %d)\n", r, g, b);
		}
		else
			rnd_message(RND_MSG_ERROR, "pnm_load(): ignoring invalid transparent pixel: need 3 integers (got: %s)\n", comment+18);
	}
}

#define GETLINE \
while(fgets(line, sizeof(line) - 1, f) != NULL) { \
	if (*line == '#') {\
		decode_comment(pxm, line+1); \
		continue; \
	} \
	break; \
}

static int pnm_load(rnd_hidlib_t *hidlib, rnd_pixmap_t *pxm, const char *fn)
{
	FILE *f;
	char *s, line[1024];
	unsigned char *o;
	int n, type;

	f = rnd_fopen(hidlib, fn, "rb");
	if (f == NULL)
		return -1;

	pxm->tr = pxm->tg = 127;
	pxm->tb = 128;
	pxm->has_transp = 0;

	GETLINE;
	if ((line[0] != 'P') || ((line[1] != '4') && (line[1] != '5') && (line[1] != '6')) || (line[2] != '\n')) {
		fclose(f);
		return -1;
	}
	type = line[1];


	GETLINE;
	s = strchr(line, ' ');
	if (s == NULL) {
		fclose(f);
		return -1;
	}
	*s = '\0';
	s++;
	pxm->sx = atoi(line);
	pxm->sy = atoi(s);

	if ((pxm->sx <= 0) || (pxm->sy <= 0) || (pxm->sx > 100000) || (pxm->sy > 100000)) {
		fclose(f);
		return -1;
	}

	n = pxm->sx * pxm->sy;
	pxm->size = n * 3;
	o = pxm->p = malloc(pxm->size);

	switch(type) {
		case '6':
			GETLINE;
			if (atoi(line) != 255)
				goto error;
			for(; n>0; n--)
				ADDPX(pxm, fgetc(f), fgetc(f), fgetc(f), 0);
			break;
		case '5':
			fgets(line, sizeof(line) - 1, f);
			for(; n>0; n--) {
				int px = fgetc(f);
				ADDPX(pxm, px, px, px, 0);
			}
			break;
		case '4':
			for(;n>0;) {
				int m, c, px;
				c = fgetc(f);
				for(m = 0; m < 8; m++) {
					px = (c & 128) ? 0 : 255;
					ADDPX(pxm, px, px, px, 0);
					c <<= 1;
					n--;
				}
			}
			break;
	}
	fclose(f);
	return 0;

	error:;
	free(pxm->p);
	pxm->p = NULL;
	fclose(f);
	return 0;
}

static const rnd_pixmap_import_t pxm_pnm_imp = {
	"pnm",
	pnm_load
};

int pplg_check_ver_import_pxm_pnm(int ver_needed) { return 0; }

void pplg_uninit_import_pxm_pnm(void)
{
	rnd_pixmap_unreg_import_all(import_pxm_pnm_cookie);
}

int pplg_init_import_pxm_pnm(void)
{
	RND_API_CHK_VER;
	rnd_pixmap_reg_import(&pxm_pnm_imp, import_pxm_pnm_cookie);
	return 0;
}
