/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

#include <genvector/vtp0.h>
#include <genht/htsi.h>
#include <genht/hash.h>

#include "data.h"
#include "data_it.h"
#include "data_list.h"
#include "remove.h"
#include "undo.h"

#include "extobj.h"


static htsi_t pcb_extobj_n2i;  /* extobj name -> index */
vtp0_t pcb_extobj_i2o;         /* extobj_idx -> (pcb_ext_obj_t *) */
int pcb_extobj_invalid;        /* this changes upon each new extobj reg, forcing the caches to be invalidated eventually */

void pcb_extobj_unreg(pcb_extobj_t *o)
{
	if (!o->registered)
		return;
	htsi_pop(&pcb_extobj_n2i, o->name);
	vtp0_set(&pcb_extobj_i2o, o->idx, NULL);
	o->registered = 0;
	o->idx = 0;
}

void pcb_extobj_reg(pcb_extobj_t *o)
{
	o->idx = pcb_extobj_i2o.used;
	vtp0_append(&pcb_extobj_i2o, o);
	o->registered = 0;
	htsi_set(&pcb_extobj_n2i, (char *)o->name, o->idx);
	pcb_extobj_invalid = -o->idx;
}

pcb_extobj_t *pcb_extobj_lookup(const char *name)
{
	int idx = htsi_get(&pcb_extobj_n2i, name);
	if ((idx <= 0) || (idx >= pcb_extobj_i2o.used))
		return NULL;
	return pcb_extobj_i2o.array[idx];
}

int pcb_extobj_lookup_idx(const char *name)
{
	return htsi_get(&pcb_extobj_n2i, (char *)name);
}

void pcb_extobj_init(void)
{
	vtp0_init(&pcb_extobj_i2o);
	htsi_init(&pcb_extobj_n2i, strhash, strkeyeq);

	vtp0_append(&pcb_extobj_i2o, NULL); /* idx=0 is reserved for no extended obj */
}


void pcb_extobj_uninit(void)
{
	size_t n;

	for(n = 0; n < pcb_extobj_i2o.used; n++) {
		pcb_extobj_t *o = pcb_extobj_i2o.array[n];
		if (o != NULL)
			pcb_extobj_unreg(o);
	}
	htsi_uninit(&pcb_extobj_n2i);
	vtp0_uninit(&pcb_extobj_i2o);
}

PCB_INLINE pcb_data_t *pcb_extobj_parent_data(pcb_any_obj_t *obj)
{
	if (obj->parent_type == PCB_PARENT_DATA)
		return obj->parent.data;
	if (obj->parent_type == PCB_PARENT_LAYER)
		return obj->parent.layer->parent.data;
	return NULL;
}

void pcb_extobj_float_new_spawn(pcb_extobj_t *eo, pcb_subc_t *subc_copy_from, pcb_any_obj_t *edit_obj)
{
	pcb_data_t *data = pcb_extobj_parent_data(edit_obj);
	pcb_board_t *pcb;
	const char *save;

	pcb = pcb_data_get_top(data);
	if (pcb == NULL)
		return;

	save = subc_copy_from->extobj;
	subc_copy_from->extobj = NULL;
	PCB_FLAG_CLEAR(PCB_FLAG_FLOATER, edit_obj);
	pcb_extobj_conv_obj_using(pcb, eo, pcb->Data, edit_obj, 1, subc_copy_from);
	subc_copy_from->extobj = save;
}


void pcb_extobj_del_pre(pcb_subc_t *sc)
{
	pcb_extobj_t *eo = pcb_extobj_get(sc);

	if ((eo != NULL) && (eo->del_pre != NULL))
		eo->del_pre(sc);
}

static pcb_subc_t *pcb_extobj_conv_list(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, vtp0_t *list, rnd_bool remove, pcb_subc_t *copy_from)
{
	pcb_subc_t *res;

	if ((eo->conv_objs == NULL) || (list->used == 0))
		return NULL;

	res = eo->conv_objs(dst, list, copy_from);

	if ((res != NULL) && remove) {
		long n;
		for(n = 0; n < list->used; n++) {
			pcb_any_obj_t *o = list->array[n];
			pcb_remove_object(o->type, o->parent.any, o, o);
		}
	}

	return res;
}

static pcb_subc_t *pcb_extobj_conv_flag_objs_using(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_data_t *src, rnd_bool remove, pcb_flag_values_t flg, pcb_subc_t *copy_from)
{
	vtp0_t list;
	pcb_subc_t *res;

	vtp0_init(&list);
	pcb_data_list_by_flag(src, &list, PCB_OBJ_ANY, flg);
	res = pcb_extobj_conv_list(pcb, eo, dst, &list, remove, copy_from);
	vtp0_uninit(&list);

	return res;
}

pcb_subc_t *pcb_extobj_conv_selected_objs(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_data_t *src, rnd_bool remove)
{
	return pcb_extobj_conv_flag_objs_using(pcb, eo, dst, src, remove, PCB_FLAG_SELECTED, NULL);
}

pcb_subc_t *pcb_extobj_conv_all_objs(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_data_t *src, rnd_bool remove)
{
	return pcb_extobj_conv_flag_objs_using(pcb, eo, dst, src, remove, 0, NULL);
}


pcb_subc_t *pcb_extobj_conv_obj_using(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_any_obj_t *src, rnd_bool remove, pcb_subc_t *copy_from)
{
	vtp0_t list;
	pcb_subc_t *res;

	vtp0_init(&list);
	vtp0_append(&list, src);
	res = pcb_extobj_conv_list(pcb, eo, dst, &list, remove, copy_from);
	vtp0_uninit(&list);

	return res;
}


pcb_subc_t *pcb_extobj_conv_obj(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_any_obj_t *src, rnd_bool remove)
{
	return pcb_extobj_conv_obj_using(pcb, eo, dst, src, remove, NULL);
}

rnd_cardinal_t pcb_extobj_sync_floater_flags(pcb_board_t *pcb, const pcb_any_obj_t *flt, int undoable, int draw)
{
	pcb_subc_t *subc;
	pcb_extobj_t *eo;
	pcb_data_it_t it;
	pcb_any_obj_t *o;
	int sel;
	rnd_cardinal_t cnt = 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, flt))
		return 0;

	subc = pcb_obj_parent_subc(flt);
	if (subc == NULL)
		return 0;

	eo = pcb_extobj_get(subc);
	if (eo == NULL)
		return 0;

	sel = PCB_FLAG_TEST(PCB_FLAG_SELECTED, flt);

	for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		/* sync selection */
		if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, o))
			continue;
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o) != sel) {
			if (undoable)
				pcb_undo_add_obj_to_flag(o);
			PCB_FLAG_ASSIGN(PCB_FLAG_SELECTED, sel, o);
			if (draw)
				pcb_draw_obj(o);
			cnt++;
		}
	}
	return cnt;
}
