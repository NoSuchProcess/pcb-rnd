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

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
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
	printf("pinout expose!\n");
}

static void pcb_dlg_pinout(long subc_id)
{
	char title[64];
	pinout_ctx_t *ctx = calloc(sizeof(pinout_ctx_t), 1);

	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_PREVIEW(ctx->dlg, pinout_expose, NULL, NULL, NULL);
	PCB_DAD_END(ctx->dlg);

	ctx->subc_id = subc_id;

	sprintf(title, "Subcircuit #%ld pinout", subc_id);
	PCB_DAD_NEW(ctx->dlg, title, "Pinout", ctx, pcb_false, pinout_close_cb);
}

static const char pcb_acts_Pinout[] = "Pinout()\n";
static const char pcb_acth_Pinout[] = "Present the subcircuit pinout box";
static fgw_error_t pcb_act_Pinout(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_pinout(42);
	PCB_ACT_IRES(0);
	return 0;
}
