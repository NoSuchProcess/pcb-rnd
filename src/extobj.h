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

#ifndef PCB_EXT_OBJ_H
#define PCB_EXT_OBJ_H

#include <genvector/vtp0.h>

#include "board.h"
#include "data.h"
#include "obj_common.h"
#include "obj_subc.h"
#include "obj_subc_parent.h"
#include "draw.h"


typedef struct pcb_extobj_s pcb_extobj_t;

struct pcb_extobj_s {
	/* static data - filled in by the extobj code */
	const char *name;
	void (*draw_mark)(pcb_draw_info_t *info, pcb_subc_t *obj); /* called when drawing the subc marks (instead of drawing the dashed outline and diamond origin) */
	void (*float_pre)(pcb_subc_t *subc, pcb_any_obj_t *floater); /* called before an extobj floater is edited in any way - must not free() the floater */
	void (*float_geo)(pcb_subc_t *subc, pcb_any_obj_t *floater); /* called after the geometry of an extobj floater is changed - must not free() the floater */
	void (*float_new)(pcb_subc_t *subc, pcb_any_obj_t *floater); /* called when a floater object is split so a new floater is created */
	void (*chg_attr)(pcb_subc_t *subc, const char *key, const char *value); /* called after an attribute changed; value == NULL means attribute is deleted */
	void (*del_pre)(pcb_subc_t *subc); /* called before the extobj subcircuit is deleted - should free any internal cache, but shouldn't delete the subcircuit */
	pcb_subc_t *(*conv_objs)(pcb_data_t *dst, vtp0_t *objs); /* called to convert objects into an extobj subc; returns NULL on error; objects should not be changed */

	/* dynamic data - filled in by core */
	int idx;
	unsigned registered:1;
};

void pcb_extobj_init(void);
void pcb_extobj_uninit(void);

void pcb_extobj_unreg(pcb_extobj_t *o);
void pcb_extobj_reg(pcb_extobj_t *o);

pcb_extobj_t *pcb_extobj_lookup(const char *name);

/* given an edit-object, resolve the corresponding extobj's subc, or return
   NULL if there's none */
pcb_subc_t *pcb_extobj_get_subcobj_by_attr(pcb_any_obj_t *editobj);

/* Create a new subc for for new the edit_obj; if subc_copy_from is not NULL,
   copy all data from there (including objects). */
void pcb_extobj_new_subc(pcb_any_obj_t *edit_obj, pcb_subc_t *subc_copy_from);

/* Called to remove the subc of an edit object floater; returns 0 on no
   action, 1 if it removed an extobj subc */
PCB_INLINE int pcb_extobj_del_floater(pcb_any_obj_t *edit_obj);


/* called (by the subc code) before an edit-obj is removed */
void pcb_extobj_del_pre(pcb_subc_t *edit_obj);

/* Creates and returns a new extended object on dst, using (selected|all)
   object(s) of src. If the operation was successful and remove is true,
   remove selected objects. */
pcb_subc_t *pcb_extobj_conv_selected_objs(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_data_t *src, pcb_bool remove);
pcb_subc_t *pcb_extobj_conv_all_objs(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_data_t *src, pcb_bool remove);
pcb_subc_t *pcb_extobj_conv_obj(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_any_obj_t *src, pcb_bool remove);


int pcb_extobj_lookup_idx(const char *name);

extern int pcb_extobj_invalid; /* this changes upon each new extobj reg, forcing the caches to be invalidated eventually */
extern vtp0_t pcb_extobj_i2o;  /* extobj_idx -> (pcb_ext_obj_t *) */

PCB_INLINE pcb_extobj_t *pcb_extobj_get(pcb_subc_t *obj)
{
	pcb_extobj_t **eo;

	if ((obj == NULL) || (obj->extobj == NULL) || (obj->extobj_idx == pcb_extobj_invalid))
		return NULL; /* known negative */

	if (obj->extobj_idx <= 0) { /* invalid idx cache - look up by name */
		obj->extobj_idx = pcb_extobj_lookup_idx(obj->extobj);
		if (obj->extobj_idx == 0) { /* no such name */
			obj->extobj_idx = pcb_extobj_invalid; /* make the next lookup cheaper */
			return NULL;
		}
	}

	eo = (pcb_extobj_t **)vtp0_get(&pcb_extobj_i2o, obj->extobj_idx, 0);
	if ((eo == NULL) || (*eo == NULL)) /* extobj backend got deregistered meanwhile */
		obj->extobj_idx = pcb_extobj_invalid; /* make the next lookup cheaper */
	return *eo;
}

PCB_INLINE void pcb_extobj_chg_attr(pcb_any_obj_t *obj, const char *key, const char *value)
{
	pcb_subc_t *subc = (pcb_subc_t *)obj;
	pcb_extobj_t *eo;

	if ((obj->type != PCB_OBJ_SUBC) || (PCB_FLAG_TEST(PCB_FLAG_FLOATER, obj)))
		return;

	eo = pcb_extobj_get(subc);
	if ((eo != NULL) && (eo->chg_attr != NULL))
		eo->chg_attr(subc, key, value);
}

PCB_INLINE int pcb_extobj_del_floater(pcb_any_obj_t *flt)
{
	pcb_subc_t *subc;
	pcb_extobj_t *eo;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, flt))
		return 0;

	subc = pcb_obj_parent_subc(flt);
	if (subc == NULL)
		return 0;

	eo = pcb_extobj_get(subc);
	if (eo == NULL)
		return 0; /* do not delete non-extobjs */

	pcb_subc_remove(subc);
	return 1;
}

PCB_INLINE void pcb_extobj_float_new(pcb_any_obj_t *flt)
{
	pcb_subc_t *subc = pcb_obj_parent_subc(flt);
	pcb_extobj_t *eo;

	if (subc == NULL)
		return;

	eo = pcb_extobj_get(subc);

	if ((eo != NULL) && (eo->float_new != NULL))
		eo->float_new(subc, flt);
}

PCB_INLINE pcb_any_obj_t *pcb_extobj_float_pre(pcb_any_obj_t *flt)
{
	pcb_subc_t *subc;
	pcb_extobj_t *eo;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, flt))
		return NULL;

	subc = pcb_obj_parent_subc(flt);
	if (subc == NULL)
		return NULL;

	eo = pcb_extobj_get(subc);

	if ((eo != NULL) && (eo->float_pre != NULL)) {
		eo->float_pre(subc, flt);
		return flt;
	}

	return NULL;
}

PCB_INLINE void pcb_extobj_float_geo(pcb_any_obj_t *flt)
{
	pcb_subc_t *subc;
	pcb_extobj_t *eo;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, flt))
		return;

	subc = pcb_obj_parent_subc(flt);
	if (subc == NULL)
		return;

	eo = pcb_extobj_get(subc);

	if ((eo != NULL) && (eo->float_geo != NULL))
		eo->float_geo(subc, flt);
}

#endif
