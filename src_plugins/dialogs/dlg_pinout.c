/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include <genvector/gds_char.h>
#include "build_run.h"
#include <librnd/core/rnd_printf.h>
#include "obj_subc_parent.h"
#include "draw.h"
#include "obj_term.h"
#include <librnd/poly/rtree.h>
#include "search.h"
#include "search_r.h"
#include "netlist.h"

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb; /* for netlist lookups */
	pcb_data_t *data;
	long subc_id;

	int w_lab_num, w_lab_name, w_lab_net;

	pcb_subc_t *tempsc; /* non-persistent, should be used only within the scope of a callback, recirsively down */
} pinout_ctx_t;

pinout_ctx_t pinout_ctx;

static void pinout_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	pinout_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	free(ctx);
}


static void pinout_expose(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_gc_t gc, const rnd_hid_expose_ctx_t *e)
{
	pinout_ctx_t *ctx = prv->user_ctx;
	void *r1, *r2, *r3;

	pcb_objtype_t type = pcb_search_obj_by_id_(ctx->data, &r1, &r2, &r3, ctx->subc_id, PCB_OBJ_SUBC);
	if (type == PCB_OBJ_SUBC) {
		pcb_subc_t *sc = r2;
		int orig_po = pcb_draw_force_termlab;
		pcb_draw_force_termlab = rnd_true;
		pcb_subc_draw_preview(sc, &e->view);
		pcb_draw_force_termlab = orig_po;
	}
	else {
		char tmp[128];
		rnd_box_t bbox;
		sprintf(tmp, "Subcircuit #%ld not found.", ctx->subc_id);
		bbox.X1 = bbox.Y1 = 0;
		bbox.X2 = bbox.Y2 = RND_MM_TO_COORD(10);
		rnd_dad_preview_zoomto(attrib, &bbox);
		rnd_render->set_color(gc, rnd_color_red);
		pcb_text_draw_string_simple(NULL, tmp, RND_MM_TO_COORD(1), RND_MM_TO_COORD(20), 1.0, 1.0, 0, 0, 0, 0, 0, 0);
	}
}

static rnd_r_dir_t pinout_mouse_search_cb(void *closure, pcb_any_obj_t *obj, void *box)
{
	pinout_ctx_t *ctx = closure;
	rnd_hid_attr_val_t val;

	if ((obj->term != NULL) && (pcb_obj_parent_subc(obj) == ctx->tempsc) && (obj->term != NULL)) {
		val.str = obj->term;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->w_lab_num, &val);
		val.str = pcb_attribute_get(&obj->Attributes, "name");
		if (val.str != NULL)
			rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->w_lab_name, &val);
		if (ctx->pcb != NULL) {
			{
				pcb_net_term_t *term = pcb_net_find_by_obj(&ctx->pcb->netlist[PCB_NETLIST_EDITED], obj);
				if (term != NULL) {
					val.str = term->parent.net->name;
					rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->w_lab_net, &val);
				}
			}
		}
	}
	return RND_R_DIR_NOT_FOUND;
}

static rnd_bool pinout_mouse(rnd_hid_attribute_t *attrib, rnd_hid_preview_t *prv, rnd_hid_mouse_ev_t kind, rnd_coord_t x, rnd_coord_t y)
{
	if (kind == RND_HID_MOUSE_RELEASE) {
		pinout_ctx_t *ctx = prv->user_ctx;
		void *r1, *r2, *r3;
		pcb_objtype_t type;
		rnd_box_t b;
		rnd_hid_attr_val_t val;

		val.str = "n/a";
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->w_lab_num, &val);
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->w_lab_name, &val);
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->w_lab_net, &val);

		type = pcb_search_obj_by_id_(ctx->data, &r1, &r2, &r3, ctx->subc_id, PCB_OBJ_SUBC);
		if (type != PCB_OBJ_SUBC)
			return rnd_false;
		ctx->tempsc = r2;

		b.X1 = x;
		b.Y1 = y;
		b.X2 = x+1;
		b.Y2 = y+1;
		pcb_search_data_by_loc(ctx->data, PCB_TERM_OBJ_TYPES, &b, pinout_mouse_search_cb, ctx);
		ctx->tempsc = NULL;
	}
	
	return rnd_false;
}

static void pcb_dlg_pinout(pcb_board_t *pcb, pcb_data_t *data, pcb_subc_t *sc)
{
	char title[64];
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	pinout_ctx_t *ctx = calloc(sizeof(pinout_ctx_t), 1);

	ctx->pcb = pcb;
	ctx->data = data;
	ctx->subc_id = sc->ID;
	RND_DAD_BEGIN_VBOX(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_PREVIEW(ctx->dlg, pinout_expose, pinout_mouse, NULL, NULL, &sc->BoundingBox, 200, 200, ctx);
		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Term ID:");
			RND_DAD_LABEL(ctx->dlg, "");
				ctx->w_lab_num = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Term name:");
			RND_DAD_LABEL(ctx->dlg, "");
				ctx->w_lab_name = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		RND_DAD_BEGIN_HBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Net:");
			RND_DAD_LABEL(ctx->dlg, "");
				ctx->w_lab_net = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BUTTON_CLOSES(ctx->dlg, clbtn);
	RND_DAD_END(ctx->dlg);

	if (sc->refdes != NULL)
		sprintf(title, "Subcircuit #%ld (%s) pinout", sc->ID, sc->refdes);
	else
		sprintf(title, "Subcircuit #%ld pinout", sc->ID);
	RND_DAD_NEW("pinout", ctx->dlg, title, ctx, rnd_false, pinout_close_cb);
}

static const char pcb_acts_Pinout[] = "Pinout()\n";
static const char pcb_acth_Pinout[] = "Present the subcircuit pinout box";
static fgw_error_t pcb_act_Pinout(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	void *r1, *r2, *r3;
	rnd_coord_t x, y;
	pcb_objtype_t type;

	rnd_hid_get_coords("Click on a subcircuit", &x, &y, 0);
	type = pcb_search_obj_by_location(PCB_OBJ_SUBC, &r1, &r2, &r3, x, y, 1);
	if (type == PCB_OBJ_SUBC) {
		pcb_subc_t *sc = r2;
		pcb_dlg_pinout(PCB, PCB->Data, sc);
		RND_ACT_IRES(0);
	}
	else {
		rnd_message(RND_MSG_ERROR, "pinout dialog: there's no subcircuit there\n");
		RND_ACT_IRES(-1);
	}
	return 0;
}
