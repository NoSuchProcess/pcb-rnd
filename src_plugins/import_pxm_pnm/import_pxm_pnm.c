/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  pixmap loader for pnm
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "plugins.h"
#include "pixmap.h"
#include "safe_fs.h"

static const char *import_pxm_pnm_cookie = "import_pxm_pnm";

#define ADDPX(pxm, r_, g_, b_, transparent) \
do { \
	int r = r_, g = g_, b = b_; \
	if ((r < 0) || (g < 0) || (b < 0)) \
		goto error; \
	if (transparent) { \
		*o++ = pxm->tr; \
		*o++ = pxm->tg; \
		*o++ = pxm->tb; \
	} \
	else { \
		if ((r == pxm->tr) && (g == pxm->tg) && (b == pxm->tb)) \
			b--; \
		*o++ = r; \
		*o++ = g; \
		*o++ = b; \
	} \
} while(0)

static int pnm_load(pcb_hidlib_t *hidlib, pcb_pixmap_t *pxm, const char *fn)
{
	FILE *f;
	char *s, line[1024];
	unsigned char *o;
	int n, type;

	f = pcb_fopen(hidlib, fn, "rb");
	if (f == NULL)
		return -1;
	
	fgets(line, sizeof(line) - 1, f);
	if ((line[0] != 'P') || ((line[1] != '4') && (line[1] != '5') && (line[1] != '6')) || (line[2] != '\n')) {
		fclose(f);
		return -1;
	}
	type = line[1];

	fgets(line, sizeof(line) - 1, f);
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

	pxm->tr = pxm->tg = 127;
	pxm->tb = 128;
	n = pxm->sx * pxm->sy;
	o = pxm->p = malloc(n * 3);

	switch(type) {
		case '6':
			fgets(line, sizeof(line) - 1, f);
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

static const pcb_pixmap_import_t pxm_pnm_imp = {
	"pnm",
	pnm_load
};

int pplg_check_ver_import_pxm_pnm(int ver_needed) { return 0; }

void pplg_uninit_import_pxm_pnm(void)
{
	pcb_pixmap_unreg_import_all(import_pxm_pnm_cookie);
}

#include "dolists.h"
int pplg_init_import_pxm_pnm(void)
{
	PCB_API_CHK_VER;
	pcb_pixmap_reg_import(&pxm_pnm_imp, import_pxm_pnm_cookie);
	return 0;
}
