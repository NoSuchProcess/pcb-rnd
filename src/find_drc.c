/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

#include "drc.h"
#include "compat_nls.h"

#include "obj_arc_draw.h"
#include "obj_pad_draw.h"
#include "obj_rat_draw.h"
#include "obj_line_draw.h"
#include "obj_elem_draw.h"
#include "obj_poly_draw.h"
#include "obj_pinvia_draw.h"

/* DRC related functions */

static void GotoError(void);
static pcb_bool DRCFind(int What, void *ptr1, void *ptr2, void *ptr3);

static pcb_drc_violation_t
	* pcb_drc_violation_new(const char *title,
													const char *explanation,
													Coord x, Coord y,
													Angle angle,
													pcb_bool have_measured,
													Coord measured_value,
													Coord required_value, int object_count, long int *object_id_list, int *object_type_list)
{
	pcb_drc_violation_t *violation = (pcb_drc_violation_t *) malloc(sizeof(pcb_drc_violation_t));

	violation->title = pcb_strdup(title);
	violation->explanation = pcb_strdup(explanation);
	violation->x = x;
	violation->y = y;
	violation->angle = angle;
	violation->have_measured = have_measured;
	violation->measured_value = measured_value;
	violation->required_value = required_value;
	violation->object_count = object_count;
	violation->object_id_list = object_id_list;
	violation->object_type_list = object_type_list;

	return violation;
}

static void pcb_drc_violation_free(pcb_drc_violation_t * violation)
{
	free(violation->title);
	free(violation->explanation);
	free(violation);
}

static gds_t drc_dialog_message;
static void reset_drc_dialog_message(void)
{
	gds_truncate(&drc_dialog_message, 0);
	if (gui->drc_gui != NULL)
		gui->drc_gui->reset_drc_dialog_message();
}

static void append_drc_dialog_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	pcb_append_vprintf(&drc_dialog_message, fmt, ap);
	va_end(ap);
}

static void GotoError(void);

/*----------------------------------------------------------------------------
 * Build a list of the of offending items by ID. (Currently just "thing")
 */
static void BuildObjectList(int *object_count, long int **object_id_list, int **object_type_list)
{
	*object_count = 0;
	*object_id_list = NULL;
	*object_type_list = NULL;

	switch (thing_type) {
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_POLYGON:
	case PCB_TYPE_PIN:
	case PCB_TYPE_VIA:
	case PCB_TYPE_PAD:
	case PCB_TYPE_ELEMENT:
	case PCB_TYPE_RATLINE:
		*object_count = 1;
		*object_id_list = (long int *) malloc(sizeof(long int));
		*object_type_list = (int *) malloc(sizeof(int));
		**object_id_list = ((pcb_any_obj_t *) thing_ptr3)->ID;
		**object_type_list = thing_type;
		return;

	default:
		fprintf(stderr, _("Internal error in BuildObjectList: unknown object type %i\n"), thing_type);
	}
}



/*----------------------------------------------------------------------------
 * Locate the coordinatates of offending item (thing)
 */
static void LocateError(Coord * x, Coord * y)
{
	switch (thing_type) {
	case PCB_TYPE_LINE:
		{
			pcb_line_t *line = (pcb_line_t *) thing_ptr3;
			*x = (line->Point1.X + line->Point2.X) / 2;
			*y = (line->Point1.Y + line->Point2.Y) / 2;
			break;
		}
	case PCB_TYPE_ARC:
		{
			pcb_arc_t *arc = (pcb_arc_t *) thing_ptr3;
			*x = arc->X;
			*y = arc->Y;
			break;
		}
	case PCB_TYPE_POLYGON:
		{
			pcb_polygon_t *polygon = (pcb_polygon_t *) thing_ptr3;
			*x = (polygon->Clipped->contours->xmin + polygon->Clipped->contours->xmax) / 2;
			*y = (polygon->Clipped->contours->ymin + polygon->Clipped->contours->ymax) / 2;
			break;
		}
	case PCB_TYPE_PIN:
	case PCB_TYPE_VIA:
		{
			pcb_pin_t *pin = (pcb_pin_t *) thing_ptr3;
			*x = pin->X;
			*y = pin->Y;
			break;
		}
	case PCB_TYPE_PAD:
		{
			pcb_pad_t *pad = (pcb_pad_t *) thing_ptr3;
			*x = (pad->Point1.X + pad->Point2.X) / 2;
			*y = (pad->Point1.Y + pad->Point2.Y) / 2;
			break;
		}
	case PCB_TYPE_ELEMENT:
		{
			pcb_element_t *element = (pcb_element_t *) thing_ptr3;
			*x = element->MarkX;
			*y = element->MarkY;
			break;
		}
	default:
		return;
	}
}


static void append_drc_violation(pcb_drc_violation_t * violation)
{
	if (gui->drc_gui != NULL) {
		gui->drc_gui->append_drc_violation(violation);
	}
	else {
		/* Fallback to formatting the violation message as text */
		append_drc_dialog_message("%s\n", violation->title);
		append_drc_dialog_message(_("%m+near %$mD\n"), conf_core.editor.grid_unit->allow, violation->x, violation->y);
		GotoError();
	}

	if (gui->drc_gui == NULL || gui->drc_gui->log_drc_violations) {
		Message(PCB_MSG_DEFAULT, _("WARNING!  Design Rule error - %s\n"), violation->title);
		Message(PCB_MSG_DEFAULT, _("%m+near location %$mD\n"), conf_core.editor.grid_unit->allow, violation->x, violation->y);
	}
}


/*
 * message when asked about continuing DRC checks after next
 * violation is found.
 */
#define DRC_CONTINUE _("Press Next to continue DRC checking")
#define DRC_NEXT _("Next")
#define DRC_CANCEL _("Cancel")

static int throw_drc_dialog(void)
{
	int r;

	if (gui->drc_gui != NULL) {
		r = gui->drc_gui->throw_drc_dialog();
	}
	else {
		/* Fallback to formatting the violation message as text */
		append_drc_dialog_message(DRC_CONTINUE);
		r = gui->confirm_dialog(drc_dialog_message.array, DRC_CANCEL, DRC_NEXT);
		reset_drc_dialog_message();
	}
	return r;
}

/* DRC clearance callback */
static pcb_r_dir_t drc_callback(pcb_data_t *data, pcb_layer_t *layer, pcb_polygon_t *polygon, int type, void *ptr1, void *ptr2)
{
	const char *message;
	Coord x, y;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	pcb_drc_violation_t *violation;

	pcb_line_t *line = (pcb_line_t *) ptr2;
	pcb_arc_t *arc = (pcb_arc_t *) ptr2;
	pcb_pin_t *pin = (pcb_pin_t *) ptr2;
	pcb_pad_t *pad = (pcb_pad_t *) ptr2;

	thing_type = type;
	thing_ptr1 = ptr1;
	thing_ptr2 = ptr2;
	thing_ptr3 = ptr2;
	switch (type) {
	case PCB_TYPE_LINE:
		if (line->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, line);
			message = _("Line with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case PCB_TYPE_ARC:
		if (arc->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, arc);
			message = _("Arc with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case PCB_TYPE_PAD:
		if (pad->Clearance && pad->Clearance < 2 * PCB->Bloat)
			if (IsPadInPolygon(pad, polygon)) {
				AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
				SET_FLAG(TheFlag, pad);
				message = _("Pad with insufficient clearance inside polygon\n");
				goto doIsBad;
			}
		break;
	case PCB_TYPE_PIN:
		if (pin->Clearance && pin->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, pin);
			message = _("Pin with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case PCB_TYPE_VIA:
		if (pin->Clearance && pin->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, pin);
			message = _("Via with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	default:
		Message(PCB_MSG_DEFAULT, "hace: Bad Plow object in callback\n");
	}
	return R_DIR_NOT_FOUND;

doIsBad:
	AddObjectToFlagUndoList(PCB_TYPE_POLYGON, layer, polygon, polygon);
	SET_FLAG(PCB_FLAG_FOUND, polygon);
	DrawPolygon(layer, polygon);
	DrawObject(type, ptr1, ptr2);
	drcerr_count++;
	LocateError(&x, &y);
	BuildObjectList(&object_count, &object_id_list, &object_type_list);
	violation = pcb_drc_violation_new(message, _("Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short."), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																		pcb_false,	/* MEASUREMENT OF ERROR UNKNOWN */
																		0,	/* MAGNITUDE OF ERROR UNKNOWN */
																		PCB->Bloat, object_count, object_id_list, object_type_list);
	append_drc_violation(violation);
	pcb_drc_violation_free(violation);
	free(object_id_list);
	free(object_type_list);
	if (!throw_drc_dialog()) {
		IsBad = pcb_true;
		return R_DIR_FOUND_CONTINUE;
	}
	IncrementUndoSerialNumber();
	Undo(pcb_true);
	return R_DIR_NOT_FOUND;
}



/*-----------------------------------------------------------------------------
 * Check for DRC violations
 * see if the connectivity changes when everything is bloated, or shrunk
 */
int DRCAll(void)
{
	Coord x, y;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	pcb_drc_violation_t *violation;
	int tmpcnt;
	int nopastecnt = 0;

	reset_drc_dialog_message();

	IsBad = pcb_false;
	drcerr_count = 0;
	SaveStackAndVisibility();
	ResetStackAndVisibility();
	hid_action("LayersChanged");
	InitConnectionLookup();

	TheFlag = PCB_FLAG_FOUND | PCB_FLAG_DRC | PCB_FLAG_SELECTED;

	if (ResetConnections(pcb_true)) {
		IncrementUndoSerialNumber();
		Draw();
	}

	User = pcb_false;

	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (!TEST_FLAG(PCB_FLAG_DRC, pin)
					&& DRCFind(PCB_TYPE_PIN, (void *) element, (void *) pin, (void *) pin)) {
				IsBad = pcb_true;
				break;
			}
		}
		END_LOOP;
		if (IsBad)
			break;
		PAD_LOOP(element);
		{

			/* count up how many pads have no solderpaste openings */
			if (TEST_FLAG(PCB_FLAG_NOPASTE, pad))
				nopastecnt++;

			if (!TEST_FLAG(PCB_FLAG_DRC, pad)
					&& DRCFind(PCB_TYPE_PAD, (void *) element, (void *) pad, (void *) pad)) {
				IsBad = pcb_true;
				break;
			}
		}
		END_LOOP;
		if (IsBad)
			break;
	}
	END_LOOP;
	if (!IsBad)
		VIA_LOOP(PCB->Data);
	{
		if (!TEST_FLAG(PCB_FLAG_DRC, via)
				&& DRCFind(PCB_TYPE_VIA, (void *) via, (void *) via, (void *) via)) {
			IsBad = pcb_true;
			break;
		}
	}
	END_LOOP;

	TheFlag = (IsBad) ? PCB_FLAG_DRC : (PCB_FLAG_FOUND | PCB_FLAG_DRC | PCB_FLAG_SELECTED);
	ResetConnections(pcb_false);
	TheFlag = PCB_FLAG_SELECTED;
	/* check minimum widths and polygon clearances */
	if (!IsBad) {
		COPPERLINE_LOOP(PCB->Data);
		{
			/* check line clearances in polygons */
			PlowsPolygon(PCB->Data, PCB_TYPE_LINE, layer, line, drc_callback);
			if (IsBad)
				break;
			if (line->Thickness < PCB->minWid) {
				AddObjectToFlagUndoList(PCB_TYPE_LINE, layer, line, line);
				SET_FLAG(TheFlag, line);
				DrawLine(layer, line);
				drcerr_count++;
				SetThing(PCB_TYPE_LINE, layer, line, line);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Line width is too thin"), _("Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					line->Thickness, PCB->minWid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(pcb_false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		COPPERARC_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, PCB_TYPE_ARC, layer, arc, drc_callback);
			if (IsBad)
				break;
			if (arc->Thickness < PCB->minWid) {
				AddObjectToFlagUndoList(PCB_TYPE_ARC, layer, arc, arc);
				SET_FLAG(TheFlag, arc);
				DrawArc(layer, arc);
				drcerr_count++;
				SetThing(PCB_TYPE_ARC, layer, arc, arc);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Arc width is too thin"), _("Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					arc->Thickness, PCB->minWid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(pcb_false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		ALLPIN_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, PCB_TYPE_PIN, element, pin, drc_callback);
			if (IsBad)
				break;
			if (!TEST_FLAG(PCB_FLAG_HOLE, pin) && pin->Thickness - pin->DrillingHole < 2 * PCB->minRing) {
				AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
				SET_FLAG(TheFlag, pin);
				DrawPin(pin);
				drcerr_count++;
				SetThing(PCB_TYPE_PIN, element, pin, pin);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Pin annular ring too small"), _("Annular rings that are too small may erode during etching,\n" "resulting in a broken connection"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					(pin->Thickness - pin->DrillingHole) / 2,
																					PCB->minRing, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(pcb_false);
			}
			if (pin->DrillingHole < PCB->minDrill) {
				AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
				SET_FLAG(TheFlag, pin);
				DrawPin(pin);
				drcerr_count++;
				SetThing(PCB_TYPE_PIN, element, pin, pin);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Pin drill size is too small"), _("Process rules dictate the minimum drill size which can be used"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					pin->DrillingHole, PCB->minDrill, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(pcb_false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		ALLPAD_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, PCB_TYPE_PAD, element, pad, drc_callback);
			if (IsBad)
				break;
			if (pad->Thickness < PCB->minWid) {
				AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
				SET_FLAG(TheFlag, pad);
				DrawPad(pad);
				drcerr_count++;
				SetThing(PCB_TYPE_PAD, element, pad, pad);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Pad is too thin"), _("Pads which are too thin may erode during etching,\n" "resulting in a broken or unreliable connection"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					pad->Thickness, PCB->minWid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(pcb_false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		VIA_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, PCB_TYPE_VIA, via, via, drc_callback);
			if (IsBad)
				break;
			if (!TEST_FLAG(PCB_FLAG_HOLE, via) && via->Thickness - via->DrillingHole < 2 * PCB->minRing) {
				AddObjectToFlagUndoList(PCB_TYPE_VIA, via, via, via);
				SET_FLAG(TheFlag, via);
				DrawVia(via);
				drcerr_count++;
				SetThing(PCB_TYPE_VIA, via, via, via);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Via annular ring too small"), _("Annular rings that are too small may erode during etching,\n" "resulting in a broken connection"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					(via->Thickness - via->DrillingHole) / 2,
																					PCB->minRing, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(pcb_false);
			}
			if (via->DrillingHole < PCB->minDrill) {
				AddObjectToFlagUndoList(PCB_TYPE_VIA, via, via, via);
				SET_FLAG(TheFlag, via);
				DrawVia(via);
				drcerr_count++;
				SetThing(PCB_TYPE_VIA, via, via, via);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Via drill size is too small"), _("Process rules dictate the minimum drill size which can be used"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					via->DrillingHole, PCB->minDrill, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(pcb_false);
			}
		}
		END_LOOP;
	}

	FreeConnectionLookupMemory();
	TheFlag = PCB_FLAG_FOUND;
	Bloat = 0;

	/* check silkscreen minimum widths outside of elements */
	/* XXX - need to check text and polygons too! */
	TheFlag = PCB_FLAG_SELECTED;
	if (!IsBad) {
		SILKLINE_LOOP(PCB->Data);
		{
			if (line->Thickness < PCB->minSlk) {
				SET_FLAG(TheFlag, line);
				DrawLine(layer, line);
				drcerr_count++;
				SetThing(PCB_TYPE_LINE, layer, line, line);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Silk line is too thin"), _("Process specifications dictate a minimum silkscreen feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					line->Thickness, PCB->minSlk, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
			}
		}
		ENDALL_LOOP;
	}

	/* check silkscreen minimum widths inside of elements */
	/* XXX - need to check text and polygons too! */
	TheFlag = PCB_FLAG_SELECTED;
	if (!IsBad) {
		ELEMENT_LOOP(PCB->Data);
		{
			tmpcnt = 0;
			ELEMENTLINE_LOOP(element);
			{
				if (line->Thickness < PCB->minSlk)
					tmpcnt++;
			}
			END_LOOP;
			if (tmpcnt > 0) {
				const char *title;
				const char *name;
				char *buffer;
				int buflen;

				SET_FLAG(TheFlag, element);
				DrawElement(element);
				drcerr_count++;
				SetThing(PCB_TYPE_ELEMENT, element, element, element);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);

				title = _("Element %s has %i silk lines which are too thin");
				name = UNKNOWN(NAMEONPCB_NAME(element));

				/* -4 is for the %s and %i place-holders */
				/* +11 is the max printed length for a 32 bit integer */
				/* +1 is for the \0 termination */
				buflen = strlen(title) - 4 + strlen(name) + 11 + 1;
				buffer = (char *) malloc(buflen);
				pcb_snprintf(buffer, buflen, title, name, tmpcnt);

				violation = pcb_drc_violation_new(buffer, _("Process specifications dictate a minimum silkscreen\n" "feature-width that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					0,	/* MINIMUM OFFENDING WIDTH UNKNOWN */
																					PCB->minSlk, object_count, object_id_list, object_type_list);
				free(buffer);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
			}
		}
		END_LOOP;
	}


	if (IsBad) {
		IncrementUndoSerialNumber();
	}


	RestoreStackAndVisibility();
	hid_action("LayersChanged");
	gui->invalidate_all();

	if (nopastecnt > 0) {
		Message(PCB_MSG_DEFAULT, _("Warning:  %d pad%s the nopaste flag set.\n"), nopastecnt, nopastecnt > 1 ? "s have" : " has");
	}
	return IsBad ? -drcerr_count : drcerr_count;
}



/*-----------------------------------------------------------------------------
 * Check for DRC violations on a single net starting from the pad or pin
 * sees if the connectivity changes when everything is bloated, or shrunk
 */
static pcb_bool DRCFind(int What, void *ptr1, void *ptr2, void *ptr3)
{
	Coord x, y;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	pcb_drc_violation_t *violation;

	if (PCB->Shrink != 0) {
		Bloat = -PCB->Shrink;
		TheFlag = PCB_FLAG_DRC | PCB_FLAG_SELECTED;
		ListStart(What, ptr1, ptr2, ptr3);
		DoIt(pcb_true, pcb_false);
		/* ok now the shrunk net has the PCB_FLAG_SELECTED set */
		DumpList();
		TheFlag = PCB_FLAG_FOUND;
		ListStart(What, ptr1, ptr2, ptr3);
		Bloat = 0;
		drc = pcb_true;									/* abort the search if we find anything not already found */
		if (DoIt(pcb_true, pcb_false)) {
			DumpList();
			/* make the flag changes undoable */
			TheFlag = PCB_FLAG_FOUND | PCB_FLAG_SELECTED;
			ResetConnections(pcb_false);
			User = pcb_true;
			drc = pcb_false;
			Bloat = -PCB->Shrink;
			TheFlag = PCB_FLAG_SELECTED;
			ListStart(What, ptr1, ptr2, ptr3);
			DoIt(pcb_true, pcb_true);
			DumpList();
			ListStart(What, ptr1, ptr2, ptr3);
			TheFlag = PCB_FLAG_FOUND;
			Bloat = 0;
			drc = pcb_true;
			DoIt(pcb_true, pcb_true);
			DumpList();
			User = pcb_false;
			drc = pcb_false;
			drcerr_count++;
			LocateError(&x, &y);
			BuildObjectList(&object_count, &object_id_list, &object_type_list);
			violation = pcb_drc_violation_new(_("Potential for broken trace"), _("Insufficient overlap between objects can lead to broken tracks\n" "due to registration errors with old wheel style photo-plotters."), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																				pcb_false,	/* MEASUREMENT OF ERROR UNKNOWN */
																				0,	/* MAGNITUDE OF ERROR UNKNOWN */
																				PCB->Shrink, object_count, object_id_list, object_type_list);
			append_drc_violation(violation);
			pcb_drc_violation_free(violation);
			free(object_id_list);
			free(object_type_list);

			if (!throw_drc_dialog())
				return (pcb_true);
			IncrementUndoSerialNumber();
			Undo(pcb_true);
		}
		DumpList();
	}
	/* now check the bloated condition */
	drc = pcb_false;
	ResetConnections(pcb_false);
	TheFlag = PCB_FLAG_FOUND;
	ListStart(What, ptr1, ptr2, ptr3);
	Bloat = PCB->Bloat;
	drc = pcb_true;
	while (DoIt(pcb_true, pcb_false)) {
		DumpList();
		/* make the flag changes undoable */
		TheFlag = PCB_FLAG_FOUND | PCB_FLAG_SELECTED;
		ResetConnections(pcb_false);
		User = pcb_true;
		drc = pcb_false;
		Bloat = 0;
		TheFlag = PCB_FLAG_SELECTED;
		ListStart(What, ptr1, ptr2, ptr3);
		DoIt(pcb_true, pcb_true);
		DumpList();
		TheFlag = PCB_FLAG_FOUND;
		ListStart(What, ptr1, ptr2, ptr3);
		Bloat = PCB->Bloat;
		drc = pcb_true;
		DoIt(pcb_true, pcb_true);
		DumpList();
		drcerr_count++;
		LocateError(&x, &y);
		BuildObjectList(&object_count, &object_id_list, &object_type_list);
		violation = pcb_drc_violation_new(_("Copper areas too close"), _("Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short."), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																			pcb_false,	/* MEASUREMENT OF ERROR UNKNOWN */
																			0,	/* MAGNITUDE OF ERROR UNKNOWN */
																			PCB->Bloat, object_count, object_id_list, object_type_list);
		append_drc_violation(violation);
		pcb_drc_violation_free(violation);
		free(object_id_list);
		free(object_type_list);
		User = pcb_false;
		drc = pcb_false;
		if (!throw_drc_dialog())
			return (pcb_true);
		IncrementUndoSerialNumber();
		Undo(pcb_true);
		/* highlight the rest of the encroaching net so it's not reported again */
		TheFlag |= PCB_FLAG_SELECTED;
		Bloat = 0;
		ListStart(thing_type, thing_ptr1, thing_ptr2, thing_ptr3);
		DoIt(pcb_true, pcb_true);
		DumpList();
		drc = pcb_true;
		Bloat = PCB->Bloat;
		ListStart(What, ptr1, ptr2, ptr3);
	}
	drc = pcb_false;
	DumpList();
	TheFlag = PCB_FLAG_FOUND | PCB_FLAG_SELECTED;
	ResetConnections(pcb_false);
	return (pcb_false);
}

/*----------------------------------------------------------------------------
 * center the display to show the offending item (thing)
 */
static void GotoError(void)
{
	Coord X, Y;

	LocateError(&X, &Y);

	switch (thing_type) {
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_POLYGON:
		ChangeGroupVisibility(GetLayerNumber(PCB->Data, (pcb_layer_t *) thing_ptr1), pcb_true, pcb_true);
	}
	CenterDisplay(X, Y);
}
