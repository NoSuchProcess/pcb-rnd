/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <string.h>

#include "pstk_compat.h"
#include "obj_pstk_inlines.h"
#include "compat_misc.h"

#define sqr(o) ((o)*(o))

/* emulate old pcb-rnd "pin shape" feature */
static void octa_shape(pcb_pstk_poly_t *dst, pcb_coord_t x0, pcb_coord_t y0, pcb_coord_t radius, int style)
{
	double xm[8], ym[8];

	pcb_pstk_shape_alloc_poly(dst, 8);

	pcb_poly_square_pin_factors(style, xm, ym);

	dst->x[7] = x0 + pcb_round(radius * 0.5) * xm[7];
	dst->y[7] = y0 + pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * ym[7];
	dst->x[6] = x0 + pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * xm[6];
	dst->y[6] = y0 + pcb_round(radius * 0.5) * ym[6];
	dst->x[5] = x0 - pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * xm[5];
	dst->y[5] = y0 + pcb_round(radius * 0.5) * ym[5];
	dst->x[4] = x0 - pcb_round(radius * 0.5) * xm[4];
	dst->y[4] = y0 + pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * ym[4];
	dst->x[3] = x0 - pcb_round(radius * 0.5) * xm[3];
	dst->y[3] = y0 - pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * ym[3];
	dst->x[2] = x0 - pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * xm[2];
	dst->y[2] = y0 - pcb_round(radius * 0.5) * ym[2];
	dst->x[1] = x0 + pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * xm[1];
	dst->y[1] = y0 - pcb_round(radius * 0.5) * ym[1];
	dst->x[0] = x0 + pcb_round(radius * 0.5) * xm[0];
	dst->y[0] = y0 - pcb_round(radius * PCB_TAN_22_5_DEGREE_2) * ym[0];
}

/* emulate the old 'square flag' feature */
static void square_shape(pcb_pstk_poly_t *dst, pcb_coord_t x0, pcb_coord_t y0, pcb_coord_t radius, int style)
{
	pcb_coord_t r2 = pcb_round(radius * 0.5);

	pcb_pstk_shape_alloc_poly(dst, 4);

	dst->x[0] = x0 - r2; dst->y[0] = y0 - r2;
	dst->x[1] = x0 + r2; dst->y[1] = y0 - r2;
	dst->x[2] = x0 + r2; dst->y[2] = y0 + r2;
	dst->x[3] = x0 - r2; dst->y[3] = y0 + r2;
}


static int compat_via_shape_gen(pcb_pstk_shape_t *dst, pcb_pstk_compshape_t cshape, pcb_coord_t pad_dia)
{
	if ((cshape >= PCB_PSTK_COMPAT_SHAPED) && (cshape <= PCB_PSTK_COMPAT_SHAPED_END)) {
		dst->shape = PCB_PSSH_POLY;
		octa_shape(&dst->data.poly, 0, 0, pad_dia, cshape);
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
			octa_shape(&dst->data.poly, 0, 0, pad_dia, 1);
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

pcb_pstk_t *pcb_pstk_new_compat_via(pcb_data_t *data, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_coord_t pad_dia, pcb_coord_t clearance, pcb_coord_t mask, pcb_pstk_compshape_t cshape, pcb_bool plated)
{
	pcb_pstk_proto_t proto;
	pcb_pstk_shape_t shape[5]; /* max number of shapes: 3 coppers, 2 masks */
	pcb_cardinal_t pid;
	pcb_pstk_shape_t copper_master, mask_master;
	pcb_pstk_tshape_t tshp;
	int n;

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));
	memset(&copper_master, 0, sizeof(copper_master));
	memset(&mask_master, 0, sizeof(mask_master));

	tshp.len = 3 + (mask > 0 ? 2 : 0);
	tshp.shape = shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;

/* we need to generate the shape only once as it's the same on all */
	if (compat_via_shape_gen(&copper_master, cshape, pad_dia) != 0)
		return NULL;

	for(n = 0; n < 3; n++)
		memcpy(&shape[n], &copper_master, sizeof(copper_master));
	shape[0].layer_mask = PCB_LYT_COPPER | PCB_LYT_TOP;    shape[0].comb = 0;
	shape[1].layer_mask = PCB_LYT_COPPER | PCB_LYT_BOTTOM; shape[1].comb = 0;
	shape[2].layer_mask = PCB_LYT_COPPER | PCB_LYT_INTERN; shape[2].comb = 0;

	if (mask > 0) {
		if (compat_via_shape_gen(&mask_master, cshape, mask) != 0)
			return NULL;
		
		memcpy(&shape[3], &mask_master, sizeof(mask_master));
		memcpy(&shape[4], &mask_master, sizeof(mask_master));
		shape[3].layer_mask = PCB_LYT_MASK | PCB_LYT_TOP;      shape[3].comb = PCB_LYC_SUB;
		shape[4].layer_mask = PCB_LYT_MASK | PCB_LYT_BOTTOM;   shape[4].comb = PCB_LYC_SUB;
	}

	proto.hdia = drill_dia;
	proto.hplated = plated;

	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);
	if (pid == PCB_PADSTACK_INVALID) {
		compat_shape_free(&copper_master);
		return NULL;
	}

	compat_shape_free(&copper_master);

	if (mask > 0)
		compat_shape_free(&mask_master);

	return pcb_pstk_new(data, pid, x, y, clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
}

static pcb_pstk_compshape_t get_old_shape_square(pcb_coord_t *dia, const pcb_pstk_shape_t *shp)
{
	double len2, l, a;
	int n;

	/* cross-bars must be of equal length */
	if (sqr(shp->data.poly.x[0] - shp->data.poly.x[2]) + sqr(shp->data.poly.y[0] - shp->data.poly.y[2]) != sqr(shp->data.poly.x[1] - shp->data.poly.x[3]) + sqr(shp->data.poly.y[1] - shp->data.poly.y[3]))
		return PCB_PSTK_COMPAT_INVALID;

	/* all sides must be of equal length */
	len2 = fabs(sqr(shp->data.poly.x[3] - shp->data.poly.x[0]) + sqr(shp->data.poly.y[3] - shp->data.poly.y[0]));
	for(n = 0; n < 3; n++) {
		l = fabs(sqr(shp->data.poly.x[n+1] - shp->data.poly.x[n]) + sqr(shp->data.poly.y[n+1] - shp->data.poly.y[n]));
		if (fabs(l - len2) > 10)
			return PCB_PSTK_COMPAT_INVALID;
	}

	/* must be axis aligned */
	a = atan2(shp->data.poly.y[3] - shp->data.poly.y[0], shp->data.poly.x[3] - shp->data.poly.x[0]) * PCB_RAD_TO_DEG;
	if (fmod(a, 90.0) > 0.1)
		return PCB_PSTK_COMPAT_INVALID;

	/* found a valid square */
	*dia = sqrt(len2);
	return PCB_PSTK_COMPAT_SQUARE;
}


static pcb_pstk_compshape_t get_old_shape_octa(pcb_coord_t *dia, const pcb_pstk_shape_t *shp)
{
	double x[8], y[8], xm[8], ym[8], minx = 0.0, miny = 0.0, tmp;
	int shi, n, shift, found;

	/* collect offsets */
	for(n = 0; n < 8; n++) {
		x[n] = shp->data.poly.x[n];
		y[n] = shp->data.poly.y[n];
	}

	/* compensate for the octagon shape */
	x[7] /= 0.5;
	y[7] /= PCB_TAN_22_5_DEGREE_2;
	x[6] /= PCB_TAN_22_5_DEGREE_2;
	y[6] /= 0.5;
	x[5] /= PCB_TAN_22_5_DEGREE_2;
	y[5] /= 0.5;
	x[4] /= 0.5;
	y[4] /= PCB_TAN_22_5_DEGREE_2;
	x[3] /= 0.5;
	y[3] /= PCB_TAN_22_5_DEGREE_2;
	x[2] /= PCB_TAN_22_5_DEGREE_2;
	y[2] /= 0.5;
	x[1] /= PCB_TAN_22_5_DEGREE_2;
	y[1] /= 0.5;
	x[0] /= 0.5;
	y[0] /= PCB_TAN_22_5_DEGREE_2;

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
		pcb_poly_square_pin_factors(shi, xm, ym);
		found = 1;
		for(n = 0; n < 8; n++) {
			if ((xm[n] != x[n]) || (ym[n] != y[n])) {
				found = 0;
				break;
			}
		}
		if (found) {
			*dia = sqrt(sqr(minx) + sqr(miny)) / sqrt(2);
			return shi;
		}
	}

	return PCB_PSTK_COMPAT_INVALID;
}

static pcb_pstk_compshape_t get_old_shape(pcb_coord_t *dia, const pcb_pstk_shape_t *shp)
{
	switch(shp->shape) {
		case PCB_PSSH_LINE:
			return PCB_PSTK_COMPAT_INVALID;
		case PCB_PSSH_CIRC:
			if ((shp->data.circ.x != 0) || (shp->data.circ.x != 0))
				return PCB_PSTK_COMPAT_INVALID;
			*dia = shp->data.circ.dia;
			return PCB_PSTK_COMPAT_ROUND;
		case PCB_PSSH_POLY:
			if (shp->data.poly.len == 4)
				return get_old_shape_square(dia, shp);
			if (shp->data.poly.len == 8)
				return get_old_shape_octa(dia, shp);
			break;
	}
	return PCB_PSTK_COMPAT_INVALID;
}

pcb_bool pcb_pstk_export_compat_via(pcb_pstk_t *ps, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t *drill_dia, pcb_coord_t *pad_dia, pcb_coord_t *clearance, pcb_coord_t *mask, pcb_pstk_compshape_t *cshape, pcb_bool *plated)
{
	pcb_pstk_proto_t *proto;
	pcb_pstk_tshape_t *tshp;
	int n, coppern = -1, maskn = -1;
	pcb_pstk_compshape_t old_shape[5];
	pcb_coord_t old_dia[5];

	proto = pcb_pstk_get_proto_(ps->parent.data, ps->proto);
	if ((proto == NULL) || (proto->tr.used < 1))
		return pcb_false;

	tshp = &proto->tr.array[0];

	if ((tshp->len != 3) && (tshp->len != 5))
		return pcb_false; /* allow only 3 coppers + optionally 2 masks */

	for(n = 0; n < tshp->len; n++) {
		if (tshp->shape[n].shape != tshp->shape[0].shape)
			return pcb_false; /* all shapes must be the same */
		if ((tshp->shape[n].layer_mask & PCB_LYT_ANYWHERE) == 0)
			return pcb_false;
		if (tshp->shape[n].layer_mask & PCB_LYT_COPPER) {
			coppern = n;
			continue;
		}
		if (tshp->shape[n].layer_mask & PCB_LYT_INTERN)
			return pcb_false; /* accept only copper as intern */
		if (tshp->shape[n].layer_mask & PCB_LYT_MASK) {
			maskn = n;
			continue;
		}
		return pcb_false; /* refuse anything else */
	}

	/* we must have copper shapes */
	if (coppern < 0)
		return pcb_false;

	if (tshp->shape[0].shape == PCB_PSSH_POLY)
		for(n = 1; n < tshp->len; n++)
			if (tshp->shape[n].data.poly.len != tshp->shape[0].data.poly.len)
				return pcb_false; /* all polygons must have the same number of corners */

	for(n = 0; n < tshp->len; n++) {
		old_shape[n] = get_old_shape(&old_dia[n], &tshp->shape[n]);
		if (old_shape[n] == PCB_PSTK_COMPAT_INVALID)
			return pcb_false;
		if (old_shape[n] != old_shape[0])
			return pcb_false;
	}

	/* all copper sizes must be the same, all mask sizes must be the same */
	for(n = 0; n < tshp->len; n++) {
		if ((tshp->shape[n].layer_mask & PCB_LYT_COPPER) && (fabs(old_dia[n] - old_dia[coppern]) > 10))
			return pcb_false;
		if (maskn >= 0)
			if ((tshp->shape[n].layer_mask & PCB_LYT_MASK) && (fabs(old_dia[n] - old_dia[maskn]) > 10))
				return pcb_false;
	}

	/* all went fine, collect and return data */
	*x = ps->x;
	*y = ps->y;
	*drill_dia = proto->hdia;
	*pad_dia = old_dia[coppern];
	*clearance = ps->Clearance;
	*mask = old_dia[maskn];
	*cshape = old_shape[0];
	*plated = proto->hplated;

	return pcb_true;
}

/* emulate the old 'square flag' pad */
static void pad_shape(pcb_pstk_poly_t *dst, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thickness)
{
	double d, dx, dy;
	pcb_coord_t nx, ny;
	pcb_coord_t halfthick = (thickness + 1) / 2;

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
static void gen_pad(pcb_pstk_shape_t *dst, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thickness, pcb_bool square)
{
	if (square) {
		dst->shape = PCB_PSSH_POLY;
		pad_shape(&dst->data.poly, x1, y1, x2, y2, thickness);
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

pcb_pstk_t *pcb_pstk_new_compat_pad(pcb_data_t *data, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thickness, pcb_coord_t clearance, pcb_coord_t mask, pcb_bool square)
{
	pcb_pstk_proto_t proto;
	pcb_pstk_shape_t shape[3]; /* max number of shapes: 1 copper, 1 mask, 1 paste */
	pcb_cardinal_t pid;
	pcb_pstk_tshape_t tshp;
	int n;
	pcb_coord_t cx, cy;

	cx = (x1 + x2) / 2;
	cy = (y1 + y2) / 2;

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));
	memset(&shape, 0, sizeof(shape));

	tshp.len = 3;
	tshp.shape = shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;

	gen_pad(&shape[0], x1 - cx, y1 - cy, x2 - cx, y2 - cy, thickness, square); /* copper */
	gen_pad(&shape[1], x1 - cx, y1 - cy, x2 - cx, y2 - cy, mask, square);      /* mask */
	pcb_pstk_shape_copy(&shape[2], &shape[0]);                                 /* paste is the same */

	shape[0].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; shape[0].comb = 0;
	shape[1].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK;   shape[1].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
	shape[2].layer_mask = PCB_LYT_TOP | PCB_LYT_PASTE;  shape[2].comb = PCB_LYC_AUTO;

	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);

	for(n = 0; n < 3; n++)
		compat_shape_free(&shape[n]);

	if (pid == PCB_PADSTACK_INVALID)
		return NULL;

	return pcb_pstk_new(data, pid, cx, cy, clearance/2, pcb_flag_make(PCB_FLAG_CLEARLINE));
}

