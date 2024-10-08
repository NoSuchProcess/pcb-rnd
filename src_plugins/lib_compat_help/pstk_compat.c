/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2021 Tibor 'Igor2' Palinkas
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

#include <string.h>

#include "pstk_compat.h"
#include "obj_pstk_inlines.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/rotate.h>
#include <librnd/poly/polygon1_gen.h>

#include "plug_io.h"

#define sqr(o) ((double)(o)*(double)(o))

/* set up x and y multiplier for an octa poly, depending on square pin style
   (used in early versions of pcb-rnd, before custom shape padstacks) */
void rnd_poly_square_pin_factors(int style, double *xm, double *ym)
{
	int i;
	const double factor = 2.0;

	/* reset multipliers */
	for (i = 0; i < 8; i++) {
		xm[i] = 1;
		ym[i] = 1;
	}

	style--;
	if (style & 1)
		xm[0] = xm[1] = xm[6] = xm[7] = factor;
	if (style & 2)
		xm[2] = xm[3] = xm[4] = xm[5] = factor;
	if (style & 4)
		ym[4] = ym[5] = ym[6] = ym[7] = factor;
	if (style & 8)
		ym[0] = ym[1] = ym[2] = ym[3] = factor;
}

/* emulate old pcb-rnd "pin shape" feature */
static void octa_shape(pcb_pstk_poly_t *dst, rnd_coord_t x0, rnd_coord_t y0, rnd_coord_t rx, rnd_coord_t ry, int style)
{
	double xm[8], ym[8];

	pcb_pstk_shape_alloc_poly(dst, 8);

	rnd_poly_square_pin_factors(style, xm, ym);

	dst->x[7] = x0 + rnd_round(rx * 0.5) * xm[7];
	dst->y[7] = y0 + rnd_round(ry * RND_TAN_22_5_DEGREE_2) * ym[7];
	dst->x[6] = x0 + rnd_round(rx * RND_TAN_22_5_DEGREE_2) * xm[6];
	dst->y[6] = y0 + rnd_round(ry * 0.5) * ym[6];
	dst->x[5] = x0 - rnd_round(rx * RND_TAN_22_5_DEGREE_2) * xm[5];
	dst->y[5] = y0 + rnd_round(ry * 0.5) * ym[5];
	dst->x[4] = x0 - rnd_round(rx * 0.5) * xm[4];
	dst->y[4] = y0 + rnd_round(ry * RND_TAN_22_5_DEGREE_2) * ym[4];
	dst->x[3] = x0 - rnd_round(rx * 0.5) * xm[3];
	dst->y[3] = y0 - rnd_round(ry * RND_TAN_22_5_DEGREE_2) * ym[3];
	dst->x[2] = x0 - rnd_round(rx * RND_TAN_22_5_DEGREE_2) * xm[2];
	dst->y[2] = y0 - rnd_round(ry * 0.5) * ym[2];
	dst->x[1] = x0 + rnd_round(rx * RND_TAN_22_5_DEGREE_2) * xm[1];
	dst->y[1] = y0 - rnd_round(ry * 0.5) * ym[1];
	dst->x[0] = x0 + rnd_round(rx * 0.5) * xm[0];
	dst->y[0] = y0 - rnd_round(ry * RND_TAN_22_5_DEGREE_2) * ym[0];
}

/* emulate the old 'square flag' feature */
static void square_shape(pcb_pstk_poly_t *dst, rnd_coord_t x0, rnd_coord_t y0, rnd_coord_t radius, int style)
{
	rnd_coord_t r2 = rnd_round(radius * 0.5);

	pcb_pstk_shape_alloc_poly(dst, 4);

	dst->x[0] = x0 - r2; dst->y[0] = y0 - r2;
	dst->x[1] = x0 + r2; dst->y[1] = y0 - r2;
	dst->x[2] = x0 + r2; dst->y[2] = y0 + r2;
	dst->x[3] = x0 - r2; dst->y[3] = y0 + r2;
}


static int compat_via_shape_gen(pcb_pstk_shape_t *dst, pcb_pstk_compshape_t cshape, rnd_coord_t pad_dia)
{
	if ((cshape >= PCB_PSTK_COMPAT_SHAPED) && (cshape <= PCB_PSTK_COMPAT_SHAPED_END)) {
		dst->shape = PCB_PSSH_POLY;
		octa_shape(&dst->data.poly, 0, 0, pad_dia, pad_dia, cshape);
		return 0;
	}

	switch(cshape) {
		case PCB_PSTK_COMPAT_ROUND:
			dst->shape = PCB_PSSH_CIRC;
			dst->data.circ.x = dst->data.circ.y = 0;
			dst->data.circ.dia = pad_dia;
			break;
		case PCB_PSTK_COMPAT_SQUARE:
			/* using a zero-length line is not a good idea here as rotation wouldn't affect it */
			dst->shape = PCB_PSSH_POLY;
			square_shape(&dst->data.poly, 0, 0, pad_dia, 1);
			break;
		case PCB_PSTK_COMPAT_OCTAGON:
			dst->shape = PCB_PSSH_POLY;
			octa_shape(&dst->data.poly, 0, 0, pad_dia, pad_dia, 1);
			break;
		default:
			return -1;
	}
	return 0;
}

static void compat_shape_free(pcb_pstk_shape_t *shp)
{
	if (shp->shape == PCB_PSSH_POLY)
		free(shp->data.poly.x);
}

int compat_pstk_cop_grps = 0;
static rnd_cardinal_t pcb_pstk_new_compat_via_proto_(pcb_data_t *data, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t mask, pcb_pstk_compshape_t cshape, rnd_bool plated, rnd_bool hole_clearance_hack, int bb_top, int bb_bottom)
{
	pcb_pstk_proto_t proto;
	pcb_pstk_shape_t shape[5]; /* max number of shapes: 3 coppers, 2 masks */
	rnd_cardinal_t pid;
	pcb_pstk_shape_t copper_master, mask_master;
	pcb_pstk_tshape_t tshp;
	int n;

	assert(drill_dia > 0);
	assert(mask >= 0);

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));
	memset(&copper_master, 0, sizeof(copper_master));
	memset(&mask_master, 0, sizeof(mask_master));

	tshp.shape = shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;
	proto.htop = bb_top;
	proto.hbottom = bb_bottom;

	if (plated || hole_clearance_hack) {
		long num_cop_grps = 0;

		tshp.len = 3 + (mask > 0 ? 2 : 0);

		/* we need to generate the shape only once as it's the same on all */
		if (compat_via_shape_gen(&copper_master, cshape, pad_dia) != 0)
			return -1;

		for(n = 0; n < 3; n++)
			memcpy(&shape[n], &copper_master, sizeof(copper_master));
		shape[0].layer_mask = PCB_LYT_COPPER | PCB_LYT_TOP;    shape[0].comb = 0;
		shape[1].layer_mask = PCB_LYT_COPPER | PCB_LYT_BOTTOM; shape[1].comb = 0;
		shape[2].layer_mask = PCB_LYT_COPPER | PCB_LYT_INTERN; shape[2].comb = 0;
		tshp.len = 3;

		/* remove bottom and top shape if bbvia doesn't reach those layers */
		if ((bb_bottom < 0) || (bb_top < 0)) {
			pcb_board_t *pcb = data->parent.board;

			/* normalzie negative bb_top and bb_bottom so they are always positive so 0 is easier to detect */
			if (pcb != NULL) {
				if (compat_pstk_cop_grps <= 0) {
					rnd_layergrp_id_t n;
					for(n = 0; n < pcb->LayerGroups.len; n++)
						if (pcb->LayerGroups.grp[n].ltype & PCB_LYT_COPPER)
							num_cop_grps++;
				}
				else
					num_cop_grps = compat_pstk_cop_grps;
			}

			if (bb_bottom < 0)
				bb_bottom = bb_bottom + num_cop_grps - 1;
			if (bb_top < 0)
				bb_top = bb_top + num_cop_grps - 1;
		}
		if (bb_bottom != 0) {
			memcpy(&shape[1], &shape[2], sizeof(copper_master)*1);
			tshp.len--;
		}
		if ((bb_top != 0) && (bb_top != -num_cop_grps)) {
			memcpy(&shape[0], &shape[1], sizeof(copper_master)*2);
			tshp.len--;
		}
	}
	else
		tshp.len = 0;

	if (mask > 0) {
		if (compat_via_shape_gen(&mask_master, cshape, mask) != 0)
			return -1;
	
		memcpy(&shape[tshp.len+0], &mask_master, sizeof(mask_master));
		memcpy(&shape[tshp.len+1], &mask_master, sizeof(mask_master));
		shape[tshp.len+0].layer_mask = PCB_LYT_MASK | PCB_LYT_TOP;      shape[tshp.len+0].comb = PCB_LYC_SUB + PCB_LYC_AUTO;
		shape[tshp.len+1].layer_mask = PCB_LYT_MASK | PCB_LYT_BOTTOM;   shape[tshp.len+1].comb = PCB_LYC_SUB + PCB_LYC_AUTO;
		tshp.len += 2;
	}

	proto.hdia = drill_dia;
	proto.hplated = plated;

	pcb_pstk_proto_update(&proto);
	pid = pcb_pstk_proto_insert_dup(data, &proto, 1, 0);
	if (pid == PCB_PADSTACK_INVALID) {
		compat_shape_free(&copper_master);
		return -1;
	}

	compat_shape_free(&copper_master);

	if (mask > 0)
		compat_shape_free(&mask_master);

	return pid;
}

rnd_cardinal_t pcb_pstk_new_compat_via_proto(pcb_data_t *data, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t mask, pcb_pstk_compshape_t cshape, rnd_bool plated, rnd_bool hole_clearance_hack)
{
	return pcb_pstk_new_compat_via_proto_(data, drill_dia, pad_dia, mask, cshape, plated, hole_clearance_hack, 0, 0);
}

static pcb_pstk_t *pcb_pstk_new_compat_via_(pcb_data_t *data, long int id, rnd_coord_t x, rnd_coord_t y, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t clearance, rnd_coord_t mask, pcb_pstk_compshape_t cshape, rnd_bool plated, rnd_bool hole_clearance_hack, int bb_top, int bb_bottom)
{
	rnd_cardinal_t pid;

	assert(clearance >= 0);

	if (hole_clearance_hack && !plated) {
		/* in PCB hole means unplated with clearance; emulate this by placing a
		   zero diameter copper circle on all layers and set clearance large
		   enough to cover the hole too */
		clearance = rnd_round((double)clearance + (double)drill_dia/2.0);
		pad_dia = 0.0;
	}

	/* for plated vias, positive pad is required */
	if (plated && !hole_clearance_hack)
		assert(pad_dia > drill_dia);

	pid = pcb_pstk_new_compat_via_proto_(data, drill_dia, pad_dia, mask, cshape, plated, hole_clearance_hack, bb_top, bb_bottom);
	if (pid == -1)
		return NULL;
	return pcb_pstk_new(data, -1, pid, x, y, clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
}


pcb_pstk_t *pcb_pstk_new_compat_via(pcb_data_t *data, long int id, rnd_coord_t x, rnd_coord_t y, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t clearance, rnd_coord_t mask, pcb_pstk_compshape_t cshape, rnd_bool plated)
{
	return pcb_pstk_new_compat_via_(data, id, x, y, drill_dia, pad_dia, clearance, mask, cshape, plated, 0, 0, 0);
}

pcb_pstk_t *pcb_pstk_new_compat_bbvia(pcb_data_t *data, long int id, rnd_coord_t x, rnd_coord_t y, rnd_coord_t drill_dia, rnd_coord_t pad_dia, rnd_coord_t clearance, rnd_coord_t mask, pcb_pstk_compshape_t cshape, rnd_bool plated, int bb_top, int bb_bottom)
{
	return pcb_pstk_new_compat_via_(data, id, x, y, drill_dia, pad_dia, clearance, mask, cshape, plated, 0, bb_top, bb_bottom);
}

static pcb_pstk_compshape_t get_old_shape_square(rnd_coord_t *dia, const pcb_pstk_shape_t *shp)
{
	double sq, len2, l, a;
	int n;

	sq = sqr(shp->data.poly.x[0] - shp->data.poly.x[2]) + sqr(shp->data.poly.y[0] - shp->data.poly.y[2]);
	/* cross-bars must be of equal length */
	if (sq != sqr(shp->data.poly.x[1] - shp->data.poly.x[3]) + sqr(shp->data.poly.y[1] - shp->data.poly.y[3]))
		return PCB_PSTK_COMPAT_INVALID;

	/* all sides must be of equal length */
	len2 = fabs(sqr(shp->data.poly.x[3] - shp->data.poly.x[0]) + sqr(shp->data.poly.y[3] - shp->data.poly.y[0]));
	for(n = 0; n < 3; n++) {
		l = fabs(sqr(shp->data.poly.x[n+1] - shp->data.poly.x[n]) + sqr(shp->data.poly.y[n+1] - shp->data.poly.y[n]));
		if (fabs(l - len2) > 10)
			return PCB_PSTK_COMPAT_INVALID;
	}

	/* must be axis aligned */
	a = atan2(shp->data.poly.y[3] - shp->data.poly.y[0], shp->data.poly.x[3] - shp->data.poly.x[0]) * RND_RAD_TO_DEG;
	if (fmod(a, 90.0) > 0.1)
		return PCB_PSTK_COMPAT_INVALID;

	/* found a valid square */
	*dia = rnd_round(sqrt(sq / 2.0));

	return PCB_PSTK_COMPAT_SQUARE;
}


static pcb_pstk_compshape_t get_old_shape_octa(rnd_coord_t *dia, const pcb_pstk_shape_t *shp)
{
	double x[8], y[8], xm[8], ym[8], minx = 0.0, miny = 0.0, tmp;
	int shi, n, found;

	/* collect offsets */
	for(n = 0; n < 8; n++) {
		x[n] = shp->data.poly.x[n];
		y[n] = shp->data.poly.y[n];
	}

	/* compensate for the octagon shape */
	x[7] /= 0.5;
	y[7] /= RND_TAN_22_5_DEGREE_2;
	x[6] /= RND_TAN_22_5_DEGREE_2;
	y[6] /= 0.5;
	x[5] /= RND_TAN_22_5_DEGREE_2;
	y[5] /= 0.5;
	x[4] /= 0.5;
	y[4] /= RND_TAN_22_5_DEGREE_2;
	x[3] /= 0.5;
	y[3] /= RND_TAN_22_5_DEGREE_2;
	x[2] /= RND_TAN_22_5_DEGREE_2;
	y[2] /= 0.5;
	x[1] /= RND_TAN_22_5_DEGREE_2;
	y[1] /= 0.5;
	x[0] /= 0.5;
	y[0] /= RND_TAN_22_5_DEGREE_2;

	/* check min distances */
	for(n = 0; n < 8; n++) {
		tmp = fabs(x[n]);
		if ((minx == 0.0) || (tmp < minx))
			minx = tmp;
		tmp = fabs(y[n]);
		if ((miny == 0.0) || (tmp < miny))
			miny = tmp;
	}

	if ((minx == 0.0) || (miny == 0.0))
		return PCB_PSTK_COMPAT_INVALID;

	if (minx < miny)
		miny = minx;
	else
		minx = miny;

	/* normalize the factors */
	for(n = 0; n < 8; n++) {
		x[n] = fabs(x[n] / minx);
		y[n] = fabs(y[n] / miny);
		if (fabs(x[n] - 1.0) < 0.01)
			x[n] = 1.0;
		else if (fabs(x[n] - 2.0) < 0.01)
			x[n] = 2.0;
		else
			return PCB_PSTK_COMPAT_INVALID;
		if (fabs(y[n] - 1.0) < 0.01)
			y[n] = 1.0;
		else if (fabs(y[n] - 2.0) < 0.01)
			y[n] = 2.0;
		else
			return PCB_PSTK_COMPAT_INVALID;
	}

	/* compare to all known shape factors */
	for(shi = PCB_PSTK_COMPAT_SHAPED; shi <= PCB_PSTK_COMPAT_SHAPED_END; shi++) {
		rnd_poly_square_pin_factors(shi - PCB_PSTK_COMPAT_SHAPED, xm, ym);
		found = 1;
		for(n = 0; n < 8; n++) {
			if ((xm[n] != x[n]) || (ym[n] != y[n])) {
				found = 0;
				break;
			}
		}
		if (found) {
			*dia = rnd_round(sqrt((sqr(minx) + sqr(miny)) / 2.0));
			return shi;
		}
	}

	return PCB_PSTK_COMPAT_INVALID;
}

static pcb_pstk_compshape_t get_old_shape(rnd_coord_t *dia, const pcb_pstk_shape_t *shp)
{
	switch(shp->shape) {
		case PCB_PSSH_LINE:
			return PCB_PSTK_COMPAT_INVALID;
		case PCB_PSSH_CIRC:
			if ((shp->data.circ.x != 0) || (shp->data.circ.y != 0))
				return PCB_PSTK_COMPAT_INVALID;
			*dia = shp->data.circ.dia;
			return PCB_PSTK_COMPAT_ROUND;
		case PCB_PSSH_POLY:
			if (shp->data.poly.len == 4)
				return get_old_shape_square(dia, shp);
			if (shp->data.poly.len == 8)
				return get_old_shape_octa(dia, shp);
			break;
		case PCB_PSSH_HSHADOW:
			break;
	}
	return PCB_PSTK_COMPAT_INVALID;
}

rnd_bool pcb_pstk_export_compat_proto(pcb_pstk_proto_t *proto, rnd_coord_t *drill_dia, rnd_coord_t *pad_dia, rnd_coord_t *mask, pcb_pstk_compshape_t *cshape, rnd_bool *plated)
{
	pcb_pstk_tshape_t *tshp;
	int n, coppern = -1, maskn = -1;
	pcb_pstk_compshape_t old_shape[5];
	rnd_coord_t old_dia[5];

	tshp = &proto->tr.array[0];

	if ((tshp->len != 3) && (tshp->len != 5))
		return rnd_false; /* allow only 3 coppers + optionally 2 masks */

	for(n = 0; n < tshp->len; n++) {
		if (tshp->shape[n].shape != tshp->shape[0].shape)
			return rnd_false; /* all shapes must be the same */
		if ((tshp->shape[n].layer_mask & PCB_LYT_ANYWHERE) == 0)
			return rnd_false;
		if (tshp->shape[n].layer_mask & PCB_LYT_COPPER) {
			coppern = n;
			continue;
		}
		if (tshp->shape[n].layer_mask & PCB_LYT_INTERN)
			return rnd_false; /* accept only copper as intern */
		if (tshp->shape[n].layer_mask & PCB_LYT_MASK) {
			maskn = n;
			continue;
		}
		return rnd_false; /* refuse anything else */
	}

	/* we must have copper shapes */
	if (coppern < 0)
		return rnd_false;

	if (tshp->shape[0].shape == PCB_PSSH_POLY)
		for(n = 1; n < tshp->len; n++)
			if (tshp->shape[n].data.poly.len != tshp->shape[0].data.poly.len)
				return rnd_false; /* all polygons must have the same number of corners */

	for(n = 0; n < tshp->len; n++) {
		old_shape[n] = get_old_shape(&old_dia[n], &tshp->shape[n]);
		if (old_shape[n] == PCB_PSTK_COMPAT_INVALID)
			return rnd_false;
		if (old_shape[n] != old_shape[0])
			return rnd_false;
	}

	/* all copper sizes must be the same, all mask sizes must be the same */
	for(n = 0; n < tshp->len; n++) {
		if ((tshp->shape[n].layer_mask & PCB_LYT_COPPER) && (RND_ABS(old_dia[n] - old_dia[coppern]) > 10))
			return rnd_false;
		if (maskn >= 0)
			if ((tshp->shape[n].layer_mask & PCB_LYT_MASK) && (RND_ABS(old_dia[n] - old_dia[maskn]) > 10))
				return rnd_false;
	}

	/* all went fine, collect and return data */
	*drill_dia = proto->hdia;
	*pad_dia = old_dia[coppern];
	*mask = maskn >= 0 ? old_dia[maskn] : 0;
	*cshape = old_shape[0];
	*plated = proto->hplated;

	return rnd_true;
}

rnd_bool pcb_pstk_export_compat_via(pcb_pstk_t *ps, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t *drill_dia, rnd_coord_t *pad_dia, rnd_coord_t *clearance, rnd_coord_t *mask, pcb_pstk_compshape_t *cshape, rnd_bool *plated)
{
	pcb_pstk_proto_t *proto;
	rnd_bool res;

	proto = pcb_pstk_get_proto_(ps->parent.data, ps->proto);
	if ((proto == NULL) || (proto->tr.used < 1))
		return rnd_false;

	res = pcb_pstk_export_compat_proto(proto, drill_dia, pad_dia, mask, cshape, plated);
	if (res) {
		*x = ps->x;
		*y = ps->y;
		*clearance = ps->Clearance;
		/* clerance workaround for copper smaller than hole; inverse of r25735 */
		if (*pad_dia < *drill_dia)
			*clearance -= (*drill_dia - *pad_dia)/2;
	}
	return res;
}


/* emulate the old 'square flag' pad */
static void pad_shape(pcb_pstk_poly_t *dst, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness)
{
	double d, dx, dy;
	rnd_coord_t nx, ny;
	rnd_coord_t halfthick = (thickness + 1) / 2;

	dx = x2 - x1;
	dy = y2 - y1;

	d = sqrt((double)sqr(dx) + (double)sqr(dy));
	if (d != 0) {
		double v = (double)halfthick / d;
		nx = (double)(y1 - y2) * v;
		ny = (double)(x2 - x1) * v;

		x1 -= ny;
		y1 += nx;
		x2 += ny;
		y2 -= nx;
	}
	else {
		nx = halfthick;
		ny = 0;

		y1 += nx;
		y2 -= nx;
	}

	pcb_pstk_shape_alloc_poly(dst, 4);
	dst->x[0] = x1 - nx; dst->y[0] = y1 - ny;
	dst->x[1] = x1 + nx; dst->y[1] = y1 + ny;
	dst->x[2] = x2 + nx; dst->y[2] = y2 + ny;
	dst->x[3] = x2 - nx; dst->y[3] = y2 - ny;
}

/* Generate a square or round pad of a given thickness - typically mask or copper */
static void gen_pad(pcb_pstk_shape_t *dst, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness, rnd_bool square)
{
	if (square) {
		dst->shape = PCB_PSSH_POLY;
		pad_shape(&dst->data.poly, x1, y1, x2, y2, thickness);
	}
	else if (x1 == x2 && y1 == y2) {
		dst->shape = PCB_PSSH_CIRC;
		dst->data.circ.x = x1;
		dst->data.circ.y = y1;
		dst->data.circ.dia = thickness;
	}
	else {
		dst->shape = PCB_PSSH_LINE;
		dst->data.line.x1 = x1;
		dst->data.line.y1 = y1;
		dst->data.line.x2 = x2;
		dst->data.line.y2 = y2;
		dst->data.line.thickness = thickness;
		dst->data.line.square = 0;
	}
}

pcb_pstk_t *pcb_pstk_new_compat_pad(pcb_data_t *data, long int id, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness, rnd_coord_t clearance, rnd_coord_t mask, rnd_bool square, rnd_bool nopaste, rnd_bool onotherside)
{
	pcb_layer_type_t side;
	pcb_pstk_proto_t proto;
	pcb_pstk_shape_t shape[3]; /* max number of shapes: 1 copper, 1 mask, 1 paste */
	rnd_cardinal_t pid;
	pcb_pstk_tshape_t tshp;
	int n;
	rnd_coord_t cx, cy;

	assert(thickness >= 0);
	assert(clearance >= 0);
	assert(mask >= 0);

	cx = (x1 + x2) / 2;
	cy = (y1 + y2) / 2;

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));
	memset(&shape, 0, sizeof(shape));

	tshp.len = nopaste ? 2 : 3;
	tshp.shape = shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;

	gen_pad(&shape[0], x1 - cx, y1 - cy, x2 - cx, y2 - cy, thickness, square); /* copper */
	gen_pad(&shape[1], x1 - cx, y1 - cy, x2 - cx, y2 - cy, mask, square);      /* mask */
	if (!nopaste)
		pcb_pstk_shape_copy(&shape[2], &shape[0]); /* paste is the same */

	side = onotherside ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
	shape[0].layer_mask = side | PCB_LYT_COPPER; shape[0].comb = 0;
	shape[1].layer_mask = side | PCB_LYT_MASK;   shape[1].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
	if (!nopaste) {
		shape[2].layer_mask = side | PCB_LYT_PASTE;
		shape[2].comb = PCB_LYC_AUTO;
	}

	pcb_pstk_proto_update(&proto);
	pid = pcb_pstk_proto_insert_dup(data, &proto, 1, 0);

	for(n = 0; n < tshp.len; n++)
		compat_shape_free(&shape[n]);

	if (pid == PCB_PADSTACK_INVALID)
		return NULL;

	return pcb_pstk_new(data, id, pid, cx, cy, clearance/2, pcb_flag_make(PCB_FLAG_CLEARLINE));
}

rnd_bool pcb_pstk_shape2rect(const pcb_pstk_shape_t *shape, double *w_out, double *h_out, double *cx_out, double *cy_out, double *rot_out, double *vx01_out, double *vy01_out, double *vx03_out, double *vy03_out)
{
	double x0, y0, x1, y1, x2, y2, x3, y3, w, h, cx, cy;

	if (shape->shape != PCB_PSSH_POLY)
		return rnd_false;

	if (shape->data.poly.len != 4)
		return rnd_false;

	x0 = shape->data.poly.x[0]; y0 = shape->data.poly.y[0];
	x1 = shape->data.poly.x[1]; y1 = shape->data.poly.y[1];
	x2 = shape->data.poly.x[2]; y2 = shape->data.poly.y[2];
	x3 = shape->data.poly.x[3]; y3 = shape->data.poly.y[3];
	w = rnd_distance(x0, y0, x1, y1);
	h = rnd_distance(x0, y0, x3, y3);

	if ((cx_out != NULL) || (cy_out != NULL) || (rot_out != NULL)) {
		cx = (x0+x1+x2+x3)/4.0;
		cy = (y0+y1+y2+y3)/4.0;
	}

	if (cx_out != NULL) *cx_out = cx;
	if (cy_out != NULL) *cy_out = cy;
	if (w_out != NULL) *w_out = w;
	if (h_out != NULL) *h_out = h;
	if (vx01_out != NULL) *vx01_out = (x1 - x0) / w;
	if (vy01_out != NULL) *vy01_out = (y1 - y0) / w;
	if (vx03_out != NULL) *vx03_out = (x3 - x0) / h;
	if (vy03_out != NULL) *vy03_out = (y3 - y0) / h;

	if (rot_out != NULL) {
		double mx = (x0 + x3) / 2;
		double my = (y0 + y3) / 2;
		*rot_out = atan2(my - cy, mx - cx);
	}

	return rnd_true;
}


rnd_bool pcb_pstk_export_compat_pad(pcb_pstk_t *ps, rnd_coord_t *x1, rnd_coord_t *y1, rnd_coord_t *x2, rnd_coord_t *y2, rnd_coord_t *thickness, rnd_coord_t *clearance, rnd_coord_t *mask, rnd_bool *square, rnd_bool *nopaste)
{
	pcb_pstk_proto_t *proto;
	pcb_pstk_tshape_t *tshp;
	int n, coppern = -1, maskn = -1, pasten = -1;
	pcb_layer_type_t side;
	rnd_coord_t lx1[3], ly1[3], lx2[3], ly2[3], lt[3]; /* poly->line conversion cache */
	double cs, sn, ang;

	proto = pcb_pstk_get_proto_(ps->parent.data, ps->proto);
	if ((proto == NULL) || (proto->tr.used < 1))
		return rnd_false;

	tshp = &proto->tr.array[0];

	if ((tshp->len < 2) || (tshp->len > 3))
		return rnd_false; /* allow at most a copper, a mask and a paste */

	/* determine whether we are on top or bottom */
	side = tshp->shape[0].layer_mask & (PCB_LYT_TOP | PCB_LYT_BOTTOM);
	if ((side == 0) || (side == (PCB_LYT_TOP | PCB_LYT_BOTTOM)))
		return rnd_false;

	for(n = 0; n < tshp->len; n++) {
		if (tshp->shape[n].shape != tshp->shape[0].shape)
			return rnd_false; /* all shapes must be the same */
		if ((tshp->shape[n].layer_mask & PCB_LYT_ANYWHERE) != side)
			return rnd_false; /* all shapes must be the same side */
		if (tshp->shape[n].layer_mask & PCB_LYT_COPPER) {
			coppern = n;
			continue;
		}
		if (tshp->shape[n].layer_mask & PCB_LYT_INTERN)
			return rnd_false; /* accept only copper as intern */
		if (tshp->shape[n].layer_mask & PCB_LYT_MASK) {
			maskn = n;
			continue;
		}
		if (tshp->shape[n].layer_mask & PCB_LYT_PASTE) {
			pasten = n;
			continue;
		}
		return rnd_false; /* refuse anything else */
	}

	/* require copper and mask shapes */
	if ((coppern < 0) || (maskn < 0))
		return rnd_false;

	ang = ps->rot;
	if (ps->xmirror)
		ang = -ang;
	cs = cos(ang / RND_RAD_TO_DEG);
	sn = sin(ang / RND_RAD_TO_DEG);

	/* if the shape is poly, convert to line to make the rest of the code simpler */
	if (tshp->shape[0].shape == PCB_PSSH_POLY) {
		for(n = 0; n < tshp->len; n++) {
			double w, h, cx, cy, vx01, vy01, vx03, vy03;

			if (tshp->shape[n].shape != PCB_PSSH_POLY)
				continue;

			if (!pcb_pstk_shape2rect(&tshp->shape[n], &w, &h, &cx, &cy, NULL, &vx01, &vy01, &vx03, &vy03))
				return rnd_false;

			if (w <= h) {
				lt[n] = rnd_round(w);
				lx1[n] = rnd_round(cx + h/2.0 * vx03 - w/2.0 * vx03);
				ly1[n] = rnd_round(cy + h/2.0 * vy03 - w/2.0 * vy03);
				lx2[n] = rnd_round(cx - h/2.0 * vx03 + w/2.0 * vx03);
				ly2[n] = rnd_round(cy - h/2.0 * vy03 + w/2.0 * vy03);
			}
			else {
				lt[n] = rnd_round(h);
				lx1[n] = rnd_round(cx + w/2.0 * vx01 - h/2.0 * vx01);
				ly1[n] = rnd_round(cy + w/2.0 * vy01 - h/2.0 * vy01);
				lx2[n] = rnd_round(cx - w/2.0 * vx01 + h/2.0 * vx01);
				ly2[n] = rnd_round(cy - w/2.0 * vy01 + h/2.0 * vy01);
			}

			rnd_rotate(&lx1[n], &ly1[n], cx, cy, cs, sn);
			rnd_rotate(&lx2[n], &ly2[n], cx, cy, cs, sn);
		}
	}

	/* require all shapes to be concentric */
	for(n = 1; n < tshp->len; n++) {
		switch(tshp->shape[0].shape) {
			case PCB_PSSH_HSHADOW:
				return rnd_false;
			case PCB_PSSH_LINE:
				if (tshp->shape[0].data.line.x1 != tshp->shape[n].data.line.x1)
					return rnd_false;
				if (tshp->shape[0].data.line.y1 != tshp->shape[n].data.line.y1)
					return rnd_false;
				if (tshp->shape[0].data.line.x2 != tshp->shape[n].data.line.x2)
					return rnd_false;
				if (tshp->shape[0].data.line.y2 != tshp->shape[n].data.line.y2)
					return rnd_false;
				break;
			case PCB_PSSH_CIRC:
				if (tshp->shape[0].data.circ.x != tshp->shape[n].data.circ.x)
					return rnd_false;
				if (tshp->shape[0].data.circ.y != tshp->shape[n].data.circ.y)
					return rnd_false;
				break;
			case PCB_PSSH_POLY:
				if (lx1[0] != lx1[n])
					return rnd_false;
				if (ly1[0] != ly1[n])
					return rnd_false;
				if (lx2[0] != lx2[n])
					return rnd_false;
				if (ly2[0] != ly2[n])
					return rnd_false;
				break;
		}
	}

	/* generate the return pad (line-like) */
	switch(tshp->shape[0].shape) {
		case PCB_PSSH_LINE:
			*x1 = tshp->shape[coppern].data.line.x1 + ps->x;
			*y1 = tshp->shape[coppern].data.line.y1 + ps->y;
			*x2 = tshp->shape[coppern].data.line.x2 + ps->x;
			*y2 = tshp->shape[coppern].data.line.y2 + ps->y;
			*thickness = tshp->shape[coppern].data.line.thickness;
			*mask = tshp->shape[maskn].data.line.thickness;
			*square = tshp->shape[coppern].data.line.square;
			break;
		case PCB_PSSH_CIRC:
			*x1 = *x2 = tshp->shape[coppern].data.circ.x + ps->x;
			*y1 = *y2 = tshp->shape[coppern].data.circ.y + ps->y;
			*thickness = tshp->shape[coppern].data.circ.dia;
			*mask = tshp->shape[maskn].data.circ.dia;
			*square = 0;
			break;
		case PCB_PSSH_POLY:
			*square = 1;
			*x1 = lx1[0] + ps->x;
			*y1 = ly1[0] + ps->y;
			*x2 = lx2[0] + ps->x;
			*y2 = ly2[0] + ps->y;
			*thickness = lt[coppern];
			*mask = lt[maskn];
			break;
		case PCB_PSSH_HSHADOW:
			break;
	}

	*clearance = (ps->Clearance > 0 ? ps->Clearance * 2 : tshp->shape[0].clearance);
	*nopaste = pasten < 0;

	return rnd_true;
}

int pcb_pstk_compat_pinvia_is_hole(pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	pcb_pstk_tshape_t *tshp;
	long n;

	/* a hole requires an unplated drilled hole */
	if (proto->hplated || (proto->hdia <= 0))
		return 0;

	/* not a hole if it has any copper shape that's not a circle or is larger than 0 */
	tshp = &proto->tr.array[0];
	for(n = 0; n < tshp->len; n++) {
		if (tshp->shape[n].layer_mask & PCB_LYT_COPPER) {
			if (tshp->shape[n].shape != PCB_PSSH_CIRC)
				return 0;
			if (tshp->shape[n].data.circ.dia > 0)
				return 0;
		}
	}

	return 1;
}

pcb_flag_t pcb_pstk_compat_pinvia_flag(pcb_pstk_t *ps, pcb_pstk_compshape_t cshape, pcb_pstk_compat_t compat)
{
	pcb_flag_t flg;
	int n;

	memset(&flg, 0, sizeof(flg));
	flg.f = ps->Flags.f & PCB_PSTK_VIA_COMPAT_FLAGS;

	if (pcb_pstk_compat_pinvia_is_hole(ps))
	flg.f |= PCB_FLAG_HOLE;

	switch(cshape) {
		case PCB_PSTK_COMPAT_ROUND:
			break;
		case PCB_PSTK_COMPAT_OCTAGON:
			flg.f |= PCB_FLAG_OCTAGON;
			break;
		case PCB_PSTK_COMPAT_SQUARE:
			flg.f |= PCB_FLAG_SQUARE;
			flg.q = 1;
			break;
		default:
			if ((cshape >= PCB_PSTK_COMPAT_SHAPED) && (cshape <= PCB_PSTK_COMPAT_SHAPED_END)) {
				cshape -= PCB_PSTK_COMPAT_SHAPED;
				if (cshape == 1)
					cshape = 17;
				if ((cshape == 17) && (compat & PCB_PSTKCOMP_OLD_OCTAGON)) {
					cshape = 0;
					flg.f |= PCB_FLAG_OCTAGON;
				}
				else {
					flg.q = cshape;
					flg.f |= PCB_FLAG_SQUARE;
				}
			}
			else
				pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padstack-shape", "Failed to convert shape to old-style pin/via", "Old pin/via format is very much restricted; try to use a simpler shape (e.g. circle)");
	}

	for(n = 0; n < sizeof(flg.t) / sizeof(flg.t[0]); n++) {
		unsigned char *ot = pcb_pstk_get_thermal(ps, n, 0);
		int nt;
		if ((ot == NULL) || (*ot == 0) || !((*ot) & PCB_THERMAL_ON))
			continue;
		switch(((*ot) & ~PCB_THERMAL_ON)) {
			case PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL: nt = 1; break;
			case PCB_THERMAL_SHARP: nt = 2; break;
			case PCB_THERMAL_SOLID: nt = 3; break;
			case PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL: nt = 4; break;
			case PCB_THERMAL_ROUND: nt = 5; break;
			default: nt = 0; pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padstack-shape", "Failed to convert thermal to old-style via", "Old via format is very much restricted; try to use a simpler thermal shape");
		}
		PCB_FLAG_THERM_ASSIGN_(n, nt, flg);
	}


	if (compat & PCB_PSTKCOMP_PCB_CLEARLINE_WORKAROUND) {
		if (flg.f & PCB_FLAG_CLEARLINE) {
			/* in geda/pcb CLEARLINE overlaps with show-pin-name (and CLEARLINE is implied) */
			flg.f &= ~PCB_FLAG_CLEARLINE;
		}
		else {
			/* if there's no CLEALRINE in pcb-rnd that means we have a solid conn thermal on all layers */
			for(n = 0; n < sizeof(flg.t) / sizeof(flg.t[0]); n++)
				PCB_FLAG_THERM_ASSIGN_(n, 3, flg);
		}
	}

	return flg;
}


pcb_pstk_t *pcb_old_via_new_bbvia(pcb_data_t *data, long int id, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Thickness, rnd_coord_t Clearance, rnd_coord_t Mask, rnd_coord_t DrillingHole, const char *Name, pcb_flag_t Flags, int htop, int hbottom)
{
	pcb_pstk_t *p;
	pcb_pstk_compshape_t shp;
	int n;

	if (Flags.f & PCB_FLAG_SQUARE) {
		shp = Flags.q /*+ PCB_PSTK_COMPAT_SHAPED*/;
		if (shp == 0)
			shp = PCB_PSTK_COMPAT_SQUARE;
	}
	else if (Flags.f & PCB_FLAG_OCTAGON)
		shp = PCB_PSTK_COMPAT_OCTAGON;
	else
		shp = PCB_PSTK_COMPAT_ROUND;

	p = pcb_pstk_new_compat_via_(data, id, X, Y, DrillingHole, Thickness, Clearance/2, Mask, shp, !(Flags.f & PCB_FLAG_HOLE), 1, htop, hbottom);
	p->Flags.f |= Flags.f & PCB_PSTK_VIA_COMPAT_FLAGS;
	for(n = 0; n < sizeof(Flags.t) / sizeof(Flags.t[0]); n++) {
		int nt = PCB_THERMAL_ON, t = ((Flags.t[n/2] >> (4 * (n % 2))) & 0xf);
		if (t != 0) {
			switch(t) {
				case 1: nt |= PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL; break;
				case 2: nt |= PCB_THERMAL_SHARP; break;
				case 3: nt |= PCB_THERMAL_SOLID; break;
				case 4: nt |= PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL; break;
				case 5: nt |= PCB_THERMAL_ROUND; break;
			}
			pcb_pstk_set_thermal(p, n, nt, 0);
		}
	}

	if (Name != NULL)
		pcb_attribute_put(&p->Attributes, "name", Name);

	return p;
}

pcb_pstk_t *pcb_old_via_new(pcb_data_t *data, long int id, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Thickness, rnd_coord_t Clearance, rnd_coord_t Mask, rnd_coord_t DrillingHole, const char *Name, pcb_flag_t Flags)
{
	return pcb_old_via_new_bbvia(data, id, X, Y, Thickness, Clearance, Mask, DrillingHole, Name, Flags, 0, 0);
}


void pcb_shape_octagon(pcb_pstk_shape_t *dst, rnd_coord_t radiusx, rnd_coord_t radiusy)
{
	dst->shape = PCB_PSSH_POLY;
	octa_shape(&dst->data.poly, 0, 0, radiusx, radiusy, 0);
}
