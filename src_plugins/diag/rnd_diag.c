/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019,2021 Tibor 'Igor2' Palinkas
 *
 *  This module, diag, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/conf.h>
#include <librnd/core/error.h>
#include <librnd/core/event.h>
#include <librnd/core/hid.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_dad.h>

static void conf_dump(FILE *f, const char *prefix, int verbose, const char *match_prefix)
{
	htsp_entry_t *e;
	int pl;

	if (match_prefix != NULL)
		pl = strlen(match_prefix);

	for (e = htsp_first(rnd_conf_fields); e; e = htsp_next(rnd_conf_fields, e)) {
		rnd_conf_native_t *node = (rnd_conf_native_t *)e->value;
		if (match_prefix != NULL) {
			if (strncmp(node->hash_path, match_prefix, pl) != 0)
				continue;
		}
		rnd_conf_print_native((rnd_conf_pfn)rnd_fprintf, f, prefix, verbose, node);
	}
}


static const char pcb_acts_DumpConf[] =
	"dumpconf(native, [verbose], [prefix]) - dump the native (binary) config tree to stdout\n"
	"dumpconf(lihata, role, [prefix]) - dump in-memory lihata representation of a config tree\n"
	;
static const char pcb_acth_DumpConf[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *rnd_conf_main_root[];
extern lht_doc_t *rnd_conf_plug_root[];
static fgw_error_t pcb_act_DumpConf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op;

	RND_ACT_CONVARG(1, FGW_STR, DumpConf, op = argv[1].val.cstr);

	if (rnd_strcasecmp(op, "native") == 0) {
			int verbose = 0;
			const char *prefix = "";
			RND_ACT_MAY_CONVARG(2, FGW_INT, DumpConf, verbose = argv[2].val.nat_int);
			RND_ACT_MAY_CONVARG(3, FGW_STR, DumpConf, prefix = argv[3].val.str);
			conf_dump(stdout, prefix, verbose, NULL);
	}
	else if (rnd_strcasecmp(op, "lihata") == 0) {
		rnd_conf_role_t role;
		const char *srole, *prefix = "";
		RND_ACT_CONVARG(2, FGW_STR, DumpConf, srole = argv[2].val.str);
		RND_ACT_MAY_CONVARG(3, FGW_STR, DumpConf, prefix = argv[3].val.str);
		role = rnd_conf_role_parse(srole);
		if (role == RND_CFR_invalid) {
			rnd_message(RND_MSG_ERROR, "Invalid role: '%s'\n", argv[1]);
			RND_ACT_IRES(1);
			return 0;
		}
		if (rnd_conf_main_root[role] != NULL) {
			printf("%s### main\n", prefix);
			if (rnd_conf_main_root[role] != NULL)
				lht_dom_export(rnd_conf_main_root[role]->root, stdout, prefix);
			printf("%s### plugin\n", prefix);
			if (rnd_conf_plug_root[role] != NULL)
				lht_dom_export(rnd_conf_plug_root[role]->root, stdout, prefix);
		}
		else
			printf("%s <empty>\n", prefix);
	}
	else {
		RND_ACT_FAIL(DumpConf);
		return 1;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_EvalConf[] =
	"EvalConf(path) - evaluate a config path in different config sources to figure how it ended up in the native database\n"
	;
static const char pcb_acth_EvalConf[] = "Perform various operations on the configuration tree.";

static fgw_error_t pcb_act_EvalConf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;
	rnd_conf_native_t *nat;
	int role;

	RND_ACT_CONVARG(1, FGW_STR, EvalConf, path = argv[1].val.str);

	nat = rnd_conf_get_field(path);
	if (nat == NULL) {
		rnd_message(RND_MSG_ERROR, "EvalConf: invalid path %s - no such config setting\n", path);
		RND_ACT_IRES(-1);
		return 0;
	}

	printf("Conf node %s\n", path);
	for(role = 0; role < RND_CFR_max_real; role++) {
		lht_node_t *n;
		printf(" Role: %s\n", rnd_conf_role_name(role));
		n = rnd_conf_lht_get_at(role, path, 0);
		if (n != NULL) {
			rnd_conf_policy_t pol = -1;
			long prio = rnd_conf_default_prio[role];


			if (rnd_conf_get_policy_prio(n, &pol, &prio) == 0)
				printf("  * policy=%s\n  * prio=%ld\n", rnd_conf_policy_name(pol), prio);

			if (n->file_name != NULL)
				printf("  * from=%s:%d.%d\n", n->file_name, n->line, n->col);
			else
				printf("  * from=(unknown)\n");

			lht_dom_export(n, stdout, "  ");
		}
		else
			printf("  * not present\n");
	}

	printf(" Native:\n");
	rnd_conf_print_native((rnd_conf_pfn)rnd_fprintf, stdout, "  ", 1, nat);

	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t rnd_diag_action_list[] = {
	{"dumpconf", pcb_act_DumpConf, pcb_acth_DumpConf, pcb_acts_DumpConf},
	{"EvalConf", pcb_act_EvalConf, pcb_acth_EvalConf, pcb_acts_EvalConf},
};

void Rnd_diag_init(const char *cookie)
{
	RND_REGISTER_ACTIONS(rnd_diag_action_list, cookie)

}

