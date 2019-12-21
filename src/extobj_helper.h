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

#include "obj_subc.h"
#include "undo.h"

/*** API ***/

/* Convert an attribute to coord, return 0 on success */
PCB_INLINE int pcb_extobj_unpack_coord(const pcb_subc_t *obj, pcb_coord_t *res, const char *name);

/* Wrap the update/re-generation of an extobject-subc in this begin/end to
   make sure bbox and undo are updated properly. The end() call always returns
   0. */
PCB_INLINE void pcb_exto_regen_begin(pcb_subc_t *subc);
PCB_INLINE int pcb_exto_regen_end(pcb_subc_t *subc);

/*** implementation ***/
PCB_INLINE int pcb_extobj_unpack_coord(const pcb_subc_t *obj, pcb_coord_t *res, const char *name)
{
	double v;
	pcb_bool succ;
	const char *s = pcb_attribute_get(&obj->Attributes, name);
	if (s != NULL) {
		v = pcb_get_value(s, NULL, NULL, &succ);
		if (succ) {
			*res = v;
			return 0;
		}
	}
	return -1;
}

PCB_INLINE void pcb_exto_regen_begin(pcb_subc_t *subc)
{
	pcb_data_t *data = subc->parent.data;
	if (data->subc_tree != NULL)
		pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);
	pcb_undo_freeze_add();
}

PCB_INLINE int pcb_exto_regen_end(pcb_subc_t *subc)
{
	pcb_data_t *data = subc->parent.data;

	pcb_undo_unfreeze_add();
	pcb_subc_bbox(subc);
	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc);

	return 0;
}

PCB_INLINE pcb_subc_t *pcb_exto_create(pcb_data_t *dst, const char *eoname, const pcb_dflgmap_t *layers, pcb_coord_t ox, pcb_coord_t oy, pcb_bool on_bottom)
{
	pcb_subc_t *subc = pcb_subc_alloc();
	pcb_board_t *pcb = NULL;

	pcb_attribute_put(&subc->Attributes, "extobj", eoname);

	for(; layers->name != NULL; layers++)
		pcb_subc_layer_create(subc, layers->name, layers->lyt, layers->comb, 0, layers->purpose);

	pcb_subc_create_aux(subc, ox, oy, 0, on_bottom);

	PCB_FLAG_SET(PCB_FLAG_LOCK, subc);
	pcb_subc_bbox(subc);
	pcb_subc_reg(dst, subc);

	if (dst->parent_type == PCB_PARENT_BOARD)
		pcb = dst->parent.board;

	if (pcb != NULL) {
		if (!dst->subc_tree)
			dst->subc_tree = pcb_r_create_tree();
		pcb_r_insert_entry(dst->subc_tree, (pcb_box_t *)subc);
	}

	return subc;
}
