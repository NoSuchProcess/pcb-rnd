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

#include <librnd/core/plugins.h>
#include <librnd/poly/rtree.h>
#include <librnd/poly/rtree2_compat.h>
#include "board.h"
#include "conf_core.h"
#include "crosshair.h"
#include "data.h"
#include "event.h"


static const char pcb_ch_onpoint_cookie[] = "ch_onpoint plugin";

struct onpoint_search_info {
	pcb_crosshair_t *crosshair;
	rnd_coord_t X;
	rnd_coord_t Y;
};

static rnd_r_dir_t onpoint_line_callback(const rnd_box_t * box, void *cl)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *) cl;
	pcb_crosshair_t *crosshair = info->crosshair;
	pcb_line_t *line = (pcb_line_t *) box;

#ifdef DEBUG_ONPOINT
	rnd_trace("onpoint X=%mm Y=%mm    X1=%mm Y1=%mm X2=%mm Y2=%mm\n",
		info->X, info->Y, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
#endif
	if ((line->Point1.X == info->X && line->Point1.Y == info->Y) || (line->Point2.X == info->X && line->Point2.Y == info->Y)) {
		vtp0_append(&crosshair->onpoint_objs, line);
		PCB_FLAG_SET(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) line);
		pcb_line_invalidate_draw(NULL, line);
		return RND_R_DIR_FOUND_CONTINUE;
	}
	else
		return RND_R_DIR_NOT_FOUND;
}

#define close_enough(v1, v2) (coord_abs((v1)-(v2)) < 10)

static rnd_r_dir_t onpoint_arc_callback(const rnd_box_t * box, void *cl)
{
	struct onpoint_search_info *info = (struct onpoint_search_info *) cl;
	pcb_crosshair_t *crosshair = info->crosshair;
	pcb_arc_t *arc = (pcb_arc_t *) box;
	rnd_coord_t p1x, p1y, p2x, p2y;

	p1x = arc->X - arc->Width * cos(RND_TO_RADIANS(arc->StartAngle));
	p1y = arc->Y + arc->Height * sin(RND_TO_RADIANS(arc->StartAngle));
	p2x = arc->X - arc->Width * cos(RND_TO_RADIANS(arc->StartAngle + arc->Delta));
	p2y = arc->Y + arc->Height * sin(RND_TO_RADIANS(arc->StartAngle + arc->Delta));

#ifdef DEBUG_ONPOINT
	rnd_trace("onpoint p1=%mm;%mm p2=%mm;%mm info=%mm;%mm\n", p1x, p1y, p2x, p2y, info->X, info->Y);
#endif

	if ((close_enough(p1x, info->X) && close_enough(p1y, info->Y)) || (close_enough(p2x, info->X) && close_enough(p2y, info->Y))) {
		vtp0_append(&crosshair->onpoint_objs, arc);
		PCB_FLAG_SET(PCB_FLAG_ONPOINT, (pcb_any_obj_t *) arc);
		pcb_arc_invalidate_draw(NULL, arc);
		return RND_R_DIR_FOUND_CONTINUE;
	}
	else
		return RND_R_DIR_NOT_FOUND;
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
	vtp0_t __tmp__ = crosshair->onpoint_objs; \
	crosshair->onpoint_objs = crosshair->old_onpoint_objs; \
	crosshair->old_onpoint_objs = __tmp__; \
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
static void onpoint_work(pcb_crosshair_t * crosshair, rnd_coord_t X, rnd_coord_t Y)
{
	rnd_box_t SearchBox = rnd_point_box(X, Y);
	struct onpoint_search_info info;
	int i;
	rnd_bool redraw = rnd_false;

	op_swap(crosshair);

	/* Do not truncate to 0 because that may free the array */
	vtp0_truncate(&crosshair->onpoint_objs, 1);
	crosshair->onpoint_objs.used = 0;


	info.crosshair = crosshair;
	info.X = X;
	info.Y = Y;

	for (i = 0; i < pcb_max_layer(PCB); i++) {
		pcb_layer_t *layer = &PCB->Data->Layer[i];
		/* Only find points of arcs and lines on currently visible layers. */
		if (!layer->meta.real.vis)
			continue;
		rnd_r_search(layer->line_tree, &SearchBox, NULL, onpoint_line_callback, &info, NULL);
		rnd_r_search(layer->arc_tree, &SearchBox, NULL, onpoint_arc_callback, &info, NULL);
	}

	/* Undraw the old objects */
	for (i = 0; i < crosshair->old_onpoint_objs.used; i++) {
		pcb_any_obj_t *op = crosshair->old_onpoint_objs.array[i];

		/* only remove and redraw those which aren't in the new list */
		if (onpoint_find(&crosshair->onpoint_objs, op) != NULL)
			continue;

		PCB_FLAG_CLEAR(PCB_FLAG_ONPOINT, op);
		draw_line_or_arc(op);
		redraw = rnd_true;
	}

	/* draw the new objects */
	for (i = 0; i < crosshair->onpoint_objs.used; i++) {
		pcb_any_obj_t *op = crosshair->onpoint_objs.array[i];

		/* only draw those which aren't in the old list */
		if (onpoint_find(&crosshair->old_onpoint_objs, op) != NULL)
			continue;
		draw_line_or_arc(op);
		redraw = rnd_true;
	}

	if (redraw)
		rnd_hid_redraw(PCB);
}

static void pcb_ch_onpoint(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (conf_core.editor.highlight_on_point)
		onpoint_work(&pcb_crosshair, pcb_crosshair.X, pcb_crosshair.Y);
}

int pplg_check_ver_ch_onpoint(int ver_needed) { return 0; }

void pplg_uninit_ch_onpoint(void)
{
	rnd_event_unbind_allcookie(pcb_ch_onpoint_cookie);
}

int pplg_init_ch_onpoint(void)
{
	RND_API_CHK_VER;

	/* Initialize the onpoint data. */
	memset(&pcb_crosshair.onpoint_objs, 0, sizeof(vtp0_t));
	memset(&pcb_crosshair.old_onpoint_objs, 0, sizeof(vtp0_t));

	rnd_event_bind(PCB_EVENT_CROSSHAIR_NEW_POS, pcb_ch_onpoint, NULL, pcb_ch_onpoint_cookie);

	return 0;
}
