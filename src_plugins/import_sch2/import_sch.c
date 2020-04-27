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
#include <librnd/core/conf_hid.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/globalconst.h>

#include "board.h"
#include "plug_import.h"
#include "import_sch_conf.h"

#define MAX_ARGS 16

static conf_import_sch_t conf_import_sch;

static int do_import(void);
static const char *import_sch_cookie = "import_sch2 plugin";

#include "import_sch_dlg.c"

static int convert_attribs(void)
{
	const char *mode = rnd_attrib_get(PCB, "import::mode");
	const char *src0 = rnd_attrib_get(PCB, "import::src0");
	char tmp[32];
	int n, idx;

	if ((mode == NULL) && (src0 == NULL))
		return 0;

	for(n = 0, idx = 0; n < MAX_ARGS; n++) {
		const char *src;
		
		sprintf(tmp, "import::src%d", n);
		src = rnd_attrib_get(PCB, tmp);
		if (src != NULL) {
			pcb_conf_grow("plugins/import_sch/args", idx+1);
			pcb_conf_set(CFR_DESIGN, "plugins/import_sch/args", idx, src, POL_OVERWRITE);
			idx++;
		}
	}

	if (mode == NULL)
		mode = "gnetlist";
	else if (strcmp(mode, "make") == 0)
		mode = "cmd";
	pcb_conf_set(CFR_DESIGN, "plugins/import_sch/import_fmt", 0, mode, POL_OVERWRITE);


	if (strcmp(mode, "cmd") == 0) {
		const char *outfile = rnd_attrib_get(PCB, "import::outfile");
		const char *makefile = rnd_attrib_get(PCB, "import::makefile");
		const char *target = rnd_attrib_get(PCB, "import::target");
		gds_t cmdline;

		if (outfile == NULL) outfile = "-";
		if (target == NULL) target = "pcb_import";

		gds_init(&cmdline);
		gds_append_str(&cmdline, "make");
		if (makefile != NULL) {
			gds_append_str(&cmdline, " -f \"");
			gds_append_str(&cmdline, makefile);
			gds_append(&cmdline, '"');
		}
		gds_append(&cmdline, ' ');
		gds_append_str(&cmdline, target);

		pcb_conf_grow("plugins/import_sch/args", 2);
		pcb_conf_set(CFR_DESIGN, "plugins/import_sch/args", 0, outfile, POL_OVERWRITE);
		pcb_conf_set(CFR_DESIGN, "plugins/import_sch/args", 1, cmdline.array, POL_OVERWRITE);

		gds_uninit(&cmdline);
	}

	return 1;
}

static int do_import(void)
{
	const char **a = NULL;
	int len, n, res;
	pcb_conf_listitem_t *ci;
	const char *imp_name = conf_import_sch.plugins.import_sch.import_fmt;
	pcb_plug_import_t *p;

	if ((imp_name == NULL) || (*imp_name == '\0')) {
		if (convert_attribs()) {
			rnd_message(PCB_MSG_ERROR, "Had to convert import:: attributes to import_sch config\nNOTE: changes done to import settings will not change the old attribute values.\nFor details see: http://repo.hu/projects/pcb-rnd/help/err0001.html\n");
			imp_name = conf_import_sch.plugins.import_sch.import_fmt;
		}
		else {
			if (!PCB_HAVE_GUI_ATTR_DLG) {
				rnd_message(PCB_MSG_ERROR, "import_sch not configured; please use ImportSch(setup, ...)\n");
				return 1;
			}
			else
				return do_dialog();
		}
	}

	p = pcb_lookup_importer(imp_name);
	if (p == NULL) {
		rnd_message(PCB_MSG_ERROR, "import_sch2: can not find importer called '%s'\nIs the corresponding plugin compiled?\n", imp_name);
		return 1;
	}

	len = pcb_conflist_length((pcb_conflist_t *)&conf_import_sch.plugins.import_sch.args);
	if ((p->single_arg) && (len > 1))
		len = 1;
	a = malloc((len+1) * sizeof(char *));
	for(n = 0, ci = pcb_conflist_first((pcb_conflist_t *)&conf_import_sch.plugins.import_sch.args); ci != NULL; ci = pcb_conflist_next(ci), n++)
		a[n] = ci->val.string[0];
	rnd_message(PCB_MSG_DEBUG, "import_sch2: reimport with %s -> %p\n", imp_name, p);
	res = p->import(p, IMPORT_ASPECT_NETLIST, a, len);
	free(a);
	return res;
}

static int do_setup(int argc, fgw_arg_t *argv)
{
	int n;
	pcb_plug_import_t *p;

	if (argc < 1) {
		rnd_message(PCB_MSG_ERROR, "ImportSch: setup needs importer name\n");
		return -1;
	}

	for(n = 0; n < argc; n++) {
		if (fgw_arg_conv(&rnd_fgw, &argv[n], FGW_STR) != 0) {
			rnd_message(PCB_MSG_ERROR, "ImportSch: failed to convert argument %d to string\n", n+1);
			return -1;
		}
	}

	p = pcb_lookup_importer(argv[0].val.str);
	if (p == NULL) {
		rnd_message(PCB_MSG_ERROR, "ImportSch: importer not found: '%s'\n", argv[0].val.str);
		return -1;
	}

	if (p->single_arg && (argc != 2)) {
		rnd_message(PCB_MSG_ERROR, "ImportSch: importer '%s' requires exactly one file name argument\n", argv[0].val.str);
		return -1;
	}
	else if (p->all_filenames && (argc < 2)) {
		rnd_message(PCB_MSG_ERROR, "ImportSch: importer '%s' requires at least one file name argument\n", argv[0].val.str);
		return -1;
	}

	pcb_conf_set(CFR_DESIGN, "plugins/import_sch/import_fmt", 0, argv[0].val.str, POL_OVERWRITE);
	pcb_conf_grow("plugins/import_sch/args", argc-1);
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

	rnd_PCB_ACT_MAY_CONVARG(1, FGW_STR, ImportSch, cmd = argv[1].val.str);

	if (strcmp(cmd, "reimport") == 0) {
		RND_ACT_IRES(do_import());
		return 0;
	}
	if (strcmp(cmd, "setup") == 0) {
		RND_ACT_IRES(do_setup(argc-2, argv+2));
		return 0;
	}
	if (strcmp(cmd, "dialog") == 0) {
		RND_ACT_IRES(do_dialog());
		return 0;
	}

	RND_ACT_FAIL(ImportSch);
	RND_ACT_IRES(1);
	return 0;
}


static rnd_action_t import_sch_action_list[] = {
	{"ImportSch", pcb_act_ImportSch, pcb_acth_ImportSch, pcb_acts_ImportSch}
};

int pplg_check_ver_import_sch2(int ver_needed) { return 0; }

void pplg_uninit_import_sch2(void)
{
	rnd_remove_actions_by_cookie(import_sch_cookie);
	pcb_conf_unreg_fields("plugins/import_sch/");
	isch_dlg_uninit();
}

int pplg_init_import_sch2(void)
{
	PCB_API_CHK_VER;
	RND_REGISTER_ACTIONS(import_sch_action_list, import_sch_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_import_sch, field,isarray,type_name,cpath,cname,desc,flags);
#include "import_sch_conf_fields.h"
	isch_dlg_init();

	return 0;
}
