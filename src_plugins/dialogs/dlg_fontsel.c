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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "board.h"
#include "stub_draw.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	pcb_board_t *pcb;
	int active;
} fontsel_ctx_t;

fontsel_ctx_t fontsel_ctx;

static void fontsel_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	fontsel_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(fontsel_ctx_t));
}

void fontsel_expose_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_stub_draw_fontsel(gc, e);
}

pcb_bool fontsel_mouse_cb(pcb_hid_attribute_t *attrib, pcb_hid_preview_t *prv, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_stub_draw_fontsel_mouse_ev(NULL, NULL, kind, x, y);
}

void fontsel_free_cb(pcb_hid_attribute_t *attrib, void *user_ctx, void *hid_ctx)
{
}

static void pcb_dlg_foo(pcb_board_t *pcb)
{
	pcb_box_t vbox = {0, 0, PCB_MM_TO_COORD(55), PCB_MM_TO_COORD(55)};
	if (fontsel_ctx.active)
		return; /* do not open another */

	fontsel_ctx.pcb = pcb;
	PCB_DAD_BEGIN_VBOX(fontsel_ctx.dlg);
		PCB_DAD_COMPFLAG(fontsel_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_PREVIEW(fontsel_ctx.dlg, fontsel_expose_cb, fontsel_mouse_cb, fontsel_free_cb, &vbox, 200, 200, &fontsel_ctx);
			PCB_DAD_COMPFLAG(fontsel_ctx.dlg, PCB_HATF_EXPFILL);
	PCB_DAD_END(fontsel_ctx.dlg);

	fontsel_ctx.active = 1;
	PCB_DAD_NEW(fontsel_ctx.dlg, "Font selection", "", &fontsel_ctx, pcb_false, fontsel_close_cb);
}

static const char pcb_acts_Fontsel[] = "Fontsel2()\n";
static const char pcb_acth_Fontsel[] = "Open the font selection dialog";
static fgw_error_t pcb_act_Fontsel(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	pcb_dlg_foo(PCB);
	return 0;
}
