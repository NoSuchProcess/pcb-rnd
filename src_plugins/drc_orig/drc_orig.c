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

#include "config.h"

#include "drc.h"
#include "view.h"
#include "data_it.h"
#include "conf_core.h"
#include "find.h"
#include "event.h"
#include "plugins.h"
#include "layer_vis.h"

#include "obj_arc_draw.h"
#include "obj_rat_draw.h"
#include "obj_line_draw.h"
#include "obj_text_draw.h"
#include "obj_poly_draw.h"
#include "obj_pstk_draw.h"

#include "obj_subc_list.h"

static const char *drc_orig_cookie = "drc_orig";

TODO("find: get rid of this global state")
extern pcb_coord_t Bloat;

/* DRC clearance callback */
static pcb_r_dir_t drc_callback(pcb_data_t *data, pcb_layer_t *layer, pcb_poly_t *polygon, int type, void *ptr1, void *ptr2, void *user_data)
{
	const char *message;
	pcb_view_t *violation;
	pcb_view_list_t *lst = user_data;
	pcb_line_t *line = (pcb_line_t *)ptr2;
	pcb_arc_t *arc = (pcb_arc_t *)ptr2;
	pcb_pstk_t *ps = (pcb_pstk_t *)ptr2;

	switch (type) {
	case PCB_OBJ_LINE:
		if (line->Clearance < 2 * conf_core.design.bloat) {
			message = "Line with insufficient clearance inside polygon";
			goto doIsBad;
		}
		break;
	case PCB_OBJ_ARC:
		if (arc->Clearance < 2 * conf_core.design.bloat) {
			message = "Arc with insufficient clearance inside polygon";
			goto doIsBad;
		}
		break;
	case PCB_OBJ_PSTK:
		if (pcb_pstk_drc_check_clearance(ps, polygon, 2 * conf_core.design.bloat) != 0) {
			message = "Padstack with insufficient clearance inside polygon";
			goto doIsBad;
		}
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "hace: Bad Plow object in callback\n");
	}
	return PCB_R_DIR_NOT_FOUND;

doIsBad:
	pcb_poly_invalidate_draw(layer, polygon);
	pcb_draw_obj((pcb_any_obj_t *)ptr2);
	violation = pcb_view_new("short", message, "Circuits that are too close may bridge during imaging, etching,\n" "plating, or soldering processes resulting in a direct short.");
	pcb_drc_set_data(violation, NULL, conf_core.design.bloat);
	pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)ptr2);
	pcb_view_set_bbox_by_objs(PCB->Data, violation);
	pcb_view_list_append(lst, violation);
	return PCB_R_DIR_NOT_FOUND;
}

static int drc_text(pcb_view_list_t *lst, pcb_layer_t *layer, pcb_text_t *text, pcb_coord_t min_wid)
{
	pcb_view_t *violation;

	if (text->thickness == 0)
		return 0; /* automatic thickness is always valid - ensured by the renderer */
	if (text->thickness < min_wid) {
		pcb_text_invalidate_draw(layer, text);
		violation = pcb_view_new("thin", "Text thickness is too thin", "Process specifications dictate a minimum feature-width\nthat can reliably be reproduced");
		pcb_drc_set_data(violation, &text->thickness, min_wid);
		pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)text);
		pcb_view_set_bbox_by_objs(PCB->Data, violation);
		pcb_view_list_append(lst, violation);
	}
	return 0;
}

typedef struct {
	pcb_find_t fa, fb;
	pcb_data_t *data;
	pcb_view_list_t *lst;
	unsigned fast:1;
	unsigned shrunk:1;
} drc_ctx_t;

/* evaluates to true if obj was marked on list (fa or fb) */
#define IS_FOUND(obj, list) (PCB_DFLAG_TEST(&(obj->Flags), ctx->list.mark))

static int drc_broken_cb(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	pcb_view_t *violation;
	drc_ctx_t *ctx = fctx->user_data;

	if (arrived_from == NULL) /* ingore the starting object - it must be marked as we started from the same object in the first search */
		return 0;

	/* broken if new object is not marked in the shrunk search (fa) but
	   the arrived_from object was marked (so we notify direct breaks only) */
	if (!IS_FOUND(new_obj, fa) && IS_FOUND(arrived_from, fa)) {

		if (ctx->shrunk) {
			violation = pcb_view_new("broken", "Potential for broken trace", "Insufficient overlap between objects can lead to broken tracks\ndue to registration errors with old wheel style photo-plotters.");
			pcb_drc_set_data(violation, NULL, conf_core.design.shrink);
		}
		else {
			violation = pcb_view_new("short", "Copper areas too close", "Circuits that are too close may bridge during imaging, etching,\nplating, or soldering processes resulting in a direct short.");
			pcb_drc_set_data(violation, NULL, conf_core.design.bloat);
		}
		pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)new_obj);
		pcb_view_append_obj(violation, 1, (pcb_any_obj_t *)arrived_from);
		pcb_view_set_bbox_by_objs(ctx->data, violation);
		pcb_view_list_append(ctx->lst, violation);
		return ctx->fast; /* if fast is 1, we are aborting the search, returning the first hit only */
	}
	return 0;
}

/* Check for DRC violations on a single net starting from the pad or pin
   sees if the connectivity changes when everything is bloated, or shrunk */
static pcb_bool DRCFind(pcb_view_list_t *lst, pcb_any_obj_t *from)
{
	drc_ctx_t ctx;

	ctx.fast = 1;
	ctx.data = PCB->Data;
	ctx.lst = lst;

	memset(&ctx.fa, 0, sizeof(ctx.fa));
	memset(&ctx.fb, 0, sizeof(ctx.fb));
	ctx.fa.user_data = ctx.fb.user_data = &ctx;
	ctx.fb.found_cb = drc_broken_cb;

	/* Check for minimal overlap: shrink mark all objects on the net in fa;
	   restart the search without shrink in fb: if anything new is found, it did
	   not have enough overlap and shrink disconnected it. */
	if (conf_core.design.shrink != 0) {
		ctx.shrunk = 1;
		Bloat = -conf_core.design.shrink;
		pcb_find_from_obj(&ctx.fa, PCB->Data, from);
		Bloat = 0;
		pcb_find_from_obj(&ctx.fb, PCB->Data, from);
		pcb_find_free(&ctx.fa);
		pcb_find_free(&ctx.fb);
	}

	/* Check for minimal distance: bloat mark all objects on the net in fa;
	   restart the search without bloat in fb: if anything new is found, it did
	   not have a connection without the bloat. */
	if (conf_core.design.bloat != 0) {
		ctx.shrunk = 0;
		Bloat = 0;
		pcb_find_from_obj(&ctx.fa, PCB->Data, from);
		Bloat = conf_core.design.bloat;
		pcb_find_from_obj(&ctx.fb, PCB->Data, from);
		pcb_find_free(&ctx.fa);
		pcb_find_free(&ctx.fb);
	}

	Bloat = 0;

	return pcb_false;
}

/* search short/breaks from subcircuit terminals; returns non-zero for cancel */
static int drc_nets_from_subc_term(pcb_view_list_t *lst)
{
	unsigned long sofar = 0, total = pcb_subclist_length(&PCB->Data->subc);

	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_any_obj_t *o;
		pcb_data_it_t it;

		if (pcb_hid_progress(sofar, total, "drc_orig: Checking nets from subc terminals...") != 0)
			return 1;
		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			if (o->term == NULL) /* only terminals can be starting point of DRC net checks */
				continue;
			if (o->parent_type == PCB_PARENT_LAYER) { /* for layer objects, care about the ones on copper layers only */
				pcb_layer_type_t lyt = pcb_layer_flags_(o->parent.layer);
				if (!(lyt & PCB_LYT_COPPER))
					continue;
			}
			DRCFind(lst, o);
		}
		sofar++;
	}
	PCB_END_LOOP;
	return 0;
}

/* search short/breaks from non-subc padstacks; returns non-zero for cancel  */
static int drc_nets_from_pstk(pcb_view_list_t *lst)
{
	unsigned long sofar = 0, total = pcb_subclist_length(&PCB->Data->subc);

	PCB_PADSTACK_LOOP(PCB->Data);
	{
		if (pcb_hid_progress(sofar, total, "drc_orig: Checking nets from subc non-terminals...") != 0)
			return 1;

		if ((padstack->term == NULL) && DRCFind(lst, (pcb_any_obj_t *)padstack))
			break;
		sofar++;
	}
	PCB_END_LOOP;
	return 0;
}


/* text: check minimum widths */
void drc_all_texts(pcb_view_list_t *lst)
{
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
}

/* copper lines: check minimum widths and polygon clearances */
void drc_copper_lines(pcb_view_list_t *lst)
{
	pcb_view_t *violation;

	PCB_LINE_COPPER_LOOP(PCB->Data);
	{
		/* check line clearances in polygons */
		pcb_poly_plows(PCB->Data, PCB_OBJ_LINE, layer, line, drc_callback, lst);
		if (line->Thickness < conf_core.design.min_wid) {
			pcb_line_invalidate_draw(layer, line);
			violation = pcb_view_new("thin", "Line width is too thin", "Process specifications dictate a minimum feature-width\nthat can reliably be reproduced");
			pcb_drc_set_data(violation, &line->Thickness, conf_core.design.min_wid);
			pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)line);
			pcb_view_set_bbox_by_objs(PCB->Data, violation);
			pcb_view_list_append(lst, violation);
		}
	}
	PCB_ENDALL_LOOP;
}

/* copper arcs: check minimum widths and polygon clearances */
void drc_copper_arcs(pcb_view_list_t *lst)
{
	pcb_view_t *violation;

	PCB_ARC_COPPER_LOOP(PCB->Data);
	{
		pcb_poly_plows(PCB->Data, PCB_OBJ_ARC, layer, arc, drc_callback, lst);
		if (arc->Thickness < conf_core.design.min_wid) {
			pcb_arc_invalidate_draw(layer, arc);
			violation = pcb_view_new("thin", "Arc width is too thin", "Process specifications dictate a minimum feature-width\nthat can reliably be reproduced");
			pcb_drc_set_data(violation, &arc->Thickness, conf_core.design.min_wid);
			pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)arc);
			pcb_view_set_bbox_by_objs(PCB->Data, violation);
			pcb_view_list_append(lst, violation);
		}
	}
	PCB_ENDALL_LOOP;
}

/* non-subc padstacks: check minimum ring and polygon clearances */
void drc_global_pstks(pcb_view_list_t *lst)
{
	pcb_view_t *violation;

	PCB_PADSTACK_LOOP(PCB->Data);
	{
		pcb_coord_t ring = 0, hole = 0;
		pcb_poly_plows(PCB->Data, PCB_OBJ_PSTK, padstack, padstack, drc_callback, lst);
		pcb_pstk_drc_check_and_warn(padstack, &ring, &hole);
		if ((ring > 0) || (hole > 0)) {
			pcb_pstk_invalidate_draw(padstack);
			if (ring) {
				violation = pcb_view_new("thin", "padstack annular ring too small", "Annular rings that are too small may erode during etching,\nresulting in a broken connection");
				pcb_drc_set_data(violation, &ring, conf_core.design.min_ring);
				pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)padstack);
				pcb_view_set_bbox_by_objs(PCB->Data, violation);
				pcb_view_list_append(lst, violation);
			}
			if (hole > 0) {
				violation = pcb_view_new("drill", "Padstack drill size is too small", "Process rules dictate the minimum drill size which can be used");
				pcb_drc_set_data(violation, &hole, conf_core.design.min_drill);
				pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)padstack);
				pcb_view_set_bbox_by_objs(PCB->Data, violation);
				pcb_view_list_append(lst, violation);
			}
		}
	}
	PCB_END_LOOP;
}

/* check silkscreen minimum widths outside of subcircuits */
void drc_global_silk_lines(pcb_view_list_t *lst)
{
	pcb_view_t *violation;

TODO("DRC: need to check text and polygons too!")
	PCB_LINE_SILK_LOOP(PCB->Data);
	{
		if (line->Thickness < conf_core.design.min_slk) {
			pcb_line_invalidate_draw(layer, line);
			violation = pcb_view_new("thin", "Silk line is too thin", "Process specifications dictate a minimum silkscreen feature-width\nthat can reliably be reproduced");
			pcb_drc_set_data(violation, &line->Thickness, conf_core.design.min_slk);
			pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)line);
			pcb_view_set_bbox_by_objs(PCB->Data, violation);
			pcb_view_list_append(lst, violation);
		}
	}
	PCB_ENDALL_LOOP;
}

static void drc_beyond_extents(pcb_view_list_t *lst, pcb_data_t *data)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	pcb_view_t *violation;

	for(o = pcb_data_first(&it, data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		const char *message = NULL;
		pcb_coord_t measured, required;
		
		if (o->BoundingBox.X1 > PCB->hidlib.size_x) {
			message = "Beyond the drawing area, to the right";
			measured = o->BoundingBox.X1;
			required = PCB->hidlib.size_x;
		}
		else if (o->BoundingBox.Y1 > PCB->hidlib.size_y) {
			message = "Beyond the drawing area, to the bottom";
			measured = o->BoundingBox.Y1;
			required = PCB->hidlib.size_y;
		}
		else if (o->BoundingBox.X2 < 0) {
			message = "Beyond the drawing area, to the left";
			measured = o->BoundingBox.X2;
			required = 0;
		}
		else if (o->BoundingBox.Y2 < 0) {
			message = "Beyond the drawing area, to the top";
			measured = o->BoundingBox.Y2;
			required = 0;
		}


		if (message != NULL) {
			violation = pcb_view_new("beyond", message, "Object hard to edit or export because being outside of the drawing area.");
			pcb_drc_set_data(violation, &measured, required);
			pcb_view_append_obj(violation, 0, o);
			pcb_view_set_bbox_by_objs(PCB->Data, violation);
			pcb_view_list_append(lst, violation);
		}
	}
}

static void pcb_drc_orig(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_view_list_t *lst = &pcb_drc_lst;
	pcb_view_list_free_fields(lst);

	pcb_layervis_save_stack();
	pcb_layervis_reset_stack();
	pcb_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);

	/* actual tests */
	pcb_hid_progress(0, 0, NULL);
	if (drc_nets_from_subc_term(lst) != 0) goto out;
	pcb_hid_progress(0, 0, NULL);
	if (drc_nets_from_pstk(lst)) goto out;

	pcb_hid_progress(0, 0, NULL);
	if (pcb_hid_progress(0, 6, "drc_orig: Checking objects: text")) goto out;
	drc_all_texts(lst);
	pcb_hid_progress(0, 0, NULL);
	if (pcb_hid_progress(1, 6, "drc_orig: Checking objects: line")) goto out;
	drc_copper_lines(lst);
	pcb_hid_progress(0, 0, NULL);
	if (pcb_hid_progress(2, 6, "drc_orig: Checking objects: arc")) goto out;
	drc_copper_arcs(lst);
	pcb_hid_progress(0, 0, NULL);
	if (pcb_hid_progress(3, 6, "drc_orig: Checking objects: padstack")) goto out;
	drc_global_pstks(lst);
	pcb_hid_progress(0, 0, NULL);
	if (pcb_hid_progress(4, 6, "drc_orig: Checking objects: extent")) goto out;
	drc_beyond_extents(lst, PCB->Data);
	pcb_hid_progress(0, 0, NULL);
	if (pcb_hid_progress(5, 6, "drc_orig: Checking objects: silk")) goto out;
	drc_global_silk_lines(lst);

	out:;
	pcb_hid_progress(0, 0, NULL);
	pcb_layervis_restore_stack();
	pcb_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	pcb_gui->invalidate_all(pcb_gui);
}



int pplg_check_ver_drc_orig(int ver_needed) { return 0; }

void pplg_uninit_drc_orig(void)
{
	pcb_event_unbind_allcookie(drc_orig_cookie);
}

#include "dolists.h"
int pplg_init_drc_orig(void)
{
	PCB_API_CHK_VER;
	pcb_event_bind(PCB_EVENT_DRC_RUN, pcb_drc_orig, NULL, drc_orig_cookie);
	return 0;
}

