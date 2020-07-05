/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"
#include <librnd/core/actions.h>
#include "board.h"
#include "data.h"
#include <librnd/core/event.h>
#include "conf_core.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/hid_dad.h>
#include "funchash_core.h"
#include "obj_subc.h"
#include "search.h"
#include "dlg_layer_binding.h"

const char *pcb_lb_comp[] = { "+manual", "-manual", "+auto", "-auto", NULL };
const char *pcb_lb_types[] = { "UNKNOWN", "paste", "mask", "silk", "copper", "boundary", "mech", "doc", "virtual", NULL };
const char *pcb_lb_side[] = { "top", "bottom", "global", NULL };

typedef struct {
	int name, comp, type, offs, from, side, purpose, layer; /* widet indices */
} lb_widx_t;

typedef struct {
	const char **layer_names;
	lb_widx_t *widx;
	pcb_data_t *data;
	pcb_subc_t *subc;
	pcb_board_t *pcb;
	int no_layer; /* the "invalid layer" in the layer names enum */
	rnd_hid_attribute_t *attrs;
} lb_ctx_t;

int pcb_ly_type2enum(pcb_layer_type_t type)
{
	if (type & PCB_LYT_PASTE)        return 1;
	else if (type & PCB_LYT_MASK)    return 2;
	else if (type & PCB_LYT_SILK)    return 3;
	else if (type & PCB_LYT_COPPER)  return 4;
	else if (type & PCB_LYT_BOUNDARY) return 5;
	else if (type & PCB_LYT_MECH)    return 6;
	else if (type & PCB_LYT_DOC)     return 7;
	else if (type & PCB_LYT_VIRTUAL) return 8;
	return 0;
}

static void set_ly_type(void *hid_ctx, int wid, pcb_layer_type_t type)
{
	RND_DAD_SET_VALUE(hid_ctx, wid, lng, pcb_ly_type2enum(type));
}

static pcb_layer_type_t int2side(int i)
{
	switch(i) {
		case 0: return PCB_LYT_TOP;
		case 1: return PCB_LYT_BOTTOM;
		case 2: return 0;
	}
	return 0;
}

static int side2int(pcb_layer_type_t s)
{
	if (s & PCB_LYT_TOP) return 0;
	if (s & PCB_LYT_BOTTOM) return 1;
	return 2;
}

void pcb_get_ly_type_(int combo_type, pcb_layer_type_t *type)
{
	/* clear relevant flags */
	*type &= ~(PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE | PCB_LYT_VIRTUAL);

	/* set type */
	switch(combo_type) {
		case 1: *type |= PCB_LYT_PASTE; break;
		case 2: *type |= PCB_LYT_MASK; break;
		case 3: *type |= PCB_LYT_SILK; break;
		case 4: *type |= PCB_LYT_COPPER; break;
		case 5: *type |= PCB_LYT_BOUNDARY; break;
		case 6: *type |= PCB_LYT_MECH; break;
		case 7: *type |= PCB_LYT_DOC; break;
		case 8: *type |= PCB_LYT_VIRTUAL; break;
	}
}


static void get_ly_type(int combo_type, int combo_side, int dlg_offs, pcb_layer_type_t *type, int *offs)
{
	pcb_get_ly_type_(combo_type, type);

	if (PCB_LAYER_SIDED(*type)) {
		/* set side and offset */
		if (dlg_offs == 0) {
			*type |= int2side(combo_side);
		}
		else {
			if (combo_side != 0)
				dlg_offs = -dlg_offs;
			*type |= PCB_LYT_INTERN;
		}
	}
	*offs = dlg_offs;
}


#define layer_name_mismatch(w, layer) \
((ctx->attrs[w->name].val.str == NULL) || (strcmp(layer->name, ctx->attrs[w->name].val.str) != 0))

#define layer_purpose_mismatch(w, layer) \
((ctx->attrs[w->purpose].val.str == NULL) || (layer->meta.bound.purpose == NULL) || (strcmp(layer->meta.bound.purpose, ctx->attrs[w->purpose].val.str) != 0))

static void lb_data2dialog(void *hid_ctx, lb_ctx_t *ctx)
{
	int n;
	rnd_bool enable;

	for(n = 0; n < ctx->data->LayerN; n++) {
		lb_widx_t *w = ctx->widx + n;
		pcb_layer_t *layer = ctx->data->Layer + n;
		rnd_layer_id_t lid;
		int ofs;

		/* disable comp for copper and outline */
		enable = !(layer->meta.bound.type & PCB_LYT_COPPER) && !(layer->meta.bound.type & PCB_LYT_BOUNDARY);
		rnd_gui->attr_dlg_widget_state(hid_ctx, w->comp, enable);
		if (!enable)
			layer->comb = 0; /* copper and outline must be +manual */

		/* name and type */
		if (layer_name_mismatch(w, layer))
			RND_DAD_SET_VALUE(hid_ctx, w->name, str, rnd_strdup(layer->name));

		if (layer_purpose_mismatch(w, layer)) {
			char *purp = layer->meta.bound.purpose;
			if (purp == NULL)
				purp = "";
			RND_DAD_SET_VALUE(hid_ctx, w->purpose, str, rnd_strdup(purp));
		}

		RND_DAD_SET_VALUE(hid_ctx, w->comp, lng, layer->comb);

		set_ly_type(hid_ctx, w->type, layer->meta.bound.type);

		/* disable side for non-sided */
		if (PCB_LAYER_SIDED(layer->meta.bound.type)) {
			/* side & offset */
			RND_DAD_SET_VALUE(hid_ctx, w->side, lng, side2int(layer->meta.bound.type));
			rnd_gui->attr_dlg_widget_state(hid_ctx, w->side, 1);
		}
		else
			rnd_gui->attr_dlg_widget_state(hid_ctx, w->side, 0);

		ofs = layer->meta.bound.stack_offs;
		if (ofs < 0) {
			RND_DAD_SET_VALUE(hid_ctx, w->side, lng, 1);
			ofs = -layer->meta.bound.stack_offs;
		}
		RND_DAD_SET_VALUE(hid_ctx, w->offs, lng, ofs);

		/* enable offset only for copper */
		enable = (layer->meta.bound.type & PCB_LYT_COPPER);
		rnd_gui->attr_dlg_widget_state(hid_ctx, w->offs, enable);
		rnd_gui->attr_dlg_widget_state(hid_ctx, w->from, enable);

		/* real layer */
		if (layer->meta.bound.real != NULL)
			lid = pcb_layer_id(PCB->Data, layer->meta.bound.real);
		else
			lid = ctx->no_layer;
		RND_DAD_SET_VALUE(hid_ctx, w->layer, lng, lid);
	}
}

static void lb_dialog2data(void *hid_ctx, lb_ctx_t *ctx)
{
	int n;

	for(n = 0; n < ctx->data->LayerN; n++) {
		lb_widx_t *w = ctx->widx + n;
		pcb_layer_t *layer = ctx->data->Layer + n;

		if (layer_name_mismatch(w, layer)) {
			free((char *)layer->name);
			layer->name = rnd_strdup(ctx->attrs[w->name].val.str);
		}

		if (layer_purpose_mismatch(w, layer)) {
			const char *purp = ctx->attrs[w->purpose].val.str;
			free((char *)layer->meta.bound.purpose);
			if ((purp == NULL) || (*purp == '\0'))
				layer->meta.bound.purpose = NULL;
			else
				layer->meta.bound.purpose = rnd_strdup(purp);
		}

		layer->comb = ctx->attrs[w->comp].val.lng;
		get_ly_type(ctx->attrs[w->type].val.lng, ctx->attrs[w->side].val.lng, ctx->attrs[w->offs].val.lng, &layer->meta.bound.type, &layer->meta.bound.stack_offs);

		/* enforce some sanity rules */
		if (layer->meta.bound.type & PCB_LYT_BOUNDARY) {
			/* outline must be positive global */
			layer->comb = 0;
			layer->meta.bound.type &= ~PCB_LYT_ANYWHERE;
		}
		if (!(layer->meta.bound.type & PCB_LYT_COPPER)) {
			/* temporary: offset is useful only for copper layers */
			layer->meta.bound.stack_offs = 0;
		}
	}
}

static void lb_update_left2right(void *hid_ctx, lb_ctx_t *ctx)
{
rnd_trace("l2r!\n");
	lb_dialog2data(hid_ctx, ctx);
	if (ctx->subc != NULL) {
		if (pcb_subc_rebind(ctx->pcb, ctx->subc) > 0)
			rnd_gui->invalidate_all(rnd_gui);
	}
	else { /* buffer */
		pcb_data_binding_update(ctx->pcb, ctx->data);
		rnd_gui->invalidate_all(rnd_gui);
	}
	lb_data2dialog(hid_ctx, ctx); /* update disables */
}

static void lb_attr_chg(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	if (attr->user_data == NULL) /* do not handle widgets globally if they have data - they then have a local callback too */
		lb_update_left2right(hid_ctx, caller_data);
}

static void lb_attr_layer_chg(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	lb_ctx_t *ctx = caller_data;
	lb_widx_t *w = attr->user_data;
	int didx = w - ctx->widx;
	rnd_layer_id_t lid = attr->val.lng;
	pcb_layer_t tmply, *dstly;

	if ((lid < 0) || (lid >= PCB->Data->LayerN))
		goto skip;

	if ((didx < 0) || (didx >= ctx->data->LayerN)) {
		rnd_message(RND_MSG_ERROR, "Internal error: lb_attr_layer_chg(): invalid idx %d (%d)\n", didx, ctx->data->LayerN);
		goto skip;
	}

	/* change on the right side, target layer */
	rnd_trace("layer! %d to %d\n", didx, lid);

	memset(&tmply, 0, sizeof(tmply));
	pcb_layer_real2bound(&tmply, &PCB->Data->Layer[lid], 0);
	if (ctx->subc != NULL)
		dstly = &ctx->subc->data->Layer[didx];
	else
		dstly = &ctx->data->Layer[didx];

	dstly->meta.bound.type = tmply.meta.bound.type;
	dstly->meta.bound.stack_offs = tmply.meta.bound.stack_offs;
	free(dstly->meta.bound.purpose);
	dstly->meta.bound.purpose = tmply.meta.bound.purpose;

	lb_data2dialog(hid_ctx, ctx);

	skip:;
	lb_update_left2right(hid_ctx, ctx);
}

const char pcb_acts_LayerBinding[] = "LayerBinding(object)\nLayerBinding(buffer)\n";
const char pcb_acth_LayerBinding[] = "Change the layer binding.";
fgw_error_t pcb_act_LayerBinding(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = F_Object;
	lb_ctx_t ctx;
	int num_copper;
	rnd_hid_attr_val_t val;

	memset(&ctx, 0, sizeof(ctx));
	RND_ACT_MAY_CONVARG(1, FGW_KEYWORD, LayerBinding, op = fgw_keyword(&argv[1]));

	if (op == F_Object) {
		rnd_coord_t x, y;
		int type;
		void *ptr1, *ptr2, *ptr3;
		rnd_hid_get_coords("Click on subc to change the layer binding of", &x, &y, 0);
		type = pcb_search_screen(x, y, PCB_OBJ_SUBC, &ptr1, &ptr2, &ptr3);
		if (type != PCB_OBJ_SUBC) {
			rnd_message(RND_MSG_ERROR, "No subc under the cursor\n");
			return -1;
		}
		ctx.subc = ptr2;
		ctx.data = ctx.subc->data;
	}
	else if (op == F_Buffer) {
		ctx.data = PCB_PASTEBUFFER->Data;
	}
	else
		RND_ACT_FAIL(LayerBinding);

	{ /* interactive mode */
		int n;
		rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
		RND_DAD_DECL(dlg);

		ctx.pcb = PCB;
		ctx.widx = malloc(sizeof(lb_widx_t) * ctx.data->LayerN);
		ctx.layer_names = calloc(sizeof(char *), PCB->Data->LayerN+2);
		for(n = 0; n < PCB->Data->LayerN; n++)
			ctx.layer_names[n] = PCB->Data->Layer[n].name;
		ctx.no_layer = n;
		ctx.layer_names[n] = "invalid/unbound";
		n++;
		ctx.layer_names[n] = NULL;

		for(n = 0, num_copper = -1; n < PCB->LayerGroups.len; n++)
			if (pcb_layergrp_flags(PCB, n) & PCB_LYT_COPPER)
				num_copper++;

		RND_DAD_BEGIN_VBOX(dlg);
			RND_DAD_COMPFLAG(dlg, RND_HATF_EXPFILL);
			RND_DAD_BEGIN_TABLE(dlg, 2);
				RND_DAD_COMPFLAG(dlg, RND_HATF_SCROLL | RND_HATF_EXPFILL);
			for(n = 0; n < ctx.data->LayerN; n++) {
				lb_widx_t *w = ctx.widx+n;
				/* left side */
				RND_DAD_BEGIN_VBOX(dlg);
					if (n == 0)
						RND_DAD_LABEL(dlg, "RECIPE");
					else
						RND_DAD_LABEL(dlg, "\n");

					RND_DAD_BEGIN_HBOX(dlg);
						RND_DAD_LABEL(dlg, "Name:");
						RND_DAD_STRING(dlg);
							w->name = RND_DAD_CURRENT(dlg);
					RND_DAD_END(dlg);
					RND_DAD_BEGIN_HBOX(dlg);
						RND_DAD_ENUM(dlg, pcb_lb_comp); /* coposite */
							w->comp = RND_DAD_CURRENT(dlg);
						RND_DAD_ENUM(dlg, pcb_lb_types); /* lyt */
							w->type = RND_DAD_CURRENT(dlg);
					RND_DAD_END(dlg);
					RND_DAD_BEGIN_HBOX(dlg);
						RND_DAD_INTEGER(dlg);
							RND_DAD_MINVAL(dlg, 0);
							RND_DAD_MAXVAL(dlg, num_copper);
							w->offs = RND_DAD_CURRENT(dlg);
						RND_DAD_LABEL(dlg, "from");
							w->from = RND_DAD_CURRENT(dlg);
						RND_DAD_ENUM(dlg, pcb_lb_side);
							w->side = RND_DAD_CURRENT(dlg);
					RND_DAD_END(dlg);
					RND_DAD_BEGIN_HBOX(dlg);
						RND_DAD_LABEL(dlg, "Purpose:");
						RND_DAD_STRING(dlg);
							w->purpose = RND_DAD_CURRENT(dlg);
					RND_DAD_END(dlg);

				RND_DAD_END(dlg);

				/* right side */
				RND_DAD_BEGIN_HBOX(dlg);
					RND_DAD_LABELF(dlg, ("\n\n layer #%d ", n));
					RND_DAD_BEGIN_VBOX(dlg);
						if (n == 0)
							RND_DAD_LABEL(dlg, "BOARD LAYER");
						else
							RND_DAD_LABEL(dlg, "\n\n");
						RND_DAD_LABEL(dlg, "Automatic");
						RND_DAD_ENUM(dlg, ctx.layer_names);
							w->layer = RND_DAD_CURRENT(dlg);
							RND_DAD_CHANGE_CB(dlg, lb_attr_layer_chg);
							RND_DAD_SET_ATTR_FIELD(dlg, user_data, w);
					RND_DAD_END(dlg);
				RND_DAD_END(dlg);
			}
			RND_DAD_END(dlg);
			RND_DAD_BUTTON_CLOSES(dlg, clbtn);
		RND_DAD_END(dlg);

		ctx.attrs = dlg;

		RND_DAD_DEFSIZE(dlg, 500, 500);
		RND_DAD_NEW("layer_binding", dlg, "Layer bindings", &ctx, rnd_true, NULL);
		val.func = lb_attr_chg;
		rnd_gui->attr_dlg_property(dlg_hid_ctx, RND_HATP_GLOBAL_CALLBACK, &val);
		lb_data2dialog(dlg_hid_ctx, &ctx);

		RND_DAD_RUN(dlg);

		RND_DAD_FREE(dlg);
		free(ctx.widx);
		free(ctx.layer_names);
	}

	RND_ACT_IRES(0);
	return 0;
}
