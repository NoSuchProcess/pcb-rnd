/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  Copyright (C) 2018,2019,2020 Tibor 'Igor2' Palinkas
 *
 *  This module, report.c, was written and is Copyright (C) 1997 harry eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */


#include "config.h"

#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include "src_plugins/query/net_len.h"

#include <math.h>

#include "report.h"
#include <librnd/core/math_helper.h>
#include "crosshair.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "drill.h"
#include <librnd/core/error.h>
#include "search.h"
#include <librnd/poly/rtree.h>
#include "flag_str.h"
#include "undo.h"
#include "find.h"
#include "draw.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/misc_util.h>
#include "report_conf.h"
#include <librnd/core/compat_misc.h>
#include "layer.h"
#include "layer_ui.h"
#include "obj_term.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_parent.h"
#include <librnd/core/hid_dad.h>
#include "netlist.h"

#include <genregex/regex_sei.h>

conf_report_t conf_report;

static const char pcb_acts_Report[] =
	"Report([DrillReport|FoundPins|NetLengthTo])\n"
	"Report(NetLength, [netname])\n"
	"Report(Object|Subc, [log])\n"
	"Report(AllNetLengths, [unit])";
static const char pcb_acth_Report[] = "Produce various report.";

#define USER_UNITMASK (rnd_conf.editor.grid_unit->allow)


typedef struct rdialog_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
} rdialog_ctx_t;

static void rdialog_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	rdialog_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void rdialog(const char *name, const char *content)
{
	rdialog_ctx_t *ctx = calloc(sizeof(rdialog_ctx_t), 1);
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_LABEL(ctx->dlg, content);
		RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	RND_DAD_END(ctx->dlg);

	RND_DAD_NEW("report", ctx->dlg, name, ctx, rnd_false, rdialog_close_cb);
}


static int report_drills(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_drill_info_t *AllDrills;
	rnd_cardinal_t n;
	char *stringlist, *thestring;
	int total_drills = 0;

	AllDrills = drill_get_info(PCB->Data);
	drill_round_info(AllDrills, 100);

	for (n = 0; n < AllDrills->DrillN; n++) {
		total_drills += AllDrills->Drill[n].PinCount;
		total_drills += AllDrills->Drill[n].ViaCount;
		total_drills += AllDrills->Drill[n].UnplatedCount;
	}

	stringlist = (char *) malloc(512L + AllDrills->DrillN * 64L);

	/* Use tabs for formatting since can't count on a fixed font anymore.
	   And even that probably isn't going to work in all cases. */
	sprintf(stringlist,
					"There are %d different drill sizes used in this layout, %d holes total\n\n"
					"Drill Diam. (%s)\t# of Pins\t# of Vias\t# of Elements\t# Unplated\n",
					AllDrills->DrillN, total_drills, rnd_conf.editor.grid_unit->suffix);
	thestring = stringlist;
	while (*thestring != '\0')
		thestring++;
	for (n = 0; n < AllDrills->DrillN; n++) {
		rnd_sprintf(thestring,
								"%10m*\t\t\t%d\t\t%d\t\t%d\t\t\t\t%d\n",
								rnd_conf.editor.grid_unit->suffix,
								AllDrills->Drill[n].DrillSize,
								AllDrills->Drill[n].PinCount, AllDrills->Drill[n].ViaCount,
								AllDrills->Drill[n].ElementN, AllDrills->Drill[n].UnplatedCount);
		while (*thestring != '\0')
			thestring++;
	}
	drill_info_free(AllDrills);

	/* create dialog box */
	rdialog("Drill Report", stringlist);

	free(stringlist);
	return 0;
}


static const char pcb_acts_reportdialog[] = "ReportObject()";
static const char pcb_acth_reportdialog[] = "Report on the object under the crosshair";
/* DOC: reportdialog.html */

#define gen_locked(obj) (PCB_FLAG_TEST(PCB_FLAG_LOCK, obj) ? "It is LOCKED.\n" : "")
#define gen_term(obj) \
	(((obj)->term != NULL) ? "It is terminal " : ""), \
	(((obj)->term != NULL) ? (obj)->term : ""), \
	(((obj)->term != NULL) ? "\n" : "")

static const char *grpname(rnd_layergrp_id_t gid)
{
	pcb_layergrp_t *grp = pcb_get_layergrp(PCB, gid);
	if (grp == NULL)
		return "<invalid>";
	if ((grp->name == NULL) || (*grp->name == '\0'))
		return "<anonymous>";
	return grp->name;
}

static void report_pstk(gds_t *dst, pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto;

#ifndef NDEBUG
	if (rnd_gui->shift_is_pressed(rnd_gui))
		rnd_r_dump_tree(PCB->Data->padstack_tree, 0);
#endif
	proto = pcb_pstk_get_proto(ps);

	rnd_append_printf(dst, "%m+PADSTACK ID# %ld; Flags:%s\n"
		"(X,Y) = %$mD.\n", USER_UNITMASK, ps->ID, pcb_strflg_f2s(ps->Flags, PCB_OBJ_PSTK, NULL, 0),
		ps->x, ps->y);

	if ((proto != NULL) && (proto->hdia > 0))
		rnd_append_printf(dst, "%m+Hole diameter: %$mS", USER_UNITMASK, proto->hdia);

	rnd_append_printf(dst, "\n%s%s%s", gen_locked(ps), gen_term(ps));
}

static void report_line(gds_t *dst, pcb_line_t *line)
{
#ifndef NDEBUG
	if (rnd_gui->shift_is_pressed(rnd_gui))
		rnd_r_dump_tree(line->parent.layer->line_tree, 0);
#endif
	rnd_append_printf(dst, "%m+LINE ID# %ld;  Flags:%s\n"
		"FirstPoint(X,Y)  = %$mD, ID = %ld.\n"
		"SecondPoint(X,Y) = %$mD, ID = %ld.\n"
		"Width = %$mS.\nClearance = %$mS.\n"
		"It is on layer %d\n"
		"and has name \"%s\".\n"
		"%s"
		"%s%s", USER_UNITMASK,
		line->ID, pcb_strflg_f2s(line->Flags, PCB_OBJ_LINE, NULL, 0),
		line->Point1.X, line->Point1.Y, line->Point1.ID,
		line->Point2.X, line->Point2.Y, line->Point2.ID,
		line->Thickness, line->Clearance / 2,
		pcb_layer_id(PCB->Data, line->parent.layer),
		gen_locked(line), gen_term(line));
}

static void report_rat(gds_t *dst, pcb_rat_t *line)
{
	char *anchor1 = "n/a", *anchor2 = "n/a";
#ifndef NDEBUG
	if (rnd_gui->shift_is_pressed(rnd_gui))
		rnd_r_dump_tree(PCB->Data->rat_tree, 0);
#endif
	rnd_append_printf(dst, "%m+RAT-LINE ID# %ld;  Flags:%s\n"
		"FirstPoint(X,Y)  = %$mD; ID = %ld; "
		"connects to layer group #%d (%s).\n"
		"SecondPoint(X,Y) = %$mD; ID = %ld; "
		"connects to layer group #%d (%s).\n",
		USER_UNITMASK, line->ID, pcb_strflg_f2s(line->Flags, PCB_OBJ_LINE, NULL, 0),
		line->Point1.X, line->Point1.Y, line->Point1.ID, line->group1, grpname(line->group1),
		line->Point2.X, line->Point2.Y, line->Point2.ID, line->group2, grpname(line->group2));

	if ((line->anchor[0] != NULL) && (line->anchor[0]->len > 0))
		anchor1 = pcb_idpath2str(line->anchor[0], 0);
	if ((line->anchor[1] != NULL) && (line->anchor[1]->len > 0))
		anchor2 = pcb_idpath2str(line->anchor[1], 0);

	rnd_append_printf(dst, "Anchors: %s <-> %s\n", anchor1, anchor2);

}

static void report_arc(gds_t *dst, pcb_arc_t *arc)
{
	rnd_box_t box;
#ifndef NDEBUG
	if (rnd_gui->shift_is_pressed(rnd_gui))
		rnd_r_dump_tree(arc->parent.layer->arc_tree, 0);
#endif
	pcb_arc_get_end(arc, 0, &box.X1, &box.Y1);
	pcb_arc_get_end(arc, 1, &box.X2, &box.Y2);

	rnd_append_printf(dst, "%m+ARC ID# %ld;  Flags:%s\n"
		"CenterPoint(X,Y) = %$mD.\n"
		"Width = %$mS.\nClearance = %$mS.\n"
		"Radius = %$mS, StartAngle = %ma degrees, DeltaAngle = %ma degrees.\n"
		"Bounding Box is %$mD, %$mD.\n"
		"That makes the end points at %$mD and %$mD.\n"
		"It is on layer %d.\n"
		"%s"
		"%s%s%s", USER_UNITMASK, arc->ID, pcb_strflg_f2s(arc->Flags, PCB_OBJ_ARC, NULL, 0),
		arc->X, arc->Y,
		arc->Thickness, arc->Clearance / 2,
		arc->Width, arc->StartAngle, arc->Delta,
		arc->BoundingBox.X1, arc->BoundingBox.Y1,
		arc->BoundingBox.X2, arc->BoundingBox.Y2,
		box.X1, box.Y1,
		box.X2, box.Y2,
		pcb_layer_id(PCB->Data, arc->parent.layer), gen_locked(arc), gen_term(arc));
}

static void report_poly(gds_t *dst, pcb_poly_t *poly)
{
	const char *aunit;
	double area, u;

#ifndef NDEBUG
	if (rnd_gui->shift_is_pressed(rnd_gui))
		rnd_r_dump_tree(poly->parent.layer->polygon_tree, 0);
#endif

	aunit = rnd_conf.editor.grid_unit->suffix;
	if (aunit[1] == 'm')
		u = RND_MM_TO_COORD(1);
	else
		u = RND_MIL_TO_COORD(1);

	area = pcb_poly_area(poly);
	area = area / u;
	area = area / u;

	rnd_append_printf(dst, "%m+POLYGON ID# %ld;  Flags:%s\n"
		"Its bounding box is %$mD %$mD.\n"
		"It has %d points and could store %d more\n"
		"  without using more memory.\n"
		"It has %d holes and resides on layer %d.\n"
		"Its unclipped area is %f square %s.\n", USER_UNITMASK, poly->ID,
		pcb_strflg_f2s(poly->Flags, PCB_OBJ_POLY, NULL, 0),
		poly->BoundingBox.X1, poly->BoundingBox.Y1,
		poly->BoundingBox.X2, poly->BoundingBox.Y2,
		poly->PointN, poly->PointMax - poly->PointN,
		poly->HoleIndexN,
		pcb_layer_id(PCB->Data, poly->parent.layer),
		area, aunit);

	if (poly->enforce_clearance > 0)
		rnd_append_printf(dst, "%m+Polygon enforced min. clearance: %$mS\n", USER_UNITMASK, poly->enforce_clearance);

	rnd_append_printf(dst, "%s %s%s%s", gen_locked(poly), gen_term(poly));
}

static void report_subc(gds_t *dst, pcb_subc_t *subc)
{
#ifndef NDEBUG
	if (rnd_gui->shift_is_pressed(rnd_gui))
		rnd_r_dump_tree(PCB->Data->subc_tree, 0);
#endif
	rnd_append_printf(dst, "%m+SUBCIRCUIT ID# %ld;  Flags:%s\n"
		"BoundingBox %$mD %$mD.\n"
		"Refdes \"%s\".\n"
		"Footprint \"%s\".\n"
		"%s", USER_UNITMASK,
		subc->ID, pcb_strflg_f2s(subc->Flags, PCB_OBJ_SUBC, NULL, 0),
		subc->BoundingBox.X1, subc->BoundingBox.Y1,
		subc->BoundingBox.X2, subc->BoundingBox.Y2,
		RND_EMPTY(subc->refdes),
		RND_EMPTY(pcb_attribute_get(&subc->Attributes, "footprint")),
		gen_locked(subc));
}

static void report_text(gds_t *dst, pcb_text_t *text)
{
#ifndef NDEBUG
		if (rnd_gui->shift_is_pressed(rnd_gui))
			rnd_r_dump_tree(text->parent.layer->text_tree, 0);
#endif

	rnd_append_printf(dst, "%m+TEXT ID# %ld;  Flags:%s\n"
		"BoundingBox %$mD %$mD.\n"
		"Font id %d\nclearance %$mS\nthickness %$mS\nrotation %f\nScale: %d %f;%f\n"
		, USER_UNITMASK,
		text->ID, pcb_strflg_f2s(text->Flags, PCB_OBJ_TEXT, NULL, 0),
		text->BoundingBox.X1, text->BoundingBox.Y1,
		text->BoundingBox.X2, text->BoundingBox.Y2,
		text->fid, text->clearance, text->thickness, text->rot,
		text->Scale, text->scale_x, text->scale_y);
}

static void report_gfx(gds_t *dst, pcb_gfx_t *gfx)
{
#ifndef NDEBUG
		if (rnd_gui->shift_is_pressed(rnd_gui))
			rnd_r_dump_tree(gfx->parent.layer->gfx_tree, 0);
#endif

	rnd_append_printf(dst, "%m+GFX ID# %ld;  Flags:%s\n"
		"BoundingBox %$mD %$mD.\n"
		"Center %$mD\nSize %$mD\nrotation %f\n"
		, USER_UNITMASK,
		gfx->ID, pcb_strflg_f2s(gfx->Flags, PCB_OBJ_GFX, NULL, 0),
		gfx->BoundingBox.X1, gfx->BoundingBox.Y1,
		gfx->BoundingBox.X2, gfx->BoundingBox.Y2,
		gfx->cx, gfx->cy, gfx->sx, gfx->sx, gfx->rot);
}

static void report_point(gds_t *dst, int type, pcb_layer_t *layer, rnd_point_t *point)
{
	rnd_append_printf(dst, "%m+POINT ID# %ld.\n"
		"Located at (X,Y) = %$mD.\n"
		"It belongs to a %s on layer %d.\n", USER_UNITMASK, point->ID,
		point->X, point->Y,
		(type == PCB_OBJ_LINE_POINT) ? "line" : "polygon", pcb_layer_id(PCB->Data, layer));
}


static fgw_error_t pcb_act_report_dialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	gds_t tmp;
	void *ptr1, *ptr2, *ptr3;
	int type = REPORT_TYPES;
	char *op = NULL, *how = NULL;
	pcb_subc_t *subc;
	rnd_coord_t x, y;
	rnd_hid_get_coords("Click on object to report on", &x, &y, 0);

	RND_ACT_MAY_CONVARG(1, FGW_STR, reportdialog, op = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, reportdialog, how = argv[2].val.str);

	gds_init(&tmp);

	if (op != NULL) {
		if (rnd_strncasecmp(op, "Subc", 4) == 0)
			type = PCB_OBJ_SUBC;
	}
	type = pcb_search_screen(x, y, type, &ptr1, &ptr2, &ptr3);
	if ((type == PCB_OBJ_VOID) && (op == NULL))
		type = pcb_search_screen(x, y, REPORT_TYPES | PCB_OBJ_LOCKED, &ptr1, &ptr2, &ptr3);

	switch (type) {
		case PCB_OBJ_PSTK: report_pstk(&tmp, ptr2); break;
		case PCB_OBJ_LINE: report_line(&tmp, ptr2); break;
		case PCB_OBJ_RAT:  report_rat(&tmp, ptr2); break;
		case PCB_OBJ_ARC:  report_arc(&tmp, ptr2); break;
		case PCB_OBJ_POLY: report_poly(&tmp, ptr2); break;
		case PCB_OBJ_SUBC: report_subc(&tmp, ptr2); break;
		case PCB_OBJ_TEXT: report_text(&tmp, ptr2); break;
		case PCB_OBJ_GFX:  report_gfx(&tmp, ptr2); break;
		case PCB_OBJ_LINE_POINT:
		case PCB_OBJ_POLY_POINT: report_point(&tmp, type, ptr1, ptr2); break;
		case PCB_OBJ_VOID:
			rnd_message(RND_MSG_INFO, "Nothing found to report on\n");
			RND_ACT_IRES(1);
			return 0;
		default: rnd_append_printf(&tmp, "Unknown\n"); break;
	}

	subc = pcb_obj_parent_subc((pcb_any_obj_t *)ptr2);
	if (subc != NULL)
		rnd_append_printf(&tmp, "\nPart of subcircuit #%ld\n", subc->ID);

	{
		pcb_idpath_t *idp = pcb_obj2idpath((pcb_any_obj_t *)ptr2);
		gds_append_str(&tmp, "\nidpath: ");
		pcb_append_idpath(&tmp, idp, 0);
		pcb_idpath_destroy(idp);
	}

	/* create dialog box */
	if ((how != NULL) && (strcmp(how, "log") == 0))
		rnd_message(RND_MSG_INFO, "--- Report ---\n%s---\n", tmp.array);
	else
		rdialog("Report", tmp.array);
	gds_uninit(&tmp);

	RND_ACT_IRES(0);
	return 0;
}

static int report_found_pins(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	gds_t list;
	int col = 0;
	gdl_iterator_t sit;
	pcb_subc_t *subc;

	gds_init(&list);
	gds_append_str(&list, "The following terminals are FOUND:\n");

	subclist_foreach(&PCB->Data->subc, &sit, subc) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it))
			if ((o->term != NULL) && (PCB_FLAG_TEST(PCB_FLAG_FOUND, o)))
				rnd_append_printf(&list, "%s-%s,%c", subc->refdes, o->term, ((col++ % (conf_report.plugins.report.columns + 1)) == conf_report.plugins.report.columns) ? '\n' : ' ');
	}

	rdialog("Report", list.array);
	gds_uninit(&list);
	return 0;
}

static double xy_to_net_length(pcb_board_t *pcb, rnd_coord_t x, rnd_coord_t y, int *found, gds_t *err)
{
	long n, type, terms = 0;
	void *p1, *p2, *p3;
	pcb_qry_netseg_len_t *nl;
	pcb_qry_exec_t ec;
	double ret = -1;
	pcb_net_t *onet = NULL;

	type = pcb_search_screen(x, y, PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_PSTK, &p1, &p2, &p3);
	if (type == PCB_OBJ_VOID) {
		rnd_append_printf(err, "\nno suitable starting object there!");
		*found = 0;
		return -1;
	}

	pcb_qry_init(&ec, pcb, NULL, -1);
	nl = pcb_qry_parent_net_lenseg(&ec, (pcb_any_obj_t *)p2);
	ret = nl->len;
	*found = nl->objs.used;

	for(n = 0; n < nl->objs.used; n++) {
		pcb_net_term_t *t = pcb_net_find_by_obj(&pcb->netlist[PCB_NETLIST_EDITED], nl->objs.array[n]);
		if (t != NULL) {
			pcb_net_t *net = t->parent.net;
			assert(t->parent_type == PCB_PARENT_NET);
			if (net != NULL) {
				if (onet != net)
					rnd_append_printf(err, "\nterminals or other networks are connected (shorted)");
				onet = net;
				terms++;
			}
		}
	}

	if (onet != NULL) {
		long explen = pcb_termlist_length(&onet->conns);
		if (explen != terms)
			rnd_append_printf(err, "\nonly %ld terminals of the %ld on the network are connected!", terms, explen);
	}

	if (nl->has_nontrace)
		rnd_append_printf(err, "\n%Partial result: polygons/texts blocked the search");
	else if (nl->has_junction)
		rnd_append_printf(err, "\n%Partial result: a junction blocked the search");

	pcb_qry_uninit(&ec);

	return ret;
}

static int report_all_net_lengths(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int found;
	htsp_entry_t *e;

	for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
		pcb_net_t *net = e->value;
		pcb_net_term_t *t;
		pcb_any_obj_t *term;

		/* find the first terminal object referenced from the net that actually exist */
		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
			term = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
			if (term != NULL)
				break;
		}

		if (term != NULL) {
			char buf[50];
			const char *units_name = rnd_conf.editor.grid_unit->suffix;
			rnd_coord_t length;
			rnd_coord_t x = 0, y = 0;
			gds_t err;

			RND_ACT_MAY_CONVARG(2, FGW_STR, Report, units_name = argv[2].val.str);

			pcb_obj_center(term, &x, &y);

			gds_init(&err);
			length = xy_to_net_length(PCB_ACT_BOARD, x, y, &found, &err);

			rnd_snprintf(buf, sizeof(buf), "%$m*", units_name, length);
			if (err.used != 0)
				rnd_message(RND_MSG_INFO, "Net %s length %s, BUT BEWARE:%s\n", net->name, buf, err.array);
			else
					rnd_message(RND_MSG_INFO, "Net %s length %s\n", net->name, buf);
			gds_uninit(&err);
		}
	}

	return 0;
}

/* find any found object with a terminal ID and return the associated netname */
static const char *net_name_found(pcb_board_t *pcb)
{
	PCB_SUBC_LOOP(pcb->Data);
	{
		pcb_any_obj_t *o;
		pcb_data_it_t it;

		if (subc->refdes == NULL)
			continue;

		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			pcb_net_term_t *t;

			if (o->term == NULL)
				continue;
			if (!PCB_FLAG_TEST(PCB_FLAG_FOUND, o))
				continue;

			t = pcb_net_find_by_refdes_term(&PCB->netlist[PCB_NETLIST_EDITED], subc->refdes, o->term);
			if ((t != NULL) && (t->parent.net != NULL))
				return t->parent.net->name;
		}
	}
	PCB_END_LOOP;
	return NULL;
}

static int report_net_length_(fgw_arg_t *res, int argc, fgw_arg_t *argv, rnd_coord_t x, rnd_coord_t y)
{
	rnd_coord_t length = 0;
	int found = 0, ret;
	gds_t err;

	gds_init(&err);
	length = xy_to_net_length(PCB_ACT_BOARD, x, y, &found, &err);

	if (found) {
		char buf[50];
		const char *netname = net_name_found(PCB);

		rnd_snprintf(buf, sizeof(buf), "%$m*", rnd_conf.editor.grid_unit->suffix, length);
		if (netname)
			rnd_message(RND_MSG_INFO, "Net \"%s\" length: %s\n", netname, buf);
		else
			rnd_message(RND_MSG_INFO, "Net length: %s\n", buf);

		if (err.used != 0)
			rnd_message(RND_MSG_INFO, "BUT BEWARE: %s\n", err.array);

		ret = 0;
	}
	else {
		rnd_message(RND_MSG_ERROR, "No net under cursor.\n");
		ret = 1;
	}

	gds_uninit(&err);
	return ret;
}

static int report_net_length(fgw_arg_t *res, int argc, fgw_arg_t *argv, int split)
{

	if (split) {
		void *r1, *r2, *r3;
		pcb_line_t *l;
		pcb_layer_t *ly;
		int type;
		rnd_coord_t ox, oy, x, y;

		rnd_hid_get_coords("Click on a copper line", &x, &y, 0);

		type = pcb_search_screen(x, y, PCB_OBJ_LINE, &r1, &r2, &r3);
		if (type != PCB_OBJ_LINE) {
			rnd_message(RND_MSG_ERROR, "can't find a line to split\n");
			return -1;
		}
		l = r2;
		assert(l->parent_type == PCB_PARENT_LAYER);
		ly = l->parent.layer;
		if (!(pcb_layer_flags_(ly) & PCB_LYT_COPPER)) {
			rnd_message(RND_MSG_ERROR, "not a copper line, can't split it\n");
			return -1;
		}

#define MINDIST RND_MIL_TO_COORD(40)
		if ((rnd_distance(l->Point1.X, l->Point1.Y, x, y) < MINDIST) || (rnd_distance(l->Point2.X, l->Point2.Y, x, y) < MINDIST)) {
			rnd_message(RND_MSG_ERROR, "Can not split near the endpoint of a line\n");
			return -1;
		}
#undef MINDIST2

		rnd_message(RND_MSG_INFO, "The two arms of the net are:\n");
		rnd_r_delete_entry(ly->line_tree, (rnd_box_t *)l);
		ox = l->Point1.X; oy = l->Point1.Y; l->Point1.X = x; l->Point1.Y = y;
		rnd_r_insert_entry(ly->line_tree, (rnd_box_t *)l);
		report_net_length_(res, argc, argv, x, y);
		rnd_r_delete_entry(ly->line_tree, (rnd_box_t *)l);
		l->Point1.X = ox; l->Point1.Y = oy;
		rnd_r_insert_entry(ly->line_tree, (rnd_box_t *)l);

		rnd_r_delete_entry(ly->line_tree, (rnd_box_t *)l);
		ox = l->Point2.X; oy = l->Point2.Y; l->Point2.X = x; l->Point2.Y = y;
		rnd_r_insert_entry(ly->line_tree, (rnd_box_t *)l);
		report_net_length_(res, argc, argv, x, y);
		rnd_r_delete_entry(ly->line_tree, (rnd_box_t *)l);
		l->Point2.X = ox; l->Point2.Y = oy;
		rnd_r_insert_entry(ly->line_tree, (rnd_box_t *)l);

		PCB_FLAG_SET(PCB_FLAG_SELECTED, l);

		return 0;
	}
	else {
		rnd_coord_t x, y;
		rnd_hid_get_coords("Click on a network", &x, &y, 0);
		return report_net_length_(res, argc, argv, x, y);
	}
}

static int report_net_length_by_name(pcb_board_t *pcb, const char *tofind)
{
	const char *netname = NULL;
	pcb_net_t *net;
	rnd_coord_t length = 0;
	int found = 0;
	rnd_coord_t x, y;
	gds_t err;

	if (pcb == NULL)
		return 1;

	if (!tofind)
		return 1;

	{
		net = pcb_net_get_user(pcb, &pcb->netlist[PCB_NETLIST_EDITED], tofind);
		if (net != NULL) {
			pcb_net_term_t *term;
			pcb_any_obj_t *obj = NULL;

			netname = net->name;
			term = pcb_termlist_first(&net->conns);
			if (term == NULL) {
				rnd_message(RND_MSG_INFO, "Net found, but it has not terminals.\n");
				return 1;
			}

			obj = pcb_term_find_name(pcb, pcb->Data, PCB_LYT_COPPER, term->refdes, term->term, NULL, NULL);
			if (obj == NULL) {
				rnd_message(RND_MSG_INFO, "Net found, but its terminal %s-%s is not on the board.\n", term->refdes, term->term);
				return 1;
			}
			pcb_obj_center(obj, &x, &y);
		}
	}

	if (netname == NULL) {
		rnd_message(RND_MSG_ERROR, "No net named %s\n", tofind);
		return 1;
	}


	gds_init(&err);
	length = xy_to_net_length(pcb, x, y, &found, &err);

	if (!found) {
		if (netname != NULL)
			rnd_message(RND_MSG_INFO, "Net found, but no lines or arcs were flagged.\n");
		else
			rnd_message(RND_MSG_ERROR, "Net not found.\n");

		gds_uninit(&err);
		return 1;
	}

	{
		char buf[50];
		rnd_snprintf(buf, sizeof(buf), "%$m*", rnd_conf.editor.grid_unit->suffix, length);
		if (netname)
			rnd_message(RND_MSG_INFO, "Net \"%s\" length: %s\n", netname, buf);
		else
			rnd_message(RND_MSG_INFO, "Net length: %s\n", buf);
		if (err.used != 0)
			rnd_message(RND_MSG_INFO, "BUT BEWARE: %s\n", err.array);
	}

	gds_uninit(&err);
	return 0;
}

/* DOC: report.html */
static fgw_error_t pcb_act_Report(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *name;
	rnd_coord_t x, y;

	RND_ACT_CONVARG(1, FGW_STR, Report, cmd = argv[1].val.str);

	if (rnd_strcasecmp(cmd, "Object") == 0) {
		rnd_hid_get_coords("Click on an object", &x, &y, 0);
		return pcb_act_report_dialog(res, argc, argv);
	}
	else if (rnd_strncasecmp(cmd, "Subc", 4) == 0) {
		rnd_hid_get_coords("Click on a subcircuit", &x, &y, 0);
		return pcb_act_report_dialog(res, argc, argv);
	}

	rnd_hid_get_coords("Click on object to report on", &x, &y, 0);

	if (rnd_strcasecmp(cmd, "DrillReport") == 0)
		return report_drills(res, argc, argv);
	else if (rnd_strcasecmp(cmd, "FoundPins") == 0)
		return report_found_pins(res, argc, argv);
	else if ((rnd_strcasecmp(cmd, "NetLength") == 0) && (argc == 2))
		return report_net_length(res, argc, argv, 0);
	else if ((rnd_strcasecmp(cmd, "NetLengthTo") == 0) && (argc == 2))
		return report_net_length(res, argc, argv, 1);
	else if (rnd_strcasecmp(cmd, "AllNetLengths") == 0)
		return report_all_net_lengths(res, argc, argv);
	else if (rnd_strcasecmp(cmd, "NetLength") == 0) {
		RND_ACT_CONVARG(2, FGW_STR, Report, name = argv[2].val.str);
		return report_net_length_by_name(PCB_ACT_BOARD, name);
	}

	RND_ACT_FAIL(Report);
}

static fgw_error_t pcb_act_info(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int i, j;
	if (!PCB || !PCB->Data || !PCB->hidlib.filename) {
		printf("No PCB loaded.\n");
		return 0;
	}
	printf("Filename: %s\n", PCB->hidlib.filename);
	rnd_printf("Size: %ml x %ml mils, %mm x %mm mm\n", PCB->hidlib.size_x, PCB->hidlib.size_y, PCB->hidlib.size_x, PCB->hidlib.size_y);
	for (i = 0; i < PCB_MAX_LAYER; i++) {
		rnd_layergrp_id_t lg = pcb_layer_get_group(PCB, i);
		unsigned int gflg = pcb_layergrp_flags(PCB, lg);
		for (j = 0; j < PCB_MAX_LAYER; j++)
			putchar(j == lg ? '#' : '-');
		printf(" %c %s\n", (gflg & PCB_LYT_TOP) ? 'c' : (gflg & PCB_LYT_BOTTOM) ? 's' : '-', PCB->Data->Layer[i].name);
	}
	RND_ACT_IRES(0);
	return 0;
}

static void print_net_length_label(pcb_board_t *pcb, pcb_layer_t *ly, pcb_qry_netseg_len_t *nl, rnd_coord_t atx, rnd_coord_t aty, rnd_coord_t ldx, rnd_coord_t ldy, rnd_coord_t th)
{
	rnd_coord_t tx, ty;
	double dx, dy, len;
	char tmp[256];
	pcb_text_t *t;

	if ((ldx == 0) && (ldy == 0))
		ldx = ldy = 1;
	len = sqrt((double)ldx*(double)ldx + (double)ldy*(double)ldy);
	dx = (double)ldx / len; dy = (double)ldy / len;

	rnd_snprintf(tmp, sizeof(tmp), "%m+len=%.02$$mS via=%d%s", rnd_conf.editor.grid_unit->allow, nl->len, nl->num_vias, nl->has_junction ? " BAD" : "");

	tx = atx+dx*th*50; ty = aty+dy*th*50;
	t = pcb_text_new(ly, pcb_font(pcb, 0, 1), tx, ty, 0, 25, th, tmp, pcb_no_flags());
	if (nl->has_junction)
		t->override_color = rnd_clrdup(rnd_color_red);

	pcb_line_new(ly, t->bbox_naked.X1, t->bbox_naked.Y1, t->bbox_naked.X2, t->bbox_naked.Y1, th, 0, pcb_no_flags());
	pcb_line_new(ly, t->bbox_naked.X1, t->bbox_naked.Y1, t->bbox_naked.X1, t->bbox_naked.Y2, th, 0, pcb_no_flags());
	pcb_line_new(ly, t->bbox_naked.X2, t->bbox_naked.Y2, t->bbox_naked.X2, t->bbox_naked.Y1, th, 0, pcb_no_flags());
	pcb_line_new(ly, t->bbox_naked.X2, t->bbox_naked.Y2, t->bbox_naked.X1, t->bbox_naked.Y2, th, 0, pcb_no_flags());

	pcb_line_new(ly, atx, aty, t->bbox_naked.X1, t->bbox_naked.Y1, th, 0, pcb_no_flags());

}

static void print_net_length(pcb_board_t *pcb, pcb_layer_t *ly, pcb_qry_netseg_len_t *nl)
{
	pcb_any_obj_t *lo = NULL;
	rnd_coord_t ldx, ldy, atx, aty;
	long n;
	static const rnd_coord_t th = RND_MM_TO_COORD(0.025);
	for(n = 0; n < nl->objs.used; n++) {
		pcb_line_t *l = (pcb_line_t *)nl->objs.array[n];
		pcb_arc_t *a = (pcb_arc_t *)l;
		pcb_pstk_t *ps = (pcb_pstk_t *)l;

		switch(l->type) {
			case PCB_OBJ_LINE:
				pcb_line_new(ly, l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y, th, 0, pcb_no_flags());
				if (n == 0) {
					ldx = -(l->Point2.Y - l->Point1.Y); /* perpendicular to the line */
					ldy = +(l->Point2.X - l->Point1.X);
					atx = (l->Point1.X + l->Point2.X)/2;
					aty = (l->Point1.Y + l->Point2.Y)/2;
					lo = (pcb_any_obj_t *)l;
				}
				break;
			case PCB_OBJ_ARC:
				pcb_arc_new(ly, a->X, a->Y, a->Width, a->Height, a->StartAngle, a->Delta, th, 0, pcb_no_flags(), 0);
				if (n == 0) {
					ldx = -1;
					ldy = -1;
					atx = ps->x;
					aty = ps->y;
					lo = (pcb_any_obj_t *)l;
				}
				break;
			case PCB_OBJ_PSTK:
				pcb_arc_new(ly, ps->x, ps->y, th*4, th*4, 0, 360, th, 0, pcb_no_flags(), 0);
				if (n == 0) {
					
					atx = ps->x;
					aty = ps->y;
					ldx = ldy = 1;
					lo = (pcb_any_obj_t *)l;
				}
				break;
			default: break;
		}
	}

	if (lo != NULL)
		print_net_length_label(pcb, ly, nl, atx, aty, ldx, ldy, th/4);
}

static const char pcb_acts_NetLength[] =
	"NetLength(clear)\n"
	"NetLength(object)\n";
static const char pcb_acth_NetLength[] = "Report physical network length";
static fgw_error_t pcb_act_NetLength(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *cmd;
	static pcb_layer_t *ly = NULL;

	RND_ACT_CONVARG(1, FGW_STR, NetLength, cmd = argv[1].val.str);

	if (strcmp(cmd, "clear") == 0) {
		if (ly != NULL) {
			pcb_uilayer_free(ly);
			ly = NULL;
		}
		return 0;
	}

	/* anything below needs the layer */

	if (ly == NULL)
		ly = pcb_uilayer_alloc("NetLength", "Net length", rnd_color_blue);

	if (strcmp(cmd, "object") == 0) {
		int type;
		rnd_coord_t x, y;
		void *p1, *p2, *p3;
		pcb_qry_netseg_len_t *nl;
		pcb_qry_exec_t ec;

		rnd_hid_get_coords("Click on an object for net length measurement", &x, &y, 0);

		type = pcb_search_screen(x, y, PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_PSTK, &p1, &p2, &p3);
		if (type == PCB_OBJ_VOID)
			goto error;

		pcb_qry_init(&ec, pcb, NULL, -1);
		nl = pcb_qry_parent_net_lenseg(&ec, (pcb_any_obj_t *)p2);
		print_net_length(pcb, ly, nl);
		pcb_qry_uninit(&ec);
	}

	return 0;
	error:
	RND_ACT_IRES(-1);
	return 0;
}

rnd_action_t report_action_list[] = {
	{"ReportObject", pcb_act_report_dialog, pcb_acth_reportdialog, pcb_acts_reportdialog},
	{"Report", pcb_act_Report, pcb_acth_Report, pcb_acts_Report},
	{"NetLength", pcb_act_NetLength, pcb_acth_NetLength, pcb_acts_NetLength},
	{"Info", pcb_act_info}
};

static const char *report_cookie = "report plugin";

int pplg_check_ver_report(int ver_needed) { return 0; }

void pplg_uninit_report(void)
{
	rnd_remove_actions_by_cookie(report_cookie);
	rnd_conf_unreg_fields("plugins/report/");
}

int pplg_init_report(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(report_action_list, report_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_report, field,isarray,type_name,cpath,cname,desc,flags);
#include "report_conf_fields.h"
	return 0;
}
