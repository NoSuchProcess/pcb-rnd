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

#include "plug_import.h"
#include "import_sch_conf.h"

conf_import_sch_t conf_import_sch;

static int do_import(void)
{
	const char **a = NULL;
	int len, n, res;
	pcb_conf_listitem_t *ci;
	const char *imp_name = conf_import_sch.plugins.import_sch.import_fmt;
	pcb_plug_import_t *p;

	if ((imp_name == NULL) || (*imp_name == '\0')) {
		TODO("invoke the GUI instead\n");
		pcb_message(PCB_MSG_ERROR, "import_sch2: missing conf\n");
		return 1;
	}
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


static const char pcb_acts_ImportSch[] =
	"ImportSch()\n";
static const char pcb_acth_ImportSch[] = "Import schematics.";
/* DOC: import.html */
static fgw_error_t pcb_act_ImportSch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (argc == 1) { /* no arg means: shorthand for re-import from stored conf */
		PCB_ACT_IRES(do_import());
		return 0;
	}

	pcb_message(PCB_MSG_ERROR, "import_sch2: not yet implemented\n");
	PCB_ACT_IRES(0);
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
