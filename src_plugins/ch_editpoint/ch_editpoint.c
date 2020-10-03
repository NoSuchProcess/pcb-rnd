/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  crosshair: display edit points of the objects that are under the cursor
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/* Draw the editpoints of objects which are under the crosshair */

#include "config.h"

#include <stdio.h>

#include <genvector/vtp0.h>
#include <librnd/core/plugins.h>
#include <librnd/core/tool.h>
#include <librnd/poly/rtree.h>
#include <librnd/poly/rtree2_compat.h>
#include <librnd/core/hid_menu.h>
#include <librnd/core/hidlib_conf.h>
#include "board.h"
#include "conf_core.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "event.h"
#include "obj_pstk.h"
#include "polygon.h"
#include "search.h"
#include "tool_logic.h"
#include "../src_plugins/ch_editpoint/ch_editpoint_conf.h"

#include "../src_plugins/ch_editpoint/conf_internal.c"
#include "../src_plugins/ch_editpoint/menu_internal.c"

conf_ch_editpoint_t conf_ch_editpoint;
#define CH_EDITPOINT_CONF_FN "ch_editpoint.conf"


static const char pcb_ch_editpoint_cookie[] = "ch_editpoint plugin";

static vtp0_t editpoint_raw[2];
static vtp0_t *editpoint_objs = &editpoint_raw[0], *old_editpoint_objs = &editpoint_raw[1];

static rnd_r_dir_t editpoint_callback(const rnd_box_t *box, void *cl)
{
	pcb_crosshair_t *crosshair = cl;
	pcb_any_obj_t *obj = (pcb_any_obj_t *)box;

	switch(obj->type) {
		case PCB_OBJ_LINE: if (!pcb_is_point_on_line(crosshair->X, crosshair->Y, PCB_SLOP * rnd_pixel_slop, (pcb_line_t *)obj)) return RND_R_DIR_FOUND_CONTINUE; break;
		case PCB_OBJ_ARC:  if (!pcb_is_point_on_arc(crosshair->X, crosshair->Y, PCB_SLOP * rnd_pixel_slop, (pcb_arc_t *)obj)) return RND_R_DIR_FOUND_CONTINUE; break;
		case PCB_OBJ_POLY: if (!pcb_poly_is_point_in_p(crosshair->X, crosshair->Y, PCB_SLOP * rnd_pixel_slop, (pcb_poly_t *)obj)) return RND_R_DIR_FOUND_CONTINUE; break;
		case PCB_OBJ_PSTK: if (!pcb_is_point_in_pstk(crosshair->X, crosshair->Y, PCB_SLOP * rnd_pixel_slop, (pcb_pstk_t *)obj, NULL)) return RND_R_DIR_FOUND_CONTINUE; break;
		default: break;
	}

	vtp0_append(editpoint_objs, obj);
	obj->ind_editpoint = 1;
	return RND_R_DIR_FOUND_CONTINUE;
}

static void *editpoint_find(vtp0_t *vect, pcb_any_obj_t *obj_ptr)
{
	int n;

	for(n = 0; n < vect->used; n++) {
		pcb_any_obj_t *op = vect->array[n];
		if (op == obj_ptr)
			return op;
	}
	return NULL;
}

/* Searches for lines/arcs which have points that are exactly at the given coords
   and adds them to the object list along with their respective type. */
static void editpoint_work(pcb_crosshair_t *crosshair, rnd_coord_t X, rnd_coord_t Y)
{
	rnd_box_t SearchBox = rnd_point_box(X, Y);
	int n;
	rnd_bool redraw = rnd_false;
	vtp0_t *tmp;

	/* swap old and new */
	tmp = editpoint_objs;
	editpoint_objs = old_editpoint_objs;
	old_editpoint_objs = tmp;

	/* Do not truncate to 0 because that may free the array */
	vtp0_truncate(editpoint_objs, 1);
	editpoint_objs->used = 0;

	for(n = 0; n < pcb_max_layer(PCB); n++) {
		pcb_layer_t *layer = &PCB->Data->Layer[n];
		/* Only find points of arcs and lines on currently visible layers. */
		if (!layer->meta.real.vis)
			continue;
		rnd_r_search(layer->line_tree, &SearchBox, NULL, editpoint_callback, crosshair, NULL);
		rnd_r_search(layer->arc_tree, &SearchBox, NULL, editpoint_callback, crosshair, NULL);
		rnd_r_search(layer->polygon_tree, &SearchBox, NULL, editpoint_callback, crosshair, NULL);
	}
	rnd_r_search(PCB->Data->padstack_tree, &SearchBox, NULL, editpoint_callback, crosshair, NULL);


	/* Undraw the old objects */
	for(n = 0; n < old_editpoint_objs->used; n++) {
		pcb_any_obj_t *op = old_editpoint_objs->array[n];

		/* only remove and redraw those which aren't in the new list */
		if (editpoint_find(editpoint_objs, op) != NULL)
			continue;

		op->ind_editpoint = 0;
		pcb_draw_obj(op);
		redraw = rnd_true;
	}

	/* draw the new objects */
	for(n = 0; n < editpoint_objs->used; n++) {
		pcb_any_obj_t *op = editpoint_objs->array[n];

		/* only draw those which aren't in the old list */
		if (editpoint_find(old_editpoint_objs, op) != NULL)
			continue;
		pcb_draw_obj(op);
		redraw = rnd_true;
	}

	if (redraw)
		rnd_hid_redraw(PCB);
}

static void pcb_ch_editpoint(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_crosshair_t *ch = argv[1].d.p;
	if (conf_ch_editpoint.plugins.ch_editpoint.enable) {
		const rnd_tool_t *tool = rnd_tool_get(rnd_conf.editor.mode);
		if ((tool != NULL) && (tool->user_flags & PCB_TLF_EDIT))
			editpoint_work(ch, ch->X, ch->Y);
	}
}

int pplg_check_ver_ch_editpoint(int ver_needed) { return 0; }

void pplg_uninit_ch_editpoint(void)
{
	rnd_event_unbind_allcookie(pcb_ch_editpoint_cookie);
	vtp0_uninit(editpoint_objs);
	vtp0_uninit(old_editpoint_objs);

	rnd_conf_unreg_file(CH_EDITPOINT_CONF_FN, ch_editpoint_conf_internal);
	rnd_hid_menu_unload(rnd_gui, pcb_ch_editpoint_cookie);
	rnd_conf_unreg_fields("plugins/ch_editpoint/");
}

int pplg_init_ch_editpoint(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_file(CH_EDITPOINT_CONF_FN, ch_editpoint_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_ch_editpoint, field,isarray,type_name,cpath,cname,desc,flags);
#include "ch_editpoint_conf_fields.h"

	rnd_event_bind(PCB_EVENT_CROSSHAIR_NEW_POS, pcb_ch_editpoint, NULL, pcb_ch_editpoint_cookie);

	rnd_hid_menu_load(rnd_gui, NULL, pcb_ch_editpoint_cookie, 100, NULL, 0, ch_editpoint_menu, "plugin: ch_editpoint");

	return 0;
}
