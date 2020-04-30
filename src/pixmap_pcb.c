/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <genht/htsp.h>

#include <librnd/core/pixmap.h>
#include <librnd/core/error.h>
#include "pixmap_pcb.h"
#include "obj_common.h"

pcb_pixmap_hash_t pcb_pixmaps;

void pcb_pixmap_hash_init(pcb_pixmap_hash_t *pmhash)
{
	htpp_init(&pmhash->meta, rnd_pixmap_hash_meta, rnd_pixmap_eq_meta);
	htpp_init(&pmhash->pixels, rnd_pixmap_hash_pixels, rnd_pixmap_eq_pixels);
}

void pcb_pixmap_hash_uninit(pcb_pixmap_hash_t *pmhash)
{
	htpp_uninit(&pmhash->meta);
	htpp_uninit(&pmhash->pixels);
}

rnd_pixmap_t *pcb_pixmap_insert_neutral_or_free(pcb_pixmap_hash_t *pmhash, rnd_pixmap_t *pm)
{
	rnd_pixmap_t *r;

	if ((pm->tr_rot != 0) || pm->tr_xmirror || pm->tr_ymirror)
		return NULL;

	r = htpp_get(&pmhash->pixels, pm);
	if (r != NULL) {
		rnd_pixmap_free(pm);
		return r;
	}

	htpp_set(&pmhash->meta, pm, pm);
	htpp_set(&pmhash->pixels, pm, pm);

	return pm;
}

rnd_pixmap_t *pcb_pixmap_alloc_insert_transformed(pcb_pixmap_hash_t *pmhash, rnd_pixmap_t *ipm, rnd_angle_t rot, int xmirror, int ymirror)
{
	rnd_pixmap_t *pm;
	pcb_xform_mx_t mx = PCB_XFORM_MX_IDENT;
	long n, len;
	int x, y, ix, iy, ox, oy;
	unsigned char *ip, *op;

	if ((rot == 0) && !xmirror && !ymirror)
		return ipm; /* xformed == neutral */

	pm = calloc(sizeof(rnd_pixmap_t), 1);
	if (ipm->has_transp) {
		pm->tr = ipm->tr;
		pm->tg = ipm->tg;
		pm->tb = ipm->tb;
	}
	else
		pm->tr = pm->tg = pm->tb = 127;
	pm->neutral_oid = ipm->neutral_oid;
	pm->tr_rot = rot;
	pm->tr_xmirror = xmirror;
	pm->tr_ymirror = ymirror;
	pm->tr_xscale = 1.0;
	pm->tr_yscale = 1.0;
	pm->has_transp = 1;


	TODO("gfx: apply mirrors");
	pcb_xform_mx_rotate(mx, rot);


	pm->sx = pcb_xform_x(mx, (ipm->sx/2)+1, (ipm->sy/2)+1) * 2;
	pm->sy = pcb_xform_y(mx, -(ipm->sx/2)-1, (ipm->sy/2)-1) * 2;

	/* alloc and fill with transparent */
	len = pm->sx * pm->sy * 3;
	pm->p = malloc(len);
	for(n = 0; n < len; n += 3) {
		pm->p[n+0] = pm->tr;
		pm->p[n+1] = pm->tg;
		pm->p[n+2] = pm->tb+1;
	}

	ip = ipm->p;
	for(y = 0; y < ipm->sy; y++) {
		iy = y - ipm->sy/2;
		for(x = 0; x < ipm->sx; x++, ip+=3) {
			ix = x - ipm->sx/2;
			ox = pcb_xform_x(mx, ix, iy) + pm->sx/2;
			oy = pcb_xform_y(mx, ix, iy) + pm->sy/2;
			if ((ox < 0) || (ox >= pm->sx) || (oy < 0) || (oy >= pm->sy))
				continue;
			op = pm->p + (oy * pm->sx + ox) * 3;
			op[0] = ip[0];
			op[1] = ip[1];
			op[2] = ip[2];
		}
	}

	rnd_trace("sx;sy: %ld %ld -> %ld %ld\n", ipm->sx, ipm->sy, pm->sx, pm->sy);
TODO("create the transformed version if not in the cache already (by headers)");

	return pm;
}

void pcb_pixmap_init(void)
{
	pcb_pixmap_hash_init(&pcb_pixmaps);
}

void pcb_pixmap_uninit(void)
{
	pcb_pixmap_hash_uninit(&pcb_pixmaps);
	rnd_pixmap_uninit();
}
