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

conf_import_sch_t conf_import_sch;


typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	char **inames;
	int active; /* already open - allow only one instance */
} isch_ctx_t;

static isch_ctx_t isch_ctx;

static void isch_close_cb(void *caller_data, pcb_hid_attr_ev_t ev)
{
	isch_ctx_t *ctx = caller_data;
	PCB_DAD_FREE(ctx->dlg);
	memset(ctx, 0, sizeof(isch_ctx_t)); /* reset all states to the initial - includes ctx->active = 0; */
}

static int do_dialog(void)
{
	int len, n;
	pcb_plug_import_t *p;
	pcb_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	if (isch_ctx.active)
		return; /* do not open another */

	len = 1; /* for the NULL */
	for(p = pcb_plug_import_chain; p != NULL; p = p->next) len++;

	isch_ctx.inames = malloc(len * sizeof(char *));
	for(n = 0, p = pcb_plug_import_chain; p != NULL; p = p->next, n++)
		isch_ctx.inames[n] = pcb_strdup(p->name);
	isch_ctx.inames[n] = NULL;

	PCB_DAD_BEGIN_VBOX(isch_ctx.dlg);
		PCB_DAD_COMPFLAG(isch_ctx.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_BEGIN_HBOX(isch_ctx.dlg);
			PCB_DAD_LABEL(isch_ctx.dlg, "Format:");
			PCB_DAD_ENUM(isch_ctx.dlg, isch_ctx.inames);
		PCB_DAD_END(isch_ctx.dlg);

		PCB_DAD_BEGIN_HBOX(isch_ctx.dlg);
			PCB_DAD_BUTTON(isch_ctx.dlg, "Import!");
			PCB_DAD_BEGIN_HBOX(isch_ctx.dlg);
				PCB_DAD_COMPFLAG(isch_ctx.dlg, PCB_HATF_EXPFILL);
			PCB_DAD_END(isch_ctx.dlg);
			PCB_DAD_BUTTON_CLOSES(isch_ctx.dlg, clbtn);
		PCB_DAD_END(isch_ctx.dlg);
	PCB_DAD_END(isch_ctx.dlg);

	/* set up the context */
	isch_ctx.active = 1;

	PCB_DAD_NEW("import_sch", isch_ctx.dlg, "Import schematics/netlist", &isch_ctx, pcb_false, isch_close_cb);
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
