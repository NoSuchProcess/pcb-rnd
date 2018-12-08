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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Preferences dialog, conf tree tab */

#include "dlg_pref.h"
#include "conf.h"
#include "conf_core.h"

static int conf_tree_cmp(const void *v1, const void *v2)
{
	const htsp_entry_t **e1 = (const htsp_entry_t **) v1, **e2 = (const htsp_entry_t **) v2;
	return strcmp((*e1)->key, (*e2)->key);
}

/* Recursively create the directory and all parents in a tree */
static pcb_hid_row_t *dlg_conf_tree_mkdirp(pref_ctx_t *ctx, pcb_hid_tree_t *tree, char *path)
{
	char *cell[2] = {NULL};
	pcb_hid_row_t *parent;
	char *last;

	parent = htsp_get(&tree->paths, path);
	if (parent != NULL)
		return parent;

	last = strrchr(path, '/');

	if (last == NULL) {
		/* root dir */
		parent = htsp_get(&tree->paths, path);
		if (parent != NULL)
			return parent;
		cell[0] = pcb_strdup(path);
		return pcb_dad_tree_append(tree->attrib, NULL, cell);
	}

/* non-root-dir: get or create parent */
	*last = '\0';
	last++;
	parent = dlg_conf_tree_mkdirp(ctx, tree, path);

	cell[0] = pcb_strdup(last);
	return pcb_dad_tree_append_under(tree->attrib, parent, cell);
}

static void setup_tree(pref_ctx_t *ctx)
{
	char *cell[2] = {NULL};
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->conf.wtree];
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
	htsp_entry_t *e;
	htsp_entry_t **sorted;
	int num_paths, n;
	char path[1024];

	/* alpha sort keys for the more consistend UI */
	for(e = htsp_first(conf_fields), num_paths = 0; e; e = htsp_next(conf_fields, e))
		num_paths++;
	sorted = malloc(sizeof(htsp_entry_t *) * num_paths);
	for(e = htsp_first(conf_fields), n = 0; e; e = htsp_next(conf_fields, e), n++)
		sorted[n] = e;
	qsort(sorted, num_paths, sizeof(htsp_entry_t *), conf_tree_cmp);

	for(n = 0; n < num_paths; n++) {
		char *basename;
		pcb_hid_row_t *parent;

		e = sorted[n];
		if (strlen(e->key) > sizeof(path) - 1) {
			pcb_message(PCB_MSG_WARNING, "Warning: can't create config item for %s: path too long\n", e->key);
			continue;
		}
		strcpy(path, e->key);
		basename = strrchr(path, '/');
		if ((basename == NULL) || (basename == path)) {
			pcb_message(PCB_MSG_WARNING, "Warning: can't create config item for %s: invalid path (node in root)\n", e->key);
			continue;
		}
		*basename = '\0';
		basename++;

		parent = dlg_conf_tree_mkdirp(ctx, tree, path);
		if (parent == NULL) {
			pcb_message(PCB_MSG_WARNING, "Warning: can't create config item for %s: invalid path\n", e->key);
			continue;
		}
		cell[0] = pcb_strdup(basename);
		pcb_dad_tree_append_under(attr, parent, cell);
	}
	free(sorted);
}

static void setup_intree(pref_ctx_t *ctx)
{
	conf_role_t n;
	char *cell[5] = {NULL};
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->conf.wintree];

	for(n = 0; n < CFR_max_real; n++) {
		cell[0] = pcb_strdup(conf_role_name(n));
		pcb_dad_tree_append(attr, NULL, cell);
	}
}

void pcb_dlg_pref_conf_create(pref_ctx_t *ctx)
{
	static const char *hdr_intree[] = {"role", "prio", "policy", "value", NULL};

	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_HPANE(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);

		/* left: tree */
		PCB_DAD_TREE(ctx->dlg, 1, 1, NULL);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
			ctx->conf.wtree = PCB_DAD_CURRENT(ctx->dlg);

		/* right: details */
		PCB_DAD_BEGIN_VPANE(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			
			/* right/top: conf file */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(ctx->dlg, "name");
					ctx->conf.wname = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "desc");
					ctx->conf.wdesc = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "INPUT: configuration node (\"file\" version)");
				PCB_DAD_BEGIN_HBOX(ctx->dlg);
					PCB_DAD_TREE(ctx->dlg, 4, 0, hdr_intree); /* input state */
						ctx->conf.wintree = PCB_DAD_CURRENT(ctx->dlg);
					PCB_DAD_BEGIN_VBOX(ctx->dlg);
						PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
						PCB_DAD_LABEL(ctx->dlg, "EDIT input");
					PCB_DAD_END(ctx->dlg);
				PCB_DAD_END(ctx->dlg);
			PCB_DAD_END(ctx->dlg);

			/* right/bottom: native file */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
				PCB_DAD_LABEL(ctx->dlg, "NATIVE: in-memory conf node after the merge");
				PCB_DAD_TREE(ctx->dlg, 4, 0, hdr_intree); /* input state */
					PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
					ctx->conf.wmemtree = PCB_DAD_CURRENT(ctx->dlg);

			PCB_DAD_END(ctx->dlg);

		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	setup_tree(ctx);
	setup_intree(ctx);
}
