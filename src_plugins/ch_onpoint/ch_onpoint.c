/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  crosshair: highlight objects that are under the cursor
 *  original onpoint patch Copyright (C) 2015 Robert Drehmel
 *  pcb-rnd Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

/* Implementation of the "highlight on endpoint" functionality. This highlights
   lines and arcs when the crosshair is on of their (two) endpoints. */

#include "config.h"

#include <stdio.h>

#include <genvector/vtp0.h>
#include <librnd/core/plugins.h>
#include <librnd/hid/tool.h>
#include <librnd/poly/rtree.h>
#include <librnd/core/rnd_conf.h>
#include "tool_logic.h"
#include "board.h"
#include "conf_core.h"
#include "crosshair.h"
#include "data.h"
#include "event.h"
#include "obj_arc.h"
#include "obj_arc_draw.h"
#include "obj_line_draw.h"

static const char pcb_ch_onpoint_cookie[] = "ch_onpoint plugin";

static vtp0_t onpoint_raw[2];
static vtp0_t *onpoint_objs = &onpoint_raw[0], *old_onpoint_objs = &onpoint_raw[1];

struct onpoint_search_info {
	pcb_crosshair_t *crosshair;
	rnd_coord_t X;
	rnd_coord_t Y;
};

static rnd_rtree_dir_t onpoint_line_callback(void *cl, void *obj, const rnd_rtree_box_t *box)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *)cl;
	pcb_line_t *line = (pcb_line_t *)obj;

#ifdef DEBUG_ONPOINT
	rnd_trace("onpoint X=%mm Y=%mm    X1=%mm Y1=%mm X2=%mm Y2=%mm\n",
		info->X, info->Y, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
#endif
	if ((line->Point1.X == info->X && line->Point1.Y == info->Y) || (line->Point2.X == info->X && line->Point2.Y == info->Y)) {
		vtp0_append(onpoint_objs, line);
		line->ind_onpoint = 1;
		pcb_line_invalidate_draw(NULL, line);
		return rnd_RTREE_DIR_FOUND_CONT;
	}
	else
		return rnd_RTREE_DIR_NOT_FOUND_CONT;
}

#define close_enough(v1, v2) (coord_abs((v1)-(v2)) < 10)

static rnd_rtree_dir_t onpoint_arc_callback(void *cl, void *obj, const rnd_rtree_box_t *box)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *)cl;
	pcb_arc_t *arc = (pcb_arc_t *)obj;
	rnd_coord_t p1x, p1y, p2x, p2y;

	pcb_arc_get_end(arc, 0, &p1x, &p1y);
	pcb_arc_get_end(arc, 1, &p2x, &p2y);

#ifdef DEBUG_ONPOINT
	rnd_trace("onpoint p1=%mm;%mm p2=%mm;%mm info=%mm;%mm\n", p1x, p1y, p2x, p2y, info->X, info->Y);
#endif

	if ((close_enough(p1x, info->X) && close_enough(p1y, info->Y)) || (close_enough(p2x, info->X) && close_enough(p2y, info->Y))) {
		vtp0_append(onpoint_objs, arc);
		arc->ind_onpoint = 1;
		pcb_arc_invalidate_draw(NULL, arc);
		return rnd_RTREE_DIR_FOUND_CONT;
	}
	else
		return rnd_RTREE_DIR_NOT_FOUND_CONT;
}

static void draw_line_or_arc(pcb_any_obj_t *o)
{
	switch(o->type) {
		case PCB_OBJ_LINE: pcb_line_invalidate_draw(o->parent.layer, (pcb_line_t *)o); break;
		case PCB_OBJ_ARC:  pcb_arc_invalidate_draw(o->parent.layer, (pcb_arc_t *)o); break;
			break;
		default: assert(!"draw_line_or_arc() expects line or arc point");
	}
}


#define op_swap(crosshair) \
do { \
	vtp0_t *__tmp__ = onpoint_objs; \
	onpoint_objs = old_onpoint_objs; \
	old_onpoint_objs = __tmp__; \
} while(0)

static void *onpoint_find(vtp0_t *vect, pcb_any_obj_t *obj_ptr)
{
	int i;

	for (i = 0; i < vect->used; i++) {
		pcb_any_obj_t *op = vect->array[i];
		if (op == obj_ptr)
			return op;
	}
	return NULL;
}

/* Searches for lines/arcs which have points that are exactly at the given coords
   and adds them to the object list along with their respective type. */
static void onpoint_work(pcb_crosshair_t *crosshair, rnd_coord_t X, rnd_coord_t Y)
{
	rnd_box_t SearchBox = rnd_point_box(X, Y);
	struct onpoint_search_info info;
	int i;
	rnd_bool redraw = rnd_false;

	op_swap(crosshair);

	/* Do not truncate to 0 because that may free the array */
	vtp0_truncate(onpoint_objs, 1);
	onpoint_objs->used = 0;


	info.crosshair = crosshair;
	info.X = X;
	info.Y = Y;

	for (i = 0; i < pcb_max_layer(PCB); i++) {
		pcb_layer_t *layer = &PCB->Data->Layer[i];
		/* Only find points of arcs and lines on currently visible layers. */
		if (!layer->meta.real.vis)
			continue;
		rnd_rtree_search_any(layer->line_tree, (rnd_rtree_box_t *)&SearchBox, NULL, onpoint_line_callback, &info, NULL);
		rnd_rtree_search_any(layer->arc_tree, (rnd_rtree_box_t *)&SearchBox, NULL, onpoint_arc_callback, &info, NULL);
	}

	/* Undraw the old objects */
	for (i = 0; i < old_onpoint_objs->used; i++) {
		pcb_any_obj_t *op = old_onpoint_objs->array[i];

		/* only remove and redraw those which aren't in the new list */
		if (onpoint_find(onpoint_objs, op) != NULL)
			continue;

		op->ind_onpoint = 0;
		draw_line_or_arc(op);
		redraw = rnd_true;
	}

	/* draw the new objects */
	for (i = 0; i < onpoint_objs->used; i++) {
		pcb_any_obj_t *op = onpoint_objs->array[i];

		/* only draw those which aren't in the old list */
		if (onpoint_find(old_onpoint_objs, op) != NULL)
			continue;
		draw_line_or_arc(op);
		redraw = rnd_true;
	}

	if (redraw)
		rnd_hid_redraw(&PCB->hidlib);
}

static void pcb_ch_onpoint(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_crosshair_t *ch = argv[1].d.p;
	if (conf_core.editor.highlight_on_point) {
		const rnd_tool_t *tool = rnd_tool_get(rnd_conf.editor.mode);
		if ((tool != NULL) && (tool->user_flags & PCB_TLF_EDIT))
			onpoint_work(ch, ch->X, ch->Y);
	}
}

int pplg_check_ver_ch_onpoint(int ver_needed) { return 0; }

void pplg_uninit_ch_onpoint(void)
{
	rnd_event_unbind_allcookie(pcb_ch_onpoint_cookie);
	vtp0_uninit(onpoint_objs);
	vtp0_uninit(old_onpoint_objs);
}

int pplg_init_ch_onpoint(void)
{
	RND_API_CHK_VER;

	rnd_event_bind(PCB_EVENT_CROSSHAIR_NEW_POS, pcb_ch_onpoint, NULL, pcb_ch_onpoint_cookie);

	return 0;
}
