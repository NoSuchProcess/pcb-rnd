/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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
#include "rats.h"
#include "rtree.h"
#include "flag_str.h"
#include "macro.h"
#include "undo.h"
#include "find.h"
#include "draw.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "misc_util.h"
#include "report_conf.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "layer.h"
#include "obj_all.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_parent.h"

#include <genregex/regex_sei.h>

conf_report_t conf_report;

#define AUSAGE(x) pcb_message(PCB_MSG_INFO, "Usage:\n%s\n", (x##_syntax))
#define USER_UNITMASK (conf_core.editor.grid_unit->allow)

static int report_drills(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	DrillInfoTypePtr AllDrills;
	pcb_cardinal_t n;
	char *stringlist, *thestring;
	int total_drills = 0;

	AllDrills = GetDrillInfo(PCB->Data);
	RoundDrillInfo(AllDrills, 100);

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
	FreeDrillInfo(AllDrills);
	/* create dialog box */
	pcb_gui->report_dialog("Drill Report", stringlist);

	free(stringlist);
	return 0;
}


static const char reportdialog_syntax[] = "ReportObject()";

static const char reportdialog_help[] = "Report on the object under the crosshair";

/* %start-doc actions ReportDialog

This is a shortcut for @code{Report(Object)}.

%end-doc */

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

static int report_dialog(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	void *ptr1, *ptr2, *ptr3;
	int type = REPORT_TYPES;
	char *report = NULL;
	pcb_subc_t *subc;

	if ((argv != NULL) && (argv[0] != NULL)) {
		if (pcb_strncasecmp(argv[0], "Subc", 4) == 0)
			type = PCB_TYPE_SUBC;
	}
	type = pcb_search_screen(x, y, type, &ptr1, &ptr2, &ptr3);
	if (type == PCB_TYPE_NONE)
		type = pcb_search_screen(x, y, REPORT_TYPES | PCB_TYPE_LOCKED, &ptr1, &ptr2, &ptr3);

	switch (type) {
	case PCB_TYPE_PSTK:
		{
			pcb_pstk_t *ps;
			pcb_pstk_proto_t *proto;
			gds_t tmp;

#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_r_dump_tree(PCB->Data->padstack_tree, 0);
				return 0;
			}
#endif
			ps = (pcb_pstk_t *)ptr2;
			proto = pcb_pstk_get_proto(ps);
			gds_init(&tmp);

			pcb_append_printf(&tmp, "%m+PADSTACK ID# %ld; Flags:%s\n"
				"(X,Y) = %$mD.\n", USER_UNITMASK, ps->ID, pcb_strflg_f2s(ps->Flags, PCB_TYPE_PSTK, NULL),
				ps->x, ps->y);

			if ((proto != NULL) && (proto->hdia > 0))
				pcb_append_printf(&tmp, "%m+Hole diameter: %$mS", USER_UNITMASK, proto->hdia);

			pcb_append_printf(&tmp, "\n%s%s%s", gen_locked(ps), gen_term(ps));

			report = tmp.array;
			break;
		}

	case PCB_TYPE_LINE:
		{
			pcb_line_t *line;
#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				pcb_r_dump_tree(layer->line_tree, 0);
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
									"%s%s%s", USER_UNITMASK,
									line->ID, pcb_strflg_f2s(line->Flags, PCB_TYPE_LINE, NULL),
									line->Point1.X, line->Point1.Y, line->Point1.ID,
									line->Point2.X, line->Point2.Y, line->Point2.ID,
									line->Thickness, line->Clearance / 2,
									pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1),
									PCB_UNKNOWN(line->Number), gen_locked(line), gen_term(line));
			break;
		}
	case PCB_TYPE_RATLINE:
		{
			pcb_rat_t *line;
#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_r_dump_tree(PCB->Data->rat_tree, 0);
				return 0;
			}
#endif
			line = (pcb_rat_t *) ptr2;
			report = pcb_strdup_printf("%m+RAT-LINE ID# %ld;  Flags:%s\n"
									"FirstPoint(X,Y)  = %$mD; ID = %ld; "
									"connects to layer group #%d (%s).\n"
									"SecondPoint(X,Y) = %$mD; ID = %ld; "
									"connects to layer group #%d (%s).\n",
									USER_UNITMASK, line->ID, pcb_strflg_f2s(line->Flags, PCB_TYPE_LINE, NULL),
									line->Point1.X, line->Point1.Y, line->Point1.ID, line->group1, grpname(line->group1),
									line->Point2.X, line->Point2.Y, line->Point2.ID, line->group2, grpname(line->group2));
			break;
		}
	case PCB_TYPE_ARC:
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
									"%s%s%s", USER_UNITMASK, Arc->ID, pcb_strflg_f2s(Arc->Flags, PCB_TYPE_ARC, NULL),
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
	case PCB_TYPE_POLY:
		{
			pcb_poly_t *Polygon;
			const char *aunit;
			double area, u;

#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				pcb_r_dump_tree(layer->polygon_tree, 0);
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
									pcb_strflg_f2s(Polygon->Flags, PCB_TYPE_POLY, NULL),
									Polygon->BoundingBox.X1, Polygon->BoundingBox.Y1,
									Polygon->BoundingBox.X2, Polygon->BoundingBox.Y2,
									Polygon->PointN, Polygon->PointMax - Polygon->PointN,
									Polygon->HoleIndexN,
									pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1),
									area, aunit,
									gen_locked(Polygon), gen_term(Polygon));
			break;
		}
	case PCB_TYPE_SUBC:
		{
			pcb_subc_t *subc;
#ifndef NDEBUG
			if (pcb_gui->shift_is_pressed()) {
				pcb_r_dump_tree(PCB->Data->subc_tree, 0);
				return 0;
			}
#endif
			subc = (pcb_subc_t *)ptr2;
			report = pcb_strdup_printf("%m+SUBCIRCUIT ID# %ld;  Flags:%s\n"
									"BoundingBox %$mD %$mD.\n"
									"Refdes \"%s\".\n"
									"%s", USER_UNITMASK,
									subc->ID, pcb_strflg_f2s(subc->Flags, PCB_TYPE_ELEMENT, NULL),
									subc->BoundingBox.X1, subc->BoundingBox.Y1,
									subc->BoundingBox.X2, subc->BoundingBox.Y2,
									PCB_EMPTY(subc->refdes),
									gen_locked(subc));
			break;
		}
	case PCB_TYPE_TEXT:
#ifndef NDEBUG
		if (pcb_gui->shift_is_pressed()) {
			pcb_layer_t *layer = (pcb_layer_t *) ptr1;
			pcb_r_dump_tree(layer->text_tree, 0);
			return 0;
		}
#endif
	case PCB_TYPE_LINE_POINT:
	case PCB_TYPE_POLY_POINT:
		{
			pcb_point_t *point = (pcb_point_t *) ptr2;
			report = pcb_strdup_printf("%m+POINT ID# %ld.\n"
									"Located at (X,Y) = %$mD.\n"
									"It belongs to a %s on layer %d.\n", USER_UNITMASK, point->ID,
									point->X, point->Y,
									(type == PCB_TYPE_LINE_POINT) ? "line" : "polygon", pcb_layer_id(PCB->Data, (pcb_layer_t *) ptr1));
			break;
		}
	case PCB_TYPE_NONE:
		report = NULL;
		break;

	default:
		report = pcb_strdup_printf("Unknown\n");
		break;
	}

	if ((report == NULL) || (*report == '\0')) {
		pcb_message(PCB_MSG_INFO, _("Nothing found to report on\n"));
		return 1;
	}

	/* create dialog box */
	subc = pcb_obj_parent_subc((pcb_any_obj_t *)ptr2);
	if (subc != NULL) {
		int len = strlen(report);
		report = realloc(report, len + 256);
		sprintf(report + len, "\nPart of subcircuit #%ld\n", subc->ID);
	}

	pcb_gui->report_dialog("Report", report);
	free(report);

	return 0;
}

static int report_found_pins(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
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

	pcb_gui->report_dialog("Report", list.array);
	gds_uninit(&list);
	return 0;
}

/* Assumes that we start with a blank connection state,
 * e.g. pcb_reset_conns() has been run.
 * Does not add its own changes to the undo system
 */
static double xy_to_net_length(pcb_coord_t x, pcb_coord_t y, int *found)
{
	double length;

	length = 0;
	*found = 0;

	/* NB: The third argument here, 'false' ensures LookupConnection
	 *     does not add its changes to the undo system.
	 */
	pcb_lookup_conn(x, y, pcb_false, PCB->Grid, PCB_FLAG_FOUND);

	PCB_LINE_ALL_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, line)) {
			double l;
			int dx, dy;
			dx = line->Point1.X - line->Point2.X;
			dy = line->Point1.Y - line->Point2.Y;
			l = sqrt((double) dx * dx + (double) dy * dy);
			length += l;
			*found = 1;
		}
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_ALL_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, arc)) {
			double l;
			/* FIXME: we assume width==height here */
			l = M_PI * 2 * arc->Width * fabs(arc->Delta) / 360.0;
			length += l;
			*found = 1;
		}
	}
	PCB_ENDALL_LOOP;

	return length;
}

static int report_all_net_lengths(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int ni;
	int found;

	/* Reset all connection flags and save an undo-state to get back
	 * to the state the board was in when we started this function.
	 *
	 * After this, we don't add any changes to the undo system, but
	 * ensure we get back to a point where we can pcb_undo() our changes
	 * by resetting the connections with pcb_reset_conns() before
	 * calling pcb_undo() at the end of the procedure.
	 */
	pcb_reset_conns(pcb_true);
	pcb_undo_inc_serial();

	for (ni = 0; ni < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; ni++) {
		const char *netname = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Name + 2;
		const char *list_entry = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Entry[0].ListEntry;
		char *ename;
		char *pname;
		pcb_any_obj_t *term;

		ename = pcb_strdup(list_entry);
		pname = strchr(ename, '-');
		if (!pname) {
			free(ename);
			continue;
		}
		*pname++ = 0;

		term = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, ename, pname, 0, NULL, NULL);
		free(ename);
		if (term != NULL) {
			char buf[50];
			const char *units_name = argv[0];
			pcb_coord_t length;

			pcb_obj_center(term, &x, &y);

			if (argc < 1)
				units_name = conf_core.editor.grid_unit->suffix;

			length = xy_to_net_length(x, y, &found);

			/* Reset connectors for the next lookup */
			pcb_reset_conns(pcb_false);

			pcb_snprintf(buf, sizeof(buf), "%$m*", units_name, length);
			pcb_message(PCB_MSG_INFO, "Net %s length %s\n", netname, buf);
		}
	}

	pcb_reset_conns(pcb_false);
	pcb_undo(pcb_true);
	return 0;
}

static int report_net_length_(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_coord_t length = 0;
	char *netname = 0;
	int found = 0;

	/* Reset all connection flags and save an undo-state to get back
	 * to the state the board was in when we started this function.
	 *
	 * After this, we don't add any changes to the undo system, but
	 * ensure we get back to a point where we can pcb_undo() our changes
	 * by resetting the connections with pcb_reset_conns() before
	 * calling pcb_undo() at the end of the procedure.
	 */
	pcb_reset_conns(pcb_true);
	pcb_undo_inc_serial();

	length = xy_to_net_length(x, y, &found);

	if (!found) {
		pcb_reset_conns(pcb_false);
		pcb_undo(pcb_true);
		pcb_gui->log("No net under cursor.\n");
		return 1;
	}

#warning subc TODO: rewrite
#if 0
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, pin)) {
				int ni, nei;
				char *ename = element->Name[PCB_ELEMNAME_IDX_REFDES].TextString;
				char *pname = pin->Number;
				char *n;

				if (ename && pname) {
					n = pcb_concat(ename, "-", pname, NULL);
					for (ni = 0; ni < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; ni++)
						for (nei = 0; nei < PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].EntryN; nei++) {
							if (strcmp(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Entry[nei].ListEntry, n) == 0) {
								netname = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Name + 2;
								goto got_net_name;	/* four for loops deep */
							}
						}
				}
			}
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, pad)) {
				int ni, nei;
				char *ename = element->Name[PCB_ELEMNAME_IDX_REFDES].TextString;
				char *pname = pad->Number;
				char *n;

				if (ename && pname) {
					n = pcb_concat(ename, "-", pname, NULL);
					for (ni = 0; ni < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; ni++)
						for (nei = 0; nei < PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].EntryN; nei++) {
							if (strcmp(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Entry[nei].ListEntry, n) == 0) {
								netname = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Name + 2;
								goto got_net_name;	/* four for loops deep */
							}
						}
				}
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
#endif

	goto noelem;

got_net_name:
	pcb_reset_conns(pcb_false);
	pcb_undo(pcb_true);

noelem:;
	{
		char buf[50];
		pcb_snprintf(buf, sizeof(buf), "%$m*", conf_core.editor.grid_unit->suffix, length);
		if (netname)
			pcb_gui->log("Net \"%s\" length: %s\n", netname, buf);
		else
			pcb_gui->log("Net length: %s\n", buf);
	}

	return 0;
}

static int report_net_length(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y, int split)
{

	if (split) {
		void *r1, *r2, *r3;
		pcb_line_t *l;
		pcb_layer_t *ly;
		int type;
		pcb_coord_t ox, oy;

		pcb_gui->get_coords("Click on a copper line", &x, &y);

		type = pcb_search_screen(x, y, PCB_TYPE_LINE, &r1, &r2, &r3);
		if (type != PCB_TYPE_LINE) {
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

		pcb_gui->log("The two arms of the net are:\n");
		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		ox = l->Point1.X; oy = l->Point1.Y; l->Point1.X = x; l->Point1.Y = y;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);
		report_net_length_(argc, argv, x, y);
		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		l->Point1.X = ox; l->Point1.Y = oy;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);

		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		ox = l->Point2.X; oy = l->Point2.Y; l->Point2.X = x; l->Point2.Y = y;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);
		report_net_length_(argc, argv, x, y);
		pcb_r_delete_entry(ly->line_tree, (pcb_box_t *)l);
		l->Point2.X = ox; l->Point2.Y = oy;
		pcb_r_insert_entry(ly->line_tree, (pcb_box_t *)l);

		PCB_FLAG_SET(PCB_FLAG_SELECTED, l);

		return 0;
	}
	else {
/*		pcb_gui->get_coords("Click on a connection", &x, &y);*/
		return report_net_length_(argc, argv, x, y);
	}
}


static int report_net_length_by_name(const char *tofind, pcb_coord_t x, pcb_coord_t y)
{
	char *netname = 0;
	pcb_coord_t length = 0;
	int found = 0;
	int i;
	pcb_lib_menu_t *net;
	pcb_connection_t conn;
	int net_found = 0;
	int use_re = 0;
	re_sei_t *regex;

	if (!PCB)
		return 1;

	if (!tofind)
		return 1;

	use_re = 1;
	for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; i++) {
		net = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu + i;
		if (pcb_strcasecmp(tofind, net->Name + 2) == 0)
			use_re = 0;
	}
	if (use_re) {
		regex = re_sei_comp(tofind);
		if (re_sei_errno(regex) != 0) {
			pcb_message(PCB_MSG_ERROR, _("regexp error: %s\n"), re_error_str(re_sei_errno(regex)));
			re_sei_free(regex);
			return 1;
		}
	}

	for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; i++) {
		net = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu + i;

		if (use_re) {
			if (re_sei_exec(regex, net->Name + 2) == 0)
				continue;
		}
		else {
			if (pcb_strcasecmp(net->Name + 2, tofind))
				continue;
		}

		if (pcb_rat_seek_pad(net->Entry, &conn, pcb_false)) {
			pcb_obj_center(conn.obj, &x, &y);
			if ((conn.obj->type == PCB_OBJ_PIN) || (conn.obj->type == PCB_OBJ_PAD) || (conn.obj->term != NULL)) {
				net_found = 1;
				break;
			}
		}
	}

	if (!net_found) {
		pcb_gui->log("No net named %s\n", tofind);
		return 1;
	}

	if (use_re)
		re_sei_free(regex);

	/* Reset all connection flags and save an undo-state to get back
	 * to the state the board was in when we started.
	 *
	 * After this, we don't add any changes to the undo system, but
	 * ensure we get back to a point where we can pcb_undo() our changes
	 * by resetting the connections with pcb_reset_conns() before
	 * calling pcb_undo() when we are finished.
	 */
	pcb_reset_conns(pcb_true);
	pcb_undo_inc_serial();

	length = xy_to_net_length(x, y, &found);
	netname = net->Name + 2;

	pcb_reset_conns(pcb_false);
	pcb_undo(pcb_true);

	if (!found) {
		if (net_found)
			pcb_gui->log("Net found, but no lines or arcs were flagged.\n");
		else
			pcb_gui->log("Net not found.\n");

		return 1;
	}

	{
		char buf[50];
		pcb_snprintf(buf, sizeof(buf), "%$m*", conf_core.editor.grid_unit->suffix, length);
		if (netname)
			pcb_gui->log("Net \"%s\" length: %s\n", netname, buf);
		else
			pcb_gui->log("Net length: %s\n", buf);
	}

	return 0;
}

/* ---------------------------------------------------------------------------
 * reports on an object
 * syntax:
 */

static const char report_syntax[] = "Report(Object|DrillReport|FoundPins|NetLength|NetLengthTo|AllNetLengths|[,name])";

static const char report_help[] = "Produce various report.";

/* %start-doc actions Report

@table @code

@item Object
The object under the crosshair will be reported, describing various
aspects of the object.

@item DrillReport
A report summarizing the number of drill sizes used, and how many of
each, will be produced.

@item FoundPins
A report listing all pins and pads which are marked as ``found'' will
be produced.

@item NetLength
The name and length of the net under the crosshair will be reported to
the message log.

@item AllNetLengths
The name and length of the net under the crosshair will be reported to
the message log.  An optional parameter specifies mm, mil, pcb, or in
units

@end table

%end-doc */

static int pcb_act_report(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if ((argc < 1) || (argc > 2))
		AUSAGE(report);
	else if (pcb_strcasecmp(argv[0], "Object") == 0) {
		pcb_gui->get_coords("Click on an object", &x, &y);
		return report_dialog(argc, argv, x, y);
	}
	else if (pcb_strncasecmp(argv[0], "Subc", 4) == 0) {
		pcb_gui->get_coords("Click on a subcircuit", &x, &y);
		return report_dialog(argc, argv, x, y);
	}
	else if (pcb_strcasecmp(argv[0], "DrillReport") == 0)
		return report_drills(argc - 1, argv + 1, x, y);
	else if (pcb_strcasecmp(argv[0], "FoundPins") == 0)
		return report_found_pins(argc - 1, argv + 1, x, y);
	else if ((pcb_strcasecmp(argv[0], "NetLength") == 0) && (argc == 1))
		return report_net_length(argc - 1, argv + 1, x, y, 0);
	else if ((pcb_strcasecmp(argv[0], "NetLengthTo") == 0) && (argc == 1))
		return report_net_length(argc - 1, argv + 1, x, y, 1);
	else if (pcb_strcasecmp(argv[0], "AllNetLengths") == 0)
		return report_all_net_lengths(argc - 1, argv + 1, x, y);
	else if ((pcb_strcasecmp(argv[0], "NetLength") == 0) && (argc == 2))
		return report_net_length_by_name(argv[1], x, y);
	else if (argc == 2)
		AUSAGE(report);
	else
		PCB_AFAIL(report);
	return 1;
}

pcb_hid_action_t report_action_list[] = {
	{"ReportObject", "Click on an object", report_dialog,
	 reportdialog_help, reportdialog_syntax}
	,
	{"Report", 0, pcb_act_report,
	 report_help, report_syntax}
};

static const char *report_cookie = "report plugin";

PCB_REGISTER_ACTIONS(report_action_list, report_cookie)

int pplg_check_ver_report(int ver_needed) { return 0; }

void pplg_uninit_report(void)
{
	pcb_hid_remove_actions_by_cookie(report_cookie);
	conf_unreg_fields("plugins/report/");
}

#include "dolists.h"
int pplg_init_report(void)
{
	PCB_REGISTER_ACTIONS(report_action_list, report_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_report, field,isarray,type_name,cpath,cname,desc,flags);
#include "report_conf_fields.h"
	return 0;
}
