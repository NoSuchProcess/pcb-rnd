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

#include "config.h"

#include <genvector/gds_char.h>
#include "actions.h"
#include "build_run.h"
#include "file_loaded.h"
#include "hid_dad.h"
#include "pcb-printf.h"
#include "dlg_about.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} about_ctx_t;

about_ctx_t about_ctx;

static void about_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	about_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(about_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}


static void pcb_dlg_about(void)
{
	const char *tabs[] = { "About pcb-rnd", "Options", "Paths", "License", NULL };
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	htsp_entry_t *e;
	gds_t s;

	if (about_ctx.active)
		return;

	PCB_DAD_BEGIN_VBOX(about_ctx.dlg);
		PCB_DAD_COMPFLAG(about_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_TABBED(about_ctx.dlg, tabs);
			PCB_DAD_COMPFLAG(about_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_VBOX(about_ctx.dlg);
				PCB_DAD_LABEL(about_ctx.dlg, pcb_get_info_program());
				PCB_DAD_LABEL(about_ctx.dlg, pcb_get_info_copyright());
				PCB_DAD_LABEL(about_ctx.dlg, pcb_get_info_websites(NULL));
			PCB_DAD_END(about_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(about_ctx.dlg);
				PCB_DAD_COMPFLAG(about_ctx.dlg, /*PCB_HATF_SCROLL |*/ /*PCB_HATF_FRAME |*/ PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(about_ctx.dlg, pcb_get_info_compile_options());
					PCB_DAD_COMPFLAG(about_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(about_ctx.dlg);


			PCB_DAD_BEGIN_VBOX(about_ctx.dlg);
				PCB_DAD_COMPFLAG(about_ctx.dlg, PCB_HATF_SCROLL);
				gds_init(&s);
				for (e = htsp_first(&pcb_file_loaded); e; e = htsp_next(&pcb_file_loaded, e)) {
					htsp_entry_t *e2;
					pcb_file_loaded_t *cat = e->value;
					PCB_DAD_LABEL(about_ctx.dlg, cat->name);
					gds_truncate(&s, 0);
					for (e2 = htsp_first(&cat->data.category.children); e2; e2 = htsp_next(&cat->data.category.children, e2)) {
						pcb_file_loaded_t *file = e2->value;
						pcb_append_printf(&s, "  %s\t%s\t%s\n", file->name, file->data.file.path, file->data.file.desc);
					}
					PCB_DAD_LABEL(about_ctx.dlg, s.array);
				}
				gds_uninit(&s);
			PCB_DAD_END(about_ctx.dlg);

			PCB_DAD_BEGIN_VBOX(about_ctx.dlg);
				PCB_DAD_LABEL(about_ctx.dlg, pcb_get_info_license());
			PCB_DAD_END(about_ctx.dlg);

		PCB_DAD_END(about_ctx.dlg);
		PCB_DAD_BUTTON_CLOSES(about_ctx.dlg, clbtn);
	PCB_DAD_END(about_ctx.dlg);

	/* set up the context */
	about_ctx.active = 1;

	/* this is the modal version - please consider using the non-modal version */
	PCB_DAD_NEW("about", about_ctx.dlg, "About pcb-rnd", &about_ctx, pcb_false, about_close_cb);
}

const char pcb_acts_About[] = "About()\n";
const char pcb_acth_About[] = "Present the about box";
fgw_error_t pcb_act_About(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dlg_about();
	PCB_ACT_IRES(0);
	return 0;
}
