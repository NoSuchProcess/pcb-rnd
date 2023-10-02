/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design - font editor preview dlg
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

#include <librnd/core/config.h>
#include <librnd/hid/hid_dad.h>

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} fmprv_ctx_t;

fmprv_ctx_t fmprv_ctx;

static void fmprv_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	fmprv_ctx_t *ctx = caller_data;
	RND_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(fmprv_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void pcb_dlg_fontmode_preview(void)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};

	if (fmprv_ctx.active)
		return; /* do not open another */

	RND_DAD_BEGIN_VBOX(fmprv_ctx.dlg);
		RND_DAD_COMPFLAG(fmprv_ctx.dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(fmprv_ctx.dlg, "fmprv");
		RND_DAD_BUTTON_CLOSES(fmprv_ctx.dlg, clbtn);
	RND_DAD_END(fmprv_ctx.dlg);

	/* set up the context */
	fmprv_ctx.active = 1;

	RND_DAD_NEW("font_mode_preview", fmprv_ctx.dlg, "Font editor: preview", &fmprv_ctx, rnd_false, fmprv_close_cb);
}

const char pcb_acts_FontModePreview[] = "FontModePreview()\n";
const char pcb_acth_FontModePreview[] = "Open the font mode preview dialog that also helps editing font tables";
fgw_error_t pcb_act_FontModePreview(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_fontmode_preview();
	return 0;
}
