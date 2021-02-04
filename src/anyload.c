/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

#include <genlist/gendlist.h>
#include <genregex/regex_se.h>

#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>

#include "anyload.h"

typedef struct {
	const pcb_anyload_t *al;
	re_se_t *rx;
	const char *rx_str;
	gdl_elem_t link;
} pcb_aload_t;

static gdl_list_t anyloads;


int pcb_anyload_reg(const char *root_regex, const pcb_anyload_t *al_in)
{
	re_se_t *rx;
	pcb_aload_t *al;

	rx = re_se_comp(root_regex);
	if (rx == NULL) {
		rnd_message(RND_MSG_ERROR, "pcb_anyload_reg: failed to compile regex '%s' for '%s'\n", root_regex, al_in->cookie);
		return -1;
	}

	al = calloc(sizeof(pcb_aload_t), 1);
	al->al = al_in;
	al->rx = rx;
	al->rx_str = root_regex;

	gdl_append(&anyloads, al, link);

	return 0;
}

static void pcb_anyload_free(pcb_aload_t *al)
{
	gdl_remove(&anyloads, al, link);
	re_se_free(al->rx);
	free(al);
}

void pcb_anyload_unreg_by_cookie(const char *cookie)
{
	pcb_aload_t *al, *next;
	for(al = gdl_first(&anyloads); al != NULL; al = next) {
		next = al->link.next;
		if (al->al->cookie == cookie)
			pcb_anyload_free(al);
	}
}

void pcb_anyload_uninit(void)
{
	pcb_aload_t *al;
	while((al = gdl_first(&anyloads)) != NULL) {
		rnd_message(RND_MSG_ERROR, "pcb_anyload: '%s' left anyloader regs in\n", al->al->cookie);
		pcb_anyload_free(al);
	}
}

/* call a loader to load/install a subtree; retruns non-zero on error */
int pcb_anyload_parse_subtree(rnd_hidlib_t *hidlib, lht_node_t *subtree, rnd_conf_role_t inst_role)
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	pcb_aload_t *al;

	for(al = gdl_first(&anyloads); al != NULL; al = al->link.next)
		if (re_se_exec(al->rx, subtree->name))
			return al->al->load_subtree(al->al, pcb, subtree, inst_role);

	rnd_message(RND_MSG_ERROR, "anyload: root node '%s' is not recognized by any loader\n", subtree->name);
	return -1;
}

/* parse an anyload file, load/install all roots; retruns non-zero on any error */
int pcb_anyload_parse_anyload_v1(rnd_hidlib_t *hidlib, lht_node_t *root, rnd_conf_role_t inst_role)
{
	lht_node_t *lst, *n;
	int res = 0;

	if (root->type != LHT_HASH) {
		rnd_message(RND_MSG_ERROR, "anyload: pcb-rnd-anyload-v* root node must be a hash\n");
		return -1;
	}

	lst = lht_dom_hash_get(root, "roots");
	if (lst == NULL) {
		rnd_message(RND_MSG_WARNING, "anyload: pcb-rnd-anyload-v* without li:roots node - nothing loaded\n(this is probably not what you wanted)\n");
		return 0;
	}
	if (lst->type != LHT_LIST) {
		rnd_message(RND_MSG_WARNING, "anyload: pcb-rnd-anyload-v*/roots must be a list\n");
		return 0;
	}

	for(n = lst->data.list.first; n != NULL; n = n->next) {
		int r = pcb_anyload_parse_subtree(hidlib, n, inst_role);
		if (r != 0)
			res = r;
	}
	return res;
}

/* parse the root of a random file, which will either be an anyload pack or
   a single root one of our backends can handle; retruns non-zero on error */
int pcb_anyload_parse_root(rnd_hidlib_t *hidlib, lht_node_t *root, rnd_conf_role_t inst_role)
{
	if (strcmp(root->name, "pcb-rnd-anyload-v1") == 0)
		return pcb_anyload_parse_anyload_v1(hidlib, root, inst_role);
	return pcb_anyload_parse_subtree(hidlib, root, inst_role);
}

int pcb_anyload(rnd_hidlib_t *hidlib, const char *path, rnd_conf_role_t inst_role)
{
	char *path_free = NULL, *cwd_free = NULL;
	const char *cwd;
	int res = -1, req_anyload = 0;
	FILE *f;
	lht_doc_t *doc;
	char *errmsg;

	if (rnd_is_dir(hidlib, path)) {
		cwd = path;
		path = path_free = rnd_concat(cwd, RND_DIR_SEPARATOR_S, "anyload.lht");
		req_anyload = 1;
	}
	else {
		const char *s, *sep = NULL;

		for(s = path; *s != '\0'; s++)
			if ((*s == RND_DIR_SEPARATOR_C) || (*s == '/'))
				sep = s;

		if (sep != NULL)
			cwd = cwd_free = rnd_strndup(path, s-path);
		else
			cwd = ".";
	}

	f = rnd_fopen(hidlib, path, "r");
	if (f == NULL) {
		rnd_message(RND_MSG_ERROR, "anyload: can't open %s for read\n", path);
		goto error;
	}

	doc = lht_dom_load_stream(f, path, &errmsg);
	fclose(f);
	if (doc == NULL) {
		rnd_message(RND_MSG_ERROR, "anyload: can't load %s: %s\n", path, errmsg);
		goto error;
	}

	if (req_anyload)
		res = pcb_anyload_parse_anyload_v1(hidlib, doc->root, inst_role);
	else
		res = pcb_anyload_parse_root(hidlib, doc->root, inst_role);

	error:;
	free(path_free);
	free(cwd_free);
	return res;
}

static const char pcb_acts_AnyLoad[] = "AnyLoad(path, [role])";
static const char pcb_acth_AnyLoad[] =
	"Load \"anything\" from path; if role is specified, try to install or store\n"
	"the loaded objects persistently somewhere that is equivalent to the specified config role.";
fgw_error_t pcb_act_AnyLoad(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path = NULL, *srole = NULL;
	rnd_conf_role_t role = RND_CFR_invalid;

	RND_ACT_CONVARG(1, FGW_STR, AnyLoad, path = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, AnyLoad, srole = argv[2].val.str);

	if (srole != NULL) {
		role = rnd_conf_role_parse(srole);
		if (role == RND_CFR_invalid) {
			rnd_message(RND_MSG_ERROR, "AnyLoad: invalid role '%s'\n", srole);
			return FGW_ERR_ARG_CONV;
		}
	}

	RND_ACT_IRES(pcb_anyload(RND_ACT_HIDLIB, path, role));
	return 0;
}


static rnd_action_t anyload_action_list[] = {
	{"AnyLoad", pcb_act_AnyLoad, pcb_acth_AnyLoad, pcb_acts_AnyLoad},
};


void pcb_anyload_init2(void)
{
	RND_REGISTER_ACTIONS(anyload_action_list, NULL);
}

