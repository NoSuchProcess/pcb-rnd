/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <genvector/gds_char.h>
#include "build_run.h"
#include "pcb-printf.h"
#include "obj_subc_parent.h"
#include "draw.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_data_t *data;
	long subc_id;
} pinout_ctx_t;

pinout_ctx_t pinout_ctx;

static void pinout_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pinout_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	free(ctx);
}


static void pinout_expose(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pinout_ctx_t *ctx = prv->user_ctx;
	void *r1, *r2, *r3;

	pcb_objtype_t type = pcb_search_obj_by_id_(ctx->data, &r1, &r2, &r3, ctx->subc_id, PCB_OBJ_SUBC);
	if (type == PCB_OBJ_SUBC) {
		pcb_subc_t *sc = r2;
		int orig_po = pcb_draw_doing_pinout;
		pcb_dad_preview_zoomto(attrib, &sc->BoundingBox);

		pcb_draw_doing_pinout = pcb_true;
		pcb_subc_draw_preview(sc, &e->view);
		pcb_draw_doing_pinout = orig_po;
	}
	else {
		char tmp[128];
		pcb_box_t bbox;
		sprintf(tmp, "Subcircuit #%ld not found.");
		bbox.X1 = bbox.Y1 = 0;
		bbox.X2 = bbox.Y2 = PCB_MM_TO_COORD(10);
		pcb_dad_preview_zoomto(attrib, &bbox);
		pcb_gui->set_color(gc, "#FF0000");
		pcb_text_draw_string_simple(NULL, tmp, PCB_MM_TO_COORD(1), PCB_MM_TO_COORD(20), 100, 0, 0, 0, 0, 0, 0);
	}
}

static pcb_bool pinout_mouse(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	if (kind == PCB_HID_MOUSE_RELEASE) {
		pinout_ctx_t *ctx = prv->user_ctx;
		void *r1, *r2, *r3;
		pcb_subc_t *sc;
		pcb_pstk_t *ps;
		pcb_coord_t ox, oy ;
		pcb_objtype_t type;

		type = pcb_search_obj_by_id_(ctx->data, &r1, &r2, &r3, ctx->subc_id, PCB_OBJ_SUBC);
		if (type != PCB_OBJ_SUBC)
			return pcb_false;
		sc = r2;

		if (pcb_subc_get_origin(sc, &ox, &oy) != 0)
			return pcb_false;
/*pcb_trace("d1b %mm+%mm %mm+%mm %mm,%mm\n", x, ox, y, oy, x + ox, y + oy);*/

		type = pcb_search_obj_by_location(PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &r1, &r2, &r3, x + ox, y + oy, 1);
		if ((type != PCB_OBJ_PSTK) || (pcb_obj_parent_subc(r2) != sc))
			return pcb_false;
		ps = r2;
	}
	
	return pcb_false;
}

static void pcb_dlg_pinout(pcb_data_t *data, pcb_subc_t *sc)
{
	char title[64];
	pinout_ctx_t *ctx = calloc(sizeof(pinout_ctx_t), 1);

	ctx->data = data;
	ctx->subc_id = sc->ID;
	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		PCB_DAD_PREVIEW(ctx->dlg, pinout_expose, pinout_mouse, NULL, ctx);
	PCB_DAD_END(ctx->dlg);

	if (sc->refdes != NULL)
		sprintf(title, "Subcircuit #%ld (%s) pinout", sc->ID, sc->refdes);
	else
		sprintf(title, "Subcircuit #%ld pinout", sc->ID);
	PCB_DAD_NEW(ctx->dlg, title, "Pinout", ctx, pcb_false, pinout_close_cb);
}

static const char pcb_acts_Pinout[] = "Pinout()\n";
static const char pcb_acth_Pinout[] = "Present the subcircuit pinout box";
static fgw_error_t pcb_act_Pinout(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	void *r1, *r2, *r3;
	pcb_objtype_t type = pcb_search_obj_by_location(PCB_OBJ_SUBC, &r1, &r2, &r3, pcb_crosshair.X, pcb_crosshair.Y, 1);
	if (type == PCB_OBJ_SUBC) {
		pcb_subc_t *sc = r2;
		pcb_dlg_pinout(PCB->Data, sc);
		PCB_ACT_IRES(0);
	}
	else
		PCB_ACT_IRES(-1);
	return 0;
}
