/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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
		pcb_draw_pstks(ctx->gid, ctx->screen, 0, PCB_LYC_AUTO);
}

static void pcb_draw_paste(int side, const pcb_box_t *drawn_area)
{
	unsigned long side_lyt = side ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	pcb_layergrp_id_t gid = -1;
	comp_ctx_t cctx;
	pcb_layer_t *ly = NULL;

	pcb_layergrp_list(PCB, PCB_LYT_PASTE | side_lyt, &gid, 1);

	cctx.grp = pcb_get_layergrp(PCB, gid);

	if (cctx.grp->len > 0)
		ly = pcb_get_layer(PCB->Data, cctx.grp->lid[0]);

	cctx.pcb = PCB;
	cctx.screen = drawn_area;
	cctx.gid = gid;
	cctx.color = ly != NULL ? ly->meta.real.color : conf_core.appearance.color.paste;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = 0;

	if ((cctx.grp == NULL) || (cctx.grp->len == 0)) { /* fallback: no layers -> original code: draw a single auto-add */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, cctx.screen);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, cctx.screen);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, cctx.screen);
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
		pcb_draw_pstks(ctx->gid, ctx->screen, 0, PCB_LYC_SUB | PCB_LYC_AUTO);
}

static void pcb_draw_mask(int side, const pcb_box_t *screen)
{
	unsigned long side_lyt = side ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	pcb_layergrp_id_t gid = -1;
	comp_ctx_t cctx;
	pcb_layer_t *ly = NULL;

	pcb_layergrp_list(PCB, PCB_LYT_MASK | side_lyt, &gid, 1);

	cctx.grp = pcb_get_layergrp(PCB, gid);

	if (cctx.grp->len > 0)
		ly = pcb_get_layer(PCB->Data, cctx.grp->lid[0]);

	cctx.pcb = PCB;
	cctx.screen = screen;

	cctx.gid = gid;
	cctx.color = ly != NULL ? ly->meta.real.color : conf_core.appearance.color.mask;
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
		pcb_draw_pstks(ctx->gid, ctx->screen, 0, PCB_LYC_AUTO);
}

static int pcb_is_silk_old_style(comp_ctx_t *cctx, pcb_layer_id_t lid)
{
	if (cctx->grp == NULL)
		return 1; /* no group means no silk -> fall back to implicit */

	if ((cctx->grp->len == 1) && ((PCB->Data->Layer[lid].comb & (PCB_LYC_AUTO | PCB_LYC_SUB)) == PCB_LYC_AUTO))
		return 1; /* A single auto-positive layer -> original code: draw auto+manual */

	return 0;
}

static void pcb_draw_silk(unsigned long lyt_side, const pcb_box_t *drawn_area)
{
	pcb_layer_id_t lid;
	pcb_layergrp_id_t gid = -1;
	comp_ctx_t cctx;

	if (pcb_layer_list(PCB, PCB_LYT_SILK | lyt_side, &lid, 1) == 0)
		return;

	pcb_layergrp_list(PCB, PCB_LYT_SILK | lyt_side, &gid, 1);
	if (!PCB->LayerGroups.grp[gid].vis)
		return;

	cctx.pcb = PCB;
	cctx.screen = drawn_area;
	cctx.grp = pcb_get_layergrp(PCB, gid);
	cctx.gid = gid;
	cctx.color = PCB->Data->Layer[lid].meta.real.color;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = 0;

	if (pcb_is_silk_old_style(&cctx, lid)) {
		/* fallback: implicit layer -> original code: draw auto+manual */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, cctx.screen);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, cctx.screen);
		pcb_draw_layer(LAYER_PTR(lid), cctx.screen, NULL);
		pcb_draw_silk_auto(&cctx, &lyt_side);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, cctx.screen);
	}
	else {
		comp_draw_layer(&cctx, pcb_draw_silk_auto, &lyt_side);
		comp_finish(&cctx);
	}
}

static void pcb_draw_boundary_mech(const pcb_box_t *drawn_area)
{
	int count = 0;
	pcb_layergrp_id_t gid, goutid;
	pcb_layergrp_t *g, *goutl = NULL;
	comp_ctx_t cctx;

	cctx.pcb = PCB;
	cctx.screen = drawn_area;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = 0;


	for(gid = 0, g = PCB->LayerGroups.grp; gid < PCB->LayerGroups.len; gid++,g++) {
		int n, numobj;

		if ((g->ltype & PCB_LYT_BOUNDARY) && (g->purpi = F_uroute)) {
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
		}
		count += numobj;

		if (pcb_layer_gui_set_layer(gid, g, (numobj == 0))) {
			cctx.gid = gid;
			cctx.grp = g;

			/* boundary does NOT support compisiting, everything is drawn in positive */
			pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, cctx.screen);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, cctx.screen);
			for(n = 0; n < g->len; n++) {
				pcb_layer_t *ly = LAYER_PTR(g->lid[n]);
				cctx.color = ly->meta.real.color;
				pcb_draw_layer(ly, cctx.screen, NULL);
			}
			pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, cctx.screen);
		}
	}

	if ((count == 0) && (goutl != NULL) && (pcb_layer_gui_set_layer(goutid, goutl, 0))) {
		/* The implicit outline rectangle (or automatic outline rectanlge).
		   We should check for pcb_gui->gui here, but it's kinda cool seeing the
		   auto-outline magically disappear when you first add something to
		   the outline layer.  */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, cctx.screen);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, cctx.screen);

		pcb_gui->set_color(pcb_draw_out.fgGC, PCB->Data->Layer[goutl->lid[0]].meta.real.color);
		pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
		pcb_hid_set_line_width(pcb_draw_out.fgGC, conf_core.design.min_wid);
		pcb_gui->draw_rect(pcb_draw_out.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);

		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, cctx.screen);
	}
}


/******** misc ********/

static void pcb_draw_rats(const pcb_box_t *drawn_area)
{
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
	pcb_r_search(PCB->Data->rat_tree, drawn_area, NULL, pcb_rat_draw_callback, NULL, NULL);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
}

static void pcb_draw_assembly(unsigned int lyt_side, const pcb_box_t *drawn_area)
{
	pcb_layergrp_id_t side_group;

	if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | lyt_side, &side_group, 1) != 1)
		return;

	pcb_draw_doing_assy = pcb_true;
	pcb_hid_set_draw_faded(pcb_draw_out.fgGC, 1);
	DrawLayerGroup(side_group, drawn_area, 0);
	pcb_hid_set_draw_faded(pcb_draw_out.fgGC, 0);

	/* draw package */
	pcb_draw_silk(lyt_side, drawn_area);
	pcb_draw_doing_assy = pcb_false;
}
