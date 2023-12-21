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

#include <librnd/hid/pixmap.h>
#include <librnd/core/error.h>
#include <librnd/core/math_helper.h>
#include <librnd/core/compat_misc.h>
#include "pixmap_pcb.h"
#include "obj_common.h"

#define ROT_DEBUG 0

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

rnd_pixmap_t *pcb_pixmap_insert_neutral_dup(rnd_design_t *hl, pcb_pixmap_hash_t *pmhash, const rnd_pixmap_t *pm)
{
	rnd_pixmap_t *r;

	if ((pm->tr_rot != 0) || pm->tr_xmirror || pm->tr_ymirror)
		return NULL;

	r = htpp_get(&pmhash->pixels, pm);
	if (r != NULL)
		return r;

	r = rnd_pixmap_dup(hl, pm);
	htpp_set(&pmhash->meta, r, r);
	htpp_set(&pmhash->pixels, r, r);

	return r;
}

rnd_pixmap_t *pcb_pixmap_alloc_insert_transformed(pcb_pixmap_hash_t *pmhash, rnd_pixmap_t *ipm, rnd_angle_t rot, rnd_coord_t render_sx, rnd_coord_t render_sy, int xmirror, int ymirror)
{
	rnd_pixmap_t *opm;
	pcb_xform_mx_t mx = PCB_XFORM_MX_IDENT;
	pcb_xform_mx_t mxr = PCB_XFORM_MX_IDENT;
	long n, len, icx, icy, ocx, ocy, xo, yo, end, sx1, sx2, sy1, sy2;
	unsigned char *o, *i;
	double scx, iasp, rasp;
	int scaled = 0;

	if ((rot == 0) && !xmirror && !ymirror)
		return ipm; /* xformed == neutral */

	iasp = (double)ipm->sx / (double)ipm->sy;     /* input aspect */
	rasp = (double)render_sx / (double)render_sy; /* rendering aspect */

	if (fabs(iasp-rasp) > 0.001) {
		scx = rasp/iasp;
		scaled = 1;
	}


	opm = calloc(sizeof(rnd_pixmap_t), 1);
	if (ipm->has_transp) {
		opm->tr = ipm->tr;
		opm->tg = ipm->tg;
		opm->tb = ipm->tb;
	}
	else
		opm->tr = opm->tg = opm->tb = 127;
	opm->neutral_oid = ipm->neutral_oid;
	opm->tr_rot = rot;
	opm->tr_xmirror = xmirror;
	opm->tr_ymirror = ymirror;
	opm->tr_xscale = 1.0;
	opm->tr_yscale = 1.0;
	opm->has_transp = 1;

	if (xmirror)
		pcb_xform_mx_scale(mxr, -1.0, 1.0);

	if (ymirror)
		pcb_xform_mx_scale(mxr, 1.0, -1.0);

	if (scaled)
		pcb_xform_mx_scale(mxr, 1.0/scx, 1.0);

	pcb_xform_mx_rotate(mx, rot);
	pcb_xform_mx_rotate(mxr, -rot);

	if (scaled)
		pcb_xform_mx_scale(mx, scx, 1.0);

	sx1 = pcb_xform_x(mx, (ipm->sx/2)+1, (ipm->sy/2)+1) * 2;
	sx2 = pcb_xform_x(mx, -(ipm->sx/2)-1, +(ipm->sy/2)+1) * 2;
	sy1 = pcb_xform_y(mx, -(ipm->sx/2)-1, (ipm->sy/2)-1) * 2;
	sy2 = pcb_xform_y(mx, -(ipm->sx/2)-1, -(ipm->sy/2)+1) * 2;

	sx1 = RND_ABS(sx1); sx2 = RND_ABS(sx2);
	sy1 = RND_ABS(sy1); sy2 = RND_ABS(sy2);

	opm->sx = sx1 > sx2 ? sx1 : sx2;
	opm->sy = sy1 > sy2 ? sy1 : sy2;

	/* alloc and fill with transparent */
	len = opm->sx * opm->sy * 3;
	opm->p = malloc(len);
	for(n = 0; n < len; n += 3) {
		opm->p[n+0] = opm->tr;
		opm->p[n+1] = opm->tg;
		opm->p[n+2] = opm->tb + ROT_DEBUG;
	}


	/* center points */
	icx = ipm->sx/2;
	icy = ipm->sy/2;
	ocx = opm->sx/2;
	ocy = opm->sy/2;

	end = opm->sx * opm->sy;

	for(n = 0, o = opm->p, xo = yo = 0; n < end; n++, o+=3) {
		int oor = 0;
		long ixi, iyi;
		double XO = xo - ocx, YO = yo - ocy, XI, YI;

		/* rotate */
		XI = icx + pcb_xform_x(mxr, XO, YO);
		YI = icy + pcb_xform_y(mxr, XO, YO);

		/* get final input pixel address, clamp */
		ixi = rnd_round(XI); iyi = rnd_round(YI);
		if ((ixi < 0) || (iyi < 0) || (ixi >= ipm->sx) || (iyi >= ipm->sy))
			oor = 1;
		else
			i = ipm->p + (ixi + iyi * ipm->sx) * 3;

		if (oor) {
			/* out pixel is transparent if out-of-range or input pixel is transparent */
			o[0] = opm->tr;
			o[1] = opm->tg;
			o[2] = opm->tb + ROT_DEBUG;
		}
		else {
			o[0] = i[0];
			o[1] = i[1];
			o[2] = i[2];
		}

		xo++;
		if (xo >= opm->sx) {
			xo = 0;
			yo++;
		}

#if 0
		if (n%4 == 0) {
			o[0] = 100;
			if (oor)
				o[1] = 200;
		}
#endif

#if 0
		if ((n % dst->sx) == 0) printf("\n");
		printf("%d", i[0]/64);
#endif
	}

TODO("create the transformed version if not in the cache already (by headers)");

	return opm;
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
