/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "obj_subc.h"
#include "search.h"

static const char *lb_comp[] = { "+manual", "-manual", "+auto", "-auto", NULL };
static const char *lb_types[] = { "UNKNOWN", "paste", "mask", "silk", "copper", "boundary", "mech", "doc", "virtual", NULL };
static const char *lb_side[] = { "top", "bottom", NULL };

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
	pcb_hid_attribute_t *attrs;
} lb_ctx_t;

static int ly_type2enum(pcb_layer_type_t type)
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
	PCB_DAD_SET_VALUE(hid_ctx, wid, int_value, ly_type2enum(type));
}

static void get_ly_type_(int combo_type, pcb_layer_type_t *type)
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
	get_ly_type_(combo_type, type);

	if (PCB_LAYER_SIDED(*type)) {
		/* set side and offset */
		if (dlg_offs == 0) {
			if (combo_side == 0)
				*type |= PCB_LYT_TOP;
			else
				*type |= PCB_LYT_BOTTOM;
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
((ctx->attrs[w->name].default_val.str_value == NULL) || (strcmp(layer->name, ctx->attrs[w->name].default_val.str_value) != 0))

#define layer_purpose_mismatch(w, layer) \
((ctx->attrs[w->purpose].default_val.str_value == NULL) || (layer->meta.bound.purpose == NULL) || (strcmp(layer->meta.bound.purpose, ctx->attrs[w->purpose].default_val.str_value) != 0))

static void lb_data2dialog(void *hid_ctx, lb_ctx_t *ctx)
{
	int n;
	pcb_bool enable;

	for(n = 0; n < ctx->data->LayerN; n++) {
		lb_widx_t *w = ctx->widx + n;
		pcb_layer_t *layer = ctx->data->Layer + n;
		pcb_layer_id_t lid;
		int ofs;

		/* disable comp for copper and outline */
		enable = !(layer->meta.bound.type & PCB_LYT_COPPER) && !(layer->meta.bound.type & PCB_LYT_BOUNDARY);
		pcb_gui->attr_dlg_widget_state(hid_ctx, w->comp, enable);
		if (!enable)
			layer->comb = 0; /* copper and outline must be +manual */

		/* name and type */
		if (layer_name_mismatch(w, layer))
			PCB_DAD_SET_VALUE(hid_ctx, w->name, str_value, pcb_strdup(layer->name));

		if (layer_purpose_mismatch(w, layer)) {
			char *purp = layer->meta.bound.purpose;
			if (purp == NULL)
				purp = "";
			PCB_DAD_SET_VALUE(hid_ctx, w->purpose, str_value, pcb_strdup(purp));
		}

		PCB_DAD_SET_VALUE(hid_ctx, w->comp, int_value, layer->comb);

		set_ly_type(hid_ctx, w->type, layer->meta.bound.type);

		/* disable side for non-sided */
		if (PCB_LAYER_SIDED(layer->meta.bound.type)) {
			/* side & offset */
			PCB_DAD_SET_VALUE(hid_ctx, w->side, int_value, !!(layer->meta.bound.type & PCB_LYT_BOTTOM));
			pcb_gui->attr_dlg_widget_state(hid_ctx, w->side, 1);
		}
		else
			pcb_gui->attr_dlg_widget_state(hid_ctx, w->side, 0);

		ofs = layer->meta.bound.stack_offs;
		if (ofs < 0) {
			PCB_DAD_SET_VALUE(hid_ctx, w->side, int_value, 1);
			ofs = -layer->meta.bound.stack_offs;
		}
		PCB_DAD_SET_VALUE(hid_ctx, w->offs, int_value, ofs);

		/* enable offset only for copper */
		enable = (layer->meta.bound.type & PCB_LYT_COPPER);
		pcb_gui->attr_dlg_widget_state(hid_ctx, w->offs, enable);
		pcb_gui->attr_dlg_widget_state(hid_ctx, w->from, enable);

		/* real layer */
		if (layer->meta.bound.real != NULL)
			lid = pcb_layer_id(PCB->Data, layer->meta.bound.real);
		else
			lid = ctx->no_layer;
		PCB_DAD_SET_VALUE(hid_ctx, w->layer, int_value, lid);
		pcb_gui->attr_dlg_widget_state(hid_ctx, w->layer, 0);
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
			layer->name = pcb_strdup(ctx->attrs[w->name].default_val.str_value);
		}

		if (layer_purpose_mismatch(w, layer)) {
			const char *purp = ctx->attrs[w->purpose].default_val.str_value;
			free((char *)layer->meta.bound.purpose);
			if ((purp == NULL) || (*purp == '\0'))
				layer->meta.bound.purpose = NULL;
			else
				layer->meta.bound.purpose = pcb_strdup(purp);
		}

		layer->comb = ctx->attrs[w->comp].default_val.int_value;
		get_ly_type(ctx->attrs[w->type].default_val.int_value, ctx->attrs[w->side].default_val.int_value, ctx->attrs[w->offs].default_val.int_value, &layer->meta.bound.type, &layer->meta.bound.stack_offs);

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


static void lb_attr_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	lb_ctx_t *ctx = caller_data;
	lb_dialog2data(hid_ctx, ctx);
	if (ctx->subc != NULL) {
		if (pcb_subc_rebind(ctx->pcb, ctx->subc) > 0)
			pcb_gui->invalidate_all();
	}
	else { /* buffer */
		pcb_data_binding_update(ctx->pcb, ctx->data);
		pcb_gui->invalidate_all();
	}
	lb_data2dialog(hid_ctx, ctx); /* update disables */
}

static const char pcb_acts_LayerBinding[] = "LayerBinding(object)\nLayerBinding(selected)\nLayerBinding(buffer)\n";
static const char pcb_acth_LayerBinding[] = "Change the layer binding.";
static fgw_error_t pcb_act_LayerBinding(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = F_Object;
	lb_ctx_t ctx;
	int num_copper;
	pcb_hid_attr_val_t val;

	memset(&ctx, 0, sizeof(ctx));
	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, LayerBinding, op = fgw_keyword(&argv[1]));

	if (op == F_Object) {
		pcb_coord_t x, y;
		int type;
		void *ptr1, *ptr2, *ptr3;
		pcb_hid_get_coords("Click on subc to change the layer binding of", &x, &y, 0);
		type = pcb_search_screen(x, y, PCB_OBJ_SUBC, &ptr1, &ptr2, &ptr3);
		if (type != PCB_OBJ_SUBC) {
			pcb_message(PCB_MSG_ERROR, "No subc under the cursor\n");
			return -1;
		}
		ctx.subc = ptr2;
		ctx.data = ctx.subc->data;
	}
	else if (op == F_Selected) {
TODO("subc TODO")
		pcb_message(PCB_MSG_ERROR, "TODO\n");
		return 1;
	}
	else if (op == F_Buffer) {
		ctx.data = PCB_PASTEBUFFER->Data;
	}
	else
		PCB_ACT_FAIL(LayerBinding);

	{ /* interactive mode */
		int n;
		pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
		PCB_DAD_DECL(dlg);

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

		PCB_DAD_BEGIN_VBOX(dlg);
			PCB_DAD_COMPFLAG(dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_TABLE(dlg, 2);
				PCB_DAD_COMPFLAG(dlg, PCB_HATF_SCROLL);
			for(n = 0; n < ctx.data->LayerN; n++) {
				lb_widx_t *w = ctx.widx+n;
				/* left side */
				PCB_DAD_BEGIN_VBOX(dlg);
					if (n == 0)
						PCB_DAD_LABEL(dlg, "RECIPE");
					else
						PCB_DAD_LABEL(dlg, "\n");

					PCB_DAD_BEGIN_HBOX(dlg);
						PCB_DAD_LABEL(dlg, "Name:");
						PCB_DAD_STRING(dlg);
							w->name = PCB_DAD_CURRENT(dlg);
					PCB_DAD_END(dlg);
					PCB_DAD_BEGIN_HBOX(dlg);
						PCB_DAD_ENUM(dlg, lb_comp); /* coposite */
							w->comp = PCB_DAD_CURRENT(dlg);
						PCB_DAD_ENUM(dlg, lb_types); /* lyt */
							w->type = PCB_DAD_CURRENT(dlg);
					PCB_DAD_END(dlg);
					PCB_DAD_BEGIN_HBOX(dlg);
						PCB_DAD_INTEGER(dlg, NULL);
							PCB_DAD_MINVAL(dlg, 0);
							PCB_DAD_MAXVAL(dlg, num_copper);
							w->offs = PCB_DAD_CURRENT(dlg);
						PCB_DAD_LABEL(dlg, "from");
							w->from = PCB_DAD_CURRENT(dlg);
						PCB_DAD_ENUM(dlg, lb_side);
							w->side = PCB_DAD_CURRENT(dlg);
					PCB_DAD_END(dlg);
					PCB_DAD_BEGIN_HBOX(dlg);
						PCB_DAD_LABEL(dlg, "Purpose:");
						PCB_DAD_STRING(dlg);
							w->purpose = PCB_DAD_CURRENT(dlg);
					PCB_DAD_END(dlg);

				PCB_DAD_END(dlg);

				/* right side */
				PCB_DAD_BEGIN_HBOX(dlg);
					PCB_DAD_LABELF(dlg, ("\n\n layer #%d ", n));
					PCB_DAD_BEGIN_VBOX(dlg);
						if (n == 0)
							PCB_DAD_LABEL(dlg, "BOARD LAYER");
						else
							PCB_DAD_LABEL(dlg, "\n\n");
						PCB_DAD_LABEL(dlg, "Automatic");
						PCB_DAD_ENUM(dlg, ctx.layer_names);
							w->layer = PCB_DAD_CURRENT(dlg);
					PCB_DAD_END(dlg);
				PCB_DAD_END(dlg);
			}
			PCB_DAD_END(dlg);
			PCB_DAD_BUTTON_CLOSES(dlg, clbtn);
		PCB_DAD_END(dlg);

		ctx.attrs = dlg;

		PCB_DAD_NEW("layer_binding", dlg, "Layer bindings", &ctx, pcb_true, NULL);
		val.func = lb_attr_chg;
		pcb_gui->attr_dlg_property(dlg_hid_ctx, PCB_HATP_GLOBAL_CALLBACK, &val);
		lb_data2dialog(dlg_hid_ctx, &ctx);

		PCB_DAD_RUN(dlg);

		PCB_DAD_FREE(dlg);
		free(ctx.widx);
		free(ctx.layer_names);
	}

	PCB_ACT_IRES(0);
	return 0;
}
