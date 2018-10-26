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
	conf_listitem_t *i;
	int idx;
	const char *s;
	char *cell[4];
	char *cursor_path = NULL;
	pcb_hid_row_t *r;
	pcb_hid_attribute_t *attr;
	pcb_hid_attr_val_t hv;

	if ((pref_ctx.lib.lock) || (!pref_ctx.active))
		return;

	attr = &pref_ctx.dlg[pref_ctx.lib.wlist];
	r = pcb_dad_tree_get_selected(attr);
	if (r != NULL)
		cursor_path = pcb_strdup(r->cell[0]);

	conf_loop_list_str(&conf_core.rc.library_search_paths, i, s, idx) {
		cell[0] = (char *)i->payload;
		pcb_path_resolve(cell[0], &cell[1], 0);
		cell[2] = (char *)(i->prop.src->file_name == NULL ? "n/a" : i->prop.src->file_name);
		cell[3] = NULL;
		pcb_dad_tree_append(attr, NULL, cell);
	}

	hv.str_value = cursor_path;
	pcb_gui->attr_dlg_set_value(pref_ctx.dlg_hid_ctx, pref_ctx.lib.wlist, &hv);
	free(cursor_path);
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
	if (ctx->lib.help.active)
		PCB_DAD_FREE(ctx->lib.help.dlg);
}

static void pref_libhelp_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	pref_libhelp_ctx_t *ctx = caller_data;

	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(pref_libhelp_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static void pref_libhelp_open(pref_libhelp_ctx_t *ctx)
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

	ctx->active = 1;
	PCB_DAD_NEW(ctx->dlg, "pcb-rnd preferences: library help", "", ctx, pcb_false, pref_libhelp_close_cb);
}

static void libhelp_btn(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pref_ctx_t *ctx = caller_data;
	if (ctx->lib.help.active)
		return;
	pref_libhelp_open(&ctx->lib.help);
}

void pcb_dlg_pref_lib_create(pref_ctx_t *ctx)
{
	static const char *hdr[] = {"configured path", "actual path on the filesystem", "config source", NULL};

	PCB_DAD_LABEL(ctx->dlg, "Ordered list of footprint library search directories.");
	PCB_DAD_BUTTON(ctx->dlg, "Help: $(variables)");
		ctx->lib.whsbutton = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_CHANGE_CB(ctx->dlg, libhelp_btn);


	PCB_DAD_BEGIN_VBOX(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
		PCB_DAD_TREE(ctx->dlg, 3, 0, hdr);
			ctx->lib.wlist = PCB_DAD_CURRENT(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	PCB_DAD_BEGIN_HBOX(ctx->dlg);
		PCB_DAD_BUTTON(ctx->dlg, "Move up");
		PCB_DAD_BUTTON(ctx->dlg, "Move down");
		PCB_DAD_BUTTON(ctx->dlg, "Insert before");
		PCB_DAD_BUTTON(ctx->dlg, "Insert after");
		PCB_DAD_BUTTON(ctx->dlg, "Remove");
		PCB_DAD_BUTTON(ctx->dlg, "Edit...");
	PCB_DAD_END(ctx->dlg);

}

void pcb_dlg_pref_lib_open(pref_ctx_t *ctx)
{
	conf_native_t *cn = conf_get_field("rc/library_search_paths");
	pref_lib_conf2dlg(cn, -1);
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
