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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Local functions to draw a layer group as a composite of logical layers
   using positive and negative draw operations. Included from draw.c. */

typedef struct comp_ctx_s {
	pcb_draw_info_t *info;
	const pcb_layergrp_t *grp;
	pcb_layergrp_id_t gid;
	const pcb_color_t *color;

	unsigned thin:1;
	unsigned invert:1;
} comp_ctx_t;

static void comp_fill_board(comp_ctx_t *ctx)
{
	pcb_render->set_color(pcb_draw_out.fgGC, ctx->color);
	if (ctx->info->drawn_area == NULL)
		pcb_render->fill_rect(pcb_draw_out.fgGC, 0, 0, ctx->info->pcb->hidlib.size_x, ctx->info->pcb->hidlib.size_y);
	else
		pcb_render->fill_rect(pcb_draw_out.fgGC, ctx->info->drawn_area->X1, ctx->info->drawn_area->Y1, ctx->info->drawn_area->X2, ctx->info->drawn_area->Y2);
}

static void comp_start_sub_(comp_ctx_t *ctx)
{
	pcb_render->set_drawing_mode(pcb_gui, PCB_HID_COMP_NEGATIVE, pcb_draw_out.direct, ctx->info->drawn_area);
}

static void comp_start_add_(comp_ctx_t *ctx)
{
	pcb_render->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, ctx->info->drawn_area);
}

static void comp_start_sub(comp_ctx_t *ctx)
{
	if (ctx->thin) {
		pcb_render->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, ctx->info->drawn_area);
		pcb_render->set_color(pcb_draw_out.pmGC, ctx->color);
		return;
	}

	if (ctx->invert)
		comp_start_add_(ctx);
	else
		comp_start_sub_(ctx);
}

static void comp_start_add(comp_ctx_t *ctx)
{
	if (ctx->thin) {
		pcb_render->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, ctx->info->drawn_area);
		return;
	}

	if (ctx->invert)
		comp_start_sub_(ctx);
	else
		comp_start_add_(ctx);
}

static void comp_finish(comp_ctx_t *ctx)
{
	pcb_render->set_drawing_mode(pcb_gui, PCB_HID_COMP_FLUSH, pcb_draw_out.direct, ctx->info->drawn_area);
}

static void comp_init(comp_ctx_t *ctx, int negative)
{
	pcb_render->set_drawing_mode(pcb_gui, PCB_HID_COMP_RESET, pcb_draw_out.direct, ctx->info->drawn_area);

	if (ctx->thin)
		return;

	if (ctx->invert)
		negative = !negative;

	if ((!ctx->thin) && (negative)) {
		/* drawing the big poly for the negative */
		pcb_render->set_drawing_mode(pcb_gui, PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, ctx->info->drawn_area);
		comp_fill_board(ctx);
	}
}

/* Real composite draw: if any layer is negative, we have to use the HID API's
   composite layer draw mechanism */
static void comp_draw_layer_real(comp_ctx_t *ctx, void (*draw_auto)(comp_ctx_t *ctx, void *data), void *auto_data)
{ /* generic multi-layer rendering */
	int n, adding = -1;
	pcb_layer_t *l = pcb_get_layer(PCB->Data, ctx->grp->lid[0]);
	comp_init(ctx, (l->comb & PCB_LYC_SUB));

	for(n = 0; n < ctx->grp->len; n++) {
		int want_add;
		l = pcb_get_layer(PCB->Data, ctx->grp->lid[n]);

		want_add = ctx->thin || !(l->comb & PCB_LYC_SUB);
		if (want_add != adding) {
			if (want_add)
				comp_start_add(ctx);
			else
				comp_start_sub(ctx);
			adding = want_add;
		}

		{
			pcb_hid_gc_t old_fg = pcb_draw_out.fgGC;
			pcb_draw_out.fgGC = pcb_draw_out.pmGC;
			if ((l->comb & PCB_LYC_AUTO) && (draw_auto != NULL))
				draw_auto(ctx, auto_data);
			pcb_draw_layer(ctx->info, l);
			pcb_draw_out.fgGC = old_fg;
		}
	}
	if (!adding)
		comp_start_add(ctx);
}

int pcb_draw_layergrp_is_comp(const pcb_layergrp_t *g)
{
	int n;

	/* as long as we are doing only positive compositing, we can go without compositing */
	for(n = 0; n < g->len; n++)
		if (PCB->Data->Layer[g->lid[n]].comb & (PCB_LYC_SUB))
			return 1;

	return 0;
}

int pcb_draw_layer_is_comp(pcb_layer_id_t id)
{
	pcb_layergrp_t *g = pcb_get_layergrp(PCB, PCB->Data->Layer[id].meta.real.grp);
	if (g == NULL) return 0;
	return pcb_draw_layergrp_is_comp(g);
}

/* Draw a layer group with fake or real compositing */
static void comp_draw_layer(comp_ctx_t *ctx, void (*draw_auto)(comp_ctx_t *ctx, void *data), void *auto_data)
{
	int is_comp = pcb_draw_layergrp_is_comp(ctx->grp);

	if (is_comp)
		pcb_draw_out.direct = 0;

	comp_draw_layer_real(ctx, draw_auto, auto_data);
}

/******** generic group draws ********/

static void pcb_draw_groups_auto(comp_ctx_t *ctx, void *lym)
{
	pcb_layer_type_t *pstk_lyt_match = (pcb_layer_type_t *)lym;
	if ((ctx->info->pcb->pstk_on) && (*pstk_lyt_match != 0))
		pcb_draw_pstks(ctx->info, ctx->gid, 0, *pstk_lyt_match);
}

void pcb_draw_groups(pcb_board_t *pcb, pcb_layer_type_t lyt, int purpi, char *purpose, const pcb_box_t *screen, const pcb_color_t *default_color, pcb_layer_type_t pstk_lyt_match, int thin_draw, int invert)
{
	pcb_draw_info_t info;
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;
	comp_ctx_t cctx;

	memset(&info, 0, sizeof(info));
	info.pcb = pcb;
	info.drawn_area = screen;

	for(gid = 0, g = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,g++) {
		pcb_layer_t *ly = NULL;

		if ((g->ltype & lyt) != lyt)
			continue;

		if ((purpi != -1) && (g->purpi != purpi))
			continue;

		if ((purpose != NULL) && (strcmp(g->purpose, purpose) != 0))
			continue;

		if (g->len > 0)
			ly = pcb_get_layer(PCB->Data, g->lid[0]);
		info.layer = ly;
		cctx.grp = g;
		cctx.info = &info;
		cctx.gid = gid;
		cctx.color = ly != NULL ? &ly->meta.real.color : default_color;
		cctx.thin = thin_draw;
		cctx.invert = invert;

		if (!cctx.invert)
			pcb_draw_out.direct = 0;

		comp_draw_layer(&cctx, pcb_draw_groups_auto, &pstk_lyt_match);
		comp_finish(&cctx);
	}
}
