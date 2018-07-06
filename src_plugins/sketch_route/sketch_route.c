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
	point_t *p;
	enum {
		SIDE_LEFT = -1,
		SIDE_TERM = 0,
		SIDE_RIGHT = 1
	} side;
} sided_point_t;

typedef struct {
	int point_num;
	int point_max;
	sided_point_t *points;
} wire_t;

#define WIRE_POINTS_STEP 10

void wire_init(wire_t *w) {
	w->point_num = 0;
	w->point_max = WIRE_POINTS_STEP;
	w->points = malloc(WIRE_POINTS_STEP*sizeof(sided_point_t));
}

void wire_uninit(wire_t *w)
{
	free(w->points);
}

void wire_push_point(wire_t *w, point_t *p, int side) {
	w->points[w->point_num].p = p;
	w->points[w->point_num].side = side;
	if (++w->point_num >= w->point_max) {
		w->point_max += WIRE_POINTS_STEP;
		w->points = realloc(w->points, w->point_max*sizeof(sided_point_t));
	}
}

void wire_pop_point(wire_t *w) {
	if(w->point_num > 0)
		w->point_num--;
}


typedef struct {
	cdt_t *cdt;
	htpp_t terminals; /* key - terminal object; value - cdt point */
	pcb_layer_t *ui_layer_cdt;
} sketch_t;

static htip_t sketches;


static point_t *sketch_get_point_at_terminal(sketch_t *sk, pcb_any_obj_t *term)
{
	return htpp_getentry(&sk->terminals, term)->value;
}

static void sketch_draw_cdt(sketch_t *sk)
{
	VTEDGE_FOREACH(e, &sk->cdt->edges)
		pcb_line_new(sk->ui_layer_cdt, e->endp[0]->pos.x, e->endp[0]->pos.y, e->endp[1]->pos.x, e->endp[1]->pos.y, 1, 0, pcb_no_flags());
	VTEDGE_FOREACH_END();
}

static pcb_bool sketch_check_path(point_t *from_p, edge_t *from_e, edge_t *to_e, point_t *to_p)
{
	/* TODO */
	return pcb_true;
}

static void sketch_find_shortest_path(wire_t *corridor, wire_t **path)
{
	/* TODO */
}

static void sketch_insert_wire(sketch_t *sk, wire_t *wire)
{
	/* TODO */
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
static void tool_skline_adjust_attached_objects(void);

struct {
	pcb_any_obj_t *start_term;
	pcb_lib_menu_t *net;
	vtp0_t lines;
	point_t *start_p;
	edgelist_node_t *visited_edges;
	triangle_t *current_t;
	wire_t corridor;
	sketch_t *sketch;
} attached_path = {0};

static pcb_attached_line_t *attached_path_new_line()
{
	pcb_attached_line_t *new_l = calloc(1, sizeof(pcb_attached_line_t));
	vtp0_append(&attached_path.lines, new_l);
	return new_l;
}

static void attached_path_next_line()
{
	int last = vtp0_len(&attached_path.lines) - 1;
	pcb_attached_line_t *next_l = attached_path_new_line();
	if (last >= 0)
		next_l->Point1 = ((pcb_attached_line_t *) attached_path.lines.array[last])->Point2;
	tool_skline_adjust_attached_objects();
}

static pcb_bool attached_path_init(pcb_layer_t *layer, pcb_any_obj_t *start_term)
{
	pcb_attached_line_t *start_l;

	attached_path.sketch = sketches_get_sketch_at_layer(layer);
	attached_path.start_term = start_term;
	attached_path.net = pcb_netlist_find_net4term(PCB, start_term);
	if (attached_path.net == NULL)
		return pcb_false;

	vtp0_init(&attached_path.lines);
	start_l = attached_path_new_line();
	pcb_obj_center(start_term, &start_l->Point1.X, &start_l->Point1.Y);
	tool_skline_adjust_attached_objects();

	attached_path.start_p = sketch_get_point_at_terminal(attached_path.sketch, start_term);
	attached_path.current_t = NULL;
	attached_path.visited_edges = NULL;
	wire_init(&attached_path.corridor);
	wire_push_point(&attached_path.corridor, attached_path.start_p, SIDE_TERM);

	return pcb_true;
}

static void attached_path_uninit()
{
	int i;
	for (i = 0; i < vtp0_len(&attached_path.lines); i++)
		free(attached_path.lines.array[i]);
	vtp0_uninit(&attached_path.lines);
	edgelist_free(attached_path.visited_edges);
	attached_path.visited_edges = NULL;
}

static pcb_bool line_intersects_edge(pcb_attached_line_t *line, edge_t *edge)
{
	point_t p, q;
	p.pos.x = line->Point1.X;
	p.pos.y = line->Point1.Y;
	q.pos.x = line->Point2.X;
	q.pos.y = line->Point2.Y;
	return LINES_INTERSECT(&p, &q, edge->endp[0], edge->endp[1]);
}

static pcb_bool attached_path_next_point(point_t *end_p)
{
	int last = vtp0_len(&attached_path.lines) - 1;
	pcb_attached_line_t *attached_line = attached_path.lines.array[last];
	edge_t *entrance_e = NULL;
	wire_t corridor_ops;
	int i;

	wire_init(&corridor_ops);
#define RETURN(r) do { \
	wire_uninit(&corridor_ops); \
	return (r); \
} while (0)

	/* special case: end point is in the current triangle (or next to start point) */
	/* TODO */

	/* move along the attached line to find intersecting triangles which form a corridor */
	while(1) {
		if (attached_path.current_t == NULL) { /* we haven't left the first triangle yet */
			TRIANGLELIST_FOREACH(t, attached_path.start_p->adj_triangles)
				for (i = 0; i < 3; i++)
					if (t->e[i]->endp[0] != attached_path.start_p && t->e[i]->endp[1] != attached_path.start_p
							&& t->e[i] != entrance_e && line_intersects_edge(attached_line, t->e[i])) {
						if (sketch_check_path(attached_path.start_p, NULL, t->e[i], NULL) == pcb_false)
							RETURN(pcb_false);
						attached_path.current_t = t->adj_t[i];
						wire_push_point(&corridor_ops, t->p[i], SIDE_RIGHT);
						wire_push_point(&corridor_ops, t->p[(i+1)%3], SIDE_LEFT);
						entrance_e = t->e[i];
						attached_path.visited_edges = edgelist_prepend(attached_path.visited_edges, &entrance_e);
						goto next_triangle;
					}
			TRIANGLELIST_FOREACH_END();
		}
		else { /* next triangle in the corridor */
			triangle_t *t = attached_path.current_t;
			for (i = 0; i < 3; i++) {
				if (t->e[i] != entrance_e && line_intersects_edge(attached_line, t->e[i])) {
					edge_t *last_entrance_e = attached_path.visited_edges->item;
					entrance_e = t->e[i];
					attached_path.current_t = t->adj_t[i];
					if (t->e[i] != last_entrance_e) { /* going forward */
						if (sketch_check_path(NULL, attached_path.visited_edges->item, t->e[i], NULL) == pcb_false)
							RETURN(pcb_false);
						wire_push_point(&corridor_ops,
														t->p[t->e[(i+2)%3] == last_entrance_e ? (i+1)%3 : i],
														t->e[(i+1)%3] == last_entrance_e ? SIDE_RIGHT : SIDE_LEFT);
						attached_path.visited_edges = edgelist_prepend(attached_path.visited_edges, &entrance_e);

						if (attached_path.current_t->p[(i+2)%3] == end_p) { /* entering last triangle? */
							if (sketch_check_path(NULL, entrance_e, NULL, end_p) == pcb_false)
								RETURN(pcb_false);
							wire_push_point(&corridor_ops, end_p, SIDE_TERM);
							goto end;
						}

						goto next_triangle;
					}
					else { /* going backward */
						attached_path.visited_edges = edgelist_remove_front(attached_path.visited_edges);
						wire_push_point(&corridor_ops, NULL, 0); /* NULL means to remove point */
						if (attached_path.visited_edges == NULL) { /* came back to the first triangle */
							wire_push_point(&corridor_ops, NULL, 0);
							attached_path.current_t = NULL;
						}
						goto next_triangle;
					}
				}
			}
		}
		break;
next_triangle:
		continue;
	}

end:
	/* apply evaluated points additions/removals */
	for (i = 0; i < corridor_ops.point_num; i++) {
		if (corridor_ops.points[i].p != NULL)
			wire_push_point(&attached_path.corridor, corridor_ops.points[i].p, corridor_ops.points[i].side);
		else
			wire_pop_point(&attached_path.corridor);
	}
	attached_path_next_line();
	return pcb_true;
#undef RETURN
}

static pcb_bool attached_path_finish(pcb_any_obj_t *end_term)
{
	pcb_subc_t *subc = pcb_obj_parent_subc(end_term);
	int i;

	if(end_term != attached_path.start_term && subc != NULL) {
		char termname[128];
		pcb_snprintf(termname, sizeof(termname), "%s-%s", subc->refdes, end_term->term);
		for (i = 0; i < attached_path.net->EntryN; i++) {
			if (strcmp(attached_path.net->Entry[i].ListEntry, termname) == 0) {
				point_t *end_p;
				wire_t *wire;
				end_p = sketch_get_point_at_terminal(attached_path.sketch, end_term);
				if (attached_path_next_point(end_p) == pcb_false)
					return pcb_false;
				sketch_find_shortest_path(&attached_path.corridor, &wire);
				sketch_insert_wire(attached_path.sketch, wire);
				return pcb_true;
			}
		}
	}
	return pcb_false;
}

static void tool_skline_uninit(void)
{
	pcb_notify_crosshair_change(pcb_false);
	attached_path_uninit();
	pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_notify_crosshair_change(pcb_true);
}

static void tool_skline_notify_mode(void)
{
	int type;
	void *ptr1, *ptr2, *ptr3;
	pcb_any_obj_t *term_obj;

	switch (pcb_crosshair.AttachedObject.State) {
	case PCB_CH_STATE_FIRST:
		type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_CLASS_TERM, &ptr1, &ptr2, &ptr3);
		/* TODO: check if an object is on the current layer */
		if (type != PCB_OBJ_VOID) {
			term_obj = ptr2;
			if (term_obj->term != NULL
					&& ((type == PCB_OBJ_PSTK && pcb_pstk_shape_at(PCB, (pcb_pstk_t *) term_obj, CURRENT) != NULL)
							|| type != PCB_OBJ_PSTK)
					&& attached_path_init(CURRENT, term_obj)) {
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_LINE;
				pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;
			}
			else
				pcb_message(PCB_MSG_WARNING, _("Sketch lines can be only drawn from a terminal\n"));
		}
		break;

	case PCB_CH_STATE_SECOND:
		type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_CLASS_TERM, &ptr1, &ptr2, &ptr3);
		/* TODO: check if an object is on the current layer */
		if (type != PCB_OBJ_VOID) {
			term_obj = ptr2;
			if (term_obj->term != NULL
					&& ((type == PCB_OBJ_PSTK && pcb_pstk_shape_at(PCB, (pcb_pstk_t *) term_obj, CURRENT) != NULL)
							|| type != PCB_OBJ_PSTK)) {
				if (attached_path_finish(term_obj) == pcb_true) {
					attached_path_uninit();
					pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
					pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
				} else {
					pcb_message(PCB_MSG_WARNING, _("Cannot finish placing wire at this terminal\n"));
					pcb_message(PCB_MSG_WARNING, _("(the terminal does not belong to the routed net or is the starting terminal)\n"));
				}
			}
		} else {
			if (attached_path_next_point(NULL) == pcb_false)
				pcb_message(PCB_MSG_WARNING, _("Cannot route the wire this way\n"));
		}
		break;
	}
}

static void tool_skline_adjust_attached_objects(void)
{
	int last = vtp0_len(&attached_path.lines) - 1;
	if (last >= 0) {
		((pcb_attached_line_t *) attached_path.lines.array[last])->Point2.X = pcb_crosshair.X;
		((pcb_attached_line_t *) attached_path.lines.array[last])->Point2.Y = pcb_crosshair.Y;
	}
}

static void tool_skline_draw_attached(void)
{
	int i;
	if (pcb_crosshair.AttachedObject.Type != PCB_OBJ_VOID) {
		for (i = 0; i < vtp0_len(&attached_path.lines); i++) {
			pcb_point_t *p1 = &((pcb_attached_line_t *) attached_path.lines.array[i])->Point1;
			pcb_point_t *p2 = &((pcb_attached_line_t *) attached_path.lines.array[i])->Point2;
			pcb_gui->draw_line(pcb_crosshair.GC, p1->X, p1->Y, p2->X, p2->Y);
		}
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
