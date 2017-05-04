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

/* Local functions to draw a layer group as a composite of logical layers
   using positive and negative draw operations. Included from draw.c. */

typedef struct comp_ctx_s {
	pcb_board_t *pcb;
	const pcb_box_t *screen;
	pcb_layer_group_t *grp;
	const char *color;

	unsigned thin:1;
	unsigned invert:1;
	unsigned poly_before:1;
	unsigned poly_after:1;
} comp_ctx_t;

static void comp_fill_board(comp_ctx_t *ctx, int mask_type)
{
	/* Skip the mask drawing if the GUI doesn't want this type */
	if ((mask_type == HID_MASK_BEFORE && !ctx->poly_before) || (mask_type == HID_MASK_AFTER && !ctx->poly_after))
		return;

	pcb_gui->use_mask(mask_type);
	pcb_gui->set_color(Output.fgGC, ctx->color);
	if (ctx->screen == NULL)
		pcb_gui->fill_rect(Output.fgGC, 0, 0, ctx->pcb->MaxWidth, ctx->pcb->MaxHeight);
	else
		pcb_gui->fill_rect(Output.fgGC, ctx->screen->X1, ctx->screen->Y1, ctx->screen->X2, ctx->screen->Y2);
}

static void comp_start_sub_(comp_ctx_t *ctx)
{
	if (ctx->thin)
		pcb_gui->set_color(Output.pmGC, ctx->color);
	else
		pcb_gui->use_mask(HID_MASK_CLEAR);
}

static void comp_start_add_(comp_ctx_t *ctx)
{
	if (ctx->thin)
		pcb_gui->set_color(Output.pmGC, "erase");
	else
		pcb_gui->use_mask(HID_MASK_SET);
}

static void comp_start_sub(comp_ctx_t *ctx)
{
	if (ctx->thin)
		return;

	if (ctx->invert)
		comp_start_add_(ctx);
	else
		comp_start_sub_(ctx);
}

static void comp_start_add(comp_ctx_t *ctx)
{
	if (ctx->thin)
		return;

	if (ctx->invert)
		comp_start_sub_(ctx);
	else
		comp_start_add_(ctx);
}

static void comp_finish(comp_ctx_t *ctx)
{
	if (ctx->thin)
		return;

	pcb_gui->use_mask(HID_MASK_AFTER);
	comp_fill_board(ctx, HID_MASK_AFTER);
	pcb_gui->use_mask(HID_MASK_OFF);
}

static void comp_init(comp_ctx_t *ctx, int negative)
{
	if (ctx->thin)
		return;

	pcb_gui->use_mask(HID_MASK_INIT);

	if (ctx->invert)
		negative = !negative;

	if ((!ctx->thin) && (negative)) {
		/* old way of drawing the big poly for the negative */
		comp_fill_board(ctx, HID_MASK_BEFORE);

		if (!pcb_gui->poly_before) {
			/* new way */
			pcb_gui->use_mask(HID_MASK_SET);
			comp_fill_board(ctx, HID_MASK_SET);
		}
	}
}

static void comp_draw_layer(comp_ctx_t *ctx, void (*draw_auto)(comp_ctx_t *ctx, void *data), void *auto_data)
{ /* generic multi-layer rendering */
	int n, adding = -1;
	pcb_layer_t *l = pcb_get_layer(ctx->grp->lid[0]);
	comp_init(ctx, (l->comb & PCB_LYC_SUB));

	for(n = 0; n < ctx->grp->len; n++) {
		int want_add;
		l = pcb_get_layer(ctx->grp->lid[n]);

		want_add = ctx->thin || !(l->comb & PCB_LYC_SUB);
		if (want_add != adding) {
			if (want_add)
				comp_start_add(ctx);
			else
				comp_start_sub(ctx);
			adding = want_add;
		}

		{
			const char *old_color = l->Color;
			pcb_hid_gc_t old_fg = Output.fgGC;
			Output.fgGC = Output.pmGC;
			l->Color = ctx->color;
			if (!want_add)
				l->Color = "erase";
			if (l->comb & PCB_LYC_AUTO)
				draw_auto(ctx, auto_data);
			pcb_draw_layer(l, ctx->screen);
			l->Color = old_color;
			Output.fgGC = old_fg;
		}
	}
	if (!adding)
		comp_start_add(ctx);
}

int pcb_draw_layer_is_comp(pcb_layer_id_t id)
{
	pcb_layer_group_t *g = pcb_get_layergrp(PCB, PCB->Data->Layer[id].grp);
	if (g == NULL) return 0;

	/* silk has special rules because the original format/model had 1 (auto+)
	   silk layer in the layer group */
	if (g->type & PCB_LYT_SILK)
		return ((g->len != 1) ||
			((PCB->Data->Layer[id].comb & (PCB_LYC_AUTO | PCB_LYC_SUB)) != PCB_LYC_AUTO));

	return (g->len > 0);
}
