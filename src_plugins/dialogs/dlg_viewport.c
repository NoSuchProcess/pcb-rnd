/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  Based on sch-rnd code by the same author.
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
 *    contact lead developer: http://www.repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: http://www.repo.hu/projects/pcb-rnd/contact.html
 */

#include "config.h"

#include <stdlib.h>

#include "board.h"
#include "draw.h"
#include "event.h"

#include <librnd/core/actions.h>
#include <librnd/core/rnd_conf.h>
#include <librnd/hid/hid_dad.h>



#include "dlg_viewport.h"

typedef struct viewport_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int wpreview;
	gdl_elem_t link;
} viewport_ctx_t;

static gdl_list_t viewports;
static const char dlg_viewport_cookie[] = "viewport dialog cookie";

static void viewport_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	viewport_ctx_t *ctx = caller_data;
	gdl_remove(&viewports, ctx, link);
	free(ctx);
}

static void viewport_pcb2dlg(viewport_ctx_t *ctx)
{
	rnd_dad_preview_zoomto(&ctx->dlg[ctx->wpreview], &pcb_last_main_expose_region);
}

static rnd_bool viewport_mouse(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	return rnd_false;
}

static void viewport_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, rnd_hid_expose_ctx_t *e)
{
	viewport_ctx_t *ctx = prv->user_ctx;
	rnd_xform_t xform = {0};

	/* if view box shows any off-limit area, we need to render off-limit and background - for main expose this is done implicitlyby the HID */
	if ((e->view.X1 < ctx->pcb->hidlib.dwg.X1) || (e->view.Y1 < ctx->pcb->hidlib.dwg.Y1) || (e->view.X2 > ctx->pcb->hidlib.dwg.X2) || (e->view.Y2 > ctx->pcb->hidlib.dwg.Y2)) {
		rnd_render->set_color(gc, &rnd_conf.appearance.color.off_limit);
		rnd_render->fill_rect(gc, e->view.X1, e->view.Y1, e->view.X2, e->view.Y2);

		rnd_render->set_color(gc, &rnd_conf.appearance.color.background);
		rnd_render->fill_rect(gc, ctx->pcb->hidlib.dwg.X1, ctx->pcb->hidlib.dwg.Y1, ctx->pcb->hidlib.dwg.X2, ctx->pcb->hidlib.dwg.Y2);
	}

	pcb_expose_main(rnd_gui, e, &xform);
}


static const int viewport_dlg(pcb_board_t *pcb, const char *title)
{
	viewport_ctx_t *ctx = calloc(sizeof(viewport_ctx_t), 1);
	rnd_hid_dad_buttons_t clbtn[] = {{"close", 0}, {NULL, 0}};
	char *freeme = NULL;

	ctx->pcb = pcb;
	if (title == NULL) {
		freeme = rnd_concat("ViewPort: ", pcb->hidlib.loadname, NULL);
		title = freeme;
	}

	gdl_append(&viewports, ctx, link);

	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_PREVIEW(ctx->dlg, viewport_expose, viewport_mouse, NULL, NULL, NULL, 100, 100, ctx);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_PRV_GFLIP | RND_HATF_EXPFILL);
			ctx->wpreview = RND_DAD_CURRENT(ctx->dlg);


		RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	RND_DAD_END(ctx->dlg);

	RND_DAD_DEFSIZE(ctx->dlg, 300, 300);
	RND_DAD_NEW("ViewPortDialog", ctx->dlg, title, ctx, 0, viewport_close_cb); /* type=local */

	viewport_pcb2dlg(ctx);
	free(freeme);

	return 0;
}


const char pcb_acts_ViewPortDialog[] = "ViewPortDialog([title])";
const char pcb_acth_ViewPortDialog[] = "Bring up a viewport dialog copying the current design/zoom/pan of the main window. Optionally sets window title.\n";
fgw_error_t pcb_act_ViewPortDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_design_t *hidlib = RND_ACT_DESIGN;
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	const char *title = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, ViewPortDialog, title = argv[1].val.str);

	RND_ACT_IRES(viewport_dlg(pcb, title));

	return 0;
}

static void pcb_dlg_viewport_edit(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	viewport_ctx_t *n;

	for(n = gdl_first(&viewports); n != NULL; n = n->link.next) {
		if (n->pcb == pcb) {
			rnd_dad_preview_zoomto(&n->dlg[n->wpreview], NULL);
		}
	}

}

static void pcb_dlg_viewport_preunload(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	viewport_ctx_t *n, *next;
	rnd_dad_retovr_t retovr = {0};

	for(n = gdl_first(&viewports); n != NULL; n = next) {
		next = gdl_next(&viewports, n);
/*		if (n->pcb == pcb) - we support a single pcb for now, close all dialogs */
			rnd_hid_dad_close(n->dlg_hid_ctx, &retovr, 0);
	}
}

void pcb_dlg_viewport_init(void)
{
	rnd_event_bind(PCB_EVENT_BOARD_EDITED, pcb_dlg_viewport_edit, NULL, dlg_viewport_cookie);
	rnd_event_bind(PCB_EVENT_LAYERS_CHANGED, pcb_dlg_viewport_edit, NULL, dlg_viewport_cookie);
	rnd_event_bind(PCB_EVENT_LAYER_CHANGED_GRP, pcb_dlg_viewport_edit, NULL, dlg_viewport_cookie);
	rnd_event_bind(PCB_EVENT_LAYERVIS_CHANGED, pcb_dlg_viewport_edit, NULL, dlg_viewport_cookie);
	rnd_event_bind(RND_EVENT_LOAD_PRE, pcb_dlg_viewport_preunload, NULL, dlg_viewport_cookie);
}

void pcb_dlg_viewport_uninit(void)
{
	viewport_ctx_t *n, *next;
	rnd_dad_retovr_t retovr = {0};

	rnd_event_unbind_allcookie(dlg_viewport_cookie);

	for(n = gdl_first(&viewports); n != NULL; n = next) {
		next = gdl_next(&viewports, n);
		rnd_hid_dad_close(n->dlg_hid_ctx, &retovr, 0);
	}
}
