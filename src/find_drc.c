/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

static void GotoError(void);
static pcb_bool DRCFind(pcb_drc_list_t *lst, int What, void *ptr1, void *ptr2, void *ptr3);

static unsigned long int pcb_drc_next_uid = 0;

static pcb_drc_violation_t *pcb_drc_violation_new(
	const char *type, const char *title, const char *explanation,
	pcb_bool have_measured, pcb_coord_t measured_value,
	pcb_coord_t required_value, pcb_idpath_list_t objs[2])
{
	pcb_drc_violation_t *violation = calloc(sizeof(pcb_drc_violation_t), 1);

	pcb_drc_next_uid++;
	violation->uid = pcb_drc_next_uid;

	violation->type = pcb_strdup(type);
	violation->title = pcb_strdup(title);
	violation->explanation = pcb_strdup(explanation);
	violation->have_measured = have_measured;
	violation->measured_value = measured_value;
	violation->required_value = required_value;
	memcpy(&violation->objs, objs, sizeof(violation->objs));
	memset(objs, 0, sizeof(violation->objs));
	return violation;
}

static gds_t drc_dialog_message;
static void reset_drc_dialog_message(void)
{
	gds_truncate(&drc_dialog_message, 0);
/*	if (pcb_gui->drc_gui != NULL)
		pcb_gui->drc_gui->reset_drc_dialog_message();*/
}

static void append_drc_dialog_message(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	pcb_append_vprintf(&drc_dialog_message, fmt, ap);
	va_end(ap);
}

static void GotoError(void);

/* Build a list of the of offending items by ID */
static void drc_append_obj(pcb_idpath_list_t objs[2], pcb_any_obj_t *obj)
{
	pcb_idpath_t *idp;

	switch(obj->type) {
	case PCB_OBJ_LINE:
	case PCB_OBJ_ARC:
	case PCB_OBJ_POLY:
	case PCB_OBJ_PSTK:
	case PCB_OBJ_RAT:
		idp = pcb_obj2idpath(obj);
		if (idp == NULL)
			pcb_message(PCB_MSG_ERROR, "Internal error in drc_append_obj: can not resolve object id path\n");
		else
			pcb_idpath_list_append(&objs[0], idp);
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "Internal error in drc_append_obj: unknown object type %i\n", obj->type);
	}
}

static void append_drc_violation(pcb_drc_list_t *lst, pcb_drc_violation_t *violation)
{
#if 0
	if (pcb_gui->drc_gui != NULL) {
		pcb_gui->drc_gui->append_drc_violation(violation);
	}
	else {
		/* Fallback to formatting the violation message as text */
		append_drc_dialog_message("%s\n", violation->title);
		append_drc_dialog_message("%m+near %$mD\n", conf_core.editor.grid_unit->allow, violation->x, violation->y);
		GotoError();
	}

	if (pcb_gui->drc_gui == NULL || pcb_gui->drc_gui->log_drc_violations) {
		pcb_message(PCB_MSG_WARNING, "WARNING!  Design Rule error - %s\n", violation->title);
		pcb_message(PCB_MSG_WARNING, "%m+near location %$mD\n", conf_core.editor.grid_unit->allow, violation->x, violation->y);
	}
#endif

	pcb_drc_list_append(lst, violation);
}

void drc_auto_loc(pcb_drc_violation_t *v)
{
	int g;
	pcb_box_t b;
	pcb_any_obj_t *obj;
	pcb_idpath_t *idp;

	/* special case: no object - leave coords unloaded/invalid */
	if ((pcb_idpath_list_length(&v->objs[0]) < 1) && (pcb_idpath_list_length(&v->objs[1]) < 1))
		return;

	/* special case: single objet in group A, use the center */
	if (pcb_idpath_list_length(&v->objs[0]) == 1) {
		idp = pcb_idpath_list_first(&v->objs[0]);
		obj = pcb_idpath2obj(PCB->Data, idp);
		if (obj != NULL) {
			v->have_coord = 1;
			pcb_obj_center(obj, &v->x, &v->y);
			memcpy(&v->bbox, &obj->BoundingBox, sizeof(obj->BoundingBox));
			pcb_box_enlarge(&v->bbox, 0.25, 0.25);
			return;
		}
	}

	b.X1 = b.Y1 = PCB_MAX_COORD;
	b.X2 = b.Y2 = -PCB_MAX_COORD;
	for(g = 0; g < 2; g++) {
		for(idp = pcb_idpath_list_first(&v->objs[g]); idp != NULL; idp = pcb_idpath_list_next(idp)) {
			obj = pcb_idpath2obj(PCB->Data, idp);
			if (obj != NULL) {
				v->have_coord = 1;
				pcb_box_bump_box(&b, &obj->BoundingBox);
			}
		}
	}

	if (v->have_coord) {
		v->x = (b.X1 + b.X2)/2;
		v->y = (b.Y1 + b.Y2)/2;
		memcpy(&v->bbox, &b, sizeof(b));
		pcb_box_enlarge(&v->bbox, 0.25, 0.25);
	}
}


/* message when asked about continuing DRC checks after next violation is found. */
#define DRC_CONTINUE "Press Next to continue DRC checking"
#define DRC_NEXT "Next"
#define DRC_CANCEL "Cancel"

static int throw_drc_dialog(void)
{
	int r = 0;
#if 0
	if (pcb_gui->drc_gui != NULL) {
		r = pcb_gui->drc_gui->throw_drc_dialog();
	}
	else {
		/* Fallback to formatting the violation message as text */
		append_drc_dialog_message(DRC_CONTINUE);
		r = pcb_gui->confirm_dialog(drc_dialog_message.array, DRC_CANCEL, DRC_NEXT);
		reset_drc_dialog_message();
	}
#endif
	return r;
}

/* DRC clearance callback */
static pcb_r_dir_t drc_callback(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon, int type, void *ptr1, void *ptr2, void *user_data)
{
	const char *message;
	pcb_drc_violation_t *violation;
	pcb_line_t *line = (pcb_line_t *)ptr2;
	pcb_arc_t *arc = (pcb_arc_t *)ptr2;
	pcb_pstk_t *ps = (pcb_pstk_t *)ptr2;
	pcb_idpath_list_t objs[2];

	memset(objs, 0, sizeof(objs));

	switch (type) {
	case PCB_OBJ_LINE:
		if (line->Clearance < 2 * conf_core.design.bloat) {
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_SET(TheFlag, line);
			message = "Line with insufficient clearance inside polygon\n";
			goto doIsBad;
		}
		break;
	case PCB_OBJ_ARC:
		if (arc->Clearance < 2 * conf_core.design.bloat) {
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_SET(TheFlag, arc);
			message = "Arc with insufficient clearance inside polygon\n";
			goto doIsBad;
		}
		break;
	case PCB_OBJ_PSTK:
		if (pcb_pstk_drc_check_clearance(ps, polygon, 2 * conf_core.design.bloat) != 0) {
			pcb_undo_add_obj_to_flag(ptr2);
			PCB_FLAG_SET(TheFlag, ps);
			message = "Padstack with insufficient clearance inside polygon\n";
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
	drc_append_obj(objs, (pcb_any_obj_t *)ptr2);
	violation = pcb_drc_violation_new("short", message,
		"Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short.",
		pcb_false, /* MEASUREMENT OF ERROR UNKNOWN */
		0, /* MAGNITUDE OF ERROR UNKNOWN */
		conf_core.design.bloat, objs);
	drc_auto_loc(violation);
	if (!throw_drc_dialog()) {
		IsBad = pcb_true;
		return PCB_R_DIR_FOUND_CONTINUE;
	}
	pcb_undo_inc_serial();
	pcb_undo(pcb_true);
	return PCB_R_DIR_NOT_FOUND;
}


unsigned long pcb_obj_type2oldtype(pcb_objtype_t type);

static int drc_text(pcb_drc_list_t *lst, pcb_layer_t *layer, pcb_text_t *text, pcb_coord_t min_wid)
{
	pcb_drc_violation_t *violation;
	pcb_idpath_list_t objs[2];

	memset(objs, 0, sizeof(objs));

	if (text->thickness == 0)
		return 0; /* automatic thickness is always valid - ensured by the renderer */
	if (text->thickness < min_wid) {
		pcb_undo_add_obj_to_flag(text);
		PCB_FLAG_SET(TheFlag, text);
		pcb_text_invalidate_draw(layer, text);
		drcerr_count++;
		drc_append_obj(objs, (pcb_any_obj_t *)text);
		violation = pcb_drc_violation_new("thin",
			"Text thickness is too thin",
			"Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced",
			pcb_true, /* MEASUREMENT OF ERROR KNOWN */
			text->thickness, min_wid, objs);
		drc_auto_loc(violation);
		append_drc_violation(lst, violation);
		if (!throw_drc_dialog()) {
			IsBad = pcb_true;
			return 1;
		}
		pcb_undo_inc_serial();
		pcb_undo(pcb_false);
	}
	return 0;
}

/* Check for DRC violations see if the connectivity changes when everything is bloated, or shrunk */
int pcb_drc_all(pcb_drc_list_t *lst)
{
	pcb_drc_violation_t *violation;
	int nopastecnt = 0;
	pcb_idpath_list_t objs[2];

	memset(objs, 0, sizeof(objs));

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
			DRCFind(lst, pcb_obj_type2oldtype(o->type), (void *)subc, (void *)o, (void *)o);
		}
	}
	PCB_END_LOOP;

	if (!IsBad)
	PCB_PADSTACK_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_DRC, padstack)
				&& DRCFind(lst, PCB_OBJ_PSTK, (void *) padstack, (void *) padstack, (void *) padstack)) {
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
			if (drc_text(lst, layer, text, conf_core.design.min_wid))
				break;
		}
		PCB_ENDALL_LOOP;

		PCB_SILK_COPPER_LOOP(PCB->Data);
		{
			if (drc_text(lst, layer, text, conf_core.design.min_slk))
				break;
		}
		PCB_ENDALL_LOOP;

		PCB_LINE_COPPER_LOOP(PCB->Data);
		{
			/* check line clearances in polygons */
			pcb_poly_plows(PCB->Data, PCB_OBJ_LINE, layer, line, drc_callback, lst);
			if (IsBad)
				break;
			if (line->Thickness < conf_core.design.min_wid) {
				pcb_undo_add_obj_to_flag(line);
				PCB_FLAG_SET(TheFlag, line);
				pcb_line_invalidate_draw(layer, line);
				drcerr_count++;
				drc_append_obj(objs, (pcb_any_obj_t *)line);
				violation = pcb_drc_violation_new("thin",
					"Line width is too thin",
					"Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced",
					pcb_true, /* MEASUREMENT OF ERROR KNOWN */
					line->Thickness, conf_core.design.min_wid, objs);
				drc_auto_loc(violation);
				append_drc_violation(lst, violation);
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
			pcb_poly_plows(PCB->Data, PCB_OBJ_ARC, layer, arc, drc_callback, lst);
			if (IsBad)
				break;
			if (arc->Thickness < conf_core.design.min_wid) {
				pcb_undo_add_obj_to_flag(arc);
				PCB_FLAG_SET(TheFlag, arc);
				pcb_arc_invalidate_draw(layer, arc);
				drcerr_count++;
				drc_append_obj(objs, (pcb_any_obj_t *)arc);
				violation = pcb_drc_violation_new("thin",
					"Arc width is too thin",
					"Process specifications dictate a minimum feature-width\n" "that can reliably be reproduced",
					pcb_true, /* MEASUREMENT OF ERROR KNOWN */
					arc->Thickness, conf_core.design.min_wid, objs);
				drc_auto_loc(violation);
				append_drc_violation(lst, violation);
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
			pcb_poly_plows(PCB->Data, PCB_OBJ_PSTK, padstack, padstack, drc_callback, lst);
			if (IsBad)
				break;
			pcb_pstk_drc_check_and_warn(padstack, &ring, &hole);
			if ((ring > 0) || (hole > 0)) {
				pcb_undo_add_obj_to_flag(padstack);
				PCB_FLAG_SET(TheFlag, padstack);
				pcb_pstk_invalidate_draw(padstack);
				if (ring) {
					drcerr_count++;
					drc_append_obj(objs, (pcb_any_obj_t *)padstack);
					violation = pcb_drc_violation_new("thin",
						"padstack annular ring too small",
						"Annular rings that are too small may erode during etching,\n" "resulting in a broken connection",
						pcb_true, /* MEASUREMENT OF ERROR KNOWN */
						ring,
						conf_core.design.min_ring, objs);
					drc_auto_loc(violation);
					append_drc_violation(lst, violation);
				}
				if (hole > 0) {
					drcerr_count++;
					drc_append_obj(objs, (pcb_any_obj_t *)padstack);
					violation = pcb_drc_violation_new("drill",
						"Padstack drill size is too small",
						"Process rules dictate the minimum drill size which can be used",
						pcb_true, /* MEASUREMENT OF ERROR KNOWN */
						hole, conf_core.design.min_drill, objs);
					drc_auto_loc(violation);
					append_drc_violation(lst, violation);
				}
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
#warning DRC TODO: need to check text and polygons too!
	TheFlag = PCB_FLAG_SELECTED;
	if (!IsBad) {
		PCB_LINE_SILK_LOOP(PCB->Data);
		{
			if (line->Thickness < conf_core.design.min_slk) {
				PCB_FLAG_SET(TheFlag, line);
				pcb_line_invalidate_draw(layer, line);
				drcerr_count++;
				drc_append_obj(objs, (pcb_any_obj_t *)line);
				violation = pcb_drc_violation_new("thin",
					"Silk line is too thin",
					"Process specifications dictate a minimum silkscreen feature-width\n" "that can reliably be reproduced",
					pcb_true, /* MEASUREMENT OF ERROR KNOWN */
					line->Thickness, conf_core.design.min_slk, objs);
				drc_auto_loc(violation);
				append_drc_violation(lst, violation);
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
		pcb_message(PCB_MSG_WARNING, "Warning:  %d pad%s the nopaste flag set.\n", nopastecnt, nopastecnt > 1 ? "s have" : " has");
	}
	return IsBad ? -drcerr_count : drcerr_count;
}



/* Check for DRC violations on a single net starting from the pad or pin
   sees if the connectivity changes when everything is bloated, or shrunk */
static pcb_bool DRCFind(pcb_drc_list_t *lst, int What, void *ptr1, void *ptr2, void *ptr3)
{
	pcb_drc_violation_t *violation;
	pcb_idpath_list_t objs[2];

	memset(objs, 0, sizeof(objs));

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
			drc_append_obj(objs, (pcb_any_obj_t *)thing_ptr2);
			violation = pcb_drc_violation_new("broken",
				"Potential for broken trace",
				"Insufficient overlap between objects can lead to broken tracks\n" "due to registration errors with old wheel style photo-plotters.",
				pcb_false, /* MEASUREMENT OF ERROR UNKNOWN */
				0, /* MAGNITUDE OF ERROR UNKNOWN */
				conf_core.design.shrink, objs);
			drc_auto_loc(violation);
			append_drc_violation(lst, violation);

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
		drc_append_obj(objs, (pcb_any_obj_t *)thing_ptr2);
		violation = pcb_drc_violation_new("short",
			"Copper areas too close",
			"Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short.",
			pcb_false, /* MEASUREMENT OF ERROR UNKNOWN */
			0, /* MAGNITUDE OF ERROR UNKNOWN */
			conf_core.design.bloat, objs);
		drc_auto_loc(violation);
		append_drc_violation(lst, violation);
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

/* center the display to show the offending item (thing) */
static void GotoError(void)
{
	pcb_coord_t X, Y;

	pcb_obj_center((pcb_any_obj_t *)thing_ptr2, &X, &Y);

	switch (thing_type) {
	case PCB_OBJ_LINE:
	case PCB_OBJ_ARC:
	case PCB_OBJ_POLY:
		pcb_layervis_change_group_vis(pcb_layer_id(PCB->Data, (pcb_layer_t *) thing_ptr1), pcb_true, pcb_true);
	}
	pcb_center_display(X, Y);
}
