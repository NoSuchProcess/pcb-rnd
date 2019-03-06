/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include <math.h>

#include "report.h"
#include "math_helper.h"
#include "crosshair.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "drill.h"
#include "error.h"
#include "search.h"
#include "rtree.h"
#include "flag_str.h"
#include "macro.h"
#include "undo.h"
#include "find.h"
#include "draw.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "actions.h"
#include "misc_util.h"
#include "report_conf.h"
#include "compat_misc.h"
#include "layer.h"
#include "obj_term.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_parent.h"
#include "hid_dad.h"
#include "netlist2.h"

#include <genregex/regex_sei.h>

conf_report_t conf_report;

static const char pcb_acts_Report[] = "Report(Object|DrillReport|FoundPins|NetLength|NetLengthTo|AllNetLengths|[,name])";
static const char pcb_acth_Report[] = "Produce various report.";

#define USER_UNITMASK (conf_core.editor.grid_unit->allow)


typedef struct rdialog_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
} rdialog_ctx_t;

static void rdialog_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	rdialog_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}

static void rdialog(const char *name, const char *content)
{
	rdialog_ctx_t *ctx = calloc(sizeof(rdialog_ctx_t), 1);
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_LABEL(ctx->dlg, content);
		PCB_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_NEW("report", ctx->dlg, name, ctx, pcb_false, rdialog_close_cb);
}


static int report_drills(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_drill_info_t *AllDrills;
	pcb_cardinal_t n;
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
	   |  And even that probably isn't going to work in all cases.
	 */
	sprintf(stringlist,
					"There are %d different drill sizes used in this layout, %d holes total\n\n"
					"Drill Diam. (%s)\t# of Pins\t# of Vias\t# of Elements\t# Unplated\n",
					AllDrills->DrillN, total_drills, conf_core.editor.grid_unit->suffix);
	thestring = stringlist;
	while (*thestring != '\0')
		thestring++;
	for (n = 0; n < AllDrills->DrillN; n++) {
		pcb_sprintf(thestring,
								"%10m*\t\t\t%d\t\t%d\t\t%d\t\t\t\t%d\n",
								conf_core.editor.grid_unit->suffix,
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

static const char *grpname(pcb_layergrp_id_t gid)
{
	pcb_layergrp_t *grp = pcb_get_layergrp(PCB, gid);
	if (grp == NULL)
		return "<invalid>";
	if ((grp->name == NULL) || (*grp->name == '\0'))
		return "<anonymous>";
	return grp->name;
}

static fgw_error_t pcb_act_report_dialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	void *ptr1, *ptr2, *ptr3;
	int type = REPORT_TYPES;
	char *op = NULL, *report = NULL, *how = NULL;
	pcb_subc_t *subc;
	pcb_coord_t x, y;
	pcb_hid_get_coords("Click on object to report on", &x, &y, 0);

	PCB_ACT_MAY_CONVARG(1, FGW_STR, reportdialog, op = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, reportdialog, how = argv[2].val.str);

	if (op != NULL) {
		if (pcb_strncasecmp(op, "Subc", 4) == 0)
			type = PCB_OBJ_SUBC;
	}
	type = pcb_search_screen(x, y, type, &ptr1, &ptr2, &ptr3);
	if ((type == PCB_OBJ_VOID) && (op == NULL))
		type = pcb_search_screen(x, y, REPORT_TYPES | PCB_OBJ_LOCKED, &ptr1, &ptr2, &ptr3);

	switch (type) {
	case PCB_OBJ_PSTK:
		{
			pcb_pstk_t *ps;
			pcb_pstk_proto_t *proto;
			gds_t tmp;

#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_r_dump_tree(PCB->Data->padstack_tree, 0);
				PCB_ACT_IRES(0);
				return 0;
			}
#endif
			ps = (pcb_pstk_t *)ptr2;
			proto = pcb_pstk_get_proto(ps);
			gds_init(&tmp);

			pcb_append_printf(&tmp, "%m+PADSTACK ID# %ld; Flags:%s\n"
				"(X,Y) = %$mD.\n", USER_UNITMASK, ps->ID, pcb_strflg_f2s(ps->Flags, PCB_OBJ_PSTK, NULL, 0),
				ps->x, ps->y);

			if ((proto != NULL) && (proto->hdia > 0))
				pcb_append_printf(&tmp, "%m+Hole diameter: %$mS", USER_UNITMASK, proto->hdia);

			pcb_append_printf(&tmp, "\n%s%s%s", gen_locked(ps), gen_term(ps));

			report = tmp.array;
			break;
		}

	case PCB_OBJ_LINE:
		{
			pcb_line_t *line;
#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				pcb_r_dump_tree(layer->line_tree, 0);
				PCB_ACT_IRES(0);
				return 0;
			}
#endif
			line = (pcb_line_t *) ptr2;
			report = pcb_strdup_printf("%m+LINE ID# %ld;  Flags:%s\n"
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
									pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1),
									gen_locked(line), gen_term(line));
			break;
		}
	case PCB_OBJ_RAT:
		{
			pcb_rat_t *line;
#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_r_dump_tree(PCB->Data->rat_tree, 0);
				PCB_ACT_IRES(0);
				return 0;
			}
#endif
			line = (pcb_rat_t *) ptr2;
			report = pcb_strdup_printf("%m+RAT-LINE ID# %ld;  Flags:%s\n"
									"FirstPoint(X,Y)  = %$mD; ID = %ld; "
									"connects to layer group #%d (%s).\n"
									"SecondPoint(X,Y) = %$mD; ID = %ld; "
									"connects to layer group #%d (%s).\n",
									USER_UNITMASK, line->ID, pcb_strflg_f2s(line->Flags, PCB_OBJ_LINE, NULL, 0),
									line->Point1.X, line->Point1.Y, line->Point1.ID, line->group1, grpname(line->group1),
									line->Point2.X, line->Point2.Y, line->Point2.ID, line->group2, grpname(line->group2));
			break;
		}
	case PCB_OBJ_ARC:
		{
			pcb_arc_t *Arc;
			pcb_box_t box;
#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				pcb_r_dump_tree(layer->arc_tree, 0);
				return 0;
			}
#endif
			Arc = (pcb_arc_t *) ptr2;
		pcb_arc_get_end(Arc, 0, &box.X1, &box.Y1);
		pcb_arc_get_end(Arc, 1, &box.X2, &box.Y2);

			report = pcb_strdup_printf("%m+ARC ID# %ld;  Flags:%s\n"
									"CenterPoint(X,Y) = %$mD.\n"
									"Width = %$mS.\nClearance = %$mS.\n"
									"Radius = %$mS, StartAngle = %ma degrees, DeltaAngle = %ma degrees.\n"
									"Bounding Box is %$mD, %$mD.\n"
									"That makes the end points at %$mD and %$mD.\n"
									"It is on layer %d.\n"
									"%s"
									"%s%s%s", USER_UNITMASK, Arc->ID, pcb_strflg_f2s(Arc->Flags, PCB_OBJ_ARC, NULL, 0),
									Arc->X, Arc->Y,
									Arc->Thickness, Arc->Clearance / 2,
									Arc->Width, Arc->StartAngle, Arc->Delta,
									Arc->BoundingBox.X1, Arc->BoundingBox.Y1,
									Arc->BoundingBox.X2, Arc->BoundingBox.Y2,
									box.X1, box.Y1,
									box.X2, box.Y2,
									pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1), gen_locked(Arc), gen_term(Arc));
			break;
		}
	case PCB_OBJ_POLY:
		{
			pcb_poly_t *Polygon;
			const char *aunit;
			double area, u;

#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				pcb_r_dump_tree(layer->polygon_tree, 0);
				PCB_ACT_IRES(0);
				return 0;
			}
#endif
			Polygon = (pcb_poly_t *) ptr2;

			aunit = conf_core.editor.grid_unit->in_suffix;
			if (aunit[1] == 'm')
				u = PCB_MM_TO_COORD(1);
			else
				u = PCB_MIL_TO_COORD(1);

			area = pcb_poly_area(Polygon);
			area = area / u;
			area = area / u;

			report = pcb_strdup_printf("%m+POLYGON ID# %ld;  Flags:%s\n"
									"Its bounding box is %$mD %$mD.\n"
									"It has %d points and could store %d more\n"
									"  without using more memory.\n"
									"It has %d holes and resides on layer %d.\n"
									"Its unclipped area is %f square %s.\n"
									"%s"
									"%s%s%s", USER_UNITMASK, Polygon->ID,
									pcb_strflg_f2s(Polygon->Flags, PCB_OBJ_POLY, NULL, 0),
									Polygon->BoundingBox.X1, Polygon->BoundingBox.Y1,
									Polygon->BoundingBox.X2, Polygon->BoundingBox.Y2,
									Polygon->PointN, Polygon->PointMax - Polygon->PointN,
									Polygon->HoleIndexN,
									pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1),
									area, aunit,
									gen_locked(Polygon), gen_term(Polygon));
			break;
		}
	case PCB_OBJ_SUBC:
		{
			pcb_subc_t *subc;
#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_r_dump_tree(PCB->Data->subc_tree, 0);
				PCB_ACT_IRES(0);
				return 0;
			}
#endif
			subc = (pcb_subc_t *)ptr2;
			report = pcb_strdup_printf("%m+SUBCIRCUIT ID# %ld;  Flags:%s\n"
									"BoundingBox %$mD %$mD.\n"
									"Refdes \"%s\".\n"
									"%s", USER_UNITMASK,
									subc->ID, pcb_strflg_f2s(subc->Flags, PCB_OBJ_SUBC, NULL, 0),
									subc->BoundingBox.X1, subc->BoundingBox.Y1,
									subc->BoundingBox.X2, subc->BoundingBox.Y2,
									PCB_EMPTY(subc->refdes),
									gen_locked(subc));
			break;
		}
	case PCB_OBJ_TEXT:
#ifndef NDEBUG
		if (pcb_gui->shift_is_pressed()) {
			pcb_layer_t *layer = (pcb_layer_t *) ptr1;
			pcb_r_dump_tree(layer->text_tree, 0);
			PCB_ACT_IRES(0);
			return 0;
		}
#endif
	case PCB_OBJ_LINE_POINT:
	case PCB_OBJ_POLY_POINT:
		{
			pcb_point_t *point = (pcb_point_t *) ptr2;
			report = pcb_strdup_printf("%m+POINT ID# %ld.\n"
									"Located at (X,Y) = %$mD.\n"
									"It belongs to a %s on layer %d.\n", USER_UNITMASK, point->ID,
									point->X, point->Y,
									(type == PCB_OBJ_LINE_POINT) ? "line" : "polygon", pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1));
			break;
		}
	case PCB_OBJ_VOID:
		report = NULL;
		break;

	default:
		report = pcb_strdup_printf("Unknown\n");
		break;
	}

	if ((report == NULL) || (*report == '\0')) {
		pcb_message(PCB_MSG_INFO, "Nothing found to report on\n");
		PCB_ACT_IRES(1);
		return 0;
	}

	/* create dialog box */
	subc = pcb_obj_parent_subc((pcb_any_obj_t *)ptr2);
	if (subc != NULL) {
		int len = strlen(report);
		report = realloc(report, len + 256);
		sprintf(report + len, "\nPart of subcircuit #%ld\n", subc->ID);
	}

	if ((how != NULL) && (strcmp(how, "log") == 0))
		pcb_message(PCB_MSG_INFO, "--- Report ---\n%s---\n", report);
	else
		rdialog("Report", report);
	free(report);

	PCB_ACT_IRES(0);
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
				pcb_append_printf(&list, "%s-%s,%c", subc->refdes, o->term, ((col++ % (conf_report.plugins.report.columns + 1)) == conf_report.plugins.report.columns) ? '\n' : ' ');
	}

	rdialog("Report", list.array);
	gds_uninit(&list);
	return 0;
}

static int net_length_cb(pcb_find_t *fctx, pcb_any_obj_t *o, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	double *length = fctx->user_data;
	double dx, dy;
	pcb_line_t *line = (pcb_line_t *)o;
	pcb_arc_t *arc = (pcb_arc_t *)o;

	switch(o->type) {
		case PCB_OBJ_LINE:
			dx = line->Point1.X - line->Point2.X;
			dy = line->Point1.Y - line->Point2.Y;
			(*length) += sqrt(dx * dx + dy * dy);
			break;

		case PCB_OBJ_ARC:
			/* NOTE: this assumes circuilar arc! */
			(*length) += M_PI * 2 * arc->Width * fabs(arc->Delta) / 360.0;
			break;

		default:
			break; /* silently ignore anything else... */
	}
	return 0;
}

static double xy_to_net_length(pcb_coord_t x, pcb_coord_t y, int *found)
{
	double length = 0;
	pcb_find_t fctx;

	memset(&fctx, 0, sizeof(fctx));
	fctx.consider_rats = 0;
	fctx.user_data = &length;
	fctx.found_cb = net_length_cb;
	*found = pcb_find_from_xy(&fctx, PCB->Data, x, y) > 0;
	pcb_find_free(&fctx);

	return length;
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
			const char *units_name;
			pcb_coord_t length;
			pcb_coord_t x = 0, y = 0;

			PCB_ACT_CONVARG(1, FGW_STR, Report, units_name = argv[2].val.str);

			pcb_obj_center(term, &x, &y);

			if (argc < 1)
				units_name = conf_core.editor.grid_unit->suffix;

			length = xy_to_net_length(x, y, &found);

			pcb_snprintf(buf, sizeof(buf), "%$m*", units_name, length);
			pcb_message(PCB_MSG_INFO, "Net %s length %s\n", net->name, buf);
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
			htsp_entry_t *e;

			if (o->term == NULL)
				continue;
			if (!PCB_FLAG_TEST(PCB_FLAG_FOUND, o))
				continue;

TODO("netlist: core probably has a _get() function for this");
			for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
				pcb_net_term_t *t;
				pcb_net_t *net = e->value;
				for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t))
					if ((strcmp(subc->refdes, t->refdes) == 0) && (strcmp(o->term, t->term) == 0))
						return net->name;
			}
		}
	}
	PCB_END_LOOP;
	return NULL;
}

static int report_net_length_(fgw_arg_t *res, int argc, fgw_arg_t *argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_coord_t length = 0;
	int found = 0, ret;

	length = xy_to_net_length(x, y, &found);

	if (found) {
		char buf[50];
		const char *netname = net_name_found(PCB);

		pcb_snprintf(buf, sizeof(buf), "%$m*", conf_core.editor.grid_unit->suffix, length);
		if (netname)
			pcb_message(PCB_MSG_INFO, "Net \"%s\" length: %s\n", netname, buf);
		else
			pcb_message(PCB_MSG_INFO, "Net length: %s\n", buf);

		ret = 0;
	}
	else {
		pcb_message(PCB_MSG_ERROR, "No net under cursor.\n");
		ret = 1;
	}

	return ret;
}

static int report_net_length(fgw_arg_t *res, int argc, fgw_arg_t *argv, int split)
{

	if (split) {
		void *r1, *r2, *r3;
		pcb_line_t *l;
		pcb_layer_t *ly;
		int type;
		pcb_coord_t ox, oy, x, y;

		pcb_hid_get_coords("Click on a copper line", &x, &y, 0);

		type = pcb_search_screen(x, y, PCB_OBJ_LINE, &r1, &r2, &r3);
		if (type != PCB_OBJ_LINE) {
			pcb_message(PCB_MSG_ERROR, "can't find a line to split\n");
			return -1;
		}
		l = r2;
		assert(l->parent_type == PCB_PARENT_LAYER);
		ly = l->parent.layer;
		if (!(pcb_layer_flags_(ly) & PCB_LYT_COPPER)) {
			pcb_message(PCB_MSG_ERROR, "not a copper line, can't split it\n");
			return -1;
		}

#define MINDIST PCB_MIL_TO_COORD(40)
		if ((pcb_distance(l->Point1.X, l->Point1.Y, x, y) < MINDIST) || (pcb_distance(l->Point2.X, l->Point2.Y, x, y) < MINDIST)) {
			pcb_message(PCB_MSG_ERROR, "Can not split near the endpoint of a line\n");
			return -1;
		}
#undef MINDIST2

		pcb_message(PCB_MSG_INFO, "The two arms of the net are:\n");
		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		ox = l->Point1.X; oy = l->Point1.Y; l->Point1.X = x; l->Point1.Y = y;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);
		report_net_length_(res, argc, argv, x, y);
		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		l->Point1.X = ox; l->Point1.Y = oy;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);

		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		ox = l->Point2.X; oy = l->Point2.Y; l->Point2.X = x; l->Point2.Y = y;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);
		report_net_length_(res, argc, argv, x, y);
		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		l->Point2.X = ox; l->Point2.Y = oy;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);

		PCB_FLAG_SET(PCB_FLAG_SELECTED, l);

		return 0;
	}
	else {
		pcb_coord_t x, y;
		pcb_hid_get_coords("Click on a network", &x, &y, 0);
		return report_net_length_(res, argc, argv, x, y);
	}
}

static int report_net_length_by_name(const char *tofind)
{
	const char *netname = NULL;
	pcb_net_t *net;
	pcb_coord_t length = 0;
	int found = 0;
	pcb_coord_t x, y;

	if (!PCB)
		return 1;

	if (!tofind)
		return 1;

	{
		net = pcb_net_get_user(PCB, &PCB->netlist[PCB_NETLIST_EDITED], tofind);
		if (net != NULL) {
			pcb_net_term_t *term;
			pcb_any_obj_t *obj = NULL;

			netname = net->name;
			term = pcb_termlist_first(&net->conns);
			if (term == NULL) {
				pcb_message(PCB_MSG_INFO, "Net found, but it has not terminals.\n");
				return 1;
			}

			obj = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, term->refdes, term->term, NULL, NULL);
			if (obj == NULL) {
				pcb_message(PCB_MSG_INFO, "Net found, but its terminal %s-%s is not on the board.\n", term->refdes, term->term);
				return 1;
			}
			pcb_obj_center(obj, &x, &y);
		}
	}

	if (netname == NULL) {
		pcb_message(PCB_MSG_ERROR, "No net named %s\n", tofind);
		return 1;
	}

	length = xy_to_net_length(x, y, &found);

	if (!found) {
		if (netname != NULL)
			pcb_message(PCB_MSG_INFO, "Net found, but no lines or arcs were flagged.\n");
		else
			pcb_message(PCB_MSG_ERROR, "Net not found.\n");

		return 1;
	}

	{
		char buf[50];
		pcb_snprintf(buf, sizeof(buf), "%$m*", conf_core.editor.grid_unit->suffix, length);
		if (netname)
			pcb_message(PCB_MSG_INFO, "Net \"%s\" length: %s\n", netname, buf);
		else
			pcb_message(PCB_MSG_INFO, "Net length: %s\n", buf);
	}

	return 0;
}

/* DOC: report.html */
static fgw_error_t pcb_act_Report(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *name;
	pcb_coord_t x, y;

	PCB_ACT_CONVARG(1, FGW_STR, Report, cmd = argv[1].val.str);

	if (pcb_strcasecmp(cmd, "Object") == 0) {
		pcb_hid_get_coords("Click on an object", &x, &y, 0);
		return pcb_act_report_dialog(res, argc, argv);
	}
	else if (pcb_strncasecmp(cmd, "Subc", 4) == 0) {
		pcb_hid_get_coords("Click on a subcircuit", &x, &y, 0);
		return pcb_act_report_dialog(res, argc, argv);
	}

	pcb_hid_get_coords("Click on object to report on", &x, &y, 0);

	if (pcb_strcasecmp(cmd, "DrillReport") == 0)
		return report_drills(res, argc, argv);
	else if (pcb_strcasecmp(cmd, "FoundPins") == 0)
		return report_found_pins(res, argc, argv);
	else if ((pcb_strcasecmp(cmd, "NetLength") == 0) && (argc == 2))
		return report_net_length(res, argc, argv, 0);
	else if ((pcb_strcasecmp(cmd, "NetLengthTo") == 0) && (argc == 2))
		return report_net_length(res, argc, argv, 1);
	else if (pcb_strcasecmp(cmd, "AllNetLengths") == 0)
		return report_all_net_lengths(res, argc, argv);
	else if (pcb_strcasecmp(cmd, "NetLength") == 0) {
		PCB_ACT_CONVARG(2, FGW_STR, Report, name = argv[2].val.str);
		return report_net_length_by_name(name);
	}

	PCB_ACT_FAIL(Report);
}

pcb_action_t report_action_list[] = {
	{"ReportObject", pcb_act_report_dialog, pcb_acth_reportdialog, pcb_acts_reportdialog},
	{"Report", pcb_act_Report, pcb_acth_Report, pcb_acts_Report}
};

static const char *report_cookie = "report plugin";

PCB_REGISTER_ACTIONS(report_action_list, report_cookie)

int pplg_check_ver_report(int ver_needed) { return 0; }

void pplg_uninit_report(void)
{
	pcb_remove_actions_by_cookie(report_cookie);
	conf_unreg_fields("plugins/report/");
}

#include "dolists.h"
int pplg_init_report(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(report_action_list, report_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_report, field,isarray,type_name,cpath,cname,desc,flags);
#include "report_conf_fields.h"
	return 0;
}
