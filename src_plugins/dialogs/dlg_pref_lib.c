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

/* Preferences dialog, library tab */

#include "dlg_pref.h"
#include "conf.h"
#include "conf_core.h"

/* Current libraries from config to dialog box */
static void pref_lib_conf2dlg(conf_native_t *cfg, int arr_idx)
{
	if ((pref_ctx.lib.lock) || (!pref_ctx.active))
		return;
#warning TODO
}

/* Dialog box to current libraries in config */
static void pref_lib_dlg2conf(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;

	ctx->lib.lock++;
#warning TODO
	ctx->lib.lock--;
}

void pcb_dlg_pref_lib_close(pref_ctx_t *ctx)
{
}

static void pref_lib_help(pref_ctx_t *ctx)
{
	htsp_entry_t *e;

	PCB_DAD_LABEL(ctx->dlg, "The following $(variables) can be used in the path:");
	PCB_DAD_BEGIN_TABLE(ctx->dlg, 2);
		conf_fields_foreach(e) {
			conf_native_t *nat = e->value;
			char tmp[256];

			if (strncmp(e->key, "rc/path/", 8) != 0)
				continue;

			pcb_snprintf(tmp, sizeof(tmp), "$(rc.path.%s)", e->key + 8);
			PCB_DAD_LABEL(ctx->dlg, tmp);
			PCB_DAD_LABEL(ctx->dlg, nat->val.string[0]);
		}
	PCB_DAD_END(ctx->dlg);
}

void pcb_dlg_pref_lib_create(pref_ctx_t *ctx)
{
	PCB_DAD_LABEL(ctx->dlg, "Ordered list of footprint library search directories.");
	PCB_DAD_BUTTON(ctx->dlg, "Help: $(variables) hints");
		ctx->lib.whsbutton = PCB_DAD_CURRENT(ctx->dlg);
}

void pcb_dlg_pref_lib_init(pref_ctx_t *ctx)
{
	static conf_hid_callbacks_t cbs_spth;
	conf_native_t *cn = conf_get_field("rc/library_search_paths");

	if (cn != NULL) {
		memset(&cbs_spth, 0, sizeof(conf_hid_callbacks_t));
		cbs_spth.val_change_post = pref_lib_conf2dlg;
		conf_hid_set_cb(cn, pref_hid, &cbs_spth);
	}
}
