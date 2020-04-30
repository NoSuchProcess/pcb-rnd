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
#include <gd.h>

#include <librnd/core/plugins.h>
#include <librnd/core/pixmap.h>
#include <librnd/core/safe_fs.h>

static const char *import_pxm_gd_cookie = "import_pxm_gd";

static int gd_load(rnd_hidlib_t *hidlib, rnd_pixmap_t *pxm, const char *fn, gdImagePtr (*loader)(FILE *f))
{
	int x, y;
	gdImagePtr gdi;
	FILE *f;

	f = rnd_fopen(hidlib, fn, "rb");
	if (f == NULL)
		return -1;
	gdi = loader(f);
	if (gdi != NULL) {
		unsigned char *p;
		int r, g, b, a;
		pxm->sx = gdi->sx;
		pxm->sy = gdi->sy;
		pxm->tr = pxm->tg = 127;
		pxm->tb = 128;
		pxm->size = pxm->sx * pxm->sy * 3;
		pxm->has_transp = 0;
		p = pxm->p = malloc(pxm->size);
		for(y = 0; y < pxm->sy; y++) {
			for(x = 0; x < pxm->sx; x++) {
				int pix = gdImageGetPixel(gdi, x, y);
				r = gdImageRed(gdi, pix);
				g = gdImageGreen(gdi, pix);
				b = gdImageBlue(gdi, pix);
				a = gdImageAlpha(gdi, pix);

				if (a == 0) {
					if ((r == pxm->tr) && (g == pxm->tg) && (b == pxm->tb))
						b--;
					*p++ = r;
					*p++ = g;
					*p++ = b;
				}
				else {
					*p++ = pxm->tr;
					*p++ = pxm->tg;
					*p++ = pxm->tb;
					pxm->has_transp = 1;
				}
			}
		}
	}
	fclose(f);
	return (gdi == NULL);
}

#ifdef RND_HAVE_GDIMAGEPNG
static int gd_png_load(rnd_hidlib_t *hidlib, rnd_pixmap_t *pxm, const char *fn)
{
	return gd_load(hidlib, pxm, fn, gdImageCreateFromPng);
}

static const rnd_pixmap_import_t pxm_gd_png_imp = {
	"png (gd)",
	gd_png_load
};
#endif

#ifdef RND_HAVE_GDIMAGEJPEG
static int gd_jpg_load(rnd_hidlib_t *hidlib, rnd_pixmap_t *pxm, const char *fn)
{
	return gd_load(hidlib, pxm, fn, gdImageCreateFromJpeg);
}

static const rnd_pixmap_import_t pxm_gd_jpg_imp = {
	"jpg (gd)",
	gd_jpg_load
};
#endif

#ifdef RND_HAVE_GDIMAGEGIF
static int gd_gif_load(rnd_hidlib_t *hidlib, rnd_pixmap_t *pxm, const char *fn)
{
	return gd_load(hidlib, pxm, fn, gdImageCreateFromGif);
}

static const rnd_pixmap_import_t pxm_gd_gif_imp = {
	"gif (gd)",
	gd_gif_load
};
#endif

int pplg_check_ver_import_pxm_gd(int ver_needed) { return 0; }

void pplg_uninit_import_pxm_gd(void)
{
	rnd_pixmap_unreg_import_all(import_pxm_gd_cookie);
}

int pplg_init_import_pxm_gd(void)
{
	RND_API_CHK_VER;

#ifdef RND_HAVE_GDIMAGEPNG
	rnd_pixmap_reg_import(&pxm_gd_png_imp, import_pxm_gd_cookie);
#endif
#ifdef RND_HAVE_GDIMAGEJPEG
	rnd_pixmap_reg_import(&pxm_gd_jpg_imp, import_pxm_gd_cookie);
#endif
#ifdef RND_HAVE_GDIMAGEGIF
	rnd_pixmap_reg_import(&pxm_gd_gif_imp, import_pxm_gd_cookie);
#endif
	return 0;
}
