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

/* #included from import_sch.c to share global static variables */

typedef struct{
	RND_DAD_DECL_NOINIT(dlg)
	char **inames;
	int len;
	int wfmt, wtab, warg_ctrl, wverbose;
	int warg[MAX_ARGS], warg_box[MAX_ARGS], warg_button[MAX_ARGS];
	rnd_hidval_t timer;
	int arg_dirty;
	int active; /* already open - allow only one instance */
} isch_ctx_t;

static isch_ctx_t isch_ctx;
static int isch_conf_lock = 0;

static void isch_arg2pcb(void)
{
	int n;
	rnd_conf_listitem_t *ci;

	isch_conf_lock++;
	restart:;
	for(n = 0, ci = rnd_conflist_first((rnd_conflist_t *)&conf_import_sch.plugins.import_sch.args); ci != NULL; ci = rnd_conflist_next(ci), n++) {
		const char *newval = isch_ctx.dlg[isch_ctx.warg[n]].val.str;
		if (newval == NULL)
			newval = "";
		if (strcmp(ci->val.string[0], newval) != 0) {
			rnd_conf_set(RND_CFR_DESIGN, "plugins/import_sch/args", n, newval, RND_POL_OVERWRITE);
			goto restart; /* elements may be deleted and added with different pointers... */
		}
	}
	isch_conf_lock--;

	isch_ctx.arg_dirty = 0;
}

static void isch_timed_update_cb(rnd_hidval_t user_data)
{
	isch_arg2pcb();
}

static void isch_flush_timer(void)
{
	if (isch_ctx.arg_dirty) {
		rnd_gui->stop_timer(rnd_gui, isch_ctx.timer);
		isch_arg2pcb();
	}
}

static void isch_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	int n;
	isch_ctx_t *ctx = caller_data;

	isch_flush_timer();
	RND_DAD_FREE(ctx->dlg);
	for(n = 0; n < isch_ctx.len; n++)
		free(isch_ctx.inames[n]);
	free(isch_ctx.inames);
	memset(ctx, 0, sizeof(isch_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static int isch_cmp(const void *a_, const void *b_)
{
	pcb_plug_import_t * const *a = a_, * const *b = b_;

	if ((*a)->ui_prio < (*b)->ui_prio)
		return 1;
	if ((*a)->ui_prio > (*b)->ui_prio)
		return -1;
	return strcmp((*a)->name, (*b)->name) > 0 ? 1 : -1;
}

static void isch_switch_fmt(int target, int setconf)
{
	const pcb_plug_import_t *p = pcb_lookup_importer(isch_ctx.inames[target]);
	int len, n, controllable;

	isch_conf_lock++;
	RND_DAD_SET_VALUE(isch_ctx.dlg_hid_ctx, isch_ctx.wtab, lng, target);
	if (setconf && (p != NULL))
		rnd_conf_set(RND_CFR_DESIGN, "plugins/import_sch/import_fmt", 0, p->name, RND_POL_OVERWRITE);

	if (p == NULL) {
		len = 0;
		controllable = 0;
	}
	else if (p->single_arg) {
		len = rnd_conflist_length((rnd_conflist_t *)&conf_import_sch.plugins.import_sch.args);
		if (len < 1) {
			rnd_conf_grow("plugins/import_sch/args", 1);
			rnd_conf_set(RND_CFR_DESIGN, "plugins/import_sch/args", 0, "", RND_POL_OVERWRITE);
		}
		len = 1;
		controllable = 0;
	}
	else {
		len = rnd_conflist_length((rnd_conflist_t *)&conf_import_sch.plugins.import_sch.args);
		controllable = 1;
	}


	for(n = 0; n < MAX_ARGS; n++) {
		rnd_gui->attr_dlg_widget_hide(isch_ctx.dlg_hid_ctx, isch_ctx.warg_box[n], n >= len);
		rnd_gui->attr_dlg_widget_hide(isch_ctx.dlg_hid_ctx, isch_ctx.warg_button[n], !((p != NULL) && (p->all_filenames)));
	}

	rnd_gui->attr_dlg_widget_hide(isch_ctx.dlg_hid_ctx, isch_ctx.warg_ctrl, !controllable);
	isch_conf_lock--;
}


static void isch_pcb2dlg(void)
{
	int n, tab = 0;
	rnd_conf_listitem_t *ci;
	const char *tname = conf_import_sch.plugins.import_sch.import_fmt;

	isch_flush_timer();

	if (tname != NULL) {
		for(n = 0; n < isch_ctx.len; n++) {
			if (rnd_strcasecmp(isch_ctx.inames[n], tname) == 0) {
				tab = n;
				break;
			}
		}
	}

	for(n = 0, ci = rnd_conflist_first((rnd_conflist_t *)&conf_import_sch.plugins.import_sch.args); ci != NULL; ci = rnd_conflist_next(ci), n++)
		RND_DAD_SET_VALUE(isch_ctx.dlg_hid_ctx, isch_ctx.warg[n], str, ci->val.string[0]);

	RND_DAD_SET_VALUE(isch_ctx.dlg_hid_ctx, isch_ctx.wfmt, lng, tab);
	RND_DAD_SET_VALUE(isch_ctx.dlg_hid_ctx, isch_ctx.wverbose, lng, conf_import_sch.plugins.import_sch.verbose);
	isch_switch_fmt(tab, 0);
}

static void isch_fmt_chg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	isch_switch_fmt(isch_ctx.dlg[isch_ctx.wfmt].val.lng, 1);
}

static void isch_arg_del_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int len = rnd_conflist_length((rnd_conflist_t *)&conf_import_sch.plugins.import_sch.args);
	if (len > 0) {
		rnd_conf_del(RND_CFR_DESIGN, "plugins/import_sch/args", len-1);
		isch_pcb2dlg();
	}
}

static void isch_arg_add_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int len = rnd_conflist_length((rnd_conflist_t *)&conf_import_sch.plugins.import_sch.args);
	if (len < MAX_ARGS+1) {
		isch_conf_lock++;
		rnd_conf_grow("plugins/import_sch/args", len+1);
		rnd_conf_set(RND_CFR_DESIGN, "plugins/import_sch/args", len, "", RND_POL_OVERWRITE);
		isch_pcb2dlg();
		isch_conf_lock--;
	}
}

static void isch_browse_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	int n, idx = -1, wid = attr - isch_ctx.dlg;
	char *name;
	static char cwd[RND_PATH_MAX + 1];

	for(n = 0; n < MAX_ARGS; n++) {
		if (isch_ctx.warg_button[n] == wid) {
			idx = n;
			break;
		}
	}
	if (idx < 0)
		return;

	if (*cwd == '\0')
		rnd_get_wd(cwd);

	name = rnd_gui->fileselect(rnd_gui, "Import schematics", "Import netlist and footprints from schematics", cwd, NULL, NULL, "schematics", RND_HID_FSD_MAY_NOT_EXIST, NULL);
	if (name == NULL)
		return;

	isch_conf_lock++;
	rnd_conf_set(RND_CFR_DESIGN, "plugins/import_sch/args", idx, name, RND_POL_OVERWRITE);
	isch_pcb2dlg();
	free(name);
	isch_conf_lock--;
}


static void isch_import_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	isch_flush_timer();
	do_import();
}

static void isch_arg_chg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_hidval_t user_data;

	if (isch_ctx.arg_dirty)
		rnd_gui->stop_timer(rnd_gui, isch_ctx.timer);

	user_data.ptr = NULL;
	isch_ctx.timer = rnd_gui->add_timer(rnd_gui, isch_timed_update_cb, 1000, user_data);
	isch_ctx.arg_dirty = 1;
}

static void isch_generic_chg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	const char *nv = isch_ctx.dlg[isch_ctx.wverbose].val.lng ? "1" : "0";
	isch_conf_lock++;
	rnd_conf_set(RND_CFR_DESIGN, "plugins/import_sch/verbose", 0, nv, RND_POL_OVERWRITE);
	isch_conf_lock--;
}

static void isch_plc_cfg_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(&PCB->hidlib, "preferences", "config", "footprint_", NULL);
}


static void isch_add_tab(const pcb_plug_import_t *p)
{
	RND_DAD_BEGIN_VBOX(isch_ctx.dlg);
		RND_DAD_LABEL(isch_ctx.dlg, p->desc);
	RND_DAD_END(isch_ctx.dlg);
}

static int do_dialog(void)
{
	int len, n;
	const pcb_plug_import_t *p, **pa;
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	if (isch_ctx.active)
		return 0; /* do not open another */

	len = 0;
	for(p = pcb_plug_import_chain; p != NULL; p = p->next) len++;

	isch_ctx.arg_dirty = 0;
	isch_ctx.inames = malloc((len+1) * sizeof(char *));
	pa = malloc(len * sizeof(pcb_plug_import_t *));

	for(n = 0, p = pcb_plug_import_chain; p != NULL; p = p->next, n++)
		pa[n] = p;

	qsort(pa, len, sizeof(pcb_plug_import_t *), isch_cmp);

	for(n = 0; n < len; n++)
		isch_ctx.inames[n] = rnd_strdup(pa[n]->name);
	isch_ctx.inames[n] = NULL;
	isch_ctx.len = len;

	RND_DAD_BEGIN_VBOX(isch_ctx.dlg);
		RND_DAD_COMPFLAG(isch_ctx.dlg, RND_HATF_EXPFILL);

		RND_DAD_BEGIN_VBOX(isch_ctx.dlg); /* format box */
			RND_DAD_COMPFLAG(isch_ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_BEGIN_HBOX(isch_ctx.dlg);
				RND_DAD_LABEL(isch_ctx.dlg, "Format:");
				RND_DAD_ENUM(isch_ctx.dlg, isch_ctx.inames);
					isch_ctx.wfmt = RND_DAD_CURRENT(isch_ctx.dlg);
					RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_fmt_chg_cb);
					RND_DAD_HELP(isch_ctx.dlg, "Import file format (or plugin)");
			RND_DAD_END(isch_ctx.dlg);
			RND_DAD_BEGIN_TABBED(isch_ctx.dlg, isch_ctx.inames);
				RND_DAD_COMPFLAG(isch_ctx.dlg, RND_HATF_HIDE_TABLAB);
				isch_ctx.wtab = RND_DAD_CURRENT(isch_ctx.dlg);
				for(n = 0; n < len; n++)
					isch_add_tab(pa[n]);
			RND_DAD_END(isch_ctx.dlg);
			RND_DAD_BEGIN_VBOX(isch_ctx.dlg); /* scrollable arg list */
				RND_DAD_COMPFLAG(isch_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
				for(n = 0; n < MAX_ARGS; n++) {
					RND_DAD_BEGIN_HBOX(isch_ctx.dlg);
						isch_ctx.warg_box[n] = RND_DAD_CURRENT(isch_ctx.dlg);
						RND_DAD_STRING(isch_ctx.dlg);
							isch_ctx.warg[n] = RND_DAD_CURRENT(isch_ctx.dlg);
							RND_DAD_WIDTH_CHR(isch_ctx.dlg, 32);
							RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_arg_chg_cb);
						RND_DAD_BUTTON(isch_ctx.dlg, "browse");
							RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_browse_cb);
							isch_ctx.warg_button[n] = RND_DAD_CURRENT(isch_ctx.dlg);
					RND_DAD_END(isch_ctx.dlg);
				}
				RND_DAD_BEGIN_HBOX(isch_ctx.dlg);
					isch_ctx.warg_ctrl = RND_DAD_CURRENT(isch_ctx.dlg);
					RND_DAD_BUTTON(isch_ctx.dlg, "Del last");
						RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_arg_del_cb);
						RND_DAD_HELP(isch_ctx.dlg, "Remove the last option from the end of the list");
					RND_DAD_BUTTON(isch_ctx.dlg, "One more");
						RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_arg_add_cb);
						RND_DAD_HELP(isch_ctx.dlg, "Append one more option to the end of the list");
				RND_DAD_END(isch_ctx.dlg);
			RND_DAD_END(isch_ctx.dlg);
		RND_DAD_END(isch_ctx.dlg);

		RND_DAD_BEGIN_VBOX(isch_ctx.dlg); /* generic settings */
			RND_DAD_BEGIN_HBOX(isch_ctx.dlg);
				RND_DAD_LABEL(isch_ctx.dlg, "Verbose import:");
				RND_DAD_BOOL(isch_ctx.dlg);
					isch_ctx.wverbose = RND_DAD_CURRENT(isch_ctx.dlg);
					RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_generic_chg_cb);
			RND_DAD_END(isch_ctx.dlg);
			RND_DAD_BUTTON(isch_ctx.dlg, "Placement config...");
				RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_plc_cfg_cb);
				RND_DAD_HELP(isch_ctx.dlg, "Configure how/where newly imported subcircuits\nare placed on the board");
		RND_DAD_END(isch_ctx.dlg);

		RND_DAD_BEGIN_HBOX(isch_ctx.dlg); /* bottom buttons */
			RND_DAD_BUTTON(isch_ctx.dlg, "Import!");
				RND_DAD_CHANGE_CB(isch_ctx.dlg, isch_import_cb);
			RND_DAD_BEGIN_HBOX(isch_ctx.dlg);
				RND_DAD_COMPFLAG(isch_ctx.dlg, RND_HATF_EXPFILL);
			RND_DAD_END(isch_ctx.dlg);
			RND_DAD_BUTTON_CLOSES(isch_ctx.dlg, clbtn);
		RND_DAD_END(isch_ctx.dlg);
	RND_DAD_END(isch_ctx.dlg);

	free(pa);

	/* set up the context */
	isch_ctx.active = 1;

	RND_DAD_DEFSIZE(isch_ctx.dlg, 360, 400);
	RND_DAD_NEW("import_sch", isch_ctx.dlg, "Import schematics/netlist", &isch_ctx, rnd_false, isch_close_cb);
	isch_pcb2dlg();
	isch_switch_fmt(isch_ctx.dlg[isch_ctx.wfmt].val.lng, 1);

	return 0;
}

static void isch_cfg_chg(rnd_conf_native_t *cfg, int arr_idx)
{
	if ((isch_conf_lock == 0) && isch_ctx.active)
		isch_pcb2dlg();
}

static rnd_conf_hid_id_t cfgid;

static void isch_dlg_uninit(void)
{
	rnd_conf_hid_unreg(import_sch_cookie);
}

static void isch_dlg_init(void)
{
	static rnd_conf_hid_callbacks_t cbs;
	cfgid = rnd_conf_hid_reg(import_sch_cookie, NULL);

	cbs.val_change_post = isch_cfg_chg;

	rnd_conf_hid_set_cb(rnd_conf_get_field("plugins/import_sch/args"), cfgid, &cbs);
	rnd_conf_hid_set_cb(rnd_conf_get_field("plugins/import_sch/import_fmt"), cfgid, &cbs);
	rnd_conf_hid_set_cb(rnd_conf_get_field("plugins/import_sch/verbose"), cfgid, &cbs);
}
