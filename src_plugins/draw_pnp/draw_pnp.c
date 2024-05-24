/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "build_run.h"
#include "data.h"
#include "data_it.h"
#include "draw.h"
#include "event.h"
#include "undo.h"
#include "funchash_core.h"
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rnd_conf.h>
#include <librnd/core/conf_multi.h>
#include <librnd/hid/hid_menu.h>
#include "conf_core.h"

#include "obj_text.h"
#include "obj_text_draw.h"
#include "obj_line_draw.h"

#include "../src_plugins/draw_pnp/draw_pnp_conf.h"
#include "../src_plugins/draw_pnp/conf_internal.c"
#include "../src_plugins/draw_pnp/menu_internal.c"

conf_draw_pnp_t conf_draw_pnp;

static const char draw_pnp_cookie[] = "draw_pnp";

static pcb_draw_info_t dinfo;
static rnd_xform_t dxform;

static pcb_text_t *dtext(rnd_coord_t x, rnd_coord_t y, int scale, rnd_coord_t thick, rnd_font_id_t fid, const char *txt)
{
	static pcb_text_t t = {0};

	if (dinfo.xform == NULL) dinfo.xform = &dxform;

	t.X = x;
	t.Y = y;
	t.TextString = (char *)txt;
	t.Scale = scale;
	t.thickness = thick;
	t.fid = fid;
	t.Flags = pcb_no_flags();

	pcb_text_bbox(NULL, &t);
	t.X -= (t.bbox_naked.X2 - t.bbox_naked.X1)/2;
	t.Y -= (t.bbox_naked.Y2 - t.bbox_naked.Y1)/2;

	pcb_text_draw_(&dinfo, &t, 0, 0, PCB_TXT_TINY_ACCURATE);
	return &t;
}

/* draw a line of a specific thickness */
static void dline(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thick)
{
	pcb_line_t l = {0};

	if (dinfo.xform == NULL) dinfo.xform = &dxform;

	l.Point1.X = x1; l.Point1.Y = y1;
	l.Point2.X = x2; l.Point2.Y = y2;
	l.Thickness = thick;
	pcb_line_draw_(&dinfo, &l, 0);
}

typedef struct {
	pcb_draw_info_t *info;
	const pcb_layer_t *layer;
	long layer_flags;
} draw_pnp_t;

static rnd_rtree_dir_t draw_pnp_draw_cb(void *cl, void *obj, const rnd_rtree_box_t *box)
{
	pcb_subc_t *subc = (pcb_subc_t *)obj;
	draw_pnp_t *ctx = cl;
	rnd_coord_t x, y;
	const char *refdes;
	int on_bottom;
	rnd_coord_t frame_thick = conf_draw_pnp.plugins.draw_pnp.frame_thickness;
	rnd_coord_t term1_thick = conf_draw_pnp.plugins.draw_pnp.term1_diameter;
	pcb_data_it_t it;
	pcb_any_obj_t *o;

	/* render only for subcircuits on the same side as the layer we are rendering on */
	if (pcb_subc_get_side(subc, &on_bottom) == 0) {
		if (on_bottom && !(ctx->layer_flags & PCB_LYT_BOTTOM))
			return 0;
		if (!on_bottom && !(ctx->layer_flags & PCB_LYT_TOP))
			return 0;
	}

	pcb_subc_get_origin(subc, &x, &y);


	refdes = subc->refdes;
	if ((refdes == NULL) || (*refdes == '\0'))
		refdes = "N/A";

	dtext(x, y, conf_draw_pnp.plugins.draw_pnp.text.scale, conf_draw_pnp.plugins.draw_pnp.text.thickness, 0, refdes);

	/* draw frame */
	if (frame_thick > 0) {
		dline(subc->BoundingBox.X1, subc->BoundingBox.Y1, subc->BoundingBox.X2, subc->BoundingBox.Y1, frame_thick);
		dline(subc->BoundingBox.X2, subc->BoundingBox.Y1, subc->BoundingBox.X2, subc->BoundingBox.Y2, frame_thick);
		dline(subc->BoundingBox.X2, subc->BoundingBox.Y2, subc->BoundingBox.X1, subc->BoundingBox.Y2, frame_thick);
		dline(subc->BoundingBox.X1, subc->BoundingBox.Y2, subc->BoundingBox.X1, subc->BoundingBox.Y1, frame_thick);
	}

	/* place a dot at term 1, first object found */
	if (term1_thick > 0) {
		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_TERM); o != NULL; o = pcb_data_next(&it)) {
			if ((o->term != NULL) && (o->term[0] == '1') && (o->term[1] == '\0')) {
				rnd_coord_t x = (o->BoundingBox.X1 + o->BoundingBox.X2)/2;
				rnd_coord_t y = (o->BoundingBox.Y1 + o->BoundingBox.Y2)/2;
				dline(x, y, x, y, term1_thick);
				break;
			}
		}
	}

	return 0;
}

static void draw_pnp_plugin_draw(pcb_draw_info_t *info, const pcb_layer_t *layer)
{
	pcb_data_t *data = layer->parent.data;
	draw_pnp_t ctx;

	ctx.info = info;
	ctx.layer = layer;
	ctx.layer_flags = pcb_layer_flags_(layer);

	rnd_render->set_color(pcb_draw_out.fgGC, &layer->meta.real.color);

	rnd_rtree_search_any(data->subc_tree, (rnd_rtree_box_t *)info->drawn_area, NULL, draw_pnp_draw_cb, &ctx, NULL);

	/* render normal layer drawing objects as well */
	info->disable_plugin_draw = 1;
	pcb_draw_layer(info, layer);
	info->disable_plugin_draw = 0;
}

RND_INLINE void draw_pnp_layer_setup(pcb_layer_t *layer)
{
	const char *name;

	name = pcb_attribute_get(&layer->Attributes, "pcb-rnd::plugin_draw");
	if (!layer->is_bound && (name != NULL) && (strcmp(name, "draw_pnp") == 0)) {
		layer->plugin_draw = draw_pnp_plugin_draw;
	}
	else {
		if (layer->plugin_draw == draw_pnp_plugin_draw)
			layer->plugin_draw = NULL;
	}
}

/* set up draw_pnp layer hook on a layer that changed the relevant attribute */
static void draw_pnp_layer_attr_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((argc < 2) || (argv[1].type != RND_EVARG_PTR))
		return;

	draw_pnp_layer_setup(argv[1].d.p);
}

/* set up draw_pnp layer hook on all affected layers after loading a board */
static void draw_pnp_new_brd_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	long n;

	for(n = 0; n < pcb->Data->LayerN; n++)
		draw_pnp_layer_setup(pcb->Data->Layer+n);
}

static void draw_pnp_create_layer(pcb_board_t *pcb, pcb_layer_type_t loc)
{
	pcb_layergrp_t *g;
	rnd_layer_id_t lid;

	g = pcb_get_grp_new_misc(pcb);
	if (g == NULL)
		return;

	pcb_layergrp_set_purpose__(g, rnd_strdup("pnp"), 1);
	free(g->name);
	g->name = rnd_strdup(loc == PCB_LYT_TOP ? "pnp-top" : "pnp-bot");
	g->ltype = PCB_LYT_DOC | loc;
	g->vis = 1;
	g->open = 1;
	pcb_layergrp_undoable_created(g);
	pcb_attribute_put(&g->Attributes, "init-invis", "1");

	lid = pcb_layer_create(PCB, g - PCB->LayerGroups.grp, loc == PCB_LYT_TOP ? "pnp-top" : "pnp-bottom", 1);
	if (lid >= 0) {
		pcb_layer_t *ly = &pcb->Data->Layer[lid];
		ly->meta.real.vis = 1;
		pcb_attribute_put(&ly->Attributes, "pcb-rnd::plugin_draw", "draw_pnp");
		draw_pnp_layer_setup(ly);
	}
}

static const char pcb_acts_DrawPnpLayers[] = "DrawPnpLayers(create)\n";
static const char pcb_acth_DrawPnpLayers[] = "Create a new doc layers for pnp helper, one for top, one for bottom";
static fgw_error_t pcb_act_DrawPnpLayers(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	long n;
	int op1, found_top = 0, found_bottom = 0;


	RND_ACT_CONVARG(1, FGW_KEYWORD, DrawPnpLayers, op1 = fgw_keyword(&argv[1]));

	switch(op1) {
		case F_Create:
			for(n = 0; n < pcb->Data->LayerN; n++) {
				pcb_layer_t *layer = pcb->Data->Layer+n;
				const char *name = pcb_attribute_get(&layer->Attributes, "pcb-rnd::plugin_draw");
				if (!layer->is_bound && (name != NULL) && (strcmp(name, "draw_pnp") == 0)) {
					long layer_flags = pcb_layer_flags_(layer);
					if (layer_flags & PCB_LYT_TOP) found_top = 1;
					if (layer_flags & PCB_LYT_BOTTOM) found_bottom = 1;
				}
			}

			pcb_layergrp_inhibit_inc();
			pcb_undo_freeze_serial();

			if (!found_top) {
				rnd_message(RND_MSG_INFO, "Creating top side pnp doc layer\n");
				draw_pnp_create_layer(pcb, PCB_LYT_TOP);
			}
			else
				rnd_message(RND_MSG_INFO, "Not creating top side pnp doc layer: already exists\n");

			if (!found_bottom) {
				rnd_message(RND_MSG_INFO, "Creating bottom side pnp doc layer\n");
				draw_pnp_create_layer(pcb, PCB_LYT_BOTTOM);
			}
			else
				rnd_message(RND_MSG_INFO, "Not creating bottom side pnp doc layer: already exists\n");

			pcb_undo_unfreeze_serial();
			pcb_undo_inc_serial();
			pcb_layergrp_inhibit_dec();
			pcb_layergrp_notify_chg(pcb);

			break;
		default:
			rnd_message(RND_MSG_ERROR, "Invalid first argument for DrawPnpLayers()\n");
			RND_ACT_FAIL(DrawPnpLayers);
			return -1;
	}
	
	return 0;
}

static rnd_action_t draw_pnp_action_list[] = {
	{"drawpnplayers", pcb_act_DrawPnpLayers, pcb_acth_DrawPnpLayers, pcb_acts_DrawPnpLayers}
};


int pplg_check_ver_draw_pnp(int ver_needed) { return 0; }

void pplg_uninit_draw_pnp(void)
{
	TODO("remove layer bindings");
	rnd_event_unbind_allcookie(draw_pnp_cookie);
	rnd_remove_actions_by_cookie(draw_pnp_cookie);
	rnd_conf_plug_unreg("plugins/draw_pnp/", draw_pnp_conf_internal, draw_pnp_cookie);
	rnd_hid_menu_unload(rnd_gui, draw_pnp_cookie);
}

int pplg_init_draw_pnp(void)
{
	RND_API_CHK_VER;

	rnd_conf_plug_reg(conf_draw_pnp, draw_pnp_conf_internal, draw_pnp_cookie);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_draw_pnp, field,isarray,type_name,cpath,cname,desc,flags);
#include "draw_pnp_conf_fields.h"

	rnd_event_bind(PCB_EVENT_LAYER_PLUGIN_DRAW_CHANGE, draw_pnp_layer_attr_ev, NULL, draw_pnp_cookie);
	rnd_event_bind(RND_EVENT_LOAD_POST, draw_pnp_new_brd_ev, NULL, draw_pnp_cookie);

	RND_REGISTER_ACTIONS(draw_pnp_action_list, draw_pnp_cookie);

	rnd_hid_menu_load(rnd_gui, NULL, draw_pnp_cookie, 100, NULL, 0, draw_pnp_menu, "plugin: draw_pnp");

	return 0;
}
