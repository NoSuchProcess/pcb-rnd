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
#include "misc_util.h"

static const char *pref_conf_get_val(const lht_node_t *nd, const conf_native_t *nat, int idx);
#include "dlg_pref_confedit.c"

/* how many chars per line in conf node description (determines window width vs.
   window height */
#define DESC_WRAP_WIDTH 50

static const char *type_tabs[CFN_max+2] = /* MUST MATCH conf_native_type_t */
	{"STRING", "BOOLEAN", "INTEGER", "REAL", "COORD", "UNIT", "COLOR", "LIST", "invalid", NULL};

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
		char *basename, *bnsep;
		pcb_hid_row_t *parent;
		conf_native_t *nat;

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
		bnsep = basename;
		*basename = '\0';
		basename++;

		parent = dlg_conf_tree_mkdirp(ctx, tree, path);
		if (parent == NULL) {
			pcb_message(PCB_MSG_WARNING, "Warning: can't create config item for %s: invalid path\n", e->key);
			continue;
		}
		
		nat = e->value;
		if (nat->array_size > 1) {
			int i;
			*bnsep = '/';
			parent = dlg_conf_tree_mkdirp(ctx, tree, path);
			for(i = 0; i < nat->array_size; i++) {
				cell[0] = pcb_strdup_printf("[%d]", i);
				pcb_dad_tree_append_under(attr, parent, cell);
			}
		}
		else {
			cell[0] = pcb_strdup(basename);
			pcb_dad_tree_append_under(attr, parent, cell);
		}
	}
	free(sorted);
}

static const char *pref_conf_get_val(const lht_node_t *nd, const conf_native_t *nat, int idx)
{
	lht_node_t *ni;
	const char *val;

	switch (nd->type) {
		case LHT_TEXT: return nd->data.text.value; break;
		case LHT_LIST:
			if (nat->array_size > 1) {
				int idx2 = idx;
				val = "";
				for(ni = nd->data.list.first; ni != NULL; ni = ni->next) {
					if (idx2 == 0) {
						if (ni->type == LHT_TEXT)
							return ni->data.text.value;
						return "<invalid array item type>";
					}
					idx2--;
				}
			}
			return "<list>";
		case LHT_HASH:         return "<hash>"; break;
		case LHT_TABLE:        return "<table>"; break;
		case LHT_SYMLINK:      return "<symlink>"; break;
		case LHT_INVALID_TYPE: return "<invalid>"; break;
	}
}

static void setup_intree(pref_ctx_t *ctx, conf_native_t *nat, int idx)
{
	conf_role_t n;
	pcb_hid_attribute_t *attr = &ctx->dlg[ctx->conf.wintree];
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
	pcb_hid_row_t *r;

	pcb_dad_tree_clear(tree);

	for(n = 0; n < CFR_max_real; n++) {
		char *cell[5]= {NULL};
		cell[0] = pcb_strdup(conf_role_name(n));
		if (nat != NULL) {
			lht_node_t *nd;
			long prio = conf_default_prio[n];
			conf_policy_t pol = POL_OVERWRITE;

			nd = conf_lht_get_at(n, nat->hash_path, 0);
			if (nd != NULL) { /* role, prio, policy, value */
				conf_get_policy_prio(nd, &pol, &prio);
				cell[1] = pcb_strdup_printf("%ld", prio);
				cell[2] = pcb_strdup(conf_policy_name(pol));
				cell[3] = pcb_strdup(pref_conf_get_val(nd, nat, idx));
			}
		}
		r = pcb_dad_tree_append(attr, NULL, cell);
		r->user_data2.lng = n;
	}
}

static const char *print_conf_val(conf_native_type_t type, const confitem_t *val, char *buf, int sizebuf)
{
	const char *ret = buf;

	*buf = '\0';
	switch(type) {
		case CFN_STRING:   if (*val->string != NULL) ret = *val->string; break;
		case CFN_BOOLEAN:  strcpy(buf, *val->boolean ? "true" : "false"); break;
		case CFN_INTEGER:  sprintf(buf, "%ld", *val->integer); break;
		case CFN_REAL:     sprintf(buf, "%f", *val->real); break;
		case CFN_COORD:    pcb_snprintf(buf, sizebuf, "%mH\n%mm\n%ml", *val->coord, *val->coord, *val->coord); break;
		case CFN_UNIT:     strcpy(buf, (*val->unit)->suffix); break;
		case CFN_COLOR:    strcpy(buf, val->color->str); break;
		case CFN_LIST:     strcpy(buf, "<list>"); break;
		case CFN_max:      strcpy(buf, "<invalid-type>"); break;
	}
	return ret;
}

static void dlg_conf_select_node(pref_ctx_t *ctx, const char *path, conf_native_t *nat, int idx)
{
	pcb_hid_attr_val_t hv;
	char *tmp, buf[128];
	const char *rolename;
	lht_node_t *src;

	if ((path != NULL) && (nat == NULL))
		nat = conf_get_field(path);

	if ((path == NULL) && (nat != NULL))
		path = nat->hash_path;

	ctx->conf.selected_nat = nat;
	ctx->conf.selected_idx = idx;

	if (nat == NULL) {
		hv.str_value = "";
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wname, &hv);
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wdesc, &hv);
		setup_intree(ctx, NULL, 0);

		hv.int_value = CFN_max;
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wnattype, &hv);

		return;
	}

	hv.str_value = path == NULL ? "" : path;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wname, &hv);

	tmp = pcb_strdup(nat->description);
	pcb_text_wrap(tmp, DESC_WRAP_WIDTH, '\n', ' ');
	hv.str_value = tmp;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wdesc, &hv);
	free(tmp);

	/* display lht value */
	setup_intree(ctx, nat, idx);

	/* display native value */
	hv.int_value = nat->type;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wnattype, &hv);

	if (nat->type == CFN_LIST) {
		/* non-default: lists are manually loaded */
		pcb_hid_attribute_t *attr = &ctx->dlg[ctx->conf.wnatval[CFN_LIST]];
		pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attr->enumerations;
		conf_listitem_t *n;
		char *cell[4];

		pcb_dad_tree_clear(tree);
		for (n = conflist_first(&nat->val.list[idx]); n != NULL; n = conflist_next(n)) {
			rolename = conf_role_name(conf_lookup_role(n->prop.src));
			cell[0] = rolename == NULL ? pcb_strdup("") : pcb_strdup(rolename);
			cell[1] = pcb_strdup_printf("%ld", n->prop.prio);
			cell[2] = pcb_strdup(print_conf_val(n->type, &n->val, buf, sizeof(buf)));
			cell[3] = 0;
			pcb_dad_tree_append(attr, NULL, cell);
		}
		return;
	}

	/* default: set the value of the given node from hv loaded above */
	hv.str_value = print_conf_val(nat->type, &nat->val, buf, sizeof(buf));
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wnatval[nat->type], &hv);

	src = nat->prop[idx].src;
	if (src != NULL) {
		rolename = conf_role_name(conf_lookup_role(nat->prop[idx].src));
		hv.str_value = tmp = pcb_strdup_printf("prio: %d role: %s\nsource: %s:%d.%d", nat->prop[idx].prio, rolename, src->file_name, src->line, src->col);
	}
	else
		hv.str_value = tmp = pcb_strdup_printf("prio: %d\nsource: <not saved>", nat->prop[idx].prio);
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wsrc[nat->type], &hv);
	free(tmp);

	return;
}

static void dlg_conf_select_node_cb(pcb_hid_attribute_t *attrib, void *hid_ctx, pcb_hid_row_t *row)
{
	pcb_hid_tree_t *tree = (pcb_hid_tree_t *)attrib->enumerations;
	char *end, *end2;
	conf_native_t *nat;

	if (row == NULL) { /* deselect */
		dlg_conf_select_node((pref_ctx_t *)tree->user_ctx, NULL, NULL, 0);
		return;
	}

	end = strrchr(row->path, '/');
	if ((end != NULL) && (end[1] == '[')) {
		int idx = strtol(end+2, &end2, 10);
		/* if last segment is an [integer], it is an array */
		if (*end2 == ']') {
			char tmp[1024];
			int len = end - row->path;
			if ((len <= 0) || (len > sizeof(tmp)-1)) {
				pcb_message(PCB_MSG_WARNING, "Warning: can't show array item %s: path too long\n", row->path);
				return;
			}
			memcpy(tmp, row->path, len);
			tmp[len] = '\0';
			dlg_conf_select_node((pref_ctx_t *)tree->user_ctx, tmp, NULL, idx);
		}
		return;
	}

	/* non-array selection */
	nat = conf_get_field(row->path);
	if ((nat != NULL) && (nat->array_size > 1)) { /* array head: do not display for now */
		dlg_conf_select_node((pref_ctx_t *)tree->user_ctx, NULL, NULL, 0);
		return;
	}
	dlg_conf_select_node((pref_ctx_t *)tree->user_ctx, row->path, nat, 0);
}


void pcb_pref_dlg_conf_changed_cb(pref_ctx_t *ctx, conf_native_t *cfg, int arr_idx)
{
	if (ctx->conf.selected_nat == cfg)
		dlg_conf_select_node(ctx, NULL, cfg, ctx->conf.selected_idx);
}


static void pcb_pref_dlg_conf_filter_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr_inp)
{
	pref_ctx_t *ctx = caller_data;
	pcb_hid_attribute_t *attr;
	pcb_hid_tree_t *tree;
	const char *text;
	int have_filter_text;

	attr = &ctx->dlg[ctx->conf.wtree];
	tree = (pcb_hid_tree_t *)attr->enumerations;
	text = attr_inp->default_val.str_value;
	have_filter_text = (*text != '\0');

	/* hide or unhide everything */
	pcb_dad_tree_hide_all(tree, &tree->rows, have_filter_text);

	if (have_filter_text) /* unhide hits and all their parents */
		pcb_dad_tree_unhide_filter(tree, &tree->rows, 0, text);

	pcb_dad_tree_update_hide(attr);
}


static void build_natval(pref_ctx_t *ctx)
{
	static const char *hdr_nat[] = {"role", "prio", "value", NULL};

	PCB_DAD_BEGIN_TABBED(pref_ctx.dlg, type_tabs);
		PCB_DAD_COMPFLAG(pref_ctx.dlg, PCB_HATF_EXPFILL | PCB_HATF_HIDE_TABLAB);
		ctx->conf.wnattype = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: string");
			PCB_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[0] = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[0] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: boolean");
			PCB_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[1] = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[1] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: integer");
			PCB_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[2] = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[2] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: real");
			PCB_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[3] = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[3] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: coord");
			PCB_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[4] = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[4] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: unit");
			PCB_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[5] = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[5] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "Data type: color");
			PCB_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[6] = PCB_DAD_CURRENT(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[6] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_LABEL(ctx->dlg, "Data type: list of strings");
			ctx->conf.wsrc[7] = -1;
			PCB_DAD_TREE(ctx->dlg, 3, 0, hdr_nat); /* input state */
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
				ctx->conf.wnatval[7] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_LABEL(ctx->dlg, "(no conf node selected)");
			ctx->conf.wnatval[8] = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);
}

void pcb_dlg_pref_conf_create(pref_ctx_t *ctx)
{
	static const char *hdr_intree[] = {"role", "prio", "policy", "value", NULL};

	PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
	PCB_DAD_BEGIN_HPANE(ctx->dlg);
		PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
		ctx->conf.wmainp = PCB_DAD_CURRENT(ctx->dlg);

		/* left: tree */
		PCB_DAD_BEGIN_VBOX(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			PCB_DAD_TREE(ctx->dlg, 1, 1, NULL);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_SCROLL);
				ctx->conf.wtree = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_TREE_SET_CB(ctx->dlg, selected_cb, dlg_conf_select_node_cb);
				PCB_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
			PCB_DAD_STRING(ctx->dlg);
				PCB_DAD_HELP(ctx->dlg, "Filter text:\nlist conf nodes with\nmatching name only");
				PCB_DAD_CHANGE_CB(ctx->dlg, pcb_pref_dlg_conf_filter_cb);
				ctx->conf.wfilter = PCB_DAD_CURRENT(ctx->dlg);
		PCB_DAD_END(ctx->dlg);

		/* right: details */
		PCB_DAD_BEGIN_VPANE(ctx->dlg);
			PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL);
			
			/* right/top: conf file */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_FRAME);
				PCB_DAD_LABEL(ctx->dlg, "");
					ctx->conf.wname = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "");
					ctx->conf.wdesc = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_LABEL(ctx->dlg, "INPUT: configuration node (\"file\" version)");
				PCB_DAD_TREE(ctx->dlg, 4, 0, hdr_intree); /* input state */
					ctx->conf.wintree = PCB_DAD_CURRENT(ctx->dlg);
				PCB_DAD_BUTTON(ctx->dlg, "Edit selected input...");
					PCB_DAD_CHANGE_CB(ctx->dlg, pref_conf_edit_cb);
			PCB_DAD_END(ctx->dlg);

			/* right/bottom: native file */
			PCB_DAD_BEGIN_VBOX(ctx->dlg);
				PCB_DAD_COMPFLAG(ctx->dlg, PCB_HATF_EXPFILL | PCB_HATF_FRAME);
				PCB_DAD_LABEL(ctx->dlg, "NATIVE: in-memory conf node after the merge");
				build_natval(ctx);

			PCB_DAD_END(ctx->dlg);
		PCB_DAD_END(ctx->dlg);
	PCB_DAD_END(ctx->dlg);

	setup_tree(ctx);
	setup_intree(ctx, NULL, 0);
}

void pcb_dlg_pref_conf_open(pref_ctx_t *ctx, const char *tabarg)
{
	pcb_hid_attr_val_t hv;
	hv.real_value = 0.25;
	pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wmainp, &hv);

	if (tabarg != NULL) {
		pcb_hid_attr_val_t hv;
		hv.str_value = pcb_strdup(tabarg);
		pcb_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wfilter, &hv);
		pcb_pref_dlg_conf_filter_cb(ctx->dlg_hid_ctx, ctx, &ctx->dlg[ctx->conf.wfilter]);
		pcb_dad_tree_expcoll(&ctx->dlg[ctx->conf.wtree], NULL, 1, 1);
	}
}


