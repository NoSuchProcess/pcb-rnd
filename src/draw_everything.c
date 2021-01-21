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
	/* silk color tune */
	pcb_layergrp_t *backsilk_grp;
	rnd_color_t old_silk_color[PCB_MAX_LAYERGRP];

	/* copper ordering and virtual layers */
	legacy_vlayer_t lvly;
	char do_group[PCB_MAX_LAYERGRP]; /* This is the list of layer groups we will draw.  */
	rnd_layergrp_id_t drawn_groups[PCB_MAX_LAYERGRP]; /* This is the reverse of the order in which we draw them.  */
	int ngroups;
	rnd_layergrp_id_t component, solder, side_copper_grp;
	char lvly_inited;
	char do_group_inited;
} draw_everything_t;

/* temporarily change the color of the other-side silk */
static void drw_silk_tune_color(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_layergrp_id_t backsilk_gid;

	backsilk_gid = ((!info->xform->show_solder_side) ? pcb_layergrp_get_bottom_silk() : pcb_layergrp_get_top_silk());
	de->backsilk_grp = pcb_get_layergrp(PCB, backsilk_gid);
	if (de->backsilk_grp != NULL) {
		rnd_cardinal_t n;
		for(n = 0; n < de->backsilk_grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(PCB->Data, de->backsilk_grp->lid[n]);
			if (ly != NULL) {
				de->old_silk_color[n] = ly->meta.real.color;
				ly->meta.real.color = conf_core.appearance.color.invisible_objects;
			}
		}
	}
}

/* set back the color of the other-side silk */
static void drw_silk_restore_color(pcb_draw_info_t *info, draw_everything_t *de)
{
	if (de->backsilk_grp != NULL) {
		rnd_cardinal_t n;
		for(n = 0; n < de->backsilk_grp->len; n++) {
			pcb_layer_t *ly = pcb_get_layer(PCB->Data, de->backsilk_grp->lid[n]);
			if (ly != NULL)
				ly->meta.real.color = de->old_silk_color[n];
		}
	}
}

/* copper groups will be drawn in the order they were last selected
   in the GUI; build an array of their order */
static void drw_copper_order_UI(pcb_draw_info_t *info, draw_everything_t *de)
{
	int i;

	memset(de->do_group, 0, sizeof(de->do_group));

	de->lvly.top_fab = -1;
	de->lvly.top_assy = de->lvly.bot_assy = -1;

	for (de->ngroups = 0, i = 0; i < pcb_max_layer(PCB); i++) {
		pcb_layer_t *l = PCB_STACKLAYER(PCB, i);
		rnd_layergrp_id_t group = pcb_layer_get_group(PCB, pcb_layer_stack[i]);
		pcb_layergrp_t *grp = pcb_get_layergrp(PCB, group);
		unsigned int gflg = 0;

		if (grp != NULL)
			gflg = grp->ltype;

		if ((gflg & PCB_LYT_DOC) && (grp->purpi == F_fab)) {
			de->lvly.top_fab = group;
			de->lvly.top_fab_enable = has_auto(grp);
		}

		if ((gflg & PCB_LYT_DOC) && (grp->purpi == F_assy)) {
			if (gflg & PCB_LYT_TOP) {
				de->lvly.top_assy = group;
				de->lvly.top_assy_enable = has_auto(grp);
			}
			else if (gflg & PCB_LYT_BOTTOM) {
				de->lvly.bot_assy = group;
				de->lvly.bot_assy_enable = has_auto(grp);
			}
		}

		if ((gflg & PCB_LYT_SILK) || (gflg & PCB_LYT_DOC) || (gflg & PCB_LYT_MASK) || (gflg & PCB_LYT_PASTE) || (gflg & PCB_LYT_BOUNDARY) || (gflg & PCB_LYT_MECH)) /* do not draw silk, mask, paste and boundary here, they'll be drawn separately */
			continue;

		if (l->meta.real.vis && !de->do_group[group]) {
			de->do_group[group] = 1;
			de->drawn_groups[de->ngroups++] = group;
		}
	}
	de->lvly_inited = 1;
	de->do_group_inited = 1;
}

/* draw all layers in layerstack order */
static void drw_copper(pcb_draw_info_t *info, draw_everything_t *de)
{
	int i;

	if (!de->do_group_inited) {
		rnd_message(RND_MSG_ERROR, "draw script: drw_copper called without prior copper ordering\nCan not draw copper layers. Fix the script.\n");
		return;
	}

	for (i = de->ngroups - 1; i >= 0; i--) {
		rnd_layergrp_id_t group = de->drawn_groups[i];

		if (pcb_layer_gui_set_glayer(PCB, group, 0, &info->xform_exporter)) {
			int is_current = 0;
			rnd_layergrp_id_t cgrp = PCB_CURRLAYER(PCB)->meta.real.grp;

			if ((cgrp == de->solder) || (cgrp == de->component)) {
				/* current group is top or bottom: visibility depends on side we are looking at */
				if (group == de->side_copper_grp)
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
}

/* draw all 'invisible' (other-side) layers */
static void drw_invis1(pcb_draw_info_t *info, draw_everything_t *de)
{
	if ((info->xform_exporter != NULL) && !info->xform_exporter->check_planes && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_INVISIBLE, 0, &info->xform_exporter)) {
		pcb_layer_type_t side = PCB_LYT_INVISIBLE_SIDE();

		pcb_draw_silk_doc(info, side, PCB_LYT_DOC, 0, 1);
		pcb_draw_silk_doc(info, side, PCB_LYT_SILK, 0, 1);

		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
		rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
		rnd_render->end_layer(rnd_render);
	}
}

	/* Draw far side doc and silks */
static void drw_invis2(pcb_draw_info_t *info, draw_everything_t *de)
{
	pcb_layer_type_t ivside = PCB_LYT_INVISIBLE_SIDE();

	pcb_draw_silk_doc(info, ivside, PCB_LYT_SILK, 1, 0);
	pcb_draw_silk_doc(info, ivside, PCB_LYT_DOC, 1, 0);
}

static void drw_pstk(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
	if (rnd_render->gui) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		pcb_draw_ppv(info, info->xform->show_solder_side ? de->solder : de->component);
		info->xform = NULL; info->layer = NULL;
	}
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
}


/* Draw the solder mask if turned on */
static void drw_mask(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_layergrp_id_t gid;
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
}

static void drw_hole(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area); 
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
	draw_everything_holes(info, de->side_copper_grp);
	rnd_render->set_drawing_mode(rnd_render, RND_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
}

static void drw_paste(pcb_draw_info_t *info, draw_everything_t *de)
{
	rnd_layergrp_id_t gid;
	rnd_bool paste_empty;

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
}

static void drw_virtual(pcb_draw_info_t *info, draw_everything_t *de)
{
	draw_virtual_layers(info, &de->lvly);
	if ((rnd_render->gui) || (!info->xform_caller->omit_overlay)) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		draw_rats(info, info->drawn_area);
		draw_pins_and_pads(info, de->component, de->solder);
	}
}

static void drw_marks(pcb_draw_info_t *info, draw_everything_t *de)
{
	if (rnd_render->gui) {
		rnd_xform_t tmp;
		xform_setup(info, &tmp, NULL);
		draw_xor_marks(info);
	}
}

static void draw_everything(pcb_draw_info_t *info)
{
	draw_everything_t de;
	rnd_xform_t tmp;
	pcb_layer_type_t vside = PCB_LYT_VISIBLE_SIDE();

	de.backsilk_grp = NULL;
	de.lvly_inited = 0;
	de.do_group_inited = 0;

	xform_setup(info, &tmp, NULL);
	rnd_render->render_burst(rnd_render, RND_HID_BURST_START, info->drawn_area);
	de.solder = de.component = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &de.solder, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &de.component, 1);
	de.side_copper_grp = info->xform->show_solder_side ? de.solder : de.component;

	drw_silk_tune_color(info, &de);
	drw_copper_order_UI(info, &de);
	drw_invis1(info, &de);
	drw_invis2(info, &de);
	drw_copper(info, &de);

	if (conf_core.editor.check_planes && rnd_render->gui)
		goto finish;

	drw_pstk(info, &de);
	drw_mask(info, &de);

	/* Draw doc and silks */
	pcb_draw_silk_doc(info, PCB_LYT_INTERN, PCB_LYT_SILK, 1, 0);
	pcb_draw_silk_doc(info, PCB_LYT_INTERN, PCB_LYT_DOC, 1, 0);
	pcb_draw_silk_doc(info, 0, PCB_LYT_DOC, 1, 0);
	pcb_draw_silk_doc(info, vside, PCB_LYT_SILK, 1, 0);
	pcb_draw_silk_doc(info, vside, PCB_LYT_DOC, 1, 0);

	/* holes_after: draw holes after copper, silk and mask, to make sure it punches through everything. */
	drw_hole(info, &de);

	drw_paste(info, &de);

	pcb_draw_boundary_mech(info);

	drw_virtual(info, &de);
	draw_ui_layers(info);

	drw_marks(info, &de);

	finish:;
	drw_silk_restore_color(info, &de);
}
