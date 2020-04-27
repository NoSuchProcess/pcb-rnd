/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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

#include <assert.h>
#include <qparse/qparse.h>

#include "toolpath.h"

#include "board.h"
#include "data.h"
#include "flag.h"
#include "layer.h"
#include "layer_grp.h"
#include "layer_ui.h"
#include "obj_line.h"
#include "obj_line_op.h"
#include "obj_arc.h"
#include "obj_poly.h"
#include "obj_poly_op.h"
#include "obj_text_draw.h"
#include "polygon.h"
#include <librnd/poly/polygon1_gen.h>
#include "funchash_core.h"
#include <librnd/poly/rtree.h>

#include "src_plugins/lib_polyhelp/polyhelp.h"
#include "src_plugins/ddraft/centgeo.h"

extern const char *pcb_millpath_cookie;

RND_INLINE void sub_layer_line(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer, const pcb_line_t *line_in, int centerline)
{
	pcb_line_t line_tmp;

	memcpy(&line_tmp, line_in, sizeof(line_tmp));
	PCB_FLAG_SET(PCB_FLAG_CLEARLINE, &line_tmp);
	if (centerline) {
		line_tmp.Thickness = 1;
		line_tmp.Clearance = result->edge_clearance;
	}
	else
		line_tmp.Clearance = 1;
	pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_OBJ_LINE, &line_tmp);
}

RND_INLINE void sub_layer_arc(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer, const pcb_arc_t *arc_in, int centerline)
{
	pcb_arc_t arc_tmp;

	memcpy(&arc_tmp, arc_in, sizeof(arc_tmp));
	PCB_FLAG_SET(PCB_FLAG_CLEARLINE, &arc_tmp);
	if (centerline) {
		arc_tmp.Thickness = 1;
		arc_tmp.Clearance = result->edge_clearance;
	}
	else
		arc_tmp.Clearance = 1;
	pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_OBJ_ARC, &arc_tmp);
}

RND_INLINE void sub_layer_poly(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer, const pcb_poly_t *poly, int centerline)
{
	pcb_polyarea_t *f, *b, *ra;

	if (!PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, poly)) {
		f = poly->Clipped->f;
		b = poly->Clipped->b;
		poly->Clipped->f = poly->Clipped->b = poly->Clipped;
	}

	pcb_polyarea_boolean(result->fill->Clipped, poly->Clipped, &ra, PCB_PBO_SUB);
	pcb_polyarea_free(&result->fill->Clipped);
	result->fill->Clipped = ra;

	if (!PCB_FLAG_TEST(PCB_FLAG_FULLPOLY, poly)) {
		poly->Clipped->f = f;
		poly->Clipped->b = b;
	}
}

typedef struct {
	pcb_board_t *pcb;
	pcb_tlp_session_t *result;
	int centerline;
	pcb_layer_t *layer;
} sub_layer_text_t;

static void sub_layer_text(void *ctx_, pcb_any_obj_t *obj)
{
	sub_layer_text_t *ctx = ctx_;
	pcb_poly_t *poly = (pcb_poly_t *)obj;
	rnd_bool dummy;

	switch(obj->type) {
		case PCB_OBJ_LINE: sub_layer_line(ctx->pcb, ctx->result, ctx->layer, (pcb_line_t *)obj, ctx->centerline); break;
		case PCB_OBJ_ARC:  sub_layer_arc(ctx->pcb, ctx->result, ctx->layer, (pcb_arc_t *)obj, ctx->centerline); break;
		case PCB_OBJ_POLY:
			poly->Clipped = pcb_poly_to_polyarea(poly, &dummy);
			sub_layer_poly(ctx->pcb, ctx->result, ctx->layer, poly, ctx->centerline);
			pcb_polyarea_free(&poly->Clipped);
			break;
		default:           rnd_message(PCB_MSG_ERROR, "Internal error: toolpath sub_layer_text() invalid object type %ld\n", obj->type);
	}
}

static void sub_layer_all(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer, int centerline)
{
	pcb_rtree_it_t it;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_poly_t *poly;
	pcb_text_t *text;
	sub_layer_text_t slt;

	for(line = (pcb_line_t *)pcb_r_first(layer->line_tree, &it); line != NULL; line = (pcb_line_t *)pcb_r_next(&it))
		sub_layer_line(pcb, result, layer, line, centerline);
	pcb_r_end(&it);

	for(arc = (pcb_arc_t *)pcb_r_first(layer->arc_tree, &it); arc != NULL; arc = (pcb_arc_t *)pcb_r_next(&it))
		sub_layer_arc(pcb, result, layer, arc, centerline);
	pcb_r_end(&it);

	for(poly = (pcb_poly_t *)pcb_r_first(layer->polygon_tree, &it); poly != NULL; poly = (pcb_poly_t *)pcb_r_next(&it))
		sub_layer_poly(pcb, result, layer, poly, centerline);
	pcb_r_end(&it);

	slt.pcb = pcb;
	slt.layer = layer;
	slt.centerline = centerline;
	slt.result = result;
	for(text = (pcb_text_t *)pcb_r_first(layer->text_tree, &it); text != NULL; text = (pcb_text_t *)pcb_r_next(&it))
		pcb_text_decompose_text(NULL, text, sub_layer_text, &slt);
	pcb_r_end(&it);

}


static void sub_group_all(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp, int centerline)
{
	int n;
	for(n = 0; n < grp->len; n++) {
		pcb_layer_t *l = pcb_get_layer(PCB->Data, grp->lid[n]);
		if (l != NULL)
			sub_layer_all(pcb, result, l, centerline);
	}
}

static void sub_global_all(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layer_t *layer)
{
	pcb_pstk_t *ps, ps_tmp;
	pcb_rtree_it_t it;

	for(ps = (pcb_pstk_t *)pcb_r_first(pcb->Data->padstack_tree, &it); ps != NULL; ps = (pcb_pstk_t *)pcb_r_next(&it)) {
		memcpy(&ps_tmp, ps, sizeof(ps_tmp));
		ps_tmp.Clearance = 1;
		ps_tmp.thermals.used = 0;
		pcb_poly_sub_obj(pcb->Data, layer, result->fill, PCB_OBJ_PSTK, &ps_tmp);
	}
	pcb_r_end(&it);
}

static void setup_ui_layers(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	static pcb_color_t clr1, clr2, clr3;

	if (clr1.str[0] != '#')
		pcb_color_load_str(&clr1, "#EE9922");
	if (clr2.str[0] != '#')
		pcb_color_load_str(&clr2, "#886611");
	if (clr3.str[0] != '#')
		pcb_color_load_str(&clr3, "#FFCC55");

	if (result->res_ply == NULL)
		result->res_ply = pcb_uilayer_alloc(pcb_millpath_cookie, "mill remove", &clr1);

	if (result->res_remply == NULL)
		result->res_remply = pcb_uilayer_alloc(pcb_millpath_cookie, "mill remain", &clr3);

	if (result->res_path == NULL)
		result->res_path = pcb_uilayer_alloc(pcb_millpath_cookie, "mill toolpath", &clr2);

	if (result->fill != NULL)
		pcb_polyop_destroy(NULL, result->res_ply, result->fill);

	if (result->remain != NULL)
		pcb_polyop_destroy(NULL, result->res_remply, result->remain);

	linelist_foreach(&result->res_path->Line, &it, line) {
		pcb_lineop_destroy(NULL, result->res_path, line);
	}
}

static void setup_remove_poly(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp, int polarity)
{
	int has_otl;
	pcb_layergrp_id_t i;
	pcb_layergrp_t *g;

	result->grp = grp;

	has_otl = 0;
	for(i = 0, g = pcb->LayerGroups.grp; i < pcb->LayerGroups.len; i++,g++) {
		if ((PCB_LAYER_IS_OUTLINE(g->ltype, g->purpi)) && (!pcb_layergrp_is_pure_empty(pcb, i))) {
			has_otl = 1;
			break;
		}
	}


	if (has_otl) { /* if there's an outline layer, the remove-poly shouldn't be bigger than that */
		pcb_line_t *line;
		pcb_arc_t *arc;
		pcb_rtree_it_t it;
		rnd_box_t otlbb;
		pcb_layer_id_t lid;
		
		otlbb.X1 = otlbb.Y1 = PCB_MAX_COORD;
		otlbb.X2 = otlbb.Y2 = -PCB_MAX_COORD;

		for(lid = 0; lid < pcb->Data->LayerN; lid++) {
			pcb_layer_t *l;
			if (!PCB_LAYER_IS_OUTLINE(pcb_layer_flags(PCB, lid), pcb_layer_purpose(PCB, lid, NULL)))
				continue;

			l = pcb_get_layer(PCB->Data, lid);
			if (l == NULL)
				continue;

			for(line = (pcb_line_t *)pcb_r_first(l->line_tree, &it); line != NULL; line = (pcb_line_t *)pcb_r_next(&it))
				rnd_box_bump_box(&otlbb, (rnd_box_t *)line);
			pcb_r_end(&it);

			for(arc = (pcb_arc_t *)pcb_r_first(l->arc_tree, &it); arc != NULL; arc = (pcb_arc_t *)pcb_r_next(&it))
				rnd_box_bump_box(&otlbb, (rnd_box_t *)arc);
			pcb_r_end(&it);
		}
		result->fill = pcb_poly_new_from_rectangle(result->res_ply, otlbb.X1, otlbb.Y1, otlbb.X2, otlbb.Y2, 0, pcb_flag_make(PCB_FLAG_FULLPOLY));
		result->remain = pcb_poly_new_from_rectangle(result->res_remply, otlbb.X1, otlbb.Y1, otlbb.X2, otlbb.Y2, 0, pcb_flag_make(PCB_FLAG_FULLPOLY));
	}
	else {
		result->fill = pcb_poly_new_from_rectangle(result->res_ply, 0, 0, pcb->hidlib.size_x, pcb->hidlib.size_y, 0, pcb_flag_make(PCB_FLAG_FULLPOLY));
		result->remain = pcb_poly_new_from_rectangle(result->res_remply, 0, 0, pcb->hidlib.size_x, pcb->hidlib.size_y, 0, pcb_flag_make(PCB_FLAG_FULLPOLY));
	}

	pcb_poly_init_clip(pcb->Data, result->res_ply, result->fill);
	pcb_poly_init_clip(pcb->Data, result->res_remply, result->remain);

	sub_group_all(pcb, result, result->grp, 0);
	if (has_otl)
		for(i = 0, g = pcb->LayerGroups.grp; i < pcb->LayerGroups.len; i++,g++)
			if (PCB_LAYER_IS_OUTLINE(g->ltype, g->purpi))
				sub_group_all(pcb, result, g, 1);


	{ /* apply all layers within the group */
		long n;
		for(n = 0; n < grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(pcb->Data, grp->lid[n]);
			if (ly != NULL)
				sub_global_all(pcb, result, ly);
		}
	}

	/* remove fill from remain */
	{
		pcb_polyarea_t *rp;
		pcb_polyarea_boolean(result->remain->Clipped, result->fill->Clipped, &rp, PCB_PBO_SUB);
		pcb_polyarea_free(&result->remain->Clipped);
		result->remain->Clipped = rp;
		result->remain->Flags.f |= PCB_FLAG_FULLPOLY;
	}

	/* for positive polarity, simply swap the two polygons to invert the scene */
	if (polarity > 0) {
		pcb_poly_t *tmp = result->remain;
		result->remain = result->fill;
		result->fill = tmp;
	}
}

static rnd_cardinal_t trace_contour(pcb_board_t *pcb, pcb_tlp_session_t *result, int tool_idx, rnd_coord_t extra_offs)
{
	pcb_poly_it_t it;
	pcb_polyarea_t *pa;
	rnd_coord_t tool_dia = result->tools->dia[tool_idx];
	rnd_cardinal_t cnt = 0;
	
	for(pa = pcb_poly_island_first(result->fill, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		pcb_pline_t *pl = pcb_poly_contour(&it);
		if (pl != NULL) { /* we have a contour */
			pcb_pline_to_lines(result->res_path, pl, tool_dia + extra_offs, 0, pcb_no_flags());
			cnt++;
			for(pl = pcb_poly_hole_first(&it); pl != NULL; pl = pcb_poly_hole_next(&it)) {
				pcb_pline_to_lines(result->res_path, pl, tool_dia + extra_offs, 0, pcb_no_flags());
				cnt++;
			}
		}
	}
	return cnt;
}

static long trace_spiral(pcb_board_t *pcb, pcb_tlp_session_t *result, int tool_idx, rnd_coord_t extra_offs, long passes)
{
	long pass = 0;
	rnd_coord_t tool_dia = result->tools->dia[tool_idx];

	for(;;) {
		if ((passes > 0) && (pass >= passes))
			return pass;
		if (trace_contour(pcb, result, tool_idx, extra_offs) <= 0)
			return pass;
		extra_offs += tool_dia;
		pass++;
	}
}

/*** remove cuts that would cut into remaining copper ***/
typedef struct {
	pcb_tlp_session_t *result;
	pcb_any_obj_t *cut;
} pline_ctx_t;

pcb_rtree_dir_t fix_overcuts_in_seg(void *ctx_, void *obj, const pcb_rtree_box_t *box)
{
	pline_ctx_t *ctx = ctx_;
	pcb_line_t *l = (pcb_line_t *)ctx->cut, lo, vl;
	rnd_box_t ip;
	double offs[2], ox, oy, vx, vy, len, r;

	assert(ctx->cut->type == PCB_OBJ_LINE); /* need to handle arc later */

	pcb_polyarea_get_tree_seg(obj, &vl.Point1.X, &vl.Point1.Y, &vl.Point2.X, &vl.Point2.Y);

	/* ox;oy: normal vector scaled to thickness/2 */
	vx = l->Point2.X - l->Point1.X;
	vy = l->Point2.Y - l->Point1.Y;
	len = sqrt(vx*vx + vy*vy);
	r = (double)l->Thickness/2.0 - 500;
	vx /= len;
	vy /= len;
	ox = vy * r;
	oy = -vx * r;

	/* check shifted edges for intersection */
	lo.Point1.X = l->Point1.X + ox + vx*500; lo.Point1.Y = l->Point1.Y + oy + vy*500;
	lo.Point2.X = l->Point2.X + ox - vx*500; lo.Point2.Y = l->Point2.Y + oy - vy*500;
	if (pcb_intersect_cline_cline(&lo, &vl, &ip, offs)) {
/*
		pcb_layer_t *ly = &PCB->Data->Layer[1];
		pcb_line_new(ly, l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y, 1, 0, pcb_no_flags());
		pcb_line_new(ly, lo.Point1.X, lo.Point1.Y, lo.Point2.X, lo.Point2.Y, 1, 0, pcb_no_flags());
*/
		return pcb_RTREE_DIR_FOUND | pcb_RTREE_DIR_STOP;
	}

	lo.Point1.X = l->Point1.X - ox + vx*500; lo.Point1.Y = l->Point1.Y - oy + vy*500;
	lo.Point2.X = l->Point2.X - ox - vx*500; lo.Point2.Y = l->Point2.Y - oy - vy*500;
	if (pcb_intersect_cline_cline(&lo, &vl, &ip, offs)) {
/*
		pcb_layer_t *ly = &PCB->Data->Layer[1];
		pcb_line_new(ly, l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y, 1, 0, pcb_no_flags());
		pcb_line_new(ly, lo.Point1.X, lo.Point1.Y, lo.Point2.X, lo.Point2.Y, 1, 0, pcb_no_flags());
*/
		return pcb_RTREE_DIR_FOUND | pcb_RTREE_DIR_STOP;
	}

	return 0;
}

pcb_rtree_dir_t fix_overcuts_in_pline(void *ctx_, void *obj, const pcb_rtree_box_t *box)
{
	pline_ctx_t *ctx = ctx_;
	pcb_pline_t *pl = obj;

	return pcb_rtree_search_obj(pl->tree, (const pcb_rtree_box_t *)&ctx->cut->BoundingBox, fix_overcuts_in_seg, ctx);
}

static long fix_overcuts(pcb_board_t *pcb, pcb_tlp_session_t *result)
{
	pcb_line_t *line;
	gdl_iterator_t it;
	pline_ctx_t pctx;
	long error = 0;

	pctx.result = result;

	linelist_foreach(&result->res_path->Line, &it, line) {
		pcb_polyarea_t *pa;

		pa = result->remain->Clipped;
		do {
			pcb_rtree_dir_t dir;

			pctx.cut = (pcb_any_obj_t *)line;

			if (pa == NULL)
				continue;
			dir = pcb_rtree_search_obj(pa->contour_tree, (const pcb_rtree_box_t *)&line->BoundingBox, fix_overcuts_in_pline, &pctx);
			if (dir & pcb_RTREE_DIR_FOUND) { /* line crosses poly */
				error++;
				pcb_line_free(line);
				line = NULL;
			}
			else {  /* check endpoints only if side didn't intersect */
				pcb_polyarea_t *c;
				rnd_coord_t r = (line->Thickness-1)/2 - 1000;
				int within = 0;

				c = pcb_poly_from_circle(line->Point1.X, line->Point1.Y, r);
				within |= pcb_polyarea_touching(pa, c);
				pcb_polyarea_free(&c);

				if (!within) {
					c = pcb_poly_from_circle(line->Point2.X, line->Point2.Y, r);
					if (!within)
						within |= pcb_polyarea_touching(pa, c);
					pcb_polyarea_free(&c);
				}

				if (within) {
/*					error++; end circle intersection is normally harmless */
					pcb_line_free(line);
					line = NULL;
				}
			}
			
			if (line == NULL)
				break;
			pa = pa->f;
		} while(pa != result->remain->Clipped);
	}
	return error;
}


int pcb_tlp_mill_copper_layer(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp)
{
	long rem;

	setup_ui_layers(pcb, result, grp);
	setup_remove_poly(pcb, result, grp, -1);

	trace_contour(pcb, result, 0, 1000);

	rem = fix_overcuts(pcb, result);
	if (rem != 0)
		rnd_message(PCB_MSG_WARNING, "toolpath: had to remove %ld cuts, there might be short circuits;\ncheck your clearance vs. tool size!\n", rem);

	return 0;
}

#define req_setup(target) \
	if (setup != target) { \
		if (target) \
			rnd_message(PCB_MSG_ERROR, "millpath script: need to call a setup_* function before milling"); \
		else \
			rnd_message(PCB_MSG_ERROR, "millpath script: can not call multiple setup_* functions"); \
		continue; \
	}

int pcb_tlp_mill_script(pcb_board_t *pcb, pcb_tlp_session_t *result, pcb_layergrp_t *grp, const char *script)
{
	int setup = 0;
	setup_ui_layers(pcb, result, grp);

	for(;;) {
		size_t consumed;
		char **argv;
		int argc;

		while(isspace(*script) || (*script == ';')) script++;
		if (*script == '\0') break;

		argc = qparse3(script, &argv, QPARSE_DOUBLE_QUOTE | QPARSE_SINGLE_QUOTE | QPARSE_TERM_SEMICOLON | QPARSE_TERM_NEWLINE | QPARSE_SEP_COMMA, &consumed);

		if (strcmp(argv[0], "setup_negative") == 0) {
			req_setup(0);
			setup_remove_poly(pcb, result, grp, -1);
			setup = 1;
		}
		if (strcmp(argv[0], "setup_positive") == 0) {
			req_setup(0);
			setup_remove_poly(pcb, result, grp, +1);
			setup = 1;
		}
		else if (strcmp(argv[0], "trace_contour") == 0) {
			int tool = 0;
			rnd_coord_t extra = 1000;
			req_setup(1);
			if (argc > 1) tool = atoi(argv[1]);
			if (argc > 2) extra = pcb_get_value(argv[2], NULL, NULL, NULL);
			trace_contour(pcb, result, tool, extra);
		}
		else if (strcmp(argv[0], "trace_spiral") == 0) {
			long passes = -1;
			int tool = 0;
			rnd_coord_t extra = 1000;
			req_setup(1);
			if (argc > 1) tool = atoi(argv[1]);
			if (argc > 2) extra = pcb_get_value(argv[2], NULL, NULL, NULL);
			if (argc > 3) passes = strtol(argv[3], NULL, 10);
			trace_spiral(pcb, result, tool, extra, passes);
		}
		else if (strcmp(argv[0], "fix_overcuts") == 0) {
			long rem = fix_overcuts(pcb, result);
			req_setup(1);
			if (rem != 0)
				rnd_message(PCB_MSG_WARNING, "toolpath: had to remove %ld cuts, there might be short circuits;\ncheck your clearance vs. tool size!\n", rem);
		}

		qparse_free(argc, &argv);
		script += consumed;
	}
	return 0;
}

