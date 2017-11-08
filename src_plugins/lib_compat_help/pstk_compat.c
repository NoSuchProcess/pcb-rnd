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
#include "compat_misc.h"

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
	dst->x[1] = y0 - pcb_round(radius * 0.5) * ym[1];
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
	pcb_pstk_shape_t master;
	pcb_pstk_tshape_t tshp;
	int n;

	memset(&proto, 0, sizeof(proto));
	memset(&tshp, 0, sizeof(tshp));
	memset(&master, 0, sizeof(master));

	tshp.len = 3 + (mask > 0 ? 2 : 0);
	tshp.shape = shape;
	proto.tr.alloced = proto.tr.used = 1; /* has the canonical form only */
	proto.tr.array = &tshp;

/* we need to generate the shape only once as it's the same on all */
	if (compat_via_shape_gen(&master, cshape, pad_dia) != 0)
		return NULL;
	for(n = 0; n < tshp.len; n++)
		memcpy(&shape[n], &master, sizeof(master));

	shape[0].layer_mask = PCB_LYT_COPPER | PCB_LYT_TOP;    shape[0].comb = 0;
	shape[1].layer_mask = PCB_LYT_COPPER | PCB_LYT_BOTTOM; shape[1].comb = 0;
	shape[2].layer_mask = PCB_LYT_COPPER | PCB_LYT_INTERN; shape[2].comb = 0;
	shape[3].layer_mask = PCB_LYT_MASK | PCB_LYT_TOP;      shape[3].comb = PCB_LYC_SUB;
	shape[4].layer_mask = PCB_LYT_MASK | PCB_LYT_BOTTOM;   shape[4].comb = PCB_LYC_SUB;

	proto.hdia = drill_dia;
	proto.hplated = plated;

	pid = pcb_pstk_proto_insert_dup(data, &proto, 1);
	if (pid == PCB_PADSTACK_INVALID) {
		compat_shape_free(&master);
		return NULL;
	}
	compat_shape_free(&master);

	return pcb_pstk_new(data, pid, x, y, clearance, pcb_flag_make(0));
}


pcb_bool pcb_pstk_export_compat_via(pcb_pstk_t *ps, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t *drill_dia, pcb_coord_t *pad_dia, pcb_coord_t *clearance, pcb_coord_t *mask, pcb_pstk_compshape_t *cshape, pcb_bool *plated)
{
	return pcb_false;
}

