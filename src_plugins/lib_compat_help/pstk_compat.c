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
#include "compat_misc.h"

#include "plug_io.h"

#define sqr(o) ((double)(o)*(double)(o))

/* set up x and y multiplier for an octa poly, depending on square pin style */
void pcb_poly_square_pin_factors(int style, double *xm, double *ym)
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

static pcb_pstk_t *pcb_pstk_new_compat_via_(pcb_data_t *data, long int id, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_coord_t pad_dia, pcb_coord_t clearance, pcb_coord_t mask, pcb_pstk_compshape_t cshape, pcb_bool plated, pcb_bool hole_clearance_hack)
{
	pcb_pstk_proto_t proto;
	pcb_pstk_shape_t shape[5]; /* max number of shapes: 3 coppers, 2 masks */
	pcb_cardinal_t pid;
	pcb_pstk_shape_t copper_master, mask_master;
	pcb_pstk_tshape_t tshp;
	int n;

	if (hole_clearance_hack) {
		/* in PCB hole means unplated with clearance; emulate this by placing a
		   zero diameter copper circle on all layers and set clearance large
		   enough to cover the hole too */
		clearance = pcb_round((double)clearance + (double)drill_dia/2.0);
		pad_dia = 0.0;
	}

	/* for plated vias, positive pad is required */
	if (plated && !hole_clearance_hack)
		assert(pad_dia > drill_dia);

	assert(drill_dia > 0);
	assert(clearance >= 0);
	assert(mask >= 0);

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));
	memset(&copper_master, 0, sizeof(copper_master));
	memset(&mask_master, 0, sizeof(mask_master));

	tshp.shape = shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;

	if (plated || hole_clearance_hack) {
		tshp.len = 3 + (mask > 0 ? 2 : 0);

		/* we need to generate the shape only once as it's the same on all */
		if (compat_via_shape_gen(&copper_master, cshape, pad_dia) != 0)
			return NULL;

		for(n = 0; n < 3; n++)
			memcpy(&shape[n], &copper_master, sizeof(copper_master));
		shape[0].layer_mask = PCB_LYT_COPPER | PCB_LYT_TOP;    shape[0].comb = 0;
		shape[1].layer_mask = PCB_LYT_COPPER | PCB_LYT_BOTTOM; shape[1].comb = 0;
		shape[2].layer_mask = PCB_LYT_COPPER | PCB_LYT_INTERN; shape[2].comb = 0;
		tshp.len = 3;
	}
	else
		tshp.len = 0;

	if (mask > 0) {
		if (compat_via_shape_gen(&mask_master, cshape, mask) != 0)
			return NULL;
	
		memcpy(&shape[tshp.len+0], &mask_master, sizeof(mask_master));
		memcpy(&shape[tshp.len+1], &mask_master, sizeof(mask_master));
		shape[tshp.len+0].layer_mask = PCB_LYT_MASK | PCB_LYT_TOP;      shape[tshp.len+0].comb = PCB_LYC_SUB + PCB_LYC_AUTO;
		shape[tshp.len+1].layer_mask = PCB_LYT_MASK | PCB_LYT_BOTTOM;   shape[tshp.len+1].comb = PCB_LYC_SUB + PCB_LYC_AUTO;
		tshp.len += 2;
	}

	proto.hdia = drill_dia;
	proto.hplated = plated;

	pcb_pstk_proto_update(&proto);
	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);
	if (pid == PCB_PADSTACK_INVALID) {
		compat_shape_free(&copper_master);
		return NULL;
	}

	compat_shape_free(&copper_master);

	if (mask > 0)
		compat_shape_free(&mask_master);

	return pcb_pstk_new(data, -1, pid, x, y, clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
}

pcb_pstk_t *pcb_pstk_new_compat_via(pcb_data_t *data, long int id, pcb_coord_t x, pcb_coord_t y, pcb_coord_t drill_dia, pcb_coord_t pad_dia, pcb_coord_t clearance, pcb_coord_t mask, pcb_pstk_compshape_t cshape, pcb_bool plated)
{
	return pcb_pstk_new_compat_via_(data, id, x, y, drill_dia, pad_dia, clearance, mask, cshape, plated, 0);
}


static pcb_pstk_compshape_t get_old_shape_square(pcb_coord_t *dia, const pcb_pstk_shape_t *shp)
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
	a = atan2(shp->data.poly.y[3] - shp->data.poly.y[0], shp->data.poly.x[3] - shp->data.poly.x[0]) * PCB_RAD_TO_DEG;
	if (fmod(a, 90.0) > 0.1)
		return PCB_PSTK_COMPAT_INVALID;

	/* found a valid square */
	*dia = pcb_round(sqrt(sq / 2.0));

	return PCB_PSTK_COMPAT_SQUARE;
}


static pcb_pstk_compshape_t get_old_shape_octa(pcb_coord_t *dia, const pcb_pstk_shape_t *shp)
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
		pcb_poly_square_pin_factors(shi - PCB_PSTK_COMPAT_SHAPED, xm, ym);
		found = 1;
		for(n = 0; n < 8; n++) {
			if ((xm[n] != x[n]) || (ym[n] != y[n])) {
				found = 0;
				break;
			}
		}
		if (found) {
			*dia = pcb_round(sqrt((sqr(minx) + sqr(miny)) / 2.0));
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
		case PCB_PSSH_HSHADOW:
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
	*mask = maskn >= 0 ? old_dia[maskn] : 0;
	*cshape = old_shape[0];
	*plated = proto->hplated;

	/* clerance workaround for copper smaller than hole; inverse of r25735 */
	if (*pad_dia < *drill_dia)
		*clearance -= (*drill_dia - *pad_dia)/2;

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

pcb_pstk_t *pcb_pstk_new_compat_pad(pcb_data_t *data, long int id, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t thickness, pcb_coord_t clearance, pcb_coord_t mask, pcb_bool square, pcb_bool nopaste, pcb_bool onotherside)
{
	pcb_layer_type_t side;
	pcb_pstk_proto_t proto;
	pcb_pstk_shape_t shape[3]; /* max number of shapes: 1 copper, 1 mask, 1 paste */
	pcb_cardinal_t pid;
	pcb_pstk_tshape_t tshp;
	int n;
	pcb_coord_t cx, cy;

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
	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);

	for(n = 0; n < tshp.len; n++)
		compat_shape_free(&shape[n]);

	if (pid == PCB_PADSTACK_INVALID)
		return NULL;

	return pcb_pstk_new(data, id, pid, cx, cy, clearance/2, pcb_flag_make(PCB_FLAG_CLEARLINE));
}


pcb_bool pcb_pstk_export_compat_pad(pcb_pstk_t *ps, pcb_coord_t *x1, pcb_coord_t *y1, pcb_coord_t *x2, pcb_coord_t *y2, pcb_coord_t *thickness, pcb_coord_t *clearance, pcb_coord_t *mask, pcb_bool *square, pcb_bool *nopaste)
{
	pcb_pstk_proto_t *proto;
	pcb_pstk_tshape_t *tshp;
	int n, coppern = -1, maskn = -1, pasten = -1;
	pcb_layer_type_t side;
	pcb_coord_t lx1[3], ly1[3], lx2[3], ly2[3], lt[3]; /* poly->line conversion cache */

	proto = pcb_pstk_get_proto_(ps->parent.data, ps->proto);
	if ((proto == NULL) || (proto->tr.used < 1))
		return pcb_false;

	tshp = &proto->tr.array[0];

	if ((tshp->len < 2) || (tshp->len > 3))
		return pcb_false; /* allow at most a copper, a mask and a paste */

	/* determine whether we are on top or bottom */
	side = tshp->shape[0].layer_mask & (PCB_LYT_TOP | PCB_LYT_BOTTOM);
	if ((side == 0) || (side == (PCB_LYT_TOP | PCB_LYT_BOTTOM)))
		return pcb_false;

	for(n = 0; n < tshp->len; n++) {
		if (tshp->shape[n].shape != tshp->shape[0].shape)
			return pcb_false; /* all shapes must be the same */
		if ((tshp->shape[n].layer_mask & PCB_LYT_ANYWHERE) != side)
			return pcb_false; /* all shapes must be the same side */
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
		if (tshp->shape[n].layer_mask & PCB_LYT_PASTE) {
			pasten = n;
			continue;
		}
		return pcb_false; /* refuse anything else */
	}

	/* require copper and mask shapes */
	if ((coppern < 0) || (maskn < 0))
		return pcb_false;

	/* if the shape is poly, convert to line to make the rest of the code simpler */
	if (tshp->shape[0].shape == PCB_PSSH_POLY) {
		for(n = 0; n < tshp->len; n++) {
			pcb_coord_t w, h, x1, x2, y1, y2;
			double stepd, step2;
			int step;

			if (tshp->shape[n].shape != PCB_PSSH_POLY)
				continue;

			if (tshp->shape[n].data.poly.len != 4)
				return pcb_false;

			stepd = ps->rot / 90.0;
			step = stepd;
			step2 = (double)step * 90.0;
			if (fabs(ps->rot - step2) > 0.01)
				return pcb_false;

			x1 = tshp->shape[n].data.poly.x[0];
			x2 = tshp->shape[n].data.poly.x[2];
			if (x1 < x2) {
				x2 = tshp->shape[n].data.poly.x[0];
				x1 = tshp->shape[n].data.poly.x[2];
			}
			y1 = tshp->shape[n].data.poly.y[0];
			y2 = tshp->shape[n].data.poly.y[2];
			if (y1 < y2) {
				y2 = tshp->shape[n].data.poly.y[0];
				y1 = tshp->shape[n].data.poly.y[2];
			}
			if ((step % 2) == 1) {
				h = x1 - x2;
				w = y1 - y2;
			}
			else {
				w = x1 - x2;
				h = y1 - y2;
			}
			lt[n] = (w < h) ? w : h;
			lx1[n] = x2 + lt[n] / 2;
			ly1[n] = y2 + lt[n] / 2;
			lx2[n] = lx1[n] + (w - lt[n]);
			ly2[n] = ly1[n] + (h - lt[n]);
		}
	}

	/* require all shapes to be concentric */
	for(n = 1; n < tshp->len; n++) {
		switch(tshp->shape[0].shape) {
			case PCB_PSSH_HSHADOW:
				return pcb_false;
			case PCB_PSSH_LINE:
				if (tshp->shape[0].data.line.x1 != tshp->shape[n].data.line.x1)
					return pcb_false;
				if (tshp->shape[0].data.line.y1 != tshp->shape[n].data.line.y1)
					return pcb_false;
				if (tshp->shape[0].data.line.x2 != tshp->shape[n].data.line.x2)
					return pcb_false;
				if (tshp->shape[0].data.line.y2 != tshp->shape[n].data.line.y2)
					return pcb_false;
				break;
			case PCB_PSSH_CIRC:
				if (tshp->shape[0].data.circ.x != tshp->shape[n].data.circ.x)
					return pcb_false;
				if (tshp->shape[0].data.circ.y != tshp->shape[n].data.circ.y)
					return pcb_false;
				break;
			case PCB_PSSH_POLY:
				if (lx1[0] != lx1[n])
					return pcb_false;
				if (ly1[0] != ly1[n])
					return pcb_false;
				if (lx2[0] != lx2[n])
					return pcb_false;
				if (ly2[0] != ly2[n])
					return pcb_false;
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

	*clearance = (ps->Clearance > 0 ? ps->Clearance : tshp->shape[0].clearance) * 2;
	*nopaste = pasten < 0;

	return pcb_true;
}

pcb_flag_t pcb_pstk_compat_pinvia_flag(pcb_pstk_t *ps, pcb_pstk_compshape_t cshape)
{
	pcb_flag_t flg;
	int n;

	memset(&flg, 0, sizeof(flg));
	flg.f = ps->Flags.f & PCB_PSTK_VIA_COMPAT_FLAGS;
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
				flg.f |= PCB_FLAG_SQUARE;
				cshape -= PCB_PSTK_COMPAT_SHAPED;
				if (cshape == 1)
					cshape = 17;
				flg.q = cshape;
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
	return flg;
}


pcb_pstk_t *pcb_old_via_new(pcb_data_t *data, long int id, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, pcb_flag_t Flags)
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

	p = pcb_pstk_new_compat_via_(data, id, X, Y, DrillingHole, Thickness, Clearance/2, Mask, shp, !(Flags.f & PCB_FLAG_HOLE), 1);
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
			pcb_pstk_set_thermal(p, n, nt);
		}
	}

	if (Name != NULL)
		pcb_attribute_put(&p->Attributes, "name", Name);

	return p;
}
