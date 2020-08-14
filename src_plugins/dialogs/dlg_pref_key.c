/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/* Preferences dialog, key translation tab */

#include "dlg_pref.h"
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>

static void pref_key_brd2dlg(pref_ctx_t *ctx)
{
}

void pcb_dlg_pref_key_open(pref_ctx_t *ctx)
{
	pref_key_brd2dlg(ctx);
}

void pcb_dlg_pref_key_close(pref_ctx_t *ctx)
{
}


void pcb_dlg_pref_key_create(pref_ctx_t *ctx)
{
	RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(ctx->dlg, "TODO");
}
