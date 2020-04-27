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
#include "conf_core.h"
#include <librnd/core/hid_inlines.h>
#include <librnd/core/hid_dad.h>
#include "change.h"
#include "undo.h"

/*** API ***/

/* Convert an attribute to coord, return 0 on success */
PCB_INLINE int pcb_extobj_unpack_coord(const pcb_subc_t *obj, rnd_coord_t *res, const char *name);
PCB_INLINE int pcb_extobj_unpack_int(const pcb_subc_t *obj, int *res, const char *name);

/* Wrap the update/re-generation of an extobject-subc in this begin/end to
   make sure bbox and undo are updated properly. The end() call always returns
   0. */
PCB_INLINE void pcb_exto_regen_begin(pcb_subc_t *subc);
PCB_INLINE int pcb_exto_regen_end(pcb_subc_t *subc);

/*** implementation ***/
PCB_INLINE int pcb_extobj_unpack_coord(const pcb_subc_t *obj, rnd_coord_t *res, const char *name)
{
	double v;
	rnd_bool succ;
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

PCB_INLINE int pcb_extobj_unpack_int(const pcb_subc_t *obj, int *res, const char *name)
{
	long l;
	char *end;
	const char *s = pcb_attribute_get(&obj->Attributes, name);
	if (s != NULL) {
		l = strtol(s, &end, 10);
		if (*end == '\0') {
			*res = l;
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

PCB_INLINE pcb_subc_t *pcb_exto_create(pcb_data_t *dst, const char *eoname, const pcb_dflgmap_t *layers, rnd_coord_t ox, rnd_coord_t oy, rnd_bool on_bottom, pcb_subc_t *copy_from)
{
	pcb_subc_t *subc = pcb_subc_alloc();
	pcb_board_t *pcb = NULL;

	if (copy_from != NULL)
		pcb_subc_copy_meta(subc, copy_from);
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
		pcb_subc_rebind(pcb, subc);
		pcb_subc_bind_globals(pcb, subc);
		if (!dst->subc_tree)
			dst->subc_tree = pcb_r_create_tree();
		pcb_r_insert_entry(dst->subc_tree, (pcb_box_t *)subc);
	}

	pcb_undo_add_obj_to_create(PCB_OBJ_SUBC, subc, subc, subc);

	return subc;
}

PCB_INLINE void pcb_exto_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	rnd_coord_t x, y, unit = PCB_SUBC_AUX_UNIT;

	if (pcb_subc_get_origin(subc, &x, &y) != 0)
		return;

	pcb_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.extobj);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, -2);
	pcb_render->draw_line(pcb_draw_out.fgGC, x, y, x, y + unit);
	pcb_render->draw_line(pcb_draw_out.fgGC, x, y, x + unit/2, y);
	pcb_render->draw_line(pcb_draw_out.fgGC, x, y + unit, x + unit/2, y + unit);
	pcb_render->draw_line(pcb_draw_out.fgGC, x, y + unit/2, x + unit/3, y + unit/2);
}

/*** dialog box build ***/

PCB_INLINE void pcb_exto_dlg_gui_chg_attr(pcb_subc_t *subc, pcb_hid_attribute_t *attr, const char *newval) /* for internal use */
{
	pcb_board_t *pcb;
	pcb_data_t *data;

	if (subc->parent_type != PCB_PARENT_DATA)
		return;
	data = subc->parent.data;
	if (data->parent_type != PCB_PARENT_BOARD)
		return;
	pcb = data->parent.board;

	pcb_uchg_attr(pcb, (pcb_any_obj_t *)subc, (char *)attr->user_data, newval);
	pcb_trace("chg: %s\n", (char *)attr->user_data);
	pcb_gui->invalidate_all(pcb_gui);
}

PCB_INLINE void pcb_exto_dlg_coord_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_subc_t *subc = caller_data;
	char tmp[128];

	pcb_snprintf(tmp, sizeof(tmp), "%$mm", attr->val.crd);
	pcb_exto_dlg_gui_chg_attr(subc, attr, tmp);
}

#define pcb_exto_dlg_coord(dlg, subc, vis_name, attr_name, help) \
do { \
	pcb_hid_dad_spin_t *spin; \
	double d; \
	rnd_coord_t currval = 0; \
	const pcb_unit_t *unit_out = NULL; \
	int wid; \
	char *sval = pcb_attribute_get(&subc->Attributes, attr_name); \
	if (sval != NULL) \
		pcb_get_value_unit(sval, NULL, 0, &d, &unit_out); \
	currval = d; \
	PCB_DAD_LABEL(dlg, vis_name); \
		if (help != NULL) PCB_DAD_HELP(dlg, help); \
	PCB_DAD_COORD(dlg, ""); \
		if (help != NULL) PCB_DAD_HELP(dlg, help); \
		PCB_DAD_CHANGE_CB(dlg, pcb_exto_dlg_coord_cb); \
		wid = PCB_DAD_CURRENT(dlg); \
		dlg[wid].user_data = (void *)attr_name; \
		PCB_DAD_DEFAULT_NUM(dlg, currval); \
		spin = dlg[wid].wdata; \
		spin->unit = unit_out; \
		spin->no_unit_chg = 1; \
		pcb_dad_spin_update_internal(spin); \
} while(0)

/*		hv.crd = currval; \*/
/*		pcb_dad_spin_set_value(spin->cmp.wend, NULL, wid, &hv); \*/


PCB_INLINE void pcb_exto_dlg_str_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_subc_t *subc = caller_data;
	pcb_exto_dlg_gui_chg_attr(subc, attr, attr->val.str);
}

#define pcb_exto_dlg_str(dlg, subc, vis_name, attr_name, help) \
do { \
	int wid; \
	const char *currval = pcb_attribute_get(&subc->Attributes, attr_name); \
	if (currval == NULL) currval = ""; \
	currval = pcb_strdup(currval); \
	PCB_DAD_LABEL(dlg, vis_name); \
		if (help != NULL) PCB_DAD_HELP(dlg, help); \
	PCB_DAD_STRING(dlg); \
		if (help != NULL) PCB_DAD_HELP(dlg, help); \
		PCB_DAD_CHANGE_CB(dlg, pcb_exto_dlg_str_cb); \
		wid = PCB_DAD_CURRENT(dlg); \
		dlg[wid].user_data = (void *)attr_name; \
		PCB_DAD_DEFAULT_PTR(dlg, currval); \
} while(0)

PCB_INLINE void pcb_exto_dlg_int_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_subc_t *subc = caller_data;
	char tmp[128];

	pcb_snprintf(tmp, sizeof(tmp), "%d", attr->val.lng);
	pcb_exto_dlg_gui_chg_attr(subc, attr, tmp);
}

#define pcb_exto_dlg_int(dlg, subc, vis_name, attr_name, help) \
do { \
	rnd_coord_t currval = 0; \
	int wid; \
	char *sval = pcb_attribute_get(&subc->Attributes, attr_name); \
	if (sval != NULL) \
		currval = strtol(sval, NULL, 10); \
	PCB_DAD_LABEL(dlg, vis_name); \
		if (help != NULL) PCB_DAD_HELP(dlg, help); \
	PCB_DAD_INTEGER(dlg, ""); \
		if (help != NULL) PCB_DAD_HELP(dlg, help); \
		PCB_DAD_CHANGE_CB(dlg, pcb_exto_dlg_int_cb); \
		wid = PCB_DAD_CURRENT(dlg); \
		dlg[wid].user_data = (void *)attr_name; \
		PCB_DAD_DEFAULT_NUM(dlg, currval); \
} while(0)
