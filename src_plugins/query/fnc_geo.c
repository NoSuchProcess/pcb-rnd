/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/* Query language - geometry functions */

#include "obj_poly.h"
#include "data_it.h"

#define PCB dontuse

static int fnc_poly_num_islands(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_poly_t *poly;
	long cnt = 0;
	pcb_poly_it_t it;
	rnd_polyarea_t *pa;

	if ((argc != 1) || (argv[0].type != PCBQ_VT_OBJ))
		return -1;

	poly = (pcb_poly_t *)argv[0].data.obj;
	if (poly->type != PCB_OBJ_POLY)
		PCB_QRY_RET_INV(res);

	if (poly->Clipped != NULL) {
		if (PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, poly))
			for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it))
				cnt++;
		else
			cnt = 1;
	}

	PCB_QRY_RET_INT(res, cnt);
}

static int isc_subc_obj(pcb_qry_exec_t *ectx, pcb_find_t *fctx, pcb_subc_t *subc, pcb_any_obj_t *obj)
{
	pcb_any_obj_t *so;
	pcb_data_it_t it;
	for(so = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); so != NULL; so = pcb_data_next(&it))
		if (pcb_intersect_obj_obj(fctx, obj, so))
			return 1;
	return 0;
}

static int isc_subc_subc(pcb_qry_exec_t *ectx, pcb_find_t *fctx, pcb_subc_t *subc1, pcb_subc_t *subc2)
{
	pcb_any_obj_t *so;
	pcb_data_it_t it;
	for(so = pcb_data_first(&it, subc1->data, PCB_OBJ_CLASS_REAL); so != NULL; so = pcb_data_next(&it))
		if (isc_subc_obj(ectx, fctx, subc2, so))
			return 1;
	return 0;
}

static int overlap_(pcb_qry_exec_t *ectx, pcb_find_t *fctx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	if ((argc < 2) || (argv[0].type != PCBQ_VT_OBJ) || (argv[1].type != PCBQ_VT_OBJ))
		return -1;
	if (argc > 2) {
		PCB_QRY_ARG_CONV_TO_COORD(fctx->bloat, &argv[2], return -1);
	}
	if (argc > 3)
		return -1;

	if ((argv[0].data.obj->type == PCB_OBJ_SUBC) && (argv[1].data.obj->type == PCB_OBJ_SUBC))
		PCB_QRY_RET_INT(res, isc_subc_subc(ectx, fctx, (pcb_subc_t *)argv[0].data.obj, (pcb_subc_t *)argv[1].data.obj));
	if (argv[0].data.obj->type == PCB_OBJ_SUBC)
		PCB_QRY_RET_INT(res, isc_subc_obj(ectx, fctx, (pcb_subc_t *)argv[0].data.obj, argv[1].data.obj));
	if (argv[1].data.obj->type == PCB_OBJ_SUBC)
		PCB_QRY_RET_INT(res, isc_subc_obj(ectx, fctx, (pcb_subc_t *)argv[1].data.obj, argv[0].data.obj));

	PCB_QRY_RET_INT(res, pcb_intersect_obj_obj(fctx, argv[0].data.obj, argv[1].data.obj));
}

static int fnc_overlap(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	static pcb_find_t fctx = {0};

	fctx.ignore_clearance = 1;
	fctx.allow_noncopper_pstk = 1;
	fctx.pstk_anylayer = 1;

	return overlap_(ectx, &fctx, argc, argv, res);
}

static int fnc_intersect(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	static pcb_find_t fctx = {0};

	fctx.ignore_clearance = 1;
	fctx.allow_noncopper_pstk = 1;

	return overlap_(ectx, &fctx, argc, argv, res);
}

