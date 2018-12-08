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

/* Preferences dialog, conf tree tab */

#include "dlg_pref.h"
#include "conf.h"
#include "conf_core.h"

void pcb_dlg_pref_conf_create(pref_ctx_t *ctx)
{
	static const char *hdr_intree[] = {"role", "prio", "policy", "value", NULL};

	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_HPANE(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

		/* left: tree */
		PCB_DAD_TREE(ctx->dlg, 1, 1, NULL);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			ctx->conf.wtree = PCB_DAD_CURRENT(ctx->dlg);

		/* right: details */
		PCB_DAD_BEGIN_VPANE(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			
			/* right/top: conf file */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(ctx->dlg, "name");
					ctx->conf.wname = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "desc");
					ctx->conf.wdesc = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "INPUT: configuration node (\"file\" version)");
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_TREE(ctx->dlg, 4, 0, hdr_intree); /* input state */
						ctx->conf.wintree = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_BEGIN_VBOX(ctx->dlg);
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
						PCB_DAD_LABEL(ctx->dlg, "EDIT input");
					PCB_DAD_END(ctx->dlg);
				PCB_DAD_END(ctx->dlg);
			PCB_DAD_END(ctx->dlg);

			/* right/bottom: native file */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(ctx->dlg, "NATIVE: in-memory conf node after the merge");
				PCB_DAD_TREE(ctx->dlg, 4, 0, hdr_intree); /* input state */
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
					ctx->conf.wintree = PCB_DAD_CURRENT(ctx->dlg);

			PCB_DAD_END(ctx->dlg);

		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
}
