/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
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

#include <genregex/regex_sei.h>

conf_report_t conf_report;

#define AUSAGE(x) pcb_message(PCB_MSG_INFO, "Usage:\n%s\n", (x##_syntax))
#define USER_UNITMASK (conf_core.editor.grid_unit->allow)

static int ReportDrills(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
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
								"%10m*\t\t%d\t\t%d\t\t%d\t\t%d\n",
								conf_core.editor.grid_unit->suffix,
								AllDrills->Drill[n].DrillSize,
								AllDrills->Drill[n].PinCount, AllDrills->Drill[n].ViaCount,
								AllDrills->Drill[n].ElementN, AllDrills->Drill[n].UnplatedCount);
		while (*thestring != '\0')
			thestring++;
	}
	FreeDrillInfo(AllDrills);
	/* create dialog box */
	gui->report_dialog("Drill Report", stringlist);

	free(stringlist);
	return 0;
}


static const char reportdialog_syntax[] = "ReportObject()";

static const char reportdialog_help[] = "Report on the object under the crosshair";

/* %start-doc actions ReportDialog

This is a shortcut for @code{Report(Object)}.

%end-doc */

static int ReportDialog(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	void *ptr1, *ptr2, *ptr3;
	int type;
	char *report = NULL;

	type = SearchScreen(x, y, REPORT_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == PCB_TYPE_NONE)
		type = SearchScreen(x, y, REPORT_TYPES | PCB_TYPE_LOCKED, &ptr1, &ptr2, &ptr3);

	switch (type) {
	case PCB_TYPE_VIA:
		{
			pcb_pin_t *via;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				__r_dump_tree(PCB->Data->via_tree->root, 0);
				return 0;
			}
#endif
			via = (pcb_pin_t *) ptr2;
			if (TEST_FLAG(PCB_FLAG_HOLE, via))
				report = pcb_strdup_printf("%m+VIA ID# %ld; Flags:%s\n"
										"(X,Y) = %$mD.\n"
										"It is a pure hole of diameter %$mS.\n"
										"Name = \"%s\"."
										"%s", USER_UNITMASK, via->ID, flags_to_string(via->Flags, PCB_TYPE_VIA),
										via->X, via->Y, via->DrillingHole, EMPTY(via->Name), TEST_FLAG(PCB_FLAG_LOCK, via) ? "It is LOCKED.\n" : "");
			else
				report = pcb_strdup_printf("%m+VIA ID# %ld;  Flags:%s\n"
										"(X,Y) = %$mD.\n"
										"Copper width = %$mS. Drill width = %$mS.\n"
										"Clearance width in polygons = %$mS.\n"
										"Annulus = %$mS.\n"
										"Solder mask hole = %$mS (gap = %$mS).\n"
										"Name = \"%s\"."
										"%s", USER_UNITMASK, via->ID, flags_to_string(via->Flags, PCB_TYPE_VIA),
										via->X, via->Y,
										via->Thickness,
										via->DrillingHole,
										via->Clearance / 2,
										(via->Thickness - via->DrillingHole) / 2,
										via->Mask,
										(via->Mask - via->Thickness) / 2, EMPTY(via->Name), TEST_FLAG(PCB_FLAG_LOCK, via) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_PIN:
		{
			pcb_pin_t *Pin;
			pcb_element_t *element;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				__r_dump_tree(PCB->Data->pin_tree->root, 0);
				return 0;
			}
#endif
			Pin = (pcb_pin_t *) ptr2;
			element = (pcb_element_t *) ptr1;

			PIN_LOOP(element);
			{
				if (pin == Pin)
					break;
			}
			END_LOOP;
			if (TEST_FLAG(PCB_FLAG_HOLE, Pin))
				report = pcb_strdup_printf("%m+PIN ID# %ld; Flags:%s\n"
										"(X,Y) = %$mD.\n"
										"It is a mounting hole. Drill width = %$mS.\n"
										"It is owned by element %$mS.\n"
										"%s", USER_UNITMASK, Pin->ID, flags_to_string(Pin->Flags, PCB_TYPE_PIN),
										Pin->X, Pin->Y, Pin->DrillingHole,
										EMPTY(element->Name[1].TextString), TEST_FLAG(PCB_FLAG_LOCK, Pin) ? "It is LOCKED.\n" : "");
			else
				report = pcb_strdup_printf(
										"%m+PIN ID# %ld;  Flags:%s\n" "(X,Y) = %$mD.\n"
										"Copper width = %$mS. Drill width = %$mS.\n"
										"Clearance width to Polygon = %$mS.\n"
										"Annulus = %$mS.\n"
										"Solder mask hole = %$mS (gap = %$mS).\n"
										"Name = \"%s\".\n"
										"It is owned by element %s\n as pin number %s.\n"
										"%s", USER_UNITMASK,
										Pin->ID, flags_to_string(Pin->Flags, PCB_TYPE_PIN),
										Pin->X, Pin->Y, Pin->Thickness,
										Pin->DrillingHole,
										Pin->Clearance / 2,
										(Pin->Thickness - Pin->DrillingHole) / 2,
										Pin->Mask,
										(Pin->Mask - Pin->Thickness) / 2,
										EMPTY(Pin->Name),
										EMPTY(element->Name[1].TextString), EMPTY(Pin->Number), TEST_FLAG(PCB_FLAG_LOCK, Pin) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_LINE:
		{
			pcb_line_t *line;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				__r_dump_tree(layer->line_tree->root, 0);
				return 0;
			}
#endif
			line = (pcb_line_t *) ptr2;
			report = pcb_strdup_printf("%m+LINE ID# %ld;  Flags:%s\n"
									"FirstPoint(X,Y)  = %$mD, ID = %ld.\n"
									"SecondPoint(X,Y) = %$mD, ID = %ld.\n"
									"Width = %$mS.\nClearance width in polygons = %$mS.\n"
									"It is on layer %d\n"
									"and has name \"%s\".\n"
									"%s", USER_UNITMASK,
									line->ID, flags_to_string(line->Flags, PCB_TYPE_LINE),
									line->Point1.X, line->Point1.Y, line->Point1.ID,
									line->Point2.X, line->Point2.Y, line->Point2.ID,
									line->Thickness, line->Clearance / 2,
									GetLayerNumber(PCB->Data, (pcb_layer_t *) ptr1),
									UNKNOWN(line->Number), TEST_FLAG(PCB_FLAG_LOCK, line) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_RATLINE:
		{
			pcb_rat_t *line;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				__r_dump_tree(PCB->Data->rat_tree->root, 0);
				return 0;
			}
#endif
			line = (pcb_rat_t *) ptr2;
			report = pcb_strdup_printf("%m+RAT-LINE ID# %ld;  Flags:%s\n"
									"FirstPoint(X,Y)  = %$mD; ID = %ld; "
									"connects to layer group %d.\n"
									"SecondPoint(X,Y) = %$mD; ID = %ld; "
									"connects to layer group %d.\n",
									USER_UNITMASK, line->ID, flags_to_string(line->Flags, PCB_TYPE_LINE),
									line->Point1.X, line->Point1.Y,
									line->Point1.ID, line->group1, line->Point2.X, line->Point2.Y, line->Point2.ID, line->group2);
			break;
		}
	case PCB_TYPE_ARC:
		{
			pcb_arc_t *Arc;
			pcb_box_t *box;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				__r_dump_tree(layer->arc_tree->root, 0);
				return 0;
			}
#endif
			Arc = (pcb_arc_t *) ptr2;
			box = GetArcEnds(Arc);

			report = pcb_strdup_printf("%m+ARC ID# %ld;  Flags:%s\n"
									"CenterPoint(X,Y) = %$mD.\n"
									"Radius = %$mS, Thickness = %$mS.\n"
									"Clearance width in polygons = %$mS.\n"
									"StartAngle = %ma degrees, DeltaAngle = %ma degrees.\n"
									"Bounding Box is %$mD, %$mD.\n"
									"That makes the end points at %$mD and %$mD.\n"
									"It is on layer %d.\n"
									"%s", USER_UNITMASK, Arc->ID, flags_to_string(Arc->Flags, PCB_TYPE_ARC),
									Arc->X, Arc->Y,
									Arc->Width, Arc->Thickness,
									Arc->Clearance / 2, Arc->StartAngle, Arc->Delta,
									Arc->BoundingBox.X1, Arc->BoundingBox.Y1,
									Arc->BoundingBox.X2, Arc->BoundingBox.Y2,
									box->X1, box->Y1,
									box->X2, box->Y2,
									GetLayerNumber(PCB->Data, (pcb_layer_t *) ptr1), TEST_FLAG(PCB_FLAG_LOCK, Arc) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_POLYGON:
		{
			pcb_polygon_t *Polygon;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				pcb_layer_t *layer = (pcb_layer_t *) ptr1;
				__r_dump_tree(layer->polygon_tree->root, 0);
				return 0;
			}
#endif
			Polygon = (pcb_polygon_t *) ptr2;

			report = pcb_strdup_printf("%m+POLYGON ID# %ld;  Flags:%s\n"
									"Its bounding box is %$mD %$mD.\n"
									"It has %d points and could store %d more\n"
									"  without using more memory.\n"
									"It has %d holes and resides on layer %d.\n"
									"%s", USER_UNITMASK, Polygon->ID,
									flags_to_string(Polygon->Flags, PCB_TYPE_POLYGON),
									Polygon->BoundingBox.X1, Polygon->BoundingBox.Y1,
									Polygon->BoundingBox.X2, Polygon->BoundingBox.Y2,
									Polygon->PointN, Polygon->PointMax - Polygon->PointN,
									Polygon->HoleIndexN,
									GetLayerNumber(PCB->Data, (pcb_layer_t *) ptr1), TEST_FLAG(PCB_FLAG_LOCK, Polygon) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_PAD:
		{
			pcb_coord_t len;
			pcb_pad_t *Pad;
			pcb_element_t *element;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				__r_dump_tree(PCB->Data->pad_tree->root, 0);
				return 0;
			}
#endif
			Pad = (pcb_pad_t *) ptr2;
			element = (pcb_element_t *) ptr1;

			PAD_LOOP(element);
			{
				{
					if (pad == Pad)
						break;
				}
			}
			END_LOOP;
			len = Distance(Pad->Point1.X, Pad->Point1.Y, Pad->Point2.X, Pad->Point2.Y);
			report = pcb_strdup_printf("%m+PAD ID# %ld;  Flags:%s\n"
									"FirstPoint(X,Y)  = %$mD; ID = %ld.\n"
									"SecondPoint(X,Y) = %$mD; ID = %ld.\n"
									"Width = %$mS.  Length = %$mS.\n"
									"Clearance width in polygons = %$mS.\n"
									"Solder mask = %$mS x %$mS (gap = %$mS).\n"
									"Name = \"%s\".\n"
									"It is owned by SMD element %s\n"
									"  as pin number %s and is on the %s\n"
									"side of the board.\n"
									"%s", USER_UNITMASK, Pad->ID,
									flags_to_string(Pad->Flags, PCB_TYPE_PAD),
									Pad->Point1.X, Pad->Point1.Y, Pad->Point1.ID,
									Pad->Point2.X, Pad->Point2.Y, Pad->Point2.ID,
									Pad->Thickness, len + Pad->Thickness,
									Pad->Clearance / 2,
									Pad->Mask, len + Pad->Mask,
									(Pad->Mask - Pad->Thickness) / 2,
									EMPTY(Pad->Name),
									EMPTY(element->Name[1].TextString),
									EMPTY(Pad->Number),
									TEST_FLAG(PCB_FLAG_ONSOLDER,
														Pad) ? "solder (bottom)" : "component", TEST_FLAG(PCB_FLAG_LOCK, Pad) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_ELEMENT:
		{
			pcb_element_t *element;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				__r_dump_tree(PCB->Data->element_tree->root, 0);
				return 0;
			}
#endif
			element = (pcb_element_t *) ptr2;
			report = pcb_strdup_printf("%m+ELEMENT ID# %ld;  Flags:%s\n"
									"BoundingBox %$mD %$mD.\n"
									"Descriptive Name \"%s\".\n"
									"Name on board \"%s\".\n"
									"Part number name \"%s\".\n"
									"It is %$mS tall and is located at (X,Y) = %$mD %s.\n"
									"Mark located at point (X,Y) = %$mD.\n"
									"It is on the %s side of the board.\n"
									"%s", USER_UNITMASK,
									element->ID, flags_to_string(element->Flags, PCB_TYPE_ELEMENT),
									element->BoundingBox.X1, element->BoundingBox.Y1,
									element->BoundingBox.X2, element->BoundingBox.Y2,
									EMPTY(element->Name[0].TextString),
									EMPTY(element->Name[1].TextString),
									EMPTY(element->Name[2].TextString),
									PCB_SCALE_TEXT(FONT_CAPHEIGHT, element->Name[1].Scale),
									element->Name[1].X, element->Name[1].Y,
									TEST_FLAG(PCB_FLAG_HIDENAME, element) ? ",\n  but it's hidden" : "",
									element->MarkX, element->MarkY,
									TEST_FLAG(PCB_FLAG_ONSOLDER, element) ? "solder (bottom)" : "component",
									TEST_FLAG(PCB_FLAG_LOCK, element) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_TEXT:
#ifndef NDEBUG
		if (gui->shift_is_pressed()) {
			pcb_layer_t *layer = (pcb_layer_t *) ptr1;
			__r_dump_tree(layer->text_tree->root, 0);
			return 0;
		}
#endif
	case PCB_TYPE_ELEMENT_NAME:
		{
			char laynum[32];
			pcb_text_t *text;
#ifndef NDEBUG
			if (gui->shift_is_pressed()) {
				__r_dump_tree(PCB->Data->name_tree[NAME_INDEX()]->root, 0);
				return 0;
			}
#endif
			text = (pcb_text_t *) ptr2;

			if (type == PCB_TYPE_TEXT)
				sprintf(laynum, "It is on layer %d.", GetLayerNumber(PCB->Data, (pcb_layer_t *) ptr1));
			report = pcb_strdup_printf("%m+TEXT ID# %ld;  Flags:%s\n"
									"Located at (X,Y) = %$mD.\n"
									"Characters are %$mS tall.\n"
									"Value is \"%s\".\n"
									"Direction is %d.\n"
									"The bounding box is %$mD %$mD.\n"
									"%s\n"
									"%s", USER_UNITMASK, text->ID, flags_to_string(text->Flags, PCB_TYPE_TEXT),
									text->X, text->Y, PCB_SCALE_TEXT(FONT_CAPHEIGHT, text->Scale),
									text->TextString, text->Direction,
									text->BoundingBox.X1, text->BoundingBox.Y1,
									text->BoundingBox.X2, text->BoundingBox.Y2,
									(type == PCB_TYPE_TEXT) ? laynum : "It is an element name.", TEST_FLAG(PCB_FLAG_LOCK, text) ? "It is LOCKED.\n" : "");
			break;
		}
	case PCB_TYPE_LINE_POINT:
	case PCB_TYPE_POLYGON_POINT:
		{
			pcb_point_t *point = (pcb_point_t *) ptr2;
			report = pcb_strdup_printf("%m+POINT ID# %ld.\n"
									"Located at (X,Y) = %$mD.\n"
									"It belongs to a %s on layer %d.\n", USER_UNITMASK, point->ID,
									point->X, point->Y,
									(type == PCB_TYPE_LINE_POINT) ? "line" : "polygon", GetLayerNumber(PCB->Data, (pcb_layer_t *) ptr1));
			break;
		}
	case PCB_TYPE_NONE:
		report = NULL;
		break;

	default:
		report = pcb_strdup_printf("Unknown\n");
		break;
	}

	if ((report == NULL) || (report == '\0')) {
		pcb_message(PCB_MSG_DEFAULT, _("Nothing found to report on\n"));
		return 1;
	}

	if (report != NULL) {
		/* create dialog box */
		gui->report_dialog("Report", report);
		free(report);
	}
	return 0;
}

static int ReportFoundPins(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	gds_t list;
	int col = 0;

	gds_init(&list);
	gds_append_str(&list, "The following pins/pads are FOUND:\n");
	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (TEST_FLAG(PCB_FLAG_FOUND, pin))
				pcb_append_printf(&list, "%s-%s,%c", NAMEONPCB_NAME(element), pin->Number, ((col++ % (conf_report.plugins.report.columns + 1)) == conf_report.plugins.report.columns) ? '\n' : ' ');
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			if (TEST_FLAG(PCB_FLAG_FOUND, pad))
				pcb_append_printf(&list, "%s-%s,%c", NAMEONPCB_NAME(element), pad->Number, ((col++ % (conf_report.plugins.report.columns + 1)) == conf_report.plugins.report.columns) ? '\n' : ' ');
		}
		END_LOOP;
	}
	END_LOOP;

	gui->report_dialog("Report", list.array);
	gds_uninit(&list);
	return 0;
}

/* Assumes that we start with a blank connection state,
 * e.g. ResetConnections() has been run.
 * Does not add its own changes to the undo system
 */
static double XYtoNetLength(pcb_coord_t x, pcb_coord_t y, int *found)
{
	double length;

	length = 0;
	*found = 0;

	/* NB: The third argument here, 'false' ensures LookupConnection
	 *     does not add its changes to the undo system.
	 */
	pcb_lookup_conn(x, y, pcb_false, PCB->Grid, PCB_FLAG_FOUND);

	ALLLINE_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_FOUND, line)) {
			double l;
			int dx, dy;
			dx = line->Point1.X - line->Point2.X;
			dy = line->Point1.Y - line->Point2.Y;
			l = sqrt((double) dx * dx + (double) dy * dy);
			length += l;
			*found = 1;
		}
	}
	ENDALL_LOOP;

	ALLARC_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_FOUND, arc)) {
			double l;
			/* FIXME: we assume width==height here */
			l = M_PI * 2 * arc->Width * fabs(arc->Delta) / 360.0;
			length += l;
			*found = 1;
		}
	}
	ENDALL_LOOP;

	return length;
}

static int ReportAllNetLengths(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int ni;
	int found;

	/* Reset all connection flags and save an undo-state to get back
	 * to the state the board was in when we started this function.
	 *
	 * After this, we don't add any changes to the undo system, but
	 * ensure we get back to a point where we can Undo() our changes
	 * by resetting the connections with ResetConnections() before
	 * calling Undo() at the end of the procedure.
	 */
	ResetConnections(pcb_true);
	IncrementUndoSerialNumber();

	for (ni = 0; ni < PCB->NetlistLib[NETLIST_EDITED].MenuN; ni++) {
		const char *netname = PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Name + 2;
		const char *list_entry = PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Entry[0].ListEntry;
		char *ename;
		char *pname;
		pcb_bool got_one = 0;

		ename = pcb_strdup(list_entry);
		pname = strchr(ename, '-');
		if (!pname) {
			free(ename);
			continue;
		}
		*pname++ = 0;

		ELEMENT_LOOP(PCB->Data);
		{
			char *es = element->Name[NAMEONPCB_INDEX].TextString;
			if (es && strcmp(es, ename) == 0) {
				PIN_LOOP(element);
				{
					if (strcmp(pin->Number, pname) == 0) {
						x = pin->X;
						y = pin->Y;
						got_one = 1;
						break;
					}
				}
				END_LOOP;
				PAD_LOOP(element);
				{
					if (strcmp(pad->Number, pname) == 0) {
						x = (pad->Point1.X + pad->Point2.X) / 2;
						y = (pad->Point1.Y + pad->Point2.Y) / 2;
						got_one = 1;
						break;
					}
				}
				END_LOOP;
			}
		}
		END_LOOP;

		if (got_one) {
			char buf[50];
			const char *units_name = argv[0];
			pcb_coord_t length;

			if (argc < 1)
				units_name = conf_core.editor.grid_unit->suffix;

			length = XYtoNetLength(x, y, &found);

			/* Reset connectors for the next lookup */
			ResetConnections(pcb_false);

			pcb_snprintf(buf, sizeof(buf), "%$m*", units_name, length);
			gui->log("Net %s length %s\n", netname, buf);
		}
	}

	ResetConnections(pcb_false);
	Undo(pcb_true);
	return 0;
}

static int ReportNetLength(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_coord_t length = 0;
	char *netname = 0;
	int found = 0;

	gui->get_coords("Click on a connection", &x, &y);

	/* Reset all connection flags and save an undo-state to get back
	 * to the state the board was in when we started this function.
	 *
	 * After this, we don't add any changes to the undo system, but
	 * ensure we get back to a point where we can Undo() our changes
	 * by resetting the connections with ResetConnections() before
	 * calling Undo() at the end of the procedure.
	 */
	ResetConnections(pcb_true);
	IncrementUndoSerialNumber();

	length = XYtoNetLength(x, y, &found);

	if (!found) {
		ResetConnections(pcb_false);
		Undo(pcb_true);
		gui->log("No net under cursor.\n");
		return 1;
	}

	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (TEST_FLAG(PCB_FLAG_FOUND, pin)) {
				int ni, nei;
				char *ename = element->Name[NAMEONPCB_INDEX].TextString;
				char *pname = pin->Number;
				char *n;

				if (ename && pname) {
					n = Concat(ename, "-", pname, NULL);
					for (ni = 0; ni < PCB->NetlistLib[NETLIST_EDITED].MenuN; ni++)
						for (nei = 0; nei < PCB->NetlistLib[NETLIST_EDITED].Menu[ni].EntryN; nei++) {
							if (strcmp(PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Entry[nei].ListEntry, n) == 0) {
								netname = PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Name + 2;
								goto got_net_name;	/* four for loops deep */
							}
						}
				}
			}
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			if (TEST_FLAG(PCB_FLAG_FOUND, pad)) {
				int ni, nei;
				char *ename = element->Name[NAMEONPCB_INDEX].TextString;
				char *pname = pad->Number;
				char *n;

				if (ename && pname) {
					n = Concat(ename, "-", pname, NULL);
					for (ni = 0; ni < PCB->NetlistLib[NETLIST_EDITED].MenuN; ni++)
						for (nei = 0; nei < PCB->NetlistLib[NETLIST_EDITED].Menu[ni].EntryN; nei++) {
							if (strcmp(PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Entry[nei].ListEntry, n) == 0) {
								netname = PCB->NetlistLib[NETLIST_EDITED].Menu[ni].Name + 2;
								goto got_net_name;	/* four for loops deep */
							}
						}
				}
			}
		}
		END_LOOP;
	}
	END_LOOP;

got_net_name:
	ResetConnections(pcb_false);
	Undo(pcb_true);

	{
		char buf[50];
		pcb_snprintf(buf, sizeof(buf), "%$m*", conf_core.editor.grid_unit->suffix, length);
		if (netname)
			gui->log("Net \"%s\" length: %s\n", netname, buf);
		else
			gui->log("Net length: %s\n", buf);
	}

	return 0;
}

static int ReportNetLengthByName(const char *tofind, int x, int y)
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
	for (i = 0; i < PCB->NetlistLib[NETLIST_EDITED].MenuN; i++) {
		net = PCB->NetlistLib[NETLIST_EDITED].Menu + i;
		if (strcasecmp(tofind, net->Name + 2) == 0)
			use_re = 0;
	}
	if (use_re) {
		regex = re_sei_comp(tofind);
		if (re_sei_errno(regex) != 0) {
			pcb_message(PCB_MSG_DEFAULT, _("regexp error: %s\n"), re_error_str(re_sei_errno(regex)));
			re_sei_free(regex);
			return (1);
		}
	}

	for (i = 0; i < PCB->NetlistLib[NETLIST_EDITED].MenuN; i++) {
		net = PCB->NetlistLib[NETLIST_EDITED].Menu + i;

		if (use_re) {
			if (re_sei_exec(regex, net->Name + 2) == 0)
				continue;
		}
		else {
			if (strcasecmp(net->Name + 2, tofind))
				continue;
		}

		if (SeekPad(net->Entry, &conn, pcb_false)) {
			switch (conn.type) {
			case PCB_TYPE_PIN:
				x = ((pcb_pin_t *) (conn.ptr2))->X;
				y = ((pcb_pin_t *) (conn.ptr2))->Y;
				net_found = 1;
				break;
			case PCB_TYPE_PAD:
				x = ((pcb_pad_t *) (conn.ptr2))->Point1.X;
				y = ((pcb_pad_t *) (conn.ptr2))->Point1.Y;
				net_found = 1;
				break;
			}
			if (net_found)
				break;
		}
	}

	if (!net_found) {
		gui->log("No net named %s\n", tofind);
		return 1;
	}

	if (use_re)
		re_sei_free(regex);

	/* Reset all connection flags and save an undo-state to get back
	 * to the state the board was in when we started.
	 *
	 * After this, we don't add any changes to the undo system, but
	 * ensure we get back to a point where we can Undo() our changes
	 * by resetting the connections with ResetConnections() before
	 * calling Undo() when we are finished.
	 */
	ResetConnections(pcb_true);
	IncrementUndoSerialNumber();

	length = XYtoNetLength(x, y, &found);
	netname = net->Name + 2;

	ResetConnections(pcb_false);
	Undo(pcb_true);

	if (!found) {
		if (net_found)
			gui->log("Net found, but no lines or arcs were flagged.\n");
		else
			gui->log("Net not found.\n");

		return 1;
	}

	{
		char buf[50];
		pcb_snprintf(buf, sizeof(buf), "%$m*", conf_core.editor.grid_unit->suffix, length);
		if (netname)
			gui->log("Net \"%s\" length: %s\n", netname, buf);
		else
			gui->log("Net length: %s\n", buf);
	}

	return 0;
}

/* ---------------------------------------------------------------------------
 * reports on an object
 * syntax:
 */

static const char report_syntax[] = "Report(Object|DrillReport|FoundPins|NetLength|AllNetLengths|[,name])";

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

static int Report(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if ((argc < 1) || (argc > 2))
		AUSAGE(report);
	else if (strcasecmp(argv[0], "Object") == 0) {
		gui->get_coords("Click on an object", &x, &y);
		return ReportDialog(argc - 1, argv + 1, x, y);
	}
	else if (strcasecmp(argv[0], "DrillReport") == 0)
		return ReportDrills(argc - 1, argv + 1, x, y);
	else if (strcasecmp(argv[0], "FoundPins") == 0)
		return ReportFoundPins(argc - 1, argv + 1, x, y);
	else if ((strcasecmp(argv[0], "NetLength") == 0) && (argc == 1))
		return ReportNetLength(argc - 1, argv + 1, x, y);
	else if (strcasecmp(argv[0], "AllNetLengths") == 0)
		return ReportAllNetLengths(argc - 1, argv + 1, x, y);
	else if ((strcasecmp(argv[0], "NetLength") == 0) && (argc == 2))
		return ReportNetLengthByName(argv[1], x, y);
	else if (argc == 2)
		AUSAGE(report);
	else
		PCB_AFAIL(report);
	return 1;
}

pcb_hid_action_t report_action_list[] = {
	{"ReportObject", "Click on an object", ReportDialog,
	 reportdialog_help, reportdialog_syntax}
	,
	{"Report", 0, Report,
	 report_help, report_syntax}
};

static const char *report_cookie = "report plugin";

REGISTER_ACTIONS(report_action_list, report_cookie)

static void hid_report_uninit(void)
{
	hid_remove_actions_by_cookie(report_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_report_init(void)
{
	REGISTER_ACTIONS(report_action_list, report_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_report, field,isarray,type_name,cpath,cname,desc,flags);
#include "report_conf_fields.h"
	return hid_report_uninit;
}
