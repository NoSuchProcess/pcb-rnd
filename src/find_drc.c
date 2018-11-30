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
#include "view.h"
#include "data_it.h"

#include "obj_arc_draw.h"
#include "obj_rat_draw.h"
#include "obj_line_draw.h"
#include "obj_poly_draw.h"
#include "obj_pstk_draw.h"

static pcb_bool DRCFind(pcb_view_list_t *lst, int What, void *ptr1, void *ptr2, void *ptr3);

/* Load drc-specific fields of a view; if measured_value is NULL, it is not available */
void pcb_drc_set_data(pcb_view_t *violation, const pcb_coord_t *measured_value, pcb_coord_t required_value)
{
	violation->data_type = PCB_VIEW_DRC;
	violation->data.drc.required_value = required_value;
	if (measured_value != NULL) {
		violation->data.drc.have_measured = 1;
		violation->data.drc.measured_value = *measured_value;
	}
	else
		violation->data.drc.have_measured = 0;
}

/* DRC clearance callback */
static pcb_r_dir_t drc_callback(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon, int type, void *ptr1, void *ptr2, void *user_data)
{
	const char *message;
	pcb_view_t *violation;
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
	violation = pcb_view_new("short", message, "Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short.");
	pcb_drc_set_data(violation, NULL, conf_core.design.bloat);
	pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)ptr2);
	pcb_drc_set_bbox_by_objs(PCB->Data, violation);
	pcb_undo_inc_serial();
	pcb_undo(pcb_true);
	return PCB_R_DIR_NOT_FOUND;
}


unsigned long pcb_obj_type2oldtype(pcb_objtype_t type);

static int drc_text(pcb_view_list_t *lst, pcb_layer_t *layer, pcb_text_t *text, pcb_coord_t min_wid)
{
	pcb_view_t *violation;
	pcb_idpath_list_t objs[2];

	memset(objs, 0, sizeof(objs));

	if (text->thickness == 0)
		return 0; /* automatic thickness is always valid - ensured by the renderer */
	if (text->thickness < min_wid) {
		pcb_undo_add_obj_to_flag(text);
		PCB_FLAG_SET(TheFlag, text);
		pcb_text_invalidate_draw(layer, text);
		drcerr_count++;
		violation = pcb_view_new("thin", "Text thickness is too thin", "Process specifications dictate a minimum feature-width\nthat can reliably be reproduced");
		pcb_drc_set_data(violation, &text->thickness, min_wid);
		pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)text);
		pcb_drc_set_bbox_by_objs(PCB->Data, violation);
		pcb_view_list_append(lst, violation);
		pcb_undo_inc_serial();
		pcb_undo(pcb_false);
	}
	return 0;
}

/* Check for DRC violations see if the connectivity changes when everything is bloated, or shrunk */
int pcb_drc_all(pcb_view_list_t *lst)
{
	pcb_view_t *violation;
	int nopastecnt = 0;
	pcb_idpath_list_t objs[2];

	memset(objs, 0, sizeof(objs));

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
				violation = pcb_view_new("thin", "Line width is too thin", "Process specifications dictate a minimum feature-width\nthat can reliably be reproduced");
				pcb_drc_set_data(violation, &line->Thickness, conf_core.design.min_wid);
				pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)line);
				pcb_drc_set_bbox_by_objs(PCB->Data, violation);
				pcb_view_list_append(lst, violation);
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
				violation = pcb_view_new("thin", "Arc width is too thin", "Process specifications dictate a minimum feature-width\nthat can reliably be reproduced");
				pcb_drc_set_data(violation, &arc->Thickness, conf_core.design.min_wid);
				pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)arc);
				pcb_drc_set_bbox_by_objs(PCB->Data, violation);
				pcb_view_list_append(lst, violation);
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
					violation = pcb_view_new("thin", "padstack annular ring too small", "Annular rings that are too small may erode during etching,\nresulting in a broken connection");
					pcb_drc_set_data(violation, &ring, conf_core.design.min_ring);
					pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)padstack);
					pcb_drc_set_bbox_by_objs(PCB->Data, violation);
					pcb_view_list_append(lst, violation);
				}
				if (hole > 0) {
					drcerr_count++;
					violation = pcb_view_new("drill", "Padstack drill size is too small", "Process rules dictate the minimum drill size which can be used");
					pcb_drc_set_data(violation, &hole, conf_core.design.min_drill);
					pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)padstack);
					pcb_drc_set_bbox_by_objs(PCB->Data, violation);
					pcb_view_list_append(lst, violation);
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
				violation = pcb_view_new("thin", "Silk line is too thin", "Process specifications dictate a minimum silkscreen feature-width\nthat can reliably be reproduced");
				pcb_drc_set_data(violation, &line->Thickness, conf_core.design.min_slk);
				pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)line);
				pcb_drc_set_bbox_by_objs(PCB->Data, violation);
				pcb_view_list_append(lst, violation);
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
static pcb_bool DRCFind(pcb_view_list_t *lst, int What, void *ptr1, void *ptr2, void *ptr3)
{
	pcb_view_t *violation;
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
			violation = pcb_view_new("broken", "Potential for broken trace", "Insufficient overlap between objects can lead to broken tracks\ndue to registration errors with old wheel style photo-plotters.");
			pcb_drc_set_data(violation, NULL, conf_core.design.shrink);
			pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)pcb_found_obj1);
			pcb_drc_append_obj(violation, 1, (pcb_any_obj_t *)pcb_found_obj2);
			pcb_drc_set_bbox_by_objs(PCB->Data, violation);
			pcb_view_list_append(lst, violation);

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
		violation = pcb_view_new("short", "Copper areas too close", "Circuits that are too close may bridge during imaging, etching,\nplating, or soldering processes resulting in a direct short.");
		pcb_drc_set_data(violation, NULL, conf_core.design.bloat);
		pcb_drc_append_obj(violation, 0, (pcb_any_obj_t *)pcb_found_obj1);
		pcb_drc_append_obj(violation, 1, (pcb_any_obj_t *)pcb_found_obj2);
		pcb_drc_set_bbox_by_objs(PCB->Data, violation);
		pcb_view_list_append(lst, violation);
		User = pcb_false;
		drc = pcb_false;
		pcb_undo_inc_serial();
		pcb_undo(pcb_true);
		/* highlight the rest of the encroaching net so it's not reported again */
		TheFlag |= PCB_FLAG_SELECTED;
		Bloat = 0;
		ListStart(pcb_found_obj1);
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
