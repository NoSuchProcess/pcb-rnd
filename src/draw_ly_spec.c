/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Local functions for special layer draw logics. This code is not generic;
   instead it collects all the hardwired heuristics for the special layers.
   Included from draw.c. */

/******** paste ********/

static void pcb_draw_paste_auto_(comp_ctx_t *ctx, void *side)
{
	pcb_pad_paste_draw(*(int *)side, ctx->screen);
}

static void pcb_draw_paste(int side, const pcb_box_t *drawn_area)
{
	unsigned long side_lyt = side ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	pcb_layergrp_id_t gid = -1;
	comp_ctx_t cctx;

	pcb_layergrp_list(PCB, PCB_LYT_PASTE | side_lyt, &gid, 1);

	cctx.pcb = PCB;
	cctx.screen = drawn_area;
	cctx.grp = pcb_get_layergrp(PCB, gid);
	cctx.color = conf_core.appearance.color.paste;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = 0;

	if ((cctx.grp == NULL) || (cctx.grp->len == 0)) { /* fallback: no layers -> original code: draw a single auto-add */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, cctx.screen);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, cctx.screen);
		pcb_pad_paste_draw(side, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, cctx.screen);
	}
	else {
		comp_draw_layer(&cctx, pcb_draw_paste_auto_, &side);
		comp_finish(&cctx);
	}
}

/******** mask ********/
static void pcb_draw_mask_auto(comp_ctx_t *ctx, void *side)
{
	pcb_r_search(PCB->Data->pin_tree, ctx->screen, NULL, pcb_pin_clear_callback, NULL, NULL);
	pcb_r_search(PCB->Data->via_tree, ctx->screen, NULL, pcb_pin_clear_callback, NULL, NULL);
	pcb_r_search(PCB->Data->pad_tree, ctx->screen, NULL, pcb_pad_clear_callback, side, NULL);
}

static void pcb_draw_mask(int side, const pcb_box_t *screen)
{
	unsigned long side_lyt = side ? PCB_LYT_TOP : PCB_LYT_BOTTOM;
	pcb_layergrp_id_t gid = -1;
	comp_ctx_t cctx;

	pcb_layergrp_list(PCB, PCB_LYT_MASK | side_lyt, &gid, 1);

	cctx.pcb = PCB;
	cctx.screen = screen;
	cctx.grp = pcb_get_layergrp(PCB, gid);
	cctx.color = conf_core.appearance.color.mask;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = pcb_gui->mask_invert;

	if (!cctx.invert)
		Output.direct = 0;

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
	int side = (*(unsigned long *)lyt_side == PCB_LYT_TOP ? PCB_COMPONENT_SIDE : PCB_SOLDER_SIDE);

	/* draw package */
	pcb_r_search(PCB->Data->element_tree, ctx->screen, NULL, pcb_elem_draw_callback, &side, NULL);
	pcb_r_search(PCB->Data->name_tree[PCB_ELEMNAME_IDX_VISIBLE()], ctx->screen, NULL, pcb_elem_name_draw_callback, &side, NULL);
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

	if (pcb_layer_list(PCB_LYT_SILK | lyt_side, &lid, 1) == 0)
		return;

	pcb_layergrp_list(PCB, PCB_LYT_SILK | lyt_side, &gid, 1);
	if (!PCB->LayerGroups.grp[gid].vis)
		return;

	cctx.pcb = PCB;
	cctx.screen = drawn_area;
	cctx.grp = pcb_get_layergrp(PCB, gid);
	cctx.color = /*PCB->Data->Layer[lid].Color*/ conf_core.appearance.color.element;
	cctx.thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly || conf_core.editor.wireframe_draw;
	cctx.invert = 0;

	if (pcb_is_silk_old_style(&cctx, lid)) {
		/* fallback: implicit layer -> original code: draw auto+manual */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, cctx.screen);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, cctx.screen);
		pcb_draw_layer(LAYER_PTR(lid), cctx.screen);
		pcb_draw_silk_auto(&cctx, &lyt_side);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, cctx.screen);
	}
	else {
		comp_draw_layer(&cctx, pcb_draw_silk_auto, &lyt_side);
		comp_finish(&cctx);
	}
}


/******** misc ********/

static void pcb_draw_rats(const pcb_box_t *drawn_area)
{
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);
	pcb_r_search(PCB->Data->rat_tree, drawn_area, NULL, pcb_rat_draw_callback, NULL, NULL);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);
}

static void pcb_draw_assembly(unsigned int lyt_side, const pcb_box_t *drawn_area)
{
	pcb_layergrp_id_t side_group;

	if (pcb_layergrp_list(PCB, PCB_LYT_COPPER | lyt_side, &side_group, 1) != 1)
		return;

	pcb_draw_doing_assy = pcb_true;
	pcb_gui->set_draw_faded(Output.fgGC, 1);
	DrawLayerGroup(side_group, drawn_area);
	pcb_gui->set_draw_faded(Output.fgGC, 0);

	/* draw package */
	pcb_draw_silk(lyt_side, drawn_area);
	pcb_draw_doing_assy = pcb_false;
}
