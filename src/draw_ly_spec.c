/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
 *  Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
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

/* Local functions for special layer draw logics. This code is not generic;
   instead it collects all the hardwired heuristics for the special layers.
   Included from draw.c. */

/******** paste ********/

static void pcb_draw_paste_auto_(comp_ctx_t *ctx, void *side)
{
	if (PCB->pstk_on)
		pcb_draw_pstks(ctx->info, ctx->gid, 0, PCB_LYC_AUTO);
}

static void pcb_draw_paste(pcb_draw_info_t *info, int side)
{
	unsigned long side_lyt = side ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	pcb_layergrp_id_t gid = -1;
	comp_ctx_t cctx;
	pcb_layer_t *ly = NULL;

	pcb_layergrp_list(info->pcb, PCB_LYT_PASTE | side_lyt, &gid, 1);

	cctx.grp = pcb_get_layergrp((pcb_board_t *)info->pcb, gid);

	if (cctx.grp->len > 0)
		ly = pcb_get_layer(info->pcb->Data, cctx.grp->lid[0]);

	cctx.info = info;
	cctx.gid = gid;
	cctx.color = ly != NULL ? &ly->meta.real.color : &conf_core.appearance.color.paste;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = 0;

	if ((cctx.grp == NULL) || (cctx.grp->len == 0)) { /* fallback: no layers -> original code: draw a single auto-add */
		pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
		pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
		pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
	}
	else {
		comp_draw_layer(&cctx, pcb_draw_paste_auto_, &side);
		comp_finish(&cctx);
	}
}

/******** mask ********/
static void pcb_draw_mask_auto(comp_ctx_t *ctx, void *side)
{
	if (PCB->pstk_on)
		pcb_draw_pstks(ctx->info, ctx->gid, 0, PCB_LYC_SUB | PCB_LYC_AUTO);
}

static void pcb_draw_mask(pcb_draw_info_t *info, int side)
{
	unsigned long side_lyt = side ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	pcb_layergrp_id_t gid = -1;
	comp_ctx_t cctx;
	pcb_layer_t *ly = NULL;

	pcb_layergrp_list(PCB, PCB_LYT_MASK | side_lyt, &gid, 1);

	cctx.grp = pcb_get_layergrp(PCB, gid);

	if (cctx.grp->len > 0)
		ly = pcb_get_layer(PCB->Data, cctx.grp->lid[0]);

	cctx.info = info;
	cctx.gid = gid;
	cctx.color = ly != NULL ? &ly->meta.real.color : &conf_core.appearance.color.mask;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = pcb_gui->mask_invert;

	if (!cctx.invert)
		pcb_draw_out.direct = 0;

	if ((cctx.grp == NULL) || (cctx.grp->len == 0)) { /* fallback: no layers -> original code: draw a single auto-sub */
		comp_init(&cctx, 1);
		comp_start_sub(&cctx);
		pcb_draw_mask_auto(&cctx, &side);
		comp_start_add(&cctx);
	}
	else
		comp_draw_layer(&cctx, pcb_draw_mask_auto, &side);
	comp_finish(&cctx);
}

/******** silk ********/

static void pcb_draw_silk_auto(comp_ctx_t *ctx, void *lyt_side)
{
	if (PCB->pstk_on)
		pcb_draw_pstks(ctx->info, ctx->gid, 0, PCB_LYC_AUTO);
}

static int pcb_is_silk_old_style(comp_ctx_t *cctx, pcb_layer_id_t lid)
{
	if (cctx->grp == NULL)
		return 1; /* no group means no silk -> fall back to implicit */

	if ((cctx->grp->len == 1) && ((PCB->Data->Layer[lid].comb & (PCB_LYC_AUTO | PCB_LYC_SUB)) == PCB_LYC_AUTO))
		return 1; /* A single auto-positive layer -> original code: draw auto+manual */

	return 0;
}

static void pcb_draw_silk_doc(pcb_draw_info_t *info, pcb_layer_type_t lyt_side, pcb_layer_type_t lyt_type, int setgrp, int invis)
{
	pcb_layer_id_t lid;
	pcb_layergrp_id_t gid[PCB_MAX_LAYERGRP];
	comp_ctx_t cctx;
	int len, n;

	len = pcb_layergrp_list(info->pcb, lyt_type | lyt_side, gid, PCB_MAX_LAYERGRP);
	if (len < 1)
		return;

	for(n = 0; n < len; n++) {
		if (!info->pcb->LayerGroups.grp[gid[n]].vis)
			continue;

		if (setgrp)
			if (!pcb_layer_gui_set_glayer(info->pcb, gid[n], 0, &info->xform_exporter))
				continue;

		cctx.info = info;
		cctx.gid = gid[n];
		cctx.grp = pcb_get_layergrp((pcb_board_t *)info->pcb, gid[n]);
		if ((lyt_side == 0) && (cctx.grp->ltype & PCB_LYT_ANYWHERE) != 0) /* special case for global */
			continue;
		if (cctx.grp->len == 0)
			continue;
		lid = cctx.grp->lid[0];
		cctx.color = invis ? &conf_core.appearance.color.invisible_objects : &info->pcb->Data->Layer[lid].meta.real.color;
		cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
		cctx.invert = 0;

		if ((lyt_type & PCB_LYT_SILK) && (pcb_is_silk_old_style(&cctx, lid))) {
			/* fallback: implicit layer -> original code: draw auto+manual */
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
			pcb_draw_layer(info, LAYER_PTR(lid));
			pcb_draw_silk_auto(&cctx, &lyt_side);
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
		}
		else {
			comp_draw_layer(&cctx, pcb_draw_silk_auto, &lyt_side);
			comp_finish(&cctx);
		}
		if (setgrp)
			pcb_render->end_layer(pcb_gui);
	}
}

static void remember_slot(pcb_layer_t **uslot, pcb_layer_t **pslot, int *uscore, int *pscore, const pcb_layergrp_t *g, pcb_layer_t *ly)
{
	int score;
	pcb_layer_t **dslot;
	int *dscore;

	if (!(ly->comb & PCB_LYC_AUTO))
		return;

	if (g->purpi == F_uroute) {
		dslot = uslot;
		dscore = uscore;
	}
	else if (g->purpi == F_proute) {
		dslot = pslot;
		dscore = pscore;
	}
	else
		return;

	if (g->ltype & PCB_LYT_BOUNDARY) score = 1;
	if (g->ltype & PCB_LYT_MECH) score = 2;

	if (score > *dscore) {
		*dscore = score;
		*dslot = ly;
	}
}

static void pcb_draw_boundary_mech(pcb_draw_info_t *info)
{
	int count = 0;
	pcb_layergrp_id_t gid, goutid;
	const pcb_layergrp_t *g, *goutl = NULL;
	pcb_layer_t *uslot = NULL, *pslot = NULL;
	int uscore = 0, pscore = 0;
	int plated, unplated;

	for(gid = 0, g = info->pcb->LayerGroups.grp; gid < info->pcb->LayerGroups.len; gid++,g++) {
		int n, numobj;

		if ((g->ltype & PCB_LYT_BOUNDARY) && (g->purpi == F_uroute)) {
			goutl = g;
			goutid = gid;
		}

		if (!(g->ltype & (PCB_LYT_BOUNDARY | PCB_LYT_MECH)) || (g->len < 1))
			continue;

		/* Count whether there are objects on any boundary layer:
		   don't count the objects drawn, but the objects the layer has;
		   zooming in the middle doesn't mean we need to have the implicit
		   outline */
		numobj = 0;
		for(n = 0; n < g->len; n++) {
			pcb_layer_t *ly = LAYER_PTR(g->lid[n]);
			if (ly->line_tree != NULL)
				numobj += ly->line_tree->size;
			if (ly->arc_tree != NULL)
				numobj += ly->arc_tree->size;
			if (ly->text_tree != NULL)
				numobj += ly->text_tree->size;
			if (ly->polygon_tree != NULL)
				numobj += ly->polygon_tree->size;
			remember_slot(&uslot, &pslot, &uscore, &pscore, g, ly);
		}
		count += numobj;

		if (pcb_layer_gui_set_layer(gid, g, (numobj == 0), &info->xform_exporter)) {
			/* boundary does NOT support compisiting, everything is drawn in positive */
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
			for(n = 0; n < g->len; n++) {
				pcb_layer_t *ly = LAYER_PTR(g->lid[n]);
				pcb_draw_layer(info, ly);
			}
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
			pcb_render->end_layer(pcb_gui);
		}
	}

	if ((count == 0) && (goutl != NULL) && (pcb_layer_gui_set_layer(goutid, goutl, 0, &info->xform))) {
		/* The implicit outline rectangle (or automatic outline rectanlge).
		   We should check for pcb_gui->gui here, but it's kinda cool seeing the
		   auto-outline magically disappear when you first add something to
		   the outline layer.  */
		pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
		pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);

		pcb_gui->set_color(pcb_draw_out.fgGC, &PCB->Data->Layer[goutl->lid[0]].meta.real.color);
		pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
		pcb_hid_set_line_width(pcb_draw_out.fgGC, conf_core.design.min_wid);
		pcb_gui->draw_rect(pcb_draw_out.fgGC, 0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y);

		pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
		pcb_render->end_layer(pcb_gui);
	}

	/* draw slots */
	if (((uslot == NULL) || (!uslot->meta.real.vis)) && ((pslot == NULL) || (!pslot->meta.real.vis)))
		return;

	pcb_board_count_slots(PCB, &plated, &unplated, info->drawn_area);

	if ((uslot != NULL) && (uslot->meta.real.vis)) {
		if (pcb_layer_gui_set_glayer(PCB, uslot->meta.real.grp, unplated > 0, &info->xform)) {
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
			pcb_draw_pstk_slots(info, uslot->meta.real.grp, PCB_PHOLE_UNPLATED | PCB_PHOLE_BB);
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
			pcb_render->end_layer(pcb_gui);
		}
	}
	if ((pslot != NULL) && (pslot->meta.real.vis)) {
		if (pcb_layer_gui_set_glayer(PCB, pslot->meta.real.grp, plated > 0, &info->xform)) {
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, info->drawn_area);
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, info->drawn_area);
			pcb_draw_pstk_slots(info, pslot->meta.real.grp, PCB_PHOLE_PLATED | PCB_PHOLE_BB);
			pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, info->drawn_area);
			pcb_render->end_layer(pcb_gui);
		}
	}
}


/******** misc ********/

static void pcb_draw_rats(const pcb_box_t *drawn_area)
{
	pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
	pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
	pcb_r_search(PCB->Data->rat_tree, drawn_area, NULL, pcb_rat_draw_callback, NULL, NULL);
	pcb_gui->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
}

static void pcb_draw_assembly(pcb_draw_info_t *info, pcb_layer_type_t lyt_side)
{
	pcb_layergrp_id_t side_group;

	if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | lyt_side, &side_group, 1) != 1)
		return;

	pcb_draw_doing_assy = pcb_true;
	pcb_hid_set_draw_faded(pcb_draw_out.fgGC, 1);
	pcb_draw_layer_grp(info, side_group, 0);
	pcb_hid_set_draw_faded(pcb_draw_out.fgGC, 0);

	/* draw package */
	pcb_draw_silk_doc(info, lyt_side, PCB_LYT_SILK, 0, 0);
	pcb_draw_doing_assy = pcb_false;
}
