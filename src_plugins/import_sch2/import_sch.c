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

#include "config.h"

#include <librnd/core/error.h>
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/hid_dad.h>

#include "plug_import.h"
#include "import_sch_conf.h"

#define MAX_ARGS 16

conf_import_sch_t conf_import_sch;

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	char **inames;
	int len;
	int wfmt, wtab, warg_ctrl;
	int warg[MAX_ARGS], warg_box[MAX_ARGS], warg_button[MAX_ARGS];
	pcb_hidval_t timer;
	int arg_dirty;
	int active; /* already open - allow only one instance */
} isch_ctx_t;

static isch_ctx_t isch_ctx;

static void isch_arg2pcb(void)
{
	int n;
	pcb_conf_listitem_t *ci;

	restart:;
	for(n = 0, ci = pcb_conflist_first(&conf_import_sch.plugins.import_sch.args); ci != NULL; ci = pcb_conflist_next(ci), n++) {
		const char *newval = isch_ctx.dlg[isch_ctx.warg[n]].val.str;
		if (newval == NULL)
			newval = "";
		if (strcmp(ci->val.string[0], newval) != 0) {
			pcb_conf_set(CFR_DESIGN, "plugins/import_sch/args", n, newval, POL_OVERWRITE);
			goto restart; /* elements may be deleted and added with different pointers... */
		}
	}

	isch_ctx.arg_dirty = 0;
}

static void isch_timed_update_cb(pcb_hidval_t user_data)
{
	isch_arg2pcb();
}

static void isch_flush_timer(void)
{
	if (isch_ctx.arg_dirty) {
		pcb_gui->stop_timer(pcb_gui, isch_ctx.timer);
		isch_arg2pcb();
	}
}

static void isch_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	int n;
	isch_ctx_t *ctx = caller_data;

	isch_flush_timer();
	PCB_DAD_FREE(ctx->dlg);
	for(n = 0; n < isch_ctx.len; n++)
		free(isch_ctx.inames[n]);
	free(isch_ctx.inames);
	memset(ctx, 0, sizeof(isch_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static int isch_cmp(void *a_, void *b_)
{
	pcb_plug_import_t **a = a_, **b = b_;

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

	PCB_DAD_SET_VALUE(isch_ctx.dlg_hid_ctx, isch_ctx.wtab, lng, target);
	if (setconf && (p != NULL))
		pcb_conf_set(CFR_DESIGN, "plugins/import_sch/import_fmt", 0, p->name, POL_OVERWRITE);

	if (p == NULL) {
		len = 0;
		controllable = 0;
	}
	else if (p->single_arg) {
		len = pcb_conflist_length(&conf_import_sch.plugins.import_sch.args);
		if (len < 1) {
			pcb_conf_grow("plugins/import_sch/args", 1);
			pcb_conf_set(CFR_DESIGN, "plugins/import_sch/args", 0, "", POL_OVERWRITE);
		}
		len = 1;
		controllable = 0;
	}
	else {
		len = pcb_conflist_length(&conf_import_sch.plugins.import_sch.args);
		controllable = 1;
	}


	for(n = 0; n < MAX_ARGS; n++) {
		pcb_gui->attr_dlg_widget_hide(isch_ctx.dlg_hid_ctx, isch_ctx.warg_box[n], n >= len);
		pcb_gui->attr_dlg_widget_hide(isch_ctx.dlg_hid_ctx, isch_ctx.warg_button[n], !((p != NULL) && (p->all_filenames)));
	}

	pcb_gui->attr_dlg_widget_hide(isch_ctx.dlg_hid_ctx, isch_ctx.warg_ctrl, !controllable);
}


static void isch_pcb2dlg(void)
{
	int n, len, tab = 0;
	pcb_conf_listitem_t *ci;
	const char *tname = conf_import_sch.plugins.import_sch.import_fmt;

	isch_flush_timer();

	if (tname != NULL) {
		for(n = 0; n < isch_ctx.len; n++) {
			if (pcb_strcasecmp(isch_ctx.inames[n], tname) == 0) {
				tab = n;
				break;
			}
		}
	}

	len = pcb_conflist_length(&conf_import_sch.plugins.import_sch.args);
	for(n = 0, ci = pcb_conflist_first(&conf_import_sch.plugins.import_sch.args); ci != NULL; ci = pcb_conflist_next(ci), n++)
		PCB_DAD_SET_VALUE(isch_ctx.dlg_hid_ctx, isch_ctx.warg[n], str, ci->val.string[0]);

	PCB_DAD_SET_VALUE(isch_ctx.dlg_hid_ctx, isch_ctx.wfmt, lng, tab);
	isch_switch_fmt(tab, 0);
}

static void isch_fmt_chg_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	isch_switch_fmt(isch_ctx.dlg[isch_ctx.wfmt].val.lng, 1);
}

static void isch_arg_del_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	int len = pcb_conflist_length(&conf_import_sch.plugins.import_sch.args);
	if (len > 0) {
		pcb_conf_del(CFR_DESIGN, "plugins/import_sch/args", len-1);
		isch_pcb2dlg();
	}
}

static void isch_arg_add_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	int len = pcb_conflist_length(&conf_import_sch.plugins.import_sch.args);
	if (len < MAX_ARGS+1) {
		pcb_conf_grow("plugins/import_sch/args", len+1);
		pcb_conf_set(CFR_DESIGN, "plugins/import_sch/args", len, "", POL_OVERWRITE);
		isch_pcb2dlg();
	}
}

static void isch_import_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	isch_flush_timer();
}

static void isch_arg_chg_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_hidval_t user_data;

	if (isch_ctx.arg_dirty)
		pcb_gui->stop_timer(pcb_gui, isch_ctx.timer);

	user_data.ptr = NULL;
	isch_ctx.timer = pcb_gui->add_timer(pcb_gui, isch_timed_update_cb, 1000, user_data);
	isch_ctx.arg_dirty = 1;
}

static void isch_add_tab(pcb_plug_import_t *p)
{
	PCB_DAD_BEGIN_VBOX(isch_ctx.dlg);
		PCB_DAD_LABEL(isch_ctx.dlg, p->desc);
	PCB_DAD_END(isch_ctx.dlg);
}

static int do_dialog(void)
{
	int len, n;
	const pcb_plug_import_t *p, **pa;
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
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
		isch_ctx.inames[n] = pcb_strdup(pa[n]->name);
	isch_ctx.inames[n] = NULL;
	isch_ctx.len = len;

	PCB_DAD_BEGIN_VBOX(isch_ctx.dlg);
		PCB_DAD_COMPFLAG(isch_ctx.dlg, PCB_HATF_EXPFILL);

		PCB_DAD_BEGIN_VBOX(isch_ctx.dlg); /* format box */
			PCB_DAD_COMPFLAG(isch_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_BEGIN_HBOX(isch_ctx.dlg);
				PCB_DAD_LABEL(isch_ctx.dlg, "Format:");
				PCB_DAD_ENUM(isch_ctx.dlg, isch_ctx.inames);
					isch_ctx.wfmt = PCB_DAD_CURRENT(isch_ctx.dlg);
					PCB_DAD_CHANGE_CB(isch_ctx.dlg, isch_fmt_chg_cb);
			PCB_DAD_END(isch_ctx.dlg);
			PCB_DAD_BEGIN_TABBED(isch_ctx.dlg, isch_ctx.inames);
				PCB_DAD_COMPFLAG(isch_ctx.dlg, PCB_HATF_HIDE_TABLAB);
				isch_ctx.wtab = PCB_DAD_CURRENT(isch_ctx.dlg);
				for(n = 0; n < len; n++)
					isch_add_tab(pa[n]);
			PCB_DAD_END(isch_ctx.dlg);
			PCB_DAD_BEGIN_VBOX(isch_ctx.dlg);
				PCB_DAD_COMPFLAG(isch_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
				for(n = 0; n < MAX_ARGS; n++) {
					PCB_DAD_BEGIN_HBOX(isch_ctx.dlg);
						isch_ctx.warg_box[n] = PCB_DAD_CURRENT(isch_ctx.dlg);
						PCB_DAD_STRING(isch_ctx.dlg);
							isch_ctx.warg[n] = PCB_DAD_CURRENT(isch_ctx.dlg);
							PCB_DAD_CHANGE_CB(isch_ctx.dlg, isch_arg_chg_cb);
						PCB_DAD_BUTTON(isch_ctx.dlg, "browse");
							isch_ctx.warg_button[n] = PCB_DAD_CURRENT(isch_ctx.dlg);
					PCB_DAD_END(isch_ctx.dlg);
				}
				PCB_DAD_BEGIN_HBOX(isch_ctx.dlg);
					isch_ctx.warg_ctrl = PCB_DAD_CURRENT(isch_ctx.dlg);
					PCB_DAD_BUTTON(isch_ctx.dlg, "Del last");
						PCB_DAD_CHANGE_CB(isch_ctx.dlg, isch_arg_del_cb);
					PCB_DAD_BUTTON(isch_ctx.dlg, "One more");
						PCB_DAD_CHANGE_CB(isch_ctx.dlg, isch_arg_add_cb);
				PCB_DAD_END(isch_ctx.dlg);
			PCB_DAD_END(isch_ctx.dlg);
		PCB_DAD_END(isch_ctx.dlg);

		PCB_DAD_BEGIN_VBOX(isch_ctx.dlg); /* generic settings */
		PCB_DAD_END(isch_ctx.dlg);
		PCB_DAD_BEGIN_HBOX(isch_ctx.dlg); /* bottom buttons */
			PCB_DAD_BUTTON(isch_ctx.dlg, "Import!");
				PCB_DAD_CHANGE_CB(isch_ctx.dlg, isch_import_cb);
			PCB_DAD_BEGIN_HBOX(isch_ctx.dlg);
				PCB_DAD_COMPFLAG(isch_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(isch_ctx.dlg);
			PCB_DAD_BUTTON_CLOSES(isch_ctx.dlg, clbtn);
		PCB_DAD_END(isch_ctx.dlg);
	PCB_DAD_END(isch_ctx.dlg);

	free(pa);

	/* set up the context */
	isch_ctx.active = 1;

	PCB_DAD_DEFSIZE(isch_ctx.dlg, 360, 400);
	PCB_DAD_NEW("import_sch", isch_ctx.dlg, "Import schematics/netlist", &isch_ctx, pcb_false, isch_close_cb);
	isch_pcb2dlg();
	return 0;
}

static int do_import(void)
{
	const char **a = NULL;
	int len, n, res;
	pcb_conf_listitem_t *ci;
	const char *imp_name = conf_import_sch.plugins.import_sch.import_fmt;
	pcb_plug_import_t *p;

	if ((imp_name == NULL) || (*imp_name == '\0'))
		return do_dialog();

	p = pcb_lookup_importer(imp_name);
	if (p == NULL) {
		pcb_message(PCB_MSG_ERROR, "import_sch2: can not find importer called '%s'\nIs the corresponding plugin compiled?\n", imp_name);
		return 1;
	}

	len = pcb_conflist_length(&conf_import_sch.plugins.import_sch.args);
	if ((p->single_arg) && (len > 1))
		len = 1;
	a = malloc((len+1) * sizeof(char *));
	for(n = 0, ci = pcb_conflist_first(&conf_import_sch.plugins.import_sch.args); ci != NULL; ci = pcb_conflist_next(ci), n++)
		a[n] = ci->val.string[0];
	pcb_message(PCB_MSG_ERROR, "import_sch2: reimport with %s -> %p\n", imp_name, p);
	res = p->import(p, IMPORT_ASPECT_NETLIST, a, len);
	free(a);
	return res;
}

static int do_setup(int argc, fgw_arg_t *argv)
{
	int n;
	pcb_plug_import_t *p;

	if (argc < 1) {
		pcb_message(PCB_MSG_ERROR, "ImportSch: setup needs importer name\n");
		return -1;
	}

	for(n = 0; n < argc; n++) {
		if (fgw_arg_conv(&pcb_fgw, &argv[n], FGW_STR) != 0) {
			pcb_message(PCB_MSG_ERROR, "ImportSch: failed to convert argument %d to string\n", n+1);
			return -1;
		}
	}

	p = pcb_lookup_importer(argv[0].val.str);
	if (p == NULL) {
		pcb_message(PCB_MSG_ERROR, "ImportSch: importer not found: '%s'\n", argv[0].val.str);
		return -1;
	}

	if (p->single_arg && (argc != 2)) {
		pcb_message(PCB_MSG_ERROR, "ImportSch: importer '%s' requires exactly one file name argument\n", argv[0].val.str);
		return -1;
	}
	else if (p->all_filenames && (argc < 2)) {
		pcb_message(PCB_MSG_ERROR, "ImportSch: importer '%s' requires at least one file name argument\n", argv[0].val.str);
		return -1;
	}

	pcb_conf_set(CFR_DESIGN, "plugins/import_sch/import_fmt", 0, argv[0].val.str, POL_OVERWRITE);
	for(n = 1; n < argc; n++)
		pcb_conf_set(CFR_DESIGN, "plugins/import_sch/args", n-1, argv[n].val.str, POL_OVERWRITE);

	return 0;
}

static const char pcb_acts_ImportSch[] =
	"ImportSch()\n"
	"ImportSch(reimport)\n"
	"ImportSch(setup, importer, [args...])\n";
static const char pcb_acth_ImportSch[] = "Import schematics/netlist.";
/* DOC: importsch.html */
static fgw_error_t pcb_act_ImportSch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd = "reimport";

	PCB_ACT_MAY_CONVARG(1, FGW_STR, ImportSch, cmd = argv[1].val.str);

	if (strcmp(cmd, "reimport") == 0) {
		PCB_ACT_IRES(do_import());
		return 0;
	}
	if (strcmp(cmd, "setup") == 0) {
		PCB_ACT_IRES(do_setup(argc-2, argv+2));
		return 0;
	}
	if (strcmp(cmd, "dialog") == 0) {
		PCB_ACT_IRES(do_dialog());
		return 0;
	}

	PCB_ACT_FAIL(ImportSch);
	PCB_ACT_IRES(1);
	return 0;
}

static const char *import_sch_cookie = "import_sch2 plugin";

static pcb_action_t import_sch_action_list[] = {
	{"ImportSch", pcb_act_ImportSch, pcb_acth_ImportSch, pcb_acts_ImportSch}
};

int pplg_check_ver_import_sch2(int ver_needed) { return 0; }

void pplg_uninit_import_sch2(void)
{
	pcb_remove_actions_by_cookie(import_sch_cookie);
	pcb_conf_unreg_fields("plugins/import_sch/");
}

int pplg_init_import_sch2(void)
{
	char *tmp;

	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(import_sch_action_list, import_sch_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_import_sch, field,isarray,type_name,cpath,cname,desc,flags);
#include "import_sch_conf_fields.h"

	return 0;
}
