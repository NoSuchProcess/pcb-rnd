/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Wojciech Krutnik
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "plugins.h"
#include "actions.h"
#include "board.h"
#include "compat_nls.h"
#include "data.h"
#include "event.h"
#include "obj_line_list.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_parent.h"
#include "pcb-printf.h"
#include "search.h"
#include "tool.h"
#include "layer_ui.h"

#include "cdt/cdt.h"
#include <genht/htip.h>
#include <genht/htpp.h>


const char *pcb_sketch_route_cookie = "sketch_route plugin";

typedef struct {
	pcb_lib_menu_t *net;
	pointlist_node_t *points;
} wire_t;

typedef struct {
	cdt_t *cdt;
	htpp_t terminals; /* key - terminal object; value - cdt point */
	pcb_layer_t *ui_layer_cdt;
} sketch_t;

static htip_t sketches;


static void sketch_draw_cdt(sketch_t *sk)
{
	VTEDGE_FOREACH(e, &sk->cdt->edges)
		pcb_line_new(sk->ui_layer_cdt, e->endp[0]->pos.x, e->endp[0]->pos.y, e->endp[1]->pos.x, e->endp[1]->pos.y, 1, 0, pcb_no_flags());
	VTEDGE_FOREACH_END();
}

struct search_info {
	pcb_layer_t *layer;
	sketch_t *sk;
};

static pcb_r_dir_t r_search_cb(const pcb_box_t *box, void *cl)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *) box;
	struct search_info *i = (struct search_info *) cl;
	point_t *point;

	if (obj->type == PCB_OBJ_PSTK) {
		pcb_pstk_t *pstk = (pcb_pstk_t *) obj;
		if (pcb_pstk_shape_at(PCB, pstk, i->layer) == NULL)
			return PCB_R_DIR_NOT_FOUND;
		point = cdt_insert_point(i->sk->cdt, pstk->x, pstk->y);
	}
	/* temporary: if a non-padstack obj is _not_ a terminal, then don't triangulate it */
	/* long term (for non-terminal objects):
	 * - lines should be triangulated on their endpoints and constrained
	 * - polygons should be triangulated on vertices and edges constrained
	 * - texts - same as polygons for their bbox
	 * - arcs - same as polygons for their bbox */
	else if (obj->term != NULL) {
		coord_t cx, cy;
		pcb_obj_center(obj, &cx, &cy);
		point = cdt_insert_point(i->sk->cdt, cx, cy);
	}

	if (obj->term != NULL) {
		pcb_subc_t *subc = pcb_obj_parent_subc(obj);
		if (subc != NULL && subc->refdes != NULL)
			htpp_insert(&i->sk->terminals, obj, point);
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

static void sketch_create_for_layer(sketch_t *sk, pcb_layer_t *layer)
{
	pcb_box_t bbox;
	struct search_info info;
	char name[256];

	sk->cdt = malloc(sizeof(cdt_t));
	htpp_init(&sk->terminals, ptrhash, ptrkeyeq);
	cdt_init(sk->cdt, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	bbox.X1 = 0; bbox.Y1 = 0; bbox.X2 = PCB->MaxWidth; bbox.Y2 = PCB->MaxHeight;

	info.layer = layer;
	info.sk = sk;
	pcb_r_search(PCB->Data->padstack_tree, &bbox, NULL, r_search_cb, &info, NULL);
	pcb_r_search(layer->line_tree, &bbox, NULL, r_search_cb, &info, NULL);
	pcb_r_search(layer->text_tree, &bbox, NULL, r_search_cb, &info, NULL);
	pcb_r_search(layer->polygon_tree, &bbox, NULL, r_search_cb, &info, NULL);
	pcb_r_search(layer->arc_tree, &bbox, NULL, r_search_cb, &info, NULL);

	pcb_snprintf(name, sizeof(name), "%s: CDT", layer->name);
	sk->ui_layer_cdt = pcb_uilayer_alloc(pcb_sketch_route_cookie, name, layer->meta.real.color);
	sk->ui_layer_cdt->meta.real.vis = pcb_false;
	sketch_draw_cdt(sk);
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
}

static sketch_t *sketch_alloc()
{
	sketch_t *new_sk;
	new_sk = calloc(1, sizeof(sketch_t));
	return new_sk;
}

static void sketch_uninit(sketch_t *sk)
{
	if (sk->cdt != NULL) {
		cdt_free(sk->cdt);
		free(sk->cdt);
		sk->cdt = NULL;
	}
	if (sk->ui_layer_cdt != NULL) {
		pcb_uilayer_free(sk->ui_layer_cdt);
		sk->ui_layer_cdt = NULL;
	}
	htpp_uninit(&sk->terminals);
}


static sketch_t *sketches_get_sketch_at_layer(pcb_layer_t *layer)
{
	return htip_getentry(&sketches, pcb_layer_id(PCB->Data, layer))->value;
}

static void sketches_init()
{
	pcb_layer_id_t lid[PCB_MAX_LAYER];
	int i, num;

	num = pcb_layer_list(PCB, PCB_LYT_COPPER, lid, PCB_MAX_LAYER);
	htip_init(&sketches, longhash, longkeyeq);
	for (i = 0; i < num; i++) {
		sketch_t *sk = sketch_alloc();
		sketch_create_for_layer(sk, pcb_get_layer(PCB->Data, lid[i]));
		htip_insert(&sketches, lid[i], sk);
	}
}

static void sketches_uninit()
{
	htip_entry_t *e;

	for (e = htip_first(&sketches); e; e = htip_next(&sketches, e)) {
		sketch_uninit(e->value);
		free(e->value);
		htip_delentry(&sketches, e);
	}
	htip_uninit(&sketches);
}


/*** sketch line tool ***/
struct {
	pcb_attached_line_t line;
	trianglelist_node_t corridor;
	wire_t wire;
} attached;

void tool_skline_uninit(void)
{
	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_notify_crosshair_change(pcb_true);
}

void tool_skline_notify_mode(void)
{
	int type;
	void *ptr1, *ptr2, *ptr3;
	pcb_any_obj_t *term_obj;
	switch (pcb_crosshair.AttachedObject.State) {
	case PCB_CH_STATE_FIRST:
		type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_CLASS_TERM, &ptr1, &ptr2, &ptr3);

		if (type != PCB_OBJ_VOID) {
			term_obj = ptr2;
			if (term_obj->term == NULL) {
				pcb_message(PCB_MSG_WARNING, _("Sketch lines can be only drawn from a terminal\n"));
				break;
			}
			else {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_LINE;
				pcb_obj_center(term_obj, &attached.line.Point1.X, &attached.line.Point1.Y);
				pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;
			}
		}
		break;

	case PCB_CH_STATE_SECOND:
		pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
		pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
		break;
	}
}

void tool_skline_adjust_attached_objects(void)
{
	attached.line.Point2.X = pcb_crosshair.X;
	attached.line.Point2.Y = pcb_crosshair.Y;
}

static void tool_skline_draw_attached(void)
{
	if (pcb_crosshair.AttachedObject.Type != PCB_OBJ_VOID) {
		pcb_gui->draw_line(pcb_crosshair.GC, attached.line.Point1.X, attached.line.Point1.Y,
																				 attached.line.Point2.X, attached.line.Point2.Y);
	}
}

pcb_bool tool_skline_undo_act(void)
{
	/* TODO */
	return pcb_false;
}

static pcb_tool_t tool_skline = {
	"skline", NULL, 100,
	NULL,
	tool_skline_uninit,
	tool_skline_notify_mode,
	NULL,
	tool_skline_adjust_attached_objects,
	tool_skline_draw_attached,
	tool_skline_undo_act,
	NULL,

	pcb_false
};


/*** actions ***/
static const char pcb_acts_skretriangulate[] = "skretriangulate()";
static const char pcb_acth_skretriangulate[] = "Reconstruct CDT on all layer groups";
fgw_error_t pcb_act_skretriangulate(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	sketches_uninit();
	sketches_init();

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_skline[] = "skline()";
static const char pcb_acth_skline[] = "Tool for drawing sketch lines";
fgw_error_t pcb_act_skline(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_tool_select_by_name("skline");

	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t sketch_route_action_list[] = {
	{"skretriangulate", pcb_act_skretriangulate, pcb_acth_skretriangulate, pcb_acts_skretriangulate},
	{"skline", pcb_act_skline, pcb_acth_skline, pcb_acts_skline}
};

PCB_REGISTER_ACTIONS(sketch_route_action_list, pcb_sketch_route_cookie)

int pplg_check_ver_sketch_route(int ver_needed) { return 0; }

void pplg_uninit_sketch_route(void)
{
	pcb_remove_actions_by_cookie(pcb_sketch_route_cookie);
	pcb_tool_unreg_by_cookie(pcb_sketch_route_cookie); /* should be done before pcb_tool_uninit, somehow */
	sketches_uninit();
}


#include "dolists.h"
int pplg_init_sketch_route(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(sketch_route_action_list, pcb_sketch_route_cookie)

	pcb_tool_reg(&tool_skline, pcb_sketch_route_cookie);

	return 0;
}
