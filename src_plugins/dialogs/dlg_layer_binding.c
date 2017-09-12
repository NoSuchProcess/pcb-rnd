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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "board.h"
#include "data.h"
#include "const.h"
#include "obj_subc.h"
#include "search.h"

static const char *lb_vals[] = { "foo", "bar", "baz", NULL };
static const char *lb_comp[] = { "+manual", "-manual", "+auto", "-auto", NULL };
static const char *lb_types[] = { "UNKNOWN", "paste", "mask", "silk", "copper", "outline", "virtual", NULL };
static const char *lb_side[] = { "top", "bottom", NULL };

typedef struct {
	int name, comp, type, offs, from, side, layer; /* widet indices */
} lb_widx_t;

typedef struct {
	const char **layer_names;
	lb_widx_t *widx;
	pcb_data_t *data;
	pcb_subc_t *subc;
	int no_layer; /* the "invalid layer" in the layer names enum */
} lb_ctx_t;

static void set_ly_type(void *hid_ctx, int wid, unsigned long int type)
{
	pcb_hid_attr_val_t val;

	val.int_value = 0;
	if (type & PCB_LYT_PASTE)        val.int_value = 1;
	else if (type & PCB_LYT_MASK)    val.int_value = 2;
	else if (type & PCB_LYT_SILK)    val.int_value = 3;
	else if (type & PCB_LYT_COPPER)  val.int_value = 4;
	else if (type & PCB_LYT_OUTLINE) val.int_value = 5;
	else if (type & PCB_LYT_VIRTUAL) val.int_value = 6;

	pcb_gui->attr_dlg_set_value(hid_ctx, wid, &val);
}

static void lb_data2dialog(void *hid_ctx, lb_ctx_t *ctx)
{
	int n;
	pcb_bool enable;

	for(n = 0; n < ctx->data->LayerN; n++) {
		lb_widx_t *w = ctx->widx + n;
		pcb_layer_t *layer = ctx->data->Layer + n;
		pcb_hid_attr_val_t val;

		/* name and type */
		val.str_value = layer->meta.bound.name;
		pcb_gui->attr_dlg_set_value(hid_ctx, w->name, &val);

		val.int_value = layer->meta.bound.comb;
		pcb_gui->attr_dlg_set_value(hid_ctx, w->comp, &val);

		set_ly_type(hid_ctx, w->type, layer->meta.bound.type);

		/* offset */
		val.int_value = layer->meta.bound.stack_offs;
		if (val.int_value < 0)
			val.int_value = -val.int_value;
		pcb_gui->attr_dlg_set_value(hid_ctx, w->offs, &val);

		val.int_value = !!(layer->meta.bound.type & PCB_LYT_BOTTOM);
		pcb_gui->attr_dlg_set_value(hid_ctx, w->side, &val);

		/* enable offset only for copper */
		enable = (layer->meta.bound.type & PCB_LYT_COPPER);
		pcb_gui->attr_dlg_widget_state(hid_ctx, w->offs, enable);
		pcb_gui->attr_dlg_widget_state(hid_ctx, w->from, enable);

		enable = !(layer->meta.bound.type & PCB_LYT_VIRTUAL);
		pcb_gui->attr_dlg_widget_state(hid_ctx, w->side, enable);

		/* real layer */
		if (layer->meta.bound.real != NULL)
			val.int_value = pcb_layer_id(PCB->Data, layer->meta.bound.real);
		else
			val.int_value = ctx->no_layer;
		pcb_gui->attr_dlg_set_value(hid_ctx, w->layer, &val);
	}
}

static void lb_attr_chg(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	lb_ctx_t *ctx = caller_data;
/*	lb_dialog2data(hid_ctx, ctx);*/
	lb_data2dialog(hid_ctx, ctx); /* update disables */
}

static const char pcb_acts_LayerBinding[] = "LayerBinding(object)\nLayerBinding(selected)\nLayerBinding(buffer)\n";
static const char pcb_acth_LayerBinding[] = "Change the layer binding.";
static int pcb_act_LayerBinding(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	lb_ctx_t ctx;
	int num_copper;
	pcb_hid_attr_val_t val;

	memset(&ctx, 0, sizeof(ctx));

	if ((argc == 0) || (pcb_strcasecmp(argv[0], "object") == 0)) {
		int type;
		void *ptr1, *ptr2, *ptr3;
		pcb_gui->get_coords("Click on object to change size of", &x, &y);
		type = pcb_search_screen(x, y, PCB_TYPE_SUBC, &ptr1, &ptr2, &ptr3);
		if (type != PCB_TYPE_SUBC) {
			pcb_message(PCB_MSG_ERROR, "No subc under the cursor\n");
			return -1;
		}
		ctx.subc = ptr2;
		ctx.data = ctx.subc->data;
	}
	else if (pcb_strcasecmp(argv[0], "selected") == 0) {
#warning subc TODO
		pcb_message(PCB_MSG_ERROR, "TODO\n");
		return 1;
	}
	else if (pcb_strcasecmp(argv[0], "buffer") == 0) {
#warning subc TODO
		pcb_message(PCB_MSG_ERROR, "TODO\n");
		return 1;
	}
	else
		PCB_ACT_FAIL(LayerBinding);

	{ /* interactive mode */
		int n;

		PCB_DAD_DECL(dlg);

		ctx.widx = malloc(sizeof(lb_widx_t) * ctx.data->LayerN);
		ctx.layer_names = calloc(sizeof(char *), PCB->Data->LayerN+2);
		for(n = 0; n < PCB->Data->LayerN; n++)
			ctx.layer_names[n] = PCB->Data->Layer[n].meta.real.name;
		ctx.no_layer = n;
		ctx.layer_names[n] = "invalid/unbound";
		n++;
		ctx.layer_names[n] = NULL;

		for(n = 0, num_copper = -1; n < PCB->LayerGroups.len; n++)
			if (pcb_layergrp_flags(PCB, n) & PCB_LYT_COPPER)
				num_copper++;

		PCB_DAD_BEGIN_TABLE(dlg, 2);
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

		PCB_DAD_NEW(dlg, "layer_binding", "Layer bindings", &ctx);
		val.func = lb_attr_chg;
		pcb_gui->attr_dlg_property(dlg_hid_ctx, PCB_HATP_GLOBAL_CALLBACK, &val);
		lb_data2dialog(dlg_hid_ctx, &ctx);

		PCB_DAD_RUN(dlg);

		PCB_DAD_FREE(dlg);
		free(ctx.widx);
		free(ctx.layer_names);
	}

	return 0;
}
