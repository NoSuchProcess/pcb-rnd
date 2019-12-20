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

#include <genvector/vtp0.h>
#include <genht/htsi.h>
#include <genht/hash.h>

#include "data.h"
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

void pcb_extobj_new_subc(pcb_any_obj_t *edit_obj, pcb_subc_t *subc_copy_from)
{
	pcb_data_t *data = pcb_extobj_parent_data(edit_obj);
	pcb_board_t *pcb;
	pcb_subc_t *sc;
	char tmp[128];

	pcb = pcb_data_get_top(data);
	if (pcb == NULL)
		return;

	if (subc_copy_from == NULL) {
		sc = pcb_subc_new();
		sc->ID = pcb_create_ID_get();
		pcb_subc_reg(pcb->Data, sc);
		pcb_obj_id_reg(pcb->Data, sc);
	}
	else
		sc = pcb_subc_dup_at(pcb, pcb->Data, subc_copy_from, 0, 0, pcb_false);

	pcb_undo_add_obj_to_create(PCB_OBJ_SUBC, sc, sc, sc);

TODO("put edit_obj within the new subc, set subc attributes");

	pcb_extobj_float_pre(edit_obj);
	pcb_extobj_float_geo(edit_obj);
}

void pcb_extobj_del_pre(pcb_subc_t *sc)
{
	pcb_extobj_t *eo = pcb_extobj_get(sc);
	pcb_any_obj_t *edit_obj = NULL;

	if ((eo != NULL) && (eo->del_pre != NULL))
		eo->del_pre(sc);
}
