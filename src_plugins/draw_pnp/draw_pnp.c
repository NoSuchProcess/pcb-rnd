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
#include "draw.h"
#include "event.h"
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include "stub_draw.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include "conf_core.h"

#include "obj_text.h"
#include "obj_text_draw.h"

static const char draw_pnp_cookie[] = "draw_pnp";

static pcb_draw_info_t dinfo;
static rnd_xform_t dxform;

static pcb_text_t *dtext(rnd_coord_t x, rnd_coord_t y, int scale, rnd_font_id_t fid, const char *txt)
{
	static pcb_text_t t = {0};

	if (dinfo.xform == NULL) dinfo.xform = &dxform;

	t.X = x;
	t.Y = y;
	t.TextString = (char *)txt;
	t.Scale = scale;
	t.fid = fid;
	t.Flags = pcb_no_flags();

	pcb_text_bbox(NULL, &t);
	t.X -= (t.bbox_naked.X2 - t.bbox_naked.X1)/2;
	t.Y -= (t.bbox_naked.Y2 - t.bbox_naked.Y1)/2;

	pcb_text_draw_(&dinfo, &t, 0, 0, PCB_TXT_TINY_ACCURATE);
	return &t;
}

/* draw a line of a specific thickness */
static void dline(int x1, int y1, int x2, int y2, float thick)
{
	pcb_line_t l = {0};

	if (dinfo.xform == NULL) dinfo.xform = &dxform;

	l.Point1.X = RND_MM_TO_COORD(x1);
	l.Point1.Y = RND_MM_TO_COORD(y1);
	l.Point2.X = RND_MM_TO_COORD(x2);
	l.Point2.Y = RND_MM_TO_COORD(y2);
	l.Thickness = RND_MM_TO_COORD(thick);
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

	/* render only for subcircuits on the same side as the layer we are rendering on */
	if (pcb_subc_get_side(subc, &on_bottom) == 0) {
		if (on_bottom && !(ctx->layer_flags & PCB_LYT_BOTTOM))
			return;
		if (!on_bottom && !(ctx->layer_flags & PCB_LYT_TOP))
			return;
	}

	pcb_subc_get_origin(subc, &x, &y);


	refdes = subc->refdes;
	if ((refdes == NULL) || (*refdes == '\0'))
		refdes = "N/A";

rnd_trace("draw '%s' at %mm;%mm \n", refdes, x, y);
	dtext(x, y, 100, 0, refdes);
}

static void draw_pnp_plugin_draw(pcb_draw_info_t *info, const pcb_layer_t *layer)
{
	pcb_data_t *data = layer->parent.data;
	draw_pnp_t ctx;

	rnd_trace("draw_pnp!\n");

	ctx.info = info;
	ctx.layer = layer;
	ctx.layer_flags = pcb_layer_flags_(layer);

	rnd_render->set_color(pcb_draw_out.fgGC, &layer->meta.real.color);

	rnd_rtree_search_any(data->subc_tree, (rnd_rtree_box_t *)info->drawn_area, NULL, draw_pnp_draw_cb, &ctx, NULL);


/*	pcb_draw_layer(info, layer) */
}

RND_INLINE void draw_pnp_layer_setup(pcb_layer_t *layer)
{
	const char *name;

	name = pcb_attribute_get(&layer->Attributes, "pcb-rnd::plugin_draw");
	if (!layer->is_bound && (name != NULL) && (strcmp(name, "draw_pnp") == 0)) {
		rnd_trace("install draw_pnp!\n");
		layer->plugin_draw = draw_pnp_plugin_draw;
	}
	else {
		if (layer->plugin_draw == draw_pnp_plugin_draw) {
			layer->plugin_draw = NULL;
			rnd_trace("uninstall draw_pnp!\n");
		}
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


int pplg_check_ver_draw_pnp(int ver_needed) { return 0; }

void pplg_uninit_draw_pnp(void)
{
	rnd_event_unbind_allcookie(draw_pnp_cookie);
}

int pplg_init_draw_pnp(void)
{
	RND_API_CHK_VER;
	rnd_event_bind(PCB_EVENT_LAYER_PLUGIN_DRAW_CHANGE, draw_pnp_layer_attr_ev, NULL, draw_pnp_cookie);
	rnd_event_bind(RND_EVENT_LOAD_POST, draw_pnp_new_brd_ev, NULL, draw_pnp_cookie);
	return 0;
}
