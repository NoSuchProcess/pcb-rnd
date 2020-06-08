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

/* Query language - special object accessor functions */

#include "thermal.h"

#define PCB dontuse

static int fnc_thermal_on(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj, *lo;
	pcb_layer_t *ly = NULL;
	unsigned char *t;

	if ((argc != 2) || (argv[0].type != PCBQ_VT_OBJ) || (argv[1].type != PCBQ_VT_OBJ))
		return -1;

	obj = (pcb_any_obj_t *)argv[0].data.obj;
	lo = (pcb_any_obj_t *)argv[1].data.obj;

	if (lo->type == PCB_OBJ_LAYER)
		ly = pcb_layer_get_real((pcb_layer_t *)lo);
	else if (lo->type & PCB_OBJ_CLASS_LAYER)
		ly = pcb_layer_get_real(lo->parent.layer);
	else
		return -1;

	if (ly == NULL)
		return -1;

	if (obj->type != PCB_OBJ_PSTK) {
		if (pcb_layer_get_real(obj->parent.layer) != ly)
			PCB_QRY_RET_STR(res, "");
		t = &obj->thermal;
	}
	else
		t = pcb_obj_common_get_thermal(obj, pcb_layer_id(ectx->pcb->Data, ly), 0);

	if (t == NULL)
		PCB_QRY_RET_STR(res, "");

	pcb_thermal_bits2chars(res->data.local_str.tmp, t[0]);

	PCB_QRY_RET_STR(res, res->data.local_str.tmp);
}

