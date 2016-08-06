/* $Id$ */

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

/* DRC related functions */

static DrcViolationType
	* pcb_drc_violation_new(const char *title,
													const char *explanation,
													Coord x, Coord y,
													Angle angle,
													bool have_measured,
													Coord measured_value,
													Coord required_value, int object_count, long int *object_id_list, int *object_type_list)
{
	DrcViolationType *violation = (DrcViolationType *) malloc(sizeof(DrcViolationType));

	violation->title = strdup(title);
	violation->explanation = strdup(explanation);
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

static void pcb_drc_violation_free(DrcViolationType * violation)
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

static void append_drc_violation(DrcViolationType * violation)
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
		Message(_("WARNING!  Design Rule error - %s\n"), violation->title);
		Message(_("%m+near location %$mD\n"), conf_core.editor.grid_unit->allow, violation->x, violation->y);
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

static int drc_callback(DataTypePtr data, LayerTypePtr layer, PolygonTypePtr polygon, int type, void *ptr1, void *ptr2)
{
	char *message;
	Coord x, y;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	DrcViolationType *violation;

	LineTypePtr line = (LineTypePtr) ptr2;
	ArcTypePtr arc = (ArcTypePtr) ptr2;
	PinTypePtr pin = (PinTypePtr) ptr2;
	PadTypePtr pad = (PadTypePtr) ptr2;

	thing_type = type;
	thing_ptr1 = ptr1;
	thing_ptr2 = ptr2;
	thing_ptr3 = ptr2;
	switch (type) {
	case LINE_TYPE:
		if (line->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, line);
			message = _("Line with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case ARC_TYPE:
		if (arc->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, arc);
			message = _("Arc with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case PAD_TYPE:
		if (pad->Clearance && pad->Clearance < 2 * PCB->Bloat)
			if (IsPadInPolygon(pad, polygon)) {
				AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
				SET_FLAG(TheFlag, pad);
				message = _("Pad with insufficient clearance inside polygon\n");
				goto doIsBad;
			}
		break;
	case PIN_TYPE:
		if (pin->Clearance && pin->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, pin);
			message = _("Pin with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case VIA_TYPE:
		if (pin->Clearance && pin->Clearance < 2 * PCB->Bloat) {
			AddObjectToFlagUndoList(type, ptr1, ptr2, ptr2);
			SET_FLAG(TheFlag, pin);
			message = _("Via with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	default:
		Message("hace: Bad Plow object in callback\n");
	}
	return 0;

doIsBad:
	AddObjectToFlagUndoList(POLYGON_TYPE, layer, polygon, polygon);
	SET_FLAG(FOUNDFLAG, polygon);
	DrawPolygon(layer, polygon);
	DrawObject(type, ptr1, ptr2);
	drcerr_count++;
	LocateError(&x, &y);
	BuildObjectList(&object_count, &object_id_list, &object_type_list);
	violation = pcb_drc_violation_new(message, _("Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short."), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																		FALSE,	/* MEASUREMENT OF ERROR UNKNOWN */
																		0,	/* MAGNITUDE OF ERROR UNKNOWN */
																		PCB->Bloat, object_count, object_id_list, object_type_list);
	append_drc_violation(violation);
	pcb_drc_violation_free(violation);
	free(object_id_list);
	free(object_type_list);
	if (!throw_drc_dialog()) {
		IsBad = true;
		return 1;
	}
	IncrementUndoSerialNumber();
	Undo(true);
	return 0;
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
	DrcViolationType *violation;
	int tmpcnt;
	int nopastecnt = 0;

	reset_drc_dialog_message();

	IsBad = false;
	drcerr_count = 0;
	SaveStackAndVisibility();
	ResetStackAndVisibility();
	hid_action("LayersChanged");
	InitConnectionLookup();

	TheFlag = FOUNDFLAG | DRCFLAG | SELECTEDFLAG;

	if (ResetConnections(true)) {
		IncrementUndoSerialNumber();
		Draw();
	}

	User = false;

	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (!TEST_FLAG(DRCFLAG, pin)
					&& DRCFind(PIN_TYPE, (void *) element, (void *) pin, (void *) pin)) {
				IsBad = true;
				break;
			}
		}
		END_LOOP;
		if (IsBad)
			break;
		PAD_LOOP(element);
		{

			/* count up how many pads have no solderpaste openings */
			if (TEST_FLAG(NOPASTEFLAG, pad))
				nopastecnt++;

			if (!TEST_FLAG(DRCFLAG, pad)
					&& DRCFind(PAD_TYPE, (void *) element, (void *) pad, (void *) pad)) {
				IsBad = true;
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
		if (!TEST_FLAG(DRCFLAG, via)
				&& DRCFind(VIA_TYPE, (void *) via, (void *) via, (void *) via)) {
			IsBad = true;
			break;
		}
	}
	END_LOOP;

	TheFlag = (IsBad) ? DRCFLAG : (FOUNDFLAG | DRCFLAG | SELECTEDFLAG);
	ResetConnections(false);
	TheFlag = SELECTEDFLAG;
	/* check minimum widths and polygon clearances */
	if (!IsBad) {
		COPPERLINE_LOOP(PCB->Data);
		{
			/* check line clearances in polygons */
			PlowsPolygon(PCB->Data, LINE_TYPE, layer, line, drc_callback);
			if (IsBad)
				break;
			if (line->Thickness < PCB->minWid) {
				AddObjectToFlagUndoList(LINE_TYPE, layer, line, line);
				SET_FLAG(TheFlag, line);
				DrawLine(layer, line);
				drcerr_count++;
				SetThing(LINE_TYPE, layer, line, line);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Line width is too thin"), _("Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					line->Thickness, PCB->minWid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		COPPERARC_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, ARC_TYPE, layer, arc, drc_callback);
			if (IsBad)
				break;
			if (arc->Thickness < PCB->minWid) {
				AddObjectToFlagUndoList(ARC_TYPE, layer, arc, arc);
				SET_FLAG(TheFlag, arc);
				DrawArc(layer, arc);
				drcerr_count++;
				SetThing(ARC_TYPE, layer, arc, arc);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Arc width is too thin"), _("Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					arc->Thickness, PCB->minWid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		ALLPIN_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, PIN_TYPE, element, pin, drc_callback);
			if (IsBad)
				break;
			if (!TEST_FLAG(HOLEFLAG, pin) && pin->Thickness - pin->DrillingHole < 2 * PCB->minRing) {
				AddObjectToFlagUndoList(PIN_TYPE, element, pin, pin);
				SET_FLAG(TheFlag, pin);
				DrawPin(pin);
				drcerr_count++;
				SetThing(PIN_TYPE, element, pin, pin);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Pin annular ring too small"), _("Annular rings that are too small may erode during etching,\n" "resulting in a broken connection"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					(pin->Thickness - pin->DrillingHole) / 2,
																					PCB->minRing, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(false);
			}
			if (pin->DrillingHole < PCB->minDrill) {
				AddObjectToFlagUndoList(PIN_TYPE, element, pin, pin);
				SET_FLAG(TheFlag, pin);
				DrawPin(pin);
				drcerr_count++;
				SetThing(PIN_TYPE, element, pin, pin);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Pin drill size is too small"), _("Process rules dictate the minimum drill size which can be used"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					pin->DrillingHole, PCB->minDrill, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		ALLPAD_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, PAD_TYPE, element, pad, drc_callback);
			if (IsBad)
				break;
			if (pad->Thickness < PCB->minWid) {
				AddObjectToFlagUndoList(PAD_TYPE, element, pad, pad);
				SET_FLAG(TheFlag, pad);
				DrawPad(pad);
				drcerr_count++;
				SetThing(PAD_TYPE, element, pad, pad);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Pad is too thin"), _("Pads which are too thin may erode during etching,\n" "resulting in a broken or unreliable connection"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					pad->Thickness, PCB->minWid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(false);
			}
		}
		ENDALL_LOOP;
	}
	if (!IsBad) {
		VIA_LOOP(PCB->Data);
		{
			PlowsPolygon(PCB->Data, VIA_TYPE, via, via, drc_callback);
			if (IsBad)
				break;
			if (!TEST_FLAG(HOLEFLAG, via) && via->Thickness - via->DrillingHole < 2 * PCB->minRing) {
				AddObjectToFlagUndoList(VIA_TYPE, via, via, via);
				SET_FLAG(TheFlag, via);
				DrawVia(via);
				drcerr_count++;
				SetThing(VIA_TYPE, via, via, via);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Via annular ring too small"), _("Annular rings that are too small may erode during etching,\n" "resulting in a broken connection"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					(via->Thickness - via->DrillingHole) / 2,
																					PCB->minRing, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(false);
			}
			if (via->DrillingHole < PCB->minDrill) {
				AddObjectToFlagUndoList(VIA_TYPE, via, via, via);
				SET_FLAG(TheFlag, via);
				DrawVia(via);
				drcerr_count++;
				SetThing(VIA_TYPE, via, via, via);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Via drill size is too small"), _("Process rules dictate the minimum drill size which can be used"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					via->DrillingHole, PCB->minDrill, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
				IncrementUndoSerialNumber();
				Undo(false);
			}
		}
		END_LOOP;
	}

	FreeConnectionLookupMemory();
	TheFlag = FOUNDFLAG;
	Bloat = 0;

	/* check silkscreen minimum widths outside of elements */
	/* XXX - need to check text and polygons too! */
	TheFlag = SELECTEDFLAG;
	if (!IsBad) {
		SILKLINE_LOOP(PCB->Data);
		{
			if (line->Thickness < PCB->minSlk) {
				SET_FLAG(TheFlag, line);
				DrawLine(layer, line);
				drcerr_count++;
				SetThing(LINE_TYPE, layer, line, line);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Silk line is too thin"), _("Process specifications dictate a minimum silkscreen feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					line->Thickness, PCB->minSlk, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
					break;
				}
			}
		}
		ENDALL_LOOP;
	}

	/* check silkscreen minimum widths inside of elements */
	/* XXX - need to check text and polygons too! */
	TheFlag = SELECTEDFLAG;
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
				char *title;
				char *name;
				char *buffer;
				int buflen;

				SET_FLAG(TheFlag, element);
				DrawElement(element);
				drcerr_count++;
				SetThing(ELEMENT_TYPE, element, element, element);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);

				title = _("Element %s has %i silk lines which are too thin");
				name = (char *) UNKNOWN(NAMEONPCB_NAME(element));

				/* -4 is for the %s and %i place-holders */
				/* +11 is the max printed length for a 32 bit integer */
				/* +1 is for the \0 termination */
				buflen = strlen(title) - 4 + strlen(name) + 11 + 1;
				buffer = (char *) malloc(buflen);
				snprintf(buffer, buflen, title, name, tmpcnt);

				violation = pcb_drc_violation_new(buffer, _("Process specifications dictate a minimum silkscreen\n" "feature-width that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					TRUE,	/* MEASUREMENT OF ERROR KNOWN */
																					0,	/* MINIMUM OFFENDING WIDTH UNKNOWN */
																					PCB->minSlk, object_count, object_id_list, object_type_list);
				free(buffer);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = true;
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
		Message(_("Warning:  %d pad%s the nopaste flag set.\n"), nopastecnt, nopastecnt > 1 ? "s have" : " has");
	}
	return IsBad ? -drcerr_count : drcerr_count;
}
