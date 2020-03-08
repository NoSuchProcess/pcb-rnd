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

typedef enum pcb_extobj_del_e {
	PCB_EXTODEL_NOOP,      /* do not do anything (floater is not removed either) */
	PCB_EXTODEL_FLOATER,   /* remove the floater only */
	PCB_EXTODEL_SUBC       /* remove the whole subcircuit */
} pcb_extobj_del_t;

typedef enum pcb_extobj_new_e {
	PCB_EXTONEW_FLOATER,   /* new floater created normally */
	PCB_EXTONEW_SPAWN      /* spawn a new subc the new floater will be in */
} pcb_extobj_new_t;

struct pcb_extobj_s {
	/* static data - filled in by the extobj code */
	const char *name;
	void (*draw_mark)(pcb_draw_info_t *info, pcb_subc_t *obj); /* called when drawing the subc marks (instead of drawing the dashed outline and diamond origin) */
	void (*float_pre)(pcb_subc_t *subc, pcb_any_obj_t *floater); /* called before an extobj floater is edited in any way - must not free() the floater */
	void (*float_geo)(pcb_subc_t *subc, pcb_any_obj_t *floater); /* called after the geometry of an extobj floater is changed - must not free() the floater; floater may be NULL (post-floater-deletion update on the parent subc) */
	pcb_extobj_new_t (*float_new)(pcb_subc_t *subc, pcb_any_obj_t *floater); /* called when a floater object is split so a new floater is created; defaults to PCB_EXTONEW_SPAWN if NULL */
	pcb_extobj_del_t (*float_del)(pcb_subc_t *subc, pcb_any_obj_t *floater); /* called when a floater object is to be removed; returns what the core should do; if not specified: remove the subc */
	void (*chg_attr)(pcb_subc_t *subc, const char *key, const char *value); /* called after an attribute changed; value == NULL means attribute is deleted */
	void (*del_pre)(pcb_subc_t *subc); /* called before the extobj subcircuit is deleted - should free any internal cache, but shouldn't delete the subcircuit */
	pcb_subc_t *(*conv_objs)(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from); /* called to convert objects into an extobj subc; returns NULL on error; objects should not be changed; if copy_from is not NULL, the new extobj is being created by copying copy_from (the implementation should pick up info from there, not from PCB) */
	void (*gui_propedit)(pcb_subc_t *subc); /* invoke implementation-defined GUI for editing extended object properties */

	/* dynamic data - filled in by core */
	int idx;
	unsigned registered:1;
};

void pcb_extobj_init(void);
void pcb_extobj_uninit(void);

void pcb_extobj_unreg(pcb_extobj_t *o);
void pcb_extobj_reg(pcb_extobj_t *o);

pcb_extobj_t *pcb_extobj_lookup(const char *name);

/* Called to remove the subc of an edit object floater; returns 0 on no
   action, 1 if it removed an extobj subc */
PCB_INLINE int pcb_extobj_del_floater(pcb_any_obj_t *edit_obj);


/* called (by the subc code) before an edit-obj is removed */
void pcb_extobj_del_pre(pcb_subc_t *edit_obj);

/* Creates and returns a new extended object on dst, using (selected|all)
   object(s) of src. If the operation was successful and remove is true,
   remove selected objects. The "using" variant carries an existing extobj
   of the same type that is used as a source of metadata */
pcb_subc_t *pcb_extobj_conv_selected_objs(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_data_t *src, pcb_bool remove);
pcb_subc_t *pcb_extobj_conv_all_objs(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_data_t *src, pcb_bool remove);
pcb_subc_t *pcb_extobj_conv_obj(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_any_obj_t *src, pcb_bool remove);
pcb_subc_t *pcb_extobj_conv_obj_using(pcb_board_t *pcb, const pcb_extobj_t *eo, pcb_data_t *dst, pcb_any_obj_t *src, pcb_bool remove, pcb_subc_t *copy_from);

/* for internal use: when an extobj needs to be cloned becase a new floater
   is added */
void pcb_extobj_float_new_spawn(pcb_extobj_t *eo, pcb_subc_t *subc, pcb_any_obj_t *flt);


/* Call this after selection changes on a floater - this makes sure all floaters
   are selected or unselected at once; returns the number of objects changed */
pcb_cardinal_t pcb_extobj_sync_floater_flags(pcb_board_t *pcb, const pcb_any_obj_t *flt, int undoable, int draw);


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

PCB_INLINE pcb_subc_t *pcb_extobj_get_floater_subc(pcb_any_obj_t *flt)
{
	pcb_subc_t *subc;
	pcb_extobj_t *eo;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, flt))
		return NULL;

	subc = pcb_obj_parent_subc(flt);
	if (subc == NULL)
		return NULL;

	eo = pcb_extobj_get(subc);
	if (eo == NULL)
		return NULL;

	return subc;
}

PCB_INLINE int pcb_extobj_del_floater(pcb_any_obj_t *flt)
{
	pcb_subc_t *subc;
	pcb_extobj_t *eo;
	pcb_extobj_del_t act = PCB_EXTODEL_SUBC;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, flt))
		return 0;

	subc = pcb_obj_parent_subc(flt);
	if (subc == NULL)
		return 0;

	eo = pcb_extobj_get(subc);
	if (eo == NULL)
		return 0; /* do not delete non-extobjs */

	if (eo->float_del != NULL)
		act = eo->float_del(subc, flt);

	switch(act) {
		case PCB_EXTODEL_NOOP:
			return 1;
		case PCB_EXTODEL_FLOATER:
			return 0;
		case PCB_EXTODEL_SUBC:
			pcb_subc_remove(subc);
			return 1;
	}
	return 0;
}

PCB_INLINE void pcb_extobj_float_new(pcb_any_obj_t *flt)
{
	pcb_subc_t *subc = pcb_obj_parent_subc(flt);
	pcb_extobj_t *eo;
	pcb_extobj_new_t act = PCB_EXTONEW_SPAWN;

	if (subc == NULL)
		return;

	eo = pcb_extobj_get(subc);

	if ((eo != NULL) && (eo->float_new != NULL))
		act = eo->float_new(subc, flt);

	switch(act) {
		case PCB_EXTONEW_FLOATER:
			/* no action required, the new floater got created already and got registered in the hook above */
			break;
		case PCB_EXTONEW_SPAWN:
			pcb_extobj_float_new_spawn(eo, subc, flt);
			break;
	}
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

PCB_INLINE void pcb_extobj_subc_geo(pcb_subc_t *subc)
{
	pcb_extobj_t *eo;

	eo = pcb_extobj_get(subc);

	if ((eo != NULL) && (eo->float_geo != NULL))
		eo->float_geo(subc, NULL);
}

#endif
