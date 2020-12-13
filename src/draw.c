/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
 *  Copyright (C) 2017,2018,2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
#include <librnd/core/math_helper.h>
#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/event.h>
#include "rotate.h"
#include <librnd/poly/rtree.h>
#include "stub_draw.h"
#include "layer_ui.h"
#include <librnd/core/hid_inlines.h>
#include "funchash_core.h"

#include "obj_pstk_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_rat_draw.h"
#include "obj_poly_draw.h"
#include "obj_text_draw.h"
#include "obj_subc_parent.h"
#include "obj_gfx_draw.h"

#undef NDEBUG
#include <assert.h>

pcb_output_t pcb_draw_out; /* global context used for drawing */

rnd_box_t pcb_draw_invalidated = { RND_COORD_MAX, RND_COORD_MAX, -RND_COORD_MAX, -RND_COORD_MAX };

int pcb_draw_force_termlab = 0;
rnd_bool pcb_draw_doing_assy = rnd_false;
static vtp0_t delayed_labels, delayed_objs, annot_objs;
rnd_bool delayed_labels_enabled = rnd_false;
rnd_bool delayed_terms_enabled = rnd_false;

static void draw_everything(pcb_draw_info_t *info);
static void pcb_draw_layer_grp(pcb_draw_info_t *info, int, int);
static void pcb_draw_obj_label(pcb_draw_info_t *info, rnd_layergrp_id_t gid, pcb_any_obj_t *obj);
static void pcb_draw_pstk_marks(pcb_draw_info_t *info);
static void pcb_draw_pstk_labels(pcb_draw_info_t *info);
static void pcb_draw_pstk_holes(pcb_draw_info_t *info, rnd_layergrp_id_t group, pcb_pstk_draw_hole_t holetype);
static void pcb_draw_ppv(pcb_draw_info_t *info, rnd_layergrp_id_t group);
static void xform_setup(pcb_draw_info_t *info, rnd_xform_t *dst, const pcb_layer_t *Layer);

/* In draw_ly_spec.c: */
static void pcb_draw_paste(pcb_draw_info_t *info, int side);
static void pcb_draw_mask(pcb_draw_info_t *info, int side);
static void pcb_draw_silk_doc(pcb_draw_info_t *info, pcb_layer_type_t lyt_side, pcb_layer_type_t lyt_type, int setgrp, int invis);
static void pcb_draw_boundary_mech(pcb_draw_info_t *info);
static void pcb_draw_rats(pcb_draw_info_t *info, const rnd_box_t *);
static void pcb_draw_assembly(pcb_draw_info_t *info, pcb_layer_type_t lyt_side);


void pcb_draw_delay_label_add(pcb_any_obj_t *obj)
{
	if (delayed_labels_enabled)
		vtp0_append(&delayed_labels, obj);
}

void pcb_draw_delay_obj_add(pcb_any_obj_t *obj)
{
	vtp0_append(&delayed_objs, obj);
}

void pcb_draw_annotation_add(pcb_any_obj_t *obj)
{
	vtp0_append(&annot_objs, obj);
}


TODO("cleanup: this should be cached")
void pcb_lighten_color(const rnd_color_t *orig, rnd_color_t *dst, double factor)
{
	rnd_color_load_int(dst, MIN(255, orig->r * factor), MIN(255, orig->g * factor), MIN(255, orig->b * factor), 255);
}

void pcb_draw_dashed_line(pcb_draw_info_t *info, rnd_hid_gc_t GC, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, unsigned int segs, rnd_bool_t cheap)
{
/* TODO: we need a real geo lib... using double here is plain wrong */
	double dx = x2-x1, dy = y2-y1;
	double len_mnt = RND_ABS(dx) + RND_ABS(dy);
	int n;
	rnd_coord_t minlen = rnd_render->coord_per_pix * 8;

	/* Ignore info->xform->bloat because a dashed line is always thin */

	if (len_mnt < minlen*2) {
		/* line too short, just draw it */
		rnd_render->draw_line(GC, x1, y1, x2, y2);
		return;
	}

	segs = (segs << 1) + 1; /* must be odd */

	if (cheap) {
		if ((segs > 3) && (len_mnt < minlen * segs))
			segs = 3;
		else if ((segs > 5) && (len_mnt < minlen * 2 * segs))
			segs = 5;
	}

	/* first seg is drawn from x1, y1 with no rounding error due to n-1 == 0 */
	for(n = 1; n < segs; n+=2)
		rnd_render->draw_line(GC,
			x1 + (dx * (double)(n-1) / (double)segs), y1 + (dy * (double)(n-1) / (double)segs),
			x1 + (dx * (double)n / (double)segs), y1 + (dy * (double)n / (double)segs));


	/* make sure the last segment is drawn properly to x2 and y2, don't leave
	   room for rounding errors */
	rnd_render->draw_line(GC,
		x2 - (dx / (double)segs), y2 - (dy / (double)segs),
		x2, y2);
}


/*
 * initiate the actual redrawing of the updated area
 */
rnd_cardinal_t pcb_draw_inhibit = 0;
void pcb_draw(void)
{
	if (pcb_draw_inhibit)
		return;
	if (pcb_draw_invalidated.X1 <= pcb_draw_invalidated.X2 && pcb_draw_invalidated.Y1 <= pcb_draw_invalidated.Y2)
		rnd_render->invalidate_lr(rnd_render, pcb_draw_invalidated.X1, pcb_draw_invalidated.X2, pcb_draw_invalidated.Y1, pcb_draw_invalidated.Y2);

	/* shrink the update block */
	pcb_draw_invalidated.X1 = pcb_draw_invalidated.Y1 = RND_COORD_MAX;
	pcb_draw_invalidated.X2 = pcb_draw_invalidated.Y2 = -RND_COORD_MAX;
}

static void draw_everything_holes(pcb_draw_info_t *info, rnd_layergrp_id_t gid)
{
	int plated, unplated;
	pcb_board_count_holes(PCB, &plated, &unplated, info->drawn_area);

	if (plated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_PLATED_DRILL, 0, &info->xform_exporter)) {
		pcb_draw_pstk_holes(info, gid, PCB_PHOLE_PLATED);
		rnd_render->end_layer(rnd_render);
	}

	if (unplated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_UNPLATED_DRILL, 0, &info->xform_exporter)) {
		pcb_draw_pstk_holes(info, gid, PCB_PHOLE_UNPLATED);
		rnd_render->end_layer(rnd_render);
	}
}

typedef struct {
	rnd_layergrp_id_t top_fab, top_assy, bot_assy;
	int top_fab_enable, top_assy_enable, bot_assy_enable;
} legacy_vlayer_t;

static int set_vlayer(pcb_draw_info_t *info, rnd_layergrp_id_t gid, int enable, pcb_virtual_layer_t vly_type)
{
	if (gid >= 0) { /* has a doc layer */
		if (!enable)
			return 0;
		return pcb_layer_gui_set_glayer(PCB, gid, 0, &info->xform_exporter);
	}

	/* fall back to the legacy implicit virtual layer mode */
	return pcb_layer_gui_set_vlayer(PCB, vly_type, 0, &info->xform_exporter);
}


static void draw_virtual_layers(pcb_draw_info_t *info, const legacy_vlayer_t *lvly_drawn)
{
	rnd_hid_expose_ctx_t hid_exp;
	rnd_xform_t tmp;

	hid_exp.view = *info->drawn_area;

	if (set_vlayer(info, lvly_drawn->top_assy, lvly_drawn->top_assy_enable, PCB_VLY_TOP_ASSY)) {
		xform_setup(info, &tmp, NULL);
		pcb_draw_assembly(info, PCB_LYT_TOP);
		rnd_render->end_layer(rnd_render);
		info->xform = NULL; info->layer = NULL;
	}

	if (set_vlayer(info, lvly_drawn->bot_assy, lvly_drawn->bot_assy_enable, PCB_VLY_BOTTOM_ASSY)) {
		xform_setup(info, &tmp, NULL);
		pcb_draw_assembly(info, PCB_LYT_BOTTOM);
		rnd_render->end_layer(rnd_render);
		info->xform = NULL; info->layer = NULL;
	}

	if (set_vlayer(info, lvly_drawn->top_fab, lvly_drawn->top_fab_enable, PCB_VLY_FAB)) {
		xform_setup(info, &tmp, NULL);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
		pcb_stub_draw_fab(info, pcb_draw_out.fgGC, &hid_exp);
		rnd_render->end_layer(rnd_render);
		info->xform = NULL; info->layer = NULL;
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_CSECT, 0, &info->xform_exporter)) {
		xform_setup(info, &tmp, NULL);
		pcb_stub_draw_csect(pcb_draw_out.fgGC, &hid_exp);
		rnd_render->end_layer(rnd_render);
		info->xform = NULL; info->layer = NULL;
	}
}

static void draw_ui_layers(pcb_draw_info_t *info)
{
	int i;
	pcb_layer_t *first, *ly;

	/* find the first ui layer in use */
	first = NULL;
	for(i = 0; i < vtp0_len(&pcb_uilayers); i++) {
		if (pcb_uilayers.array[i] != NULL) {
			first = pcb_uilayers.array[i];
			break;
		}
	}

	/* if there's any UI layer, try to draw them */
	if ((first != NULL) && pcb_layer_gui_set_g_ui(first, 0, &info->xform_exporter)) {
		int have_canvas = 0;
		for(i = 0; i < vtp0_len(&pcb_uilayers); i++) {
			ly = pcb_uilayers.array[i];
			if ((ly != NULL) && (ly->meta.real.vis)) {
				if (!have_canvas) {
					rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
					rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
					have_canvas = 1;
				}
				pcb_draw_layer(info, ly);
			}
		}
		if (have_canvas)
			rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
		rnd_render->end_layer(rnd_render);
	}
}

/* Draw subc and padstack marks in xor mode */
static void draw_xor_marks(pcb_draw_info_t *info)
{
	int per_side = conf_core.appearance.subc_layer_per_side;
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE_XOR, pcb_draw_out.direct, info->drawn_area);

	rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_round);
	rnd_hid_set_draw_xor(pcb_draw_out.fgGC, 1);

	if (PCB->SubcOn) {
		info->objcb.subc.per_side = per_side;
		rnd_r_search(PCB->Data->subc_tree, info->drawn_area, NULL, draw_subc_mark_callback, info, NULL);
	}

	rnd_hid_set_line_width(pcb_draw_out.fgGC, 0);

	if ((PCB->padstack_mark_on) && (conf_core.appearance.padstack.cross_thick > 0)) {
		rnd_hid_set_line_width(pcb_draw_out.fgGC, -conf_core.appearance.padstack.cross_thick);
		pcb_draw_pstk_marks(info);
	}

	rnd_event(&PCB->hidlib, RND_EVENT_GUI_DRAW_OVERLAY_XOR, "pp", &pcb_draw_out.fgGC, info, NULL);

	rnd_hid_set_draw_xor(pcb_draw_out.fgGC, 0);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
}

static void draw_rats(pcb_draw_info_t *info, const rnd_box_t *drawn_area)
{
	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_RATS, 0, NULL)) {
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
		pcb_draw_rats(info, drawn_area);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
		rnd_render->end_layer(rnd_render);
	}
}

static void draw_pins_and_pads(pcb_draw_info_t *info, rnd_layergrp_id_t component, rnd_layergrp_id_t solder)
{
	int per_side = conf_core.appearance.subc_layer_per_side;

	/* Draw pins' and pads' names */
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
	rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_round);
	rnd_hid_set_line_width(pcb_draw_out.fgGC, 0);
	if (PCB->SubcOn) {
		info->objcb.subc.per_side = per_side;
		rnd_r_search(PCB->Data->subc_tree, info->drawn_area, NULL, draw_subc_label_callback, info, NULL);
	}
	if (PCB->padstack_mark_on) {
		rnd_hid_set_line_width(pcb_draw_out.fgGC, -conf_core.appearance.padstack.cross_thick);
		pcb_draw_pstk_labels(info);
	}
	pcb_draw_pstk_names(info, info->xform->show_solder_side ? solder : component, info->drawn_area);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
}

static int has_auto(pcb_layergrp_t *grp)
{
	int n;
	for(n = 0; n < grp->len; n++) {
		pcb_layer_t *ly = pcb_get_layer(PCB->Data, grp->lid[n]);
		if (ly->comb & PCB_LYC_AUTO)
			return 1;
	}
	return 0;
}

static void draw_everything(pcb_draw_info_t *info)
{
	rnd_layergrp_id_t backsilk_gid;
	pcb_layergrp_t *backsilk_grp;
	rnd_color_t old_silk_color[PCB_MAX_LAYERGRP];
	int i, ngroups;
	rnd_layergrp_id_t component, solder, gid, side_copper_grp;
	/* This is the list of layer groups we will draw.  */
	char do_group[PCB_MAX_LAYERGRP];
	/* This is the reverse of the order in which we draw them.  */
	rnd_layergrp_id_t drawn_groups[PCB_MAX_LAYERGRP];
	rnd_bool paste_empty;
	legacy_vlayer_t lvly;
	rnd_xform_t tmp;

	xform_setup(info, &tmp, NULL);
	backsilk_gid = ((!info->xform->show_solder_side) ? pcb_layergrp_get_bottom_silk() : pcb_layergrp_get_top_silk());
	backsilk_grp = pcb_get_layergrp(PCB, backsilk_gid);
	if (backsilk_grp != NULL) {
		rnd_cardinal_t n;
		for(n = 0; n < backsilk_grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(PCB->Data, backsilk_grp->lid[n]);
			if (ly != NULL) {
				old_silk_color[n] = ly->meta.real.color;
				ly->meta.real.color = conf_core.appearance.color.invisible_objects;
			}
		}
	}

	rnd_render->render_burst(rnd_render, RND_HID_BURST_START, info->drawn_area);

	memset(do_group, 0, sizeof(do_group));
	lvly.top_fab = -1;
	lvly.top_assy = lvly.bot_assy = -1;
	for (ngroups = 0, i = 0; i < pcb_max_layer(PCB); i++) {
		pcb_layer_t *l = PCB_STACKLAYER(PCB, i);
		rnd_layergrp_id_t group = pcb_layer_get_group(PCB, pcb_layer_stack[i]);
		pcb_layergrp_t *grp = pcb_get_layergrp(PCB, group);
		unsigned int gflg = 0;

		if (grp != NULL)
			gflg = grp->ltype;

		if ((gflg & PCB_LYT_DOC) && (grp->purpi == F_fab)) {
			lvly.top_fab = group;
			lvly.top_fab_enable = has_auto(grp);
		}

		if ((gflg & PCB_LYT_DOC) && (grp->purpi == F_assy)) {
			if (gflg & PCB_LYT_TOP) {
				lvly.top_assy = group;
				lvly.top_assy_enable = has_auto(grp);
			}
			else if (gflg & PCB_LYT_BOTTOM) {
				lvly.bot_assy = group;
				lvly.bot_assy_enable = has_auto(grp);
			}
		}

		if ((gflg & PCB_LYT_SILK) || (gflg & PCB_LYT_DOC) || (gflg & PCB_LYT_MASK) || (gflg & PCB_LYT_PASTE) || (gflg & PCB_LYT_BOUNDARY) || (gflg & PCB_LYT_MECH)) /* do not draw silk, mask, paste and boundary here, they'll be drawn separately */
			continue;

		if (l->meta.real.vis && !do_group[group]) {
			do_group[group] = 1;
			drawn_groups[ngroups++] = group;
		}
	}

	solder = component = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &component, 1);
	side_copper_grp = info->xform->show_solder_side ? solder : component;


	/*
	 * first draw all 'invisible' stuff
	 */
	if ((info->xform_exporter != NULL) && !info->xform_exporter->check_planes && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_INVISIBLE, 0, &info->xform_exporter)) {
		pcb_layer_type_t side = PCB_LYT_INVISIBLE_SIDE();

		pcb_draw_silk_doc(info, side, PCB_LYT_DOC, 0, 1);
		pcb_draw_silk_doc(info, side, PCB_LYT_SILK, 0, 1);

		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
		rnd_render->end_layer(rnd_render);
	}

	/* Draw far side doc and silks */
	{
		pcb_layer_type_t ivside = PCB_LYT_INVISIBLE_SIDE();

		pcb_draw_silk_doc(info, ivside, PCB_LYT_SILK, 1, 0);
		pcb_draw_silk_doc(info, ivside, PCB_LYT_DOC, 1, 0);
	}

	/* draw all layers in layerstack order */
	for (i = ngroups - 1; i >= 0; i--) {
		rnd_layergrp_id_t group = drawn_groups[i];

		if (pcb_layer_gui_set_glayer(PCB, group, 0, &info->xform_exporter)) {
			int is_current = 0;
			rnd_layergrp_id_t cgrp = PCB_CURRLAYER(PCB)->meta.real.grp;

			if ((cgrp == solder) || (cgrp == component)) {
				/* current group is top or bottom: visibility depends on side we are looking at */
				if (group == side_copper_grp)
					is_current = 1;
			}
			else {
				/* internal layer displayed on top: current group is solid, others are "invisible" */
				if (group == cgrp)
					is_current = 1;
			}

			pcb_draw_layer_grp(info, group, is_current);
			rnd_render->end_layer(rnd_render);
		}
	}

	if (conf_core.editor.check_planes && rnd_render->gui)
		goto finish;

	/* Draw padstacks below silk */
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
	if (rnd_render->gui) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		pcb_draw_ppv(info, info->xform->show_solder_side ? solder : component);
		info->xform = NULL; info->layer = NULL;
	}
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);

	/* Draw the solder mask if turned on */
	gid = pcb_layergrp_get_top_mask();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, 0, &info->xform_exporter))) {
		pcb_draw_mask(info, PCB_COMPONENT_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	gid = pcb_layergrp_get_bottom_mask();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, 0, &info->xform_exporter))) {
		pcb_draw_mask(info, PCB_SOLDER_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	/* Draw doc and silks */
	{
		pcb_layer_type_t vside = PCB_LYT_VISIBLE_SIDE();

		pcb_draw_silk_doc(info, PCB_LYT_INTERN, PCB_LYT_SILK, 1, 0);
		pcb_draw_silk_doc(info, PCB_LYT_INTERN, PCB_LYT_DOC, 1, 0);
		pcb_draw_silk_doc(info, 0, PCB_LYT_DOC, 1, 0);
		pcb_draw_silk_doc(info, vside, PCB_LYT_SILK, 1, 0);
		pcb_draw_silk_doc(info, vside, PCB_LYT_DOC, 1, 0);
	}

	{ /* holes_after: draw holes after copper, silk and mask, to make sure it punches through everything. */
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area); 
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
		draw_everything_holes(info, side_copper_grp);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
	}

	gid = pcb_layergrp_get_top_paste();
	if (gid >= 0)
		paste_empty = pcb_layergrp_is_empty(PCB, gid);
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty, &info->xform_exporter))) {
		pcb_draw_paste(info, PCB_COMPONENT_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	gid = pcb_layergrp_get_bottom_paste();
	if (gid >= 0)
		paste_empty = pcb_layergrp_is_empty(PCB, gid);
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty, &info->xform_exporter))) {
		pcb_draw_paste(info, PCB_SOLDER_SIDE);
		rnd_render->end_layer(rnd_render);
	}

	pcb_draw_boundary_mech(info);

	draw_virtual_layers(info, &lvly);
	if ((rnd_render->gui) || (!info->xform_caller->omit_overlay)) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		draw_rats(info, info->drawn_area);
		draw_pins_and_pads(info, component, solder);
	}
	draw_ui_layers(info);

	if (rnd_render->gui) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		draw_xor_marks(info);
	}

	finish:;
	if (backsilk_grp != NULL) {
		rnd_cardinal_t n;
		for(n = 0; n < backsilk_grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(PCB->Data, backsilk_grp->lid[n]);
			if (ly != NULL)
				ly->meta.real.color = old_silk_color[n];
		}
	}
}

static void pcb_draw_pstks(pcb_draw_info_t *info, rnd_layergrp_id_t group, int is_current, pcb_layer_combining_t comb)
{
	pcb_layergrp_t *g = PCB->LayerGroups.grp + group;
	rnd_xform_t tmp;

	xform_setup(info, &tmp, NULL);

	info->objcb.pstk.gid = group;
	info->objcb.pstk.is_current = is_current;
	info->objcb.pstk.comb = comb;
	if (g->len > 0)
		info->objcb.pstk.layer1 = pcb_get_layer(info->pcb->Data, g->lid[0]);
	else
		info->objcb.pstk.layer1 = NULL;

	if (!info->xform->check_planes)
		rnd_r_search(info->pcb->Data->padstack_tree, info->drawn_area, NULL, pcb_pstk_draw_callback, info, NULL);

	info->xform = NULL; info->layer = NULL;
}

static void pcb_draw_pstk_marks(pcb_draw_info_t *info)
{
	rnd_r_search(PCB->Data->padstack_tree, info->drawn_area, NULL, pcb_pstk_draw_mark_callback, info, NULL);
}

static void pcb_draw_pstk_labels(pcb_draw_info_t *info)
{
	rnd_r_search(PCB->Data->padstack_tree, info->drawn_area, NULL, pcb_pstk_draw_label_callback, info, NULL);
}

static void pcb_draw_pstk_holes(pcb_draw_info_t *info, rnd_layergrp_id_t group, pcb_pstk_draw_hole_t holetype)
{
	rnd_xform_t tmp;


	if (!PCB->hole_on)
		return;

	xform_setup(info, &tmp, NULL);
	info->objcb.pstk.gid = group;
	info->objcb.pstk.holetype = holetype;
	rnd_r_search(PCB->Data->padstack_tree, info->drawn_area, NULL, pcb_pstk_draw_hole_callback, info, NULL);
	info->xform = NULL; info->layer = NULL; 
}

static void pcb_draw_pstk_slots(pcb_draw_info_t *info, rnd_layergrp_id_t group, pcb_pstk_draw_hole_t holetype)
{
	rnd_xform_t tmp;

	if (!PCB->hole_on)
		return;

	xform_setup(info, &tmp, NULL);
	info->objcb.pstk.gid = group;
	info->objcb.pstk.holetype = holetype;
	rnd_r_search(PCB->Data->padstack_tree, info->drawn_area, NULL, pcb_pstk_draw_slot_callback, info, NULL);
	info->xform = NULL; info->layer = NULL;
}

/* ---------------------------------------------------------------------------
 * Draws padstacks - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->pstk_on
 */
static void pcb_draw_ppv(pcb_draw_info_t *info, rnd_layergrp_id_t group)
{
	/* draw padstack holes - copper is drawn with each group */
	if (PCB->pstk_on || !rnd_render->gui) {
		pcb_draw_pstk_holes(info, group, PCB_PHOLE_PLATED | PCB_PHOLE_UNPLATED | PCB_PHOLE_BB);
		if (
			!rnd_render->gui /* on the gui padstacks can be turned off separately so we shouldn't enforce drawing the slot in it; on export, copper is copper, no matter if it is coming from a padstack: slot always needs to be drawn */
			&& !info->xform_caller->no_slot_in_nonmech /* keep the output cleaner in some exporters, like gerber, where solid copper is drawn anyway */
			)
			pcb_draw_pstk_slots(info, group, PCB_PHOLE_PLATED | PCB_PHOLE_UNPLATED | PCB_PHOLE_BB);
	}
}

#include "draw_label_smart.c"

/* ---------------------------------------------------------------------------
 * Draws padstacks' names - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->pstk_on
 */
void pcb_draw_pstk_names(pcb_draw_info_t *info, rnd_layergrp_id_t group, const rnd_box_t *drawn_area)
{
	if (PCB->pstk_on || !rnd_render->gui) {
		size_t n;
		for(n = 0; n < delayed_labels.used; n++)
			pcb_draw_obj_label(info, group, delayed_labels.array[n]);
	}
	pcb_label_smart_flush(info);
	delayed_labels.used = 0;
}

static void pcb_draw_delayed_objs(pcb_draw_info_t *info)
{
	size_t n;

	for(n = 0; n < delayed_objs.used; n++) {
		pcb_any_obj_t *o = delayed_objs.array[n];
		rnd_box_t *b = (rnd_box_t *)o;
		switch(o->type) {
			case PCB_OBJ_ARC:  pcb_arc_draw_term_callback(b, info); break;
			case PCB_OBJ_LINE: pcb_line_draw_term_callback(b, info); break;
			case PCB_OBJ_TEXT: pcb_text_draw_term_callback(b, info); break;
			case PCB_OBJ_POLY: pcb_poly_draw_term_callback(b, info); break;
			default:
				assert(!"Don't know how to draw delayed object");
		}
	}
	vtp0_truncate(&delayed_objs, 0);
}

static void pcb_draw_annotations(pcb_draw_info_t *info)
{
	size_t n;

	if ((rnd_render != NULL) && (rnd_render->gui)) {
		for(n = 0; n < annot_objs.used; n++) {
			pcb_any_obj_t *o = annot_objs.array[n];
			switch(o->type) {
				case PCB_OBJ_POLY: pcb_poly_draw_annotation(info, (pcb_poly_t *)o); break;
				default:
					assert(!"Don't know how to draw annotation for object");
			}
		}
	}
	vtp0_truncate(&annot_objs, 0);
}

#include "draw_composite.c"
#include "draw_ly_spec.c"

static void xform_setup(pcb_draw_info_t *info, rnd_xform_t *dst, const pcb_layer_t *Layer)
{
	info->layer = Layer;
	if ((Layer != NULL) && (!pcb_xform_is_nop(&Layer->meta.real.xform))) {
		pcb_xform_copy(dst, &Layer->meta.real.xform);
		info->xform = dst;
	}
	if (info->xform_caller != NULL) {
		if (info->xform == NULL) {
			info->xform = dst;
			pcb_xform_copy(dst, info->xform_caller);
		}
		else
			pcb_xform_add(dst, info->xform_caller);
	}
	if (info->xform_exporter != NULL) {
		if (info->xform == NULL) {
			info->xform = dst;
			pcb_xform_copy(dst, info->xform_exporter);
		}
		else
			pcb_xform_add(dst, info->xform_exporter);
	}
}

void pcb_draw_layer(pcb_draw_info_t *info, const pcb_layer_t *Layer_)
{
	unsigned int lflg = 0;
	int may_have_delayed = 0, restore_color = 0, current_grp;
	rnd_xform_t xform;
	rnd_color_t orig_color;
	pcb_layer_t *Layer = (pcb_layer_t *)Layer_; /* ugly hack until layer color is moved into info */

	xform_setup(info, &xform, Layer);

	current_grp = (Layer_->meta.real.grp == PCB_CURRLAYER(PCB)->meta.real.grp);

	if ((info->xform != NULL) && (info->xform->layer_faded)) {
		orig_color = Layer->meta.real.color;
		pcb_lighten_color(&orig_color, &Layer->meta.real.color, 0.5);
		restore_color = 1;
	}
	else if (info->xform->invis_other_groups && !current_grp) {
		orig_color = Layer->meta.real.color;
		Layer->meta.real.color = conf_core.appearance.color.invisible_objects;
		restore_color = 1;
	}

	if (info->xform->black_current_group && current_grp) {
		if (!restore_color) {
			orig_color = Layer->meta.real.color;
			restore_color = 1;
			Layer->meta.real.color = *rnd_color_black;
		}
	}

	lflg = pcb_layer_flags_(Layer);
	if (PCB_LAYERFLG_ON_VISIBLE_SIDE(lflg))
		pcb_draw_out.active_padGC = pcb_draw_out.padGC;
	else
		pcb_draw_out.active_padGC = pcb_draw_out.backpadGC;

	/* first draw all gfx that's under other layer objects */
	if (lflg & PCB_LYT_COPPER) {
		delayed_terms_enabled = rnd_true;
		rnd_r_search(Layer->gfx_tree, info->drawn_area, NULL, pcb_gfx_draw_under_callback, info, NULL);
		delayed_terms_enabled = rnd_false;
		may_have_delayed = 1;
	}
	else
		rnd_r_search(Layer->gfx_tree, info->drawn_area, NULL, pcb_gfx_draw_under_callback, info, NULL);

		/* print the non-clearing polys */
	if (lflg & PCB_LYT_COPPER) {
		delayed_terms_enabled = rnd_true;
		rnd_hid_set_line_width(pcb_draw_out.fgGC, 1);
		rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_square);
		rnd_r_search(Layer->polygon_tree, info->drawn_area, NULL, pcb_poly_draw_term_callback, info, NULL);
		delayed_terms_enabled = rnd_false;
		may_have_delayed = 1;
	}
	else {
		rnd_r_search(Layer->polygon_tree, info->drawn_area, NULL, pcb_poly_draw_callback, info, NULL);
	}

	if (info->xform->check_planes)
		goto out;

	/* draw all visible layer objects (with terminal gfx on copper) */
	if (lflg & PCB_LYT_COPPER) {
		delayed_terms_enabled = rnd_true;
		rnd_r_search(Layer->line_tree, info->drawn_area, NULL, pcb_line_draw_term_callback, info, NULL);
		rnd_r_search(Layer->arc_tree, info->drawn_area, NULL, pcb_arc_draw_term_callback, info, NULL);
		rnd_r_search(Layer->text_tree, info->drawn_area, NULL, pcb_text_draw_term_callback, info, NULL);
		rnd_r_search(Layer->gfx_tree, info->drawn_area, NULL, pcb_gfx_draw_above_callback, info, NULL);
		delayed_terms_enabled = rnd_false;
		may_have_delayed = 1;
	}
	else {
		rnd_r_search(Layer->line_tree, info->drawn_area, NULL, pcb_line_draw_callback, info, NULL);
		rnd_r_search(Layer->arc_tree, info->drawn_area, NULL, pcb_arc_draw_callback, info, NULL);
		rnd_r_search(Layer->text_tree, info->drawn_area, NULL, pcb_text_draw_callback, info, NULL);
		rnd_r_search(Layer->gfx_tree, info->drawn_area, NULL, pcb_gfx_draw_above_callback, info, NULL);
	}

	if (may_have_delayed)
		pcb_draw_delayed_objs(info);
	pcb_draw_annotations(info);

	out:;
		pcb_draw_out.active_padGC = NULL;

	if (restore_color)
		Layer->meta.real.color = orig_color;

	info->xform = NULL; info->layer = NULL;
}

static void pcb_draw_info_setup(pcb_draw_info_t *info, pcb_board_t *pcb)
{
	info->pcb = pcb;
	info->exporting = (rnd_render->exporter || rnd_render->printer);
	info->export_name = rnd_render->name;
	if (info->exporting) {
		strcpy(info->noexport_name, "noexport:");
		strncpy(info->noexport_name+9, info->export_name, sizeof(info->noexport_name)-10);
		info->noexport_name[sizeof(info->noexport_name)-1] = '\0';
	}
	else
		*info->noexport_name = '\0';
}

void pcb_draw_layer_noxform(pcb_board_t *pcb, const pcb_layer_t *Layer, const rnd_box_t *screen)
{
	pcb_draw_info_t info;
	rnd_box_t scr2;

	pcb_draw_info_setup(&info, pcb);
	info.drawn_area = screen;
	info.xform_exporter = info.xform = NULL;

	/* fix screen coord order */
	if ((screen->X2 <= screen->X1) || (screen->Y2 <= screen->Y1)) {
		scr2 = *screen;
		info.drawn_area = &scr2;
		if (scr2.X2 <= scr2.X1)
			scr2.X2 = scr2.X1+1;
		if (scr2.Y2 <= scr2.Y1)
			scr2.Y2 = scr2.Y1+1;
	}

	pcb_draw_layer(&info, Layer);
}


/* This version is about 1% slower and used rarely, thus it's all dupped
   from pcb_draw_layer() to keep the original speed there */
void pcb_draw_layer_under(pcb_board_t *pcb, const pcb_layer_t *Layer, const rnd_box_t *screen, pcb_data_t *data, rnd_xform_t *xf)
{
	pcb_draw_info_t info;
	rnd_box_t scr2;
	unsigned int lflg = 0;
	rnd_rtree_it_t it;
	pcb_any_obj_t *o;
	rnd_xform_t tmp;

	if ((screen->X2 <= screen->X1) || (screen->Y2 <= screen->Y1)) {
		scr2 = *screen;
		screen = &scr2;
		if (scr2.X2 <= scr2.X1)
			scr2.X2 = scr2.X1+1;
		if (scr2.Y2 <= scr2.Y1)
			scr2.Y2 = scr2.Y1+1;
	}

	pcb_draw_info_setup(&info, pcb);
	info.drawn_area = screen;
	info.xform_exporter = info.xform_caller = info.xform = xf;

	xform_setup(&info, &tmp, Layer);

	lflg = pcb_layer_flags_(Layer);
	if (PCB_LAYERFLG_ON_VISIBLE_SIDE(lflg))
		pcb_draw_out.active_padGC = pcb_draw_out.padGC;
	else
		pcb_draw_out.active_padGC = pcb_draw_out.backpadGC;

	/* first draw gfx that are under other layer objects */
	if (lflg & PCB_LYT_COPPER) {
		if (Layer->gfx_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->gfx_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if ((pcb_obj_is_under(o, data)) && (((pcb_gfx_t *)o)->render_under))
					pcb_gfx_draw(&info, (pcb_gfx_t *)o, 0);
	}
	else {
		if (Layer->gfx_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->gfx_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_gfx_draw_under_callback((rnd_box_t *)o, &info);
	}

	/* print the non-clearing polys */
	if (Layer->polygon_tree != NULL) {
		if (lflg & PCB_LYT_COPPER) {
			for(o = rnd_rtree_first(&it, Layer->polygon_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_poly_draw_term_callback((rnd_box_t *)o, &info);
		}
		else {
			for(o = rnd_rtree_first(&it, Layer->polygon_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_poly_draw_callback((rnd_box_t *)o, &info);
		}
	}

	if (info.xform->check_planes)
		goto out;

	/* draw all visible layer objects (with terminal gfx on copper) */
	if (lflg & PCB_LYT_COPPER) {
		if (Layer->line_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->line_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_line_draw_term_callback((rnd_box_t *)o, &info);
		if (Layer->arc_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->arc_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_arc_draw_term_callback((rnd_box_t *)o, &info);
		if (Layer->text_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->text_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_text_draw_term_callback((rnd_box_t *)o, &info);
		if (Layer->gfx_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->gfx_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if ((pcb_obj_is_under(o, data)) && !(((pcb_gfx_t *)o)->render_under))
					pcb_gfx_draw(&info, (pcb_gfx_t *)o, 0);
	}
	else {
		if (Layer->line_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->line_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_line_draw_callback((rnd_box_t *)o, &info);
		if (Layer->arc_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->arc_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_arc_draw_callback((rnd_box_t *)o, &info);
		if (Layer->text_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->text_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_text_draw_callback((rnd_box_t *)o, &info);
		if (Layer->gfx_tree != NULL)
			for(o = rnd_rtree_first(&it, Layer->gfx_tree, (rnd_rtree_box_t *)screen); o != NULL; o = rnd_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_gfx_draw_above_callback((rnd_box_t *)o, &info);
	}

	out:;
		pcb_draw_out.active_padGC = NULL;

	info.layer = NULL; info.xform = NULL;
}

/* ---------------------------------------------------------------------------
 * draws one layer group.  If the exporter is not a GUI,
 * also draws the padstacks in this layer group.
 */
static void pcb_draw_layer_grp(pcb_draw_info_t *info, int group, int is_current)
{
	int i;
	rnd_layer_id_t layernum;
	pcb_layer_t *Layer;
	rnd_cardinal_t n_entries = PCB->LayerGroups.grp[group].len;
	rnd_layer_id_t *layers = PCB->LayerGroups.grp[group].lid;
	pcb_layergrp_t *grp = pcb_get_layergrp(PCB, group);
	unsigned int gflg = grp->ltype;

	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);

	for (i = n_entries - 1; i >= 0; i--) {
		layernum = layers[i];
		Layer = info->pcb->Data->Layer + layernum;
		if (!(gflg & PCB_LYT_SILK) && Layer->meta.real.vis)
			pcb_draw_layer(info, Layer);
	}

	if ((gflg & PCB_LYT_COPPER) && (PCB->pstk_on)) {
		rnd_xform_t tmp;
		const pcb_layer_t *ly1 = NULL;

		/* figure first layer to get the transformations from */
		if (n_entries > 0)
			ly1 = info->pcb->Data->Layer + layers[0];

		xform_setup(info, &tmp, ly1);
		pcb_draw_pstks(info, group, (PCB_CURRLAYER(PCB)->meta.real.grp == group), 0);
		info->xform = NULL; info->layer = NULL;
	}

	/* this draws the holes - must be the last, so holes are drawn over everything else */
	if (!rnd_render->gui)
		pcb_draw_ppv(info, group);


	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
}

void pcb_erase_obj(int type, void *lptr, void *ptr)
{
	if (pcb_hidden_floater_gui((pcb_any_obj_t *)ptr))
		return;

	switch (type) {
	case PCB_OBJ_PSTK:
		pcb_pstk_invalidate_erase((pcb_pstk_t *) ptr);
		break;

	case PCB_OBJ_TEXT:
		pcb_text_invalidate_erase((pcb_layer_t *) lptr, (pcb_text_t *) ptr);
		break;
	case PCB_OBJ_POLY:
		pcb_poly_invalidate_erase((pcb_poly_t *) ptr);
		break;
	case PCB_OBJ_SUBC:
		EraseSubc((pcb_subc_t *)ptr);
		break;
	case PCB_OBJ_LINE:
	case PCB_OBJ_RAT:
		pcb_line_invalidate_erase((pcb_line_t *) ptr);
		break;
	case PCB_OBJ_ARC:
		pcb_arc_invalidate_erase((pcb_arc_t *) ptr);
		break;
	case PCB_OBJ_GFX:
		pcb_gfx_invalidate_erase((pcb_gfx_t *)ptr);
		break;
	default:
		rnd_message(RND_MSG_ERROR, "hace: Internal ERROR, trying to erase an unknown type\n");
	}
}


void pcb_draw_obj(pcb_any_obj_t *obj)
{
	if (pcb_hidden_floater_gui(obj))
		return;

	switch (obj->type) {
	case PCB_OBJ_PSTK:
		if (PCB->pstk_on)
			pcb_pstk_invalidate_draw((pcb_pstk_t *)obj);
		break;
	case PCB_OBJ_LINE:
		if (obj->parent.layer->meta.real.vis)
			pcb_line_invalidate_draw(obj->parent.layer, (pcb_line_t *)obj);
		break;
	case PCB_OBJ_ARC:
		if (obj->parent.layer->meta.real.vis)
			pcb_arc_invalidate_draw(obj->parent.layer, (pcb_arc_t *)obj);
		break;
	case PCB_OBJ_GFX:
		if (obj->parent.layer->meta.real.vis)
			pcb_gfx_invalidate_draw(obj->parent.layer, (pcb_gfx_t *)obj);
		break;
	case PCB_OBJ_TEXT:
		if (obj->parent.layer->meta.real.vis)
			pcb_text_invalidate_draw(obj->parent.layer, (pcb_text_t *)obj);
		break;
	case PCB_OBJ_POLY:
		if (obj->parent.layer->meta.real.vis)
			pcb_poly_invalidate_draw(obj->parent.layer, (pcb_poly_t *)obj);
		break;
	case PCB_OBJ_RAT:
		if (PCB->RatOn)
			pcb_rat_invalidate_draw((pcb_rat_t *)obj);
		break;
	case PCB_OBJ_SUBC:
	case PCB_OBJ_NET:
	case PCB_OBJ_LAYER:
	case PCB_OBJ_LAYERGRP:
	case PCB_OBJ_VOID:
	case PCB_OBJ_NET_TERM:
		break;
	}
}

static void pcb_draw_obj_label(pcb_draw_info_t *info, rnd_layergrp_id_t gid, pcb_any_obj_t *obj)
{
	rnd_bool vis_side = 1;

	if (pcb_hidden_floater(obj, info))
		return;

	/* do not show layer-object labels of the other side on non-pinout views */
	if ((!pcb_draw_force_termlab) && (obj->parent_type == PCB_PARENT_LAYER)) {
		pcb_layer_t *ly = pcb_layer_get_real(obj->parent.layer);
		if ((ly != NULL) && (ly->meta.real.grp != gid))
			vis_side = 0;
	}

	switch(obj->type) {
		case PCB_OBJ_LINE:    pcb_line_draw_label(info, (pcb_line_t *)obj, vis_side); return;
		case PCB_OBJ_ARC:     pcb_arc_draw_label(info, (pcb_arc_t *)obj, vis_side); return;
		case PCB_OBJ_GFX:     pcb_gfx_draw_label(info, (pcb_gfx_t *)obj, vis_side); return;
		case PCB_OBJ_POLY:    pcb_poly_draw_label(info, (pcb_poly_t *)obj, vis_side); return;
		case PCB_OBJ_TEXT:    pcb_text_draw_label(info, (pcb_text_t *)obj, vis_side); return;
		case PCB_OBJ_PSTK:    pcb_pstk_draw_label(info, (pcb_pstk_t *)obj, vis_side); return;
		default: break;
	}
}

/* ---------------------------------------------------------------------------
 * HID drawing callback.
 */

static void expose_begin(pcb_output_t *save, rnd_hid_t *hid)
{
	memcpy(save, &pcb_draw_out, sizeof(pcb_output_t));
	save->hid = rnd_render;

	delayed_labels_enabled = rnd_true;
	vtp0_truncate(&delayed_labels, 0);
	vtp0_truncate(&delayed_objs, 0);

	if ((rnd_render != NULL) && (!rnd_render->override_render))
		rnd_render = hid;
	pcb_draw_out.hid = rnd_render;

	pcb_draw_out.fgGC = rnd_hid_make_gc();
	pcb_draw_out.padGC = rnd_hid_make_gc();
	pcb_draw_out.backpadGC = rnd_hid_make_gc();
	pcb_draw_out.padselGC = rnd_hid_make_gc();
	pcb_draw_out.drillGC = rnd_hid_make_gc();
	pcb_draw_out.pmGC = rnd_hid_make_gc();

	if (hid->force_compositing)
		pcb_draw_out.direct = 0;
	else
		pcb_draw_out.direct = 1;

	hid->set_color(pcb_draw_out.pmGC, rnd_color_cyan);
	hid->set_color(pcb_draw_out.drillGC, rnd_color_drill);
	hid->set_color(pcb_draw_out.padGC, &conf_core.appearance.color.pin);
	hid->set_color(pcb_draw_out.backpadGC, &conf_core.appearance.color.invisible_objects);
	hid->set_color(pcb_draw_out.padselGC, &conf_core.appearance.color.selected);
	rnd_hid_set_line_width(pcb_draw_out.backpadGC, -1);
	rnd_hid_set_line_cap(pcb_draw_out.backpadGC, rnd_cap_square);
	rnd_hid_set_line_width(pcb_draw_out.padselGC, -1);
	rnd_hid_set_line_cap(pcb_draw_out.padselGC, rnd_cap_square);
	rnd_hid_set_line_width(pcb_draw_out.padGC, -1);
	rnd_hid_set_line_cap(pcb_draw_out.padGC, rnd_cap_square);
}

static void expose_end(pcb_output_t *save)
{
	rnd_hid_destroy_gc(pcb_draw_out.fgGC);
	rnd_hid_destroy_gc(pcb_draw_out.padGC);
	rnd_hid_destroy_gc(pcb_draw_out.backpadGC);
	rnd_hid_destroy_gc(pcb_draw_out.padselGC);
	rnd_hid_destroy_gc(pcb_draw_out.drillGC);
	rnd_hid_destroy_gc(pcb_draw_out.pmGC);

	pcb_draw_out.fgGC = NULL;

	delayed_labels_enabled = rnd_false;
	vtp0_truncate(&delayed_labels, 0);
	vtp0_truncate(&delayed_objs, 0);

	memcpy(&pcb_draw_out, save, sizeof(pcb_output_t));
	rnd_render = pcb_draw_out.hid;
}

void pcb_draw_setup_default_gui_xform(rnd_xform_t *dst)
{
	dst->wireframe = conf_core.editor.wireframe_draw;
	dst->thin_draw = conf_core.editor.thin_draw;
	dst->thin_draw_poly = conf_core.editor.thin_draw_poly;
	dst->check_planes = conf_core.editor.check_planes;
	dst->hide_floaters = conf_core.editor.hide_names;
	dst->show_solder_side = conf_core.editor.show_solder_side;
	dst->invis_other_groups = conf_core.appearance.invis_other_groups;
	dst->black_current_group = conf_core.appearance.black_current_group;
	dst->flag_color = 1;
}

void pcb_draw_setup_default_xform_info(rnd_hid_t *hid, pcb_draw_info_t *info)
{
	static rnd_xform_t xf_def = {0};
	static rnd_xform_t xform_main_exp;

	if (info->xform_caller == NULL) {
		if (hid->exporter) {
			/* exporters without xform should automatically inherit the default xform for exporting, which differs from what GUI does */
			memset(&xform_main_exp, 0, sizeof(xform_main_exp));
			info->xform = info->xform_exporter = &xform_main_exp;

			xform_main_exp.omit_overlay = 1; /* normally exporters shouldn't draw overlays */
			info->xform_caller = &xform_main_exp;
		}
		else if (hid->gui) {
			/* no xform means we should use the default logic designed for the GUI */
			info->xform = info->xform_exporter = info->xform_caller = &xf_def;
			pcb_draw_setup_default_gui_xform(&xf_def);
		}
	}
}

int pcb_draw_stamp = 0;
void rnd_expose_main(rnd_hid_t * hid, const rnd_hid_expose_ctx_t *ctx, rnd_xform_t *xform_caller)
{
	if (!pcb_draw_inhibit) {
		pcb_output_t save;
		pcb_draw_info_t info;

		expose_begin(&save, hid);
		pcb_draw_info_setup(&info, PCB);
		info.drawn_area = &ctx->view;
		info.xform_caller = xform_caller;
		info.xform = info.xform_exporter = NULL;
		info.layer = NULL;
		info.draw_stamp = ++pcb_draw_stamp;

		pcb_draw_setup_default_xform_info(hid, &info);

		draw_everything(&info);
		expose_end(&save);
	}
}

void rnd_expose_preview(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *e)
{
	pcb_output_t save;
	expose_begin(&save, hid);

	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, 1, &e->view);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, 1, &e->view);
	e->expose_cb(pcb_draw_out.fgGC, e);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, 1, &e->view);
	rnd_render->end_layer(rnd_render);

	expose_end(&save);
}

static const char *lab_with_intconn(const pcb_any_obj_t *term, int intconn, const char *lab, char *buff, int bufflen)
{
	if ((conf_core.editor.term_id != NULL) && (*conf_core.editor.term_id != '\0')) {
		gds_t tmp;
		if (pcb_append_dyntext(&tmp, term, conf_core.editor.term_id) == 0) {
			int len = tmp.used < (bufflen-1) ? tmp.used : (bufflen-1);
			memcpy(buff, tmp.array, len);
			buff[len] = '\0';
			gds_uninit(&tmp);
			return buff;
		}
	}

	/* fallback when template is not available or failed */
	if (intconn <= 0)
		return lab;
	rnd_snprintf(buff, bufflen, "%s[%d]", lab, intconn);

	return buff;
}

/* vert flip magic: make sure the offset is in-line with the flip and our sick y coords for vertical */
#define PCB_TERM_LABEL_SETUP(label) \
	rnd_bool flip_x = rnd_conf.editor.view.flip_x; \
	rnd_bool flip_y = rnd_conf.editor.view.flip_y; \
	pcb_font_t *font = pcb_font(PCB, 0, 0); \
	rnd_coord_t w, h, dx, dy; \
	double scl = scale/100.0; \
	if (vert) { \
		h = pcb_text_width(font, scl, label); \
		w = pcb_text_height(font, scl, label); \
	} \
	else { \
		w = pcb_text_width(font, scl, label); \
		h = pcb_text_height(font, scl, label); \
	} \
	dx = w / 2; \
	dy = h / 2; \
	if (flip_x ^ flip_y) { \
		if (vert) \
			dx = -dx; \
		else \
			dy = -dy; \
	} \
	if (vert) \
		dy = -dy; \
	if (centered) { \
		x -= dx; \
		y -= dy; \
	}


void pcb_label_draw(pcb_draw_info_t *info, rnd_coord_t x, rnd_coord_t y, double scale, rnd_bool vert, rnd_bool centered, const char *label, int prio)
{
	int mirror, direction;

	PCB_TERM_LABEL_SETUP((const unsigned char *)label);

	mirror = (flip_x ? PCB_TXT_MIRROR_X : 0) | (flip_y ? PCB_TXT_MIRROR_Y : 0);
	direction = (vert ? 1 : 0);

	if (!conf_core.appearance.smart_labels) {

		rnd_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.pin_name);

		if (rnd_render->gui)
			pcb_draw_force_termlab++;
		pcb_text_draw_string(info, font, (unsigned const char *)label, x, y, scale/100, scale/100, direction*90.0, mirror, conf_core.appearance.label_thickness, 0, 0, 0, 0, PCB_TXT_TINY_HIDE);
		if (rnd_render->gui)
			pcb_draw_force_termlab--;
	}
	else
		pcb_label_smart_add(info, x, y, scale/100.0, direction*90.0, mirror, w, h, label, prio);
}


void pcb_label_invalidate(rnd_coord_t x, rnd_coord_t y, double scale, rnd_bool vert, rnd_bool centered, const char *label)
{
	rnd_coord_t ox = x, oy = y, margin = 0;
	rnd_box_t b;
	PCB_TERM_LABEL_SETUP((const unsigned char *)label);

	dx = RND_ABS(dx);
	dy = RND_ABS(dy);
	b.X1 = ox - dx - margin;
	b.X2 = ox + dx + margin;
	b.Y1 = oy - dy - margin;
	b.Y2 = oy + dy + margin;

	pcb_draw_invalidate(&b);
}

void pcb_term_label_draw(pcb_draw_info_t *info, rnd_coord_t x, rnd_coord_t y, double scale, rnd_bool vert, rnd_bool centered, const pcb_any_obj_t *obj)
{
	char buff[128];
	int prio = 1; /* slightly lower prio than subc */
	int on_bottom, flips;
	const char *label = lab_with_intconn(obj, obj->intconn, obj->term, buff, sizeof(buff));

	flips = !!(rnd_conf.editor.view.flip_x ^ rnd_conf.editor.view.flip_y);

	switch(obj->type) {
		case PCB_OBJ_PSTK: /* top prio */
			break;
		case PCB_OBJ_LINE:
		case PCB_OBJ_ARC:
		case PCB_OBJ_TEXT:
		case PCB_OBJ_POLY:
		case PCB_OBJ_GFX:
			on_bottom = !!(pcb_layer_flags_(obj->parent.layer) & PCB_LYT_BOTTOM);
			if (on_bottom == flips)
				prio += 1;
			else
				prio += 2; /* other-side object penalty */
			break;
		default:
			prio += 10;
	}
	pcb_label_draw(info, x, y, scale, vert, centered, label, prio);
}

void pcb_term_label_invalidate(rnd_coord_t x, rnd_coord_t y, double scale, rnd_bool vert, rnd_bool centered, const pcb_any_obj_t *obj)
{
	char buff[128];
	const char *label = lab_with_intconn(obj, obj->intconn, obj->term, buff, sizeof(buff));
	pcb_label_invalidate(x, y, scale, vert, centered, label);
}

