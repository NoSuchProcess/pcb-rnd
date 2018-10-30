/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "drc.h"
#include "compat_nls.h"
#include "data_it.h"

#include "obj_arc_draw.h"
#include "obj_rat_draw.h"
#include "obj_line_draw.h"
#include "obj_poly_draw.h"
#include "obj_pstk_draw.h"

/* DRC related functions */

static void GotoError(void);
static pcb_bool DRCFind(int What, void *ptr1, void *ptr2, void *ptr3);

static pcb_drc_violation_t
	* pcb_drc_violation_new(const char *title,
													const char *explanation,
													pcb_coord_t x, pcb_coord_t y,
													pcb_angle_t angle,
													pcb_bool have_measured,
													pcb_coord_t measured_value,
													pcb_coord_t required_value, int object_count, long int *object_id_list, int *object_type_list)
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
	if (pcb_gui->drc_gui != NULL)
		pcb_gui->drc_gui->reset_drc_dialog_message();
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
	case PCB_OBJ_LINE:
	case PCB_OBJ_ARC:
	case PCB_OBJ_POLY:
	case PCB_OBJ_PSTK:
	case PCB_OBJ_RAT:
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
 * Locate the coordinates of offending item (thing)
 */
static void LocateError(pcb_coord_t * x, pcb_coord_t * y)
{
	switch (thing_type) {
	case PCB_OBJ_LINE:
		{
			pcb_line_t *line = (pcb_line_t *) thing_ptr3;
			*x = (line->Point1.X + line->Point2.X) / 2;
			*y = (line->Point1.Y + line->Point2.Y) / 2;
			break;
		}
	case PCB_OBJ_ARC:
		{
			pcb_arc_t *arc = (pcb_arc_t *) thing_ptr3;
			*x = arc->X;
			*y = arc->Y;
			break;
		}
	case PCB_OBJ_POLY:
		{
			pcb_poly_t *polygon = (pcb_poly_t *) thing_ptr3;
			*x = (polygon->Clipped->contours->xmin + polygon->Clipped->contours->xmax) / 2;
			*y = (polygon->Clipped->contours->ymin + polygon->Clipped->contours->ymax) / 2;
			break;
		}
	case PCB_OBJ_PSTK:
		{
			pcb_pstk_t *ps = (pcb_pstk_t *) thing_ptr3;
			*x = ps->x;
			*y = ps->y;
			break;
		}
	default:
		return;
	}
}


static void append_drc_violation(pcb_drc_violation_t * violation)
{
	if (pcb_gui->drc_gui != NULL) {
		pcb_gui->drc_gui->append_drc_violation(violation);
	}
	else {
		/* Fallback to formatting the violation message as text */
		append_drc_dialog_message("%s\n", violation->title);
		append_drc_dialog_message(_("%m+near %$mD\n"), conf_core.editor.grid_unit->allow, violation->x, violation->y);
		GotoError();
	}

	if (pcb_gui->drc_gui == NULL || pcb_gui->drc_gui->log_drc_violations) {
		pcb_message(PCB_MSG_WARNING, _("WARNING!  Design Rule error - %s\n"), violation->title);
		pcb_message(PCB_MSG_WARNING, _("%m+near location %$mD\n"), conf_core.editor.grid_unit->allow, violation->x, violation->y);
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

	if (pcb_gui->drc_gui != NULL) {
		r = pcb_gui->drc_gui->throw_drc_dialog();
	}
	else {
		/* Fallback to formatting the violation message as text */
		append_drc_dialog_message(DRC_CONTINUE);
		r = pcb_gui->confirm_dialog(drc_dialog_message.array, DRC_CANCEL, DRC_NEXT);
		reset_drc_dialog_message();
	}
	return r;
}

/* DRC clearance callback */
static pcb_r_dir_t drc_callback(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon, int type, void *ptr1, void *ptr2)
{
	const char *message;
	pcb_coord_t x, y;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	pcb_drc_violation_t *violation;

	pcb_line_t *line = (pcb_line_t *) ptr2;
	pcb_arc_t *arc = (pcb_arc_t *) ptr2;
	pcb_pstk_t *ps = (pcb_pstk_t *) ptr2;

	thing_type = type;
	thing_ptr1 = ptr1;
	thing_ptr2 = ptr2;
	thing_ptr3 = ptr2;
	switch (type) {
	case PCB_OBJ_LINE:
		if (line->Clearance < 2 * conf_core.design.bloat) {
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_SET(TheFlag, line);
			message = _("Line with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case PCB_OBJ_ARC:
		if (arc->Clearance < 2 * conf_core.design.bloat) {
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_SET(TheFlag, arc);
			message = _("Arc with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	case PCB_OBJ_PSTK:
		if (pcb_pstk_drc_check_clearance(ps, polygon, 2 * conf_core.design.bloat) != 0) {
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_SET(TheFlag, ps);
			message = _("Padstack with insufficient clearance inside polygon\n");
			goto doIsBad;
		}
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "hace: Bad Plow object in callback\n");
	}
	return PCB_R_DIR_NOT_FOUND;

doIsBad:
	pcb_undo_add_obj_to_flag(polygon);
	PCB_FLAG_SET(PCB_FLAG_FOUND, polygon);
	pcb_poly_invalidate_draw(layer, polygon);
	pcb_draw_obj((pcb_any_obj_t *)ptr2);
	drcerr_count++;
	LocateError(&x, &y);
	BuildObjectList(&object_count, &object_id_list, &object_type_list);
	violation = pcb_drc_violation_new(message, _("Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short."), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																		pcb_false,	/* MEASUREMENT OF ERROR UNKNOWN */
																		0,	/* MAGNITUDE OF ERROR UNKNOWN */
																		conf_core.design.bloat, object_count, object_id_list, object_type_list);
	append_drc_violation(violation);
	pcb_drc_violation_free(violation);
	free(object_id_list);
	free(object_type_list);
	if (!throw_drc_dialog()) {
		IsBad = pcb_true;
		return PCB_R_DIR_FOUND_CONTINUE;
	}
	pcb_undo_inc_serial();
	pcb_undo(pcb_true);
	return PCB_R_DIR_NOT_FOUND;
}


unsigned long pcb_obj_type2oldtype(pcb_objtype_t type);

static int drc_text(pcb_layer_t *layer, pcb_text_t *text, pcb_coord_t min_wid, pcb_coord_t *x, pcb_coord_t *y)
{
	pcb_drc_violation_t *violation;
	int object_count;
	long int *object_id_list;
	int *object_type_list;

	if (text->thickness == 0)
		return 0; /* automatic thickness is always valid - ensured by the renderer */
	if (text->thickness < min_wid) {
		pcb_undo_add_obj_to_flag(text);
		PCB_FLAG_SET(TheFlag, text);
		pcb_text_invalidate_draw(layer, text);
		drcerr_count++;
		SetThing(PCB_OBJ_TEXT, layer, text, text);
		LocateError(x, y);
		BuildObjectList(&object_count, &object_id_list, &object_type_list);
		violation = pcb_drc_violation_new(_("Text thickness is too thin"), _("Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced"), *x, *y, 0,	/* ANGLE OF ERROR UNKNOWN */
																			pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																			text->thickness, min_wid, object_count, object_id_list, object_type_list);
		append_drc_violation(violation);
		pcb_drc_violation_free(violation);
		free(object_id_list);
		free(object_type_list);
		if (!throw_drc_dialog()) {
			IsBad = pcb_true;
			return 1;
		}
		pcb_undo_inc_serial();
		pcb_undo(pcb_false);
	}
	return 0;
}

/*-----------------------------------------------------------------------------
 * Check for DRC violations
 * see if the connectivity changes when everything is bloated, or shrunk
 */
int pcb_drc_all(void)
{
	pcb_coord_t x, y;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	pcb_drc_violation_t *violation;
	int nopastecnt = 0;

	reset_drc_dialog_message();

	IsBad = pcb_false;
	drcerr_count = 0;
	pcb_layervis_save_stack();
	pcb_layervis_reset_stack();
	pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
	pcb_conn_lookup_init();

	TheFlag = PCB_FLAG_FOUND | PCB_FLAG_DRC | PCB_FLAG_SELECTED;

	if (pcb_reset_conns(pcb_true)) {
		pcb_undo_inc_serial();
		pcb_draw();
	}

	User = pcb_false;

	/* search net (bloat) from subcircuit terminals */
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_any_obj_t *o;
		pcb_data_it_t it;

		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			if (o->term == NULL) /* only terminals can be starting point of DRC net checks */
				continue;
			if (o->parent_type == PCB_PARENT_LAYER) { /* for layer objects, care about the ones on copper layers only */
				pcb_layer_type_t lyt = pcb_layer_flags_(o->parent.layer);
				if (!(lyt & PCB_LYT_COPPER))
					continue;
			}
			DRCFind(pcb_obj_type2oldtype(o->type), (void *)subc, (void *)o, (void *)o);
		}
	}
	PCB_END_LOOP;

	if (!IsBad)
	PCB_PADSTACK_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_DRC, padstack)
				&& DRCFind(PCB_OBJ_PSTK, (void *) padstack, (void *) padstack, (void *) padstack)) {
			IsBad = pcb_true;
			break;
		}
	}
	PCB_END_LOOP;

	TheFlag = (IsBad) ? PCB_FLAG_DRC : (PCB_FLAG_FOUND | PCB_FLAG_DRC | PCB_FLAG_SELECTED);
	pcb_reset_conns(pcb_false);
	TheFlag = PCB_FLAG_SELECTED;
	/* check minimum widths and polygon clearances */
	if (!IsBad) {
		PCB_TEXT_COPPER_LOOP(PCB->Data);
		{
			if (drc_text(layer, text, conf_core.design.min_wid, &x, &y))
				break;
		}
		PCB_ENDALL_LOOP;

		PCB_SILK_COPPER_LOOP(PCB->Data);
		{
			if (drc_text(layer, text, conf_core.design.min_slk, &x, &y))
				break;
		}
		PCB_ENDALL_LOOP;

		PCB_LINE_COPPER_LOOP(PCB->Data);
		{
			/* check line clearances in polygons */
			pcb_poly_plows(PCB->Data, PCB_OBJ_LINE, layer, line, drc_callback);
			if (IsBad)
				break;
			if (line->Thickness < conf_core.design.min_wid) {
				pcb_undo_add_obj_to_flag(line);
				PCB_FLAG_SET(TheFlag, line);
				pcb_line_invalidate_draw(layer, line);
				drcerr_count++;
				SetThing(PCB_OBJ_LINE, layer, line, line);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Line width is too thin"), _("Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					line->Thickness, conf_core.design.min_wid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				pcb_undo_inc_serial();
				pcb_undo(pcb_false);
			}
		}
		PCB_ENDALL_LOOP;
	}
	if (!IsBad) {
		PCB_ARC_COPPER_LOOP(PCB->Data);
		{
			pcb_poly_plows(PCB->Data, PCB_OBJ_ARC, layer, arc, drc_callback);
			if (IsBad)
				break;
			if (arc->Thickness < conf_core.design.min_wid) {
				pcb_undo_add_obj_to_flag(arc);
				PCB_FLAG_SET(TheFlag, arc);
				pcb_arc_invalidate_draw(layer, arc);
				drcerr_count++;
				SetThing(PCB_OBJ_ARC, layer, arc, arc);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Arc width is too thin"), _("Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					arc->Thickness, conf_core.design.min_wid, object_count, object_id_list, object_type_list);
				append_drc_violation(violation);
				pcb_drc_violation_free(violation);
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
				pcb_undo_inc_serial();
				pcb_undo(pcb_false);
			}
		}
		PCB_ENDALL_LOOP;
	}

	if (!IsBad) {
		PCB_PADSTACK_LOOP(PCB->Data);
		{
			pcb_coord_t ring = 0, hole = 0;
			pcb_poly_plows(PCB->Data, PCB_OBJ_PSTK, padstack, padstack, drc_callback);
			if (IsBad)
				break;
			pcb_pstk_drc_check_and_warn(padstack, &ring, &hole);
			if ((ring > 0) || (hole > 0)) {
				pcb_undo_add_obj_to_flag(padstack);
				PCB_FLAG_SET(TheFlag, padstack);
				pcb_pstk_invalidate_draw(padstack);
				if (ring) {
					drcerr_count++;
					SetThing(PCB_OBJ_PSTK, padstack, padstack, padstack);
					LocateError(&x, &y);
					BuildObjectList(&object_count, &object_id_list, &object_type_list);
					violation = pcb_drc_violation_new(_("padstack annular ring too small"), _("Annular rings that are too small may erode during etching,\n" "resulting in a broken connection"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																						pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																						ring,
																						conf_core.design.min_ring, object_count, object_id_list, object_type_list);
					append_drc_violation(violation);
					pcb_drc_violation_free(violation);
				}
				if (hole > 0) {
					drcerr_count++;
					SetThing(PCB_OBJ_PSTK, padstack, padstack, padstack);
					LocateError(&x, &y);
					BuildObjectList(&object_count, &object_id_list, &object_type_list);
					violation = pcb_drc_violation_new(_("Padstack drill size is too small"), _("Process rules dictate the minimum drill size which can be used"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																						pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																						hole, conf_core.design.min_drill, object_count, object_id_list, object_type_list);
					append_drc_violation(violation);
					pcb_drc_violation_free(violation);
				}
				free(object_id_list);
				free(object_type_list);
				if (!throw_drc_dialog()) {
					IsBad = pcb_true;
					break;
				}
			}
		}
		PCB_END_LOOP;
	}

	pcb_conn_lookup_uninit();
	TheFlag = PCB_FLAG_FOUND;
	Bloat = 0;

	/* check silkscreen minimum widths outside of subcircuits */
	/* XXX - need to check text and polygons too! */
	TheFlag = PCB_FLAG_SELECTED;
	if (!IsBad) {
		PCB_LINE_SILK_LOOP(PCB->Data);
		{
			if (line->Thickness < conf_core.design.min_slk) {
				PCB_FLAG_SET(TheFlag, line);
				pcb_line_invalidate_draw(layer, line);
				drcerr_count++;
				SetThing(PCB_OBJ_LINE, layer, line, line);
				LocateError(&x, &y);
				BuildObjectList(&object_count, &object_id_list, &object_type_list);
				violation = pcb_drc_violation_new(_("Silk line is too thin"), _("Process specifications dictate a minimum silkscreen feature-width\n" "that can reliably be reproduced"), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																					pcb_true,	/* MEASUREMENT OF ERROR KNOWN */
																					line->Thickness, conf_core.design.min_slk, object_count, object_id_list, object_type_list);
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
		PCB_ENDALL_LOOP;
	}

	if (IsBad) {
		pcb_undo_inc_serial();
	}

	pcb_layervis_restore_stack();
	pcb_event(PCB_EVENT_LAYERVIS_CHANGED, NULL);
	pcb_gui->invalidate_all();

	if (nopastecnt > 0) {
		pcb_message(PCB_MSG_WARNING, _("Warning:  %d pad%s the nopaste flag set.\n"), nopastecnt, nopastecnt > 1 ? "s have" : " has");
	}
	return IsBad ? -drcerr_count : drcerr_count;
}



/*-----------------------------------------------------------------------------
 * Check for DRC violations on a single net starting from the pad or pin
 * sees if the connectivity changes when everything is bloated, or shrunk
 */
static pcb_bool DRCFind(int What, void *ptr1, void *ptr2, void *ptr3)
{
	pcb_coord_t x, y;
	int object_count;
	long int *object_id_list;
	int *object_type_list;
	pcb_drc_violation_t *violation;

	if (conf_core.design.shrink != 0) {
		Bloat = -conf_core.design.shrink;
		TheFlag = PCB_FLAG_DRC | PCB_FLAG_SELECTED;
		ListStart(ptr2);
		DoIt(pcb_true, pcb_false);
		/* ok now the shrunk net has the PCB_FLAG_SELECTED set */
		DumpList();
		TheFlag = PCB_FLAG_FOUND;
		ListStart(ptr2);
		Bloat = 0;
		drc = pcb_true;									/* abort the search if we find anything not already found */
		if (DoIt(pcb_true, pcb_false)) {
			DumpList();
			/* make the flag changes undoable */
			TheFlag = PCB_FLAG_FOUND | PCB_FLAG_SELECTED;
			pcb_reset_conns(pcb_false);
			User = pcb_true;
			drc = pcb_false;
			Bloat = -conf_core.design.shrink;
			TheFlag = PCB_FLAG_SELECTED;
			ListStart(ptr2);
			DoIt(pcb_true, pcb_true);
			DumpList();
			ListStart(ptr2);
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
																				conf_core.design.shrink, object_count, object_id_list, object_type_list);
			append_drc_violation(violation);
			pcb_drc_violation_free(violation);
			free(object_id_list);
			free(object_type_list);

			if (!throw_drc_dialog())
				return pcb_true;
			pcb_undo_inc_serial();
			pcb_undo(pcb_true);
		}
		DumpList();
	}
	/* now check the bloated condition */
	drc = pcb_false;
	pcb_reset_conns(pcb_false);
	TheFlag = PCB_FLAG_FOUND;
	ListStart(ptr2);
	Bloat = conf_core.design.bloat;
	drc = pcb_true;
	while (DoIt(pcb_true, pcb_false)) {
		DumpList();
		/* make the flag changes undoable */
		TheFlag = PCB_FLAG_FOUND | PCB_FLAG_SELECTED;
		pcb_reset_conns(pcb_false);
		User = pcb_true;
		drc = pcb_false;
		Bloat = 0;
		TheFlag = PCB_FLAG_SELECTED;
		ListStart(ptr2);
		DoIt(pcb_true, pcb_true);
		DumpList();
		TheFlag = PCB_FLAG_FOUND;
		ListStart(ptr2);
		Bloat = conf_core.design.bloat;
		drc = pcb_true;
		DoIt(pcb_true, pcb_true);
		DumpList();
		drcerr_count++;
		LocateError(&x, &y);
		BuildObjectList(&object_count, &object_id_list, &object_type_list);
		violation = pcb_drc_violation_new(_("Copper areas too close"), _("Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short."), x, y, 0,	/* ANGLE OF ERROR UNKNOWN */
																			pcb_false,	/* MEASUREMENT OF ERROR UNKNOWN */
																			0,	/* MAGNITUDE OF ERROR UNKNOWN */
																			conf_core.design.bloat, object_count, object_id_list, object_type_list);
		append_drc_violation(violation);
		pcb_drc_violation_free(violation);
		free(object_id_list);
		free(object_type_list);
		User = pcb_false;
		drc = pcb_false;
		if (!throw_drc_dialog())
			return pcb_true;
		pcb_undo_inc_serial();
		pcb_undo(pcb_true);
		/* highlight the rest of the encroaching net so it's not reported again */
		TheFlag |= PCB_FLAG_SELECTED;
		Bloat = 0;
		ListStart(thing_ptr2);
		DoIt(pcb_true, pcb_true);
		DumpList();
		drc = pcb_true;
		Bloat = conf_core.design.bloat;
		ListStart(ptr2);
	}
	drc = pcb_false;
	DumpList();
	TheFlag = PCB_FLAG_FOUND | PCB_FLAG_SELECTED | PCB_FLAG_DRC;
	pcb_reset_conns(pcb_false);
	return pcb_false;
}

/*----------------------------------------------------------------------------
 * center the display to show the offending item (thing)
 */
static void GotoError(void)
{
	pcb_coord_t X, Y;

	LocateError(&X, &Y);

	switch (thing_type) {
	case PCB_OBJ_LINE:
	case PCB_OBJ_ARC:
	case PCB_OBJ_POLY:
		pcb_layervis_change_group_vis(pcb_layer_id(PCB->Data, (pcb_layer_t *) thing_ptr1), pcb_true, pcb_true);
	}
	pcb_center_display(X, Y);
}
