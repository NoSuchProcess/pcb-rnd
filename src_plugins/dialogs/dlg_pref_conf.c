/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
#include <librnd/core/conf.h>
#include "conf_core.h"
#include <librnd/core/misc_util.h>

static const char *pref_conf_get_val(const lht_node_t *nd, const rnd_conf_native_t *nat, int idx);
#include "dlg_pref_confedit.c"

/* how many chars per line in conf node description (determines window width vs.
   window height */
#define DESC_WRAP_WIDTH 50

static const char *type_tabs[RND_CFN_max+2] = /* MUST MATCH rnd_conf_native_type_t */
	{"STRING", "BOOLEAN", "INTEGER", "REAL", "COORD", "UNIT", "COLOR", "LIST", "invalid", NULL};

static int conf_tree_cmp(const void *v1, const void *v2)
{
	const htsp_entry_t **e1 = (const htsp_entry_t **) v1, **e2 = (const htsp_entry_t **) v2;
	return strcmp((*e1)->key, (*e2)->key);
}

static void setup_tree(pref_ctx_t *ctx)
{
	char *cell[2] = {NULL};
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->conf.wtree];
	rnd_hid_tree_t *tree = attr->wdata;
	htsp_entry_t *e;
	htsp_entry_t **sorted;
	int num_paths, n;
	char path[1024];

	/* alpha sort keys for the more consistend UI */
	for(e = htsp_first(rnd_conf_fields), num_paths = 0; e; e = htsp_next(rnd_conf_fields, e))
		num_paths++;
	sorted = malloc(sizeof(htsp_entry_t *) * num_paths);
	for(e = htsp_first(rnd_conf_fields), n = 0; e; e = htsp_next(rnd_conf_fields, e), n++)
		sorted[n] = e;
	qsort(sorted, num_paths, sizeof(htsp_entry_t *), conf_tree_cmp);

	for(n = 0; n < num_paths; n++) {
		char *basename, *bnsep;
		rnd_hid_row_t *parent;
		rnd_conf_native_t *nat;

		e = sorted[n];
		if (strlen(e->key) > sizeof(path) - 1) {
			rnd_message(RND_MSG_WARNING, "Warning: can't create config item for %s: path too long\n", e->key);
			continue;
		}
		strcpy(path, e->key);
		basename = strrchr(path, '/');
		if ((basename == NULL) || (basename == path)) {
			rnd_message(RND_MSG_WARNING, "Warning: can't create config item for %s: invalid path (node in root)\n", e->key);
			continue;
		}
		bnsep = basename;
		*basename = '\0';
		basename++;

		parent = rnd_dad_tree_mkdirp(tree, path, NULL);
		if (parent == NULL) {
			rnd_message(RND_MSG_WARNING, "Warning: can't create config item for %s: invalid path\n", e->key);
			continue;
		}
		
		nat = e->value;
		if (nat->array_size > 1) {
			int i;
			*bnsep = '/';
			parent = rnd_dad_tree_mkdirp(tree, path, NULL);
			for(i = 0; i < nat->array_size; i++) {
				cell[0] = rnd_strdup_printf("[%d]", i);
				rnd_dad_tree_append_under(attr, parent, cell);
			}
		}
		else {
			cell[0] = rnd_strdup(basename);
			rnd_dad_tree_append_under(attr, parent, cell);
		}
	}
	free(sorted);
}

static const char *pref_conf_get_val(const lht_node_t *nd, const rnd_conf_native_t *nat, int idx)
{
	lht_node_t *ni;

	switch (nd->type) {
		case LHT_TEXT: return nd->data.text.value; break;
		case LHT_LIST:
			if (nat->array_size > 1) {
				int idx2 = idx;
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
	return "<invalid-type>";
}

static void setup_intree(pref_ctx_t *ctx, rnd_conf_native_t *nat, int idx)
{
	rnd_conf_role_t n;
	rnd_hid_attribute_t *attr = &ctx->dlg[ctx->conf.wintree];
	rnd_hid_tree_t *tree = attr->wdata;
	rnd_hid_row_t *r;

	rnd_dad_tree_clear(tree);

	for(n = 0; n < RND_CFR_max_real; n++) {
		char *cell[5]= {NULL};
		cell[0] = rnd_strdup(rnd_conf_role_name(n));
		if (nat != NULL) {
			lht_node_t *nd;
			long prio = rnd_conf_default_prio[n];
			rnd_conf_policy_t pol = RND_POL_OVERWRITE;

			nd = rnd_conf_lht_get_at_mainplug(n, nat->hash_path, 1, 0);
			if (nd != NULL) { /* role, prio, policy, value */
				rnd_conf_get_policy_prio(nd, &pol, &prio);
				cell[1] = rnd_strdup_printf("%ld", prio);
				cell[2] = rnd_strdup(rnd_conf_policy_name(pol));
				cell[3] = rnd_strdup(pref_conf_get_val(nd, nat, idx));
			}
		}
		r = rnd_dad_tree_append(attr, NULL, cell);
		r->user_data2.lng = n;
	}
}

static const char *print_conf_val(rnd_conf_native_type_t type, const rnd_confitem_t *val, char *buf, int sizebuf)
{
	const char *ret = buf;

	*buf = '\0';
	switch(type) {
		case RND_CFN_STRING:   if (*val->string != NULL) ret = *val->string; break;
		case RND_CFN_BOOLEAN:  strcpy(buf, *val->boolean ? "true" : "false"); break;
		case RND_CFN_INTEGER:  sprintf(buf, "%ld", *val->integer); break;
		case RND_CFN_REAL:     sprintf(buf, "%f", *val->real); break;
		case RND_CFN_COORD:    rnd_snprintf(buf, sizebuf, "%mH\n%mm\n%ml", *val->coord, *val->coord, *val->coord); break;
		case RND_CFN_UNIT:     strcpy(buf, (*val->unit)->suffix); break;
		case RND_CFN_COLOR:    strcpy(buf, val->color->str); break;
		case RND_CFN_LIST:     strcpy(buf, "<list>"); break;
		case RND_CFN_HLIST:    strcpy(buf, "<hlist>"); break;
		case RND_CFN_max:      strcpy(buf, "<invalid-type>"); break;
	}
	return ret;
}

static void dlg_conf_select_node(pref_ctx_t *ctx, const char *path, rnd_conf_native_t *nat, int idx)
{
	rnd_hid_attr_val_t hv;
	char *tmp, buf[128];
	const char *rolename;
	lht_node_t *src;

	if ((path != NULL) && (nat == NULL))
		nat = rnd_conf_get_field(path);

	if ((path == NULL) && (nat != NULL))
		path = nat->hash_path;

	ctx->conf.selected_nat = nat;
	ctx->conf.selected_idx = idx;

	if (nat == NULL) {
		hv.str = "";
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wname, &hv);
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wdesc, &hv);
		setup_intree(ctx, NULL, 0);

		hv.lng = RND_CFN_max;
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wnattype, &hv);

		return;
	}

	hv.str = path == NULL ? "" : path;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wname, &hv);

	tmp = rnd_strdup(nat->description);
	rnd_text_wrap(tmp, DESC_WRAP_WIDTH, '\n', ' ');
	hv.str = tmp;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wdesc, &hv);
	free(tmp);

	/* display lht value */
	setup_intree(ctx, nat, idx);

	/* display native value */
	hv.lng = nat->type;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wnattype, &hv);

	if ((nat->type == RND_CFN_LIST) || (nat->type == RND_CFN_HLIST)) {
		/* non-default: lists are manually loaded */
		rnd_hid_attribute_t *attr = &ctx->dlg[ctx->conf.wnatval[nat->type]];
		rnd_hid_tree_t *tree = attr->wdata;
		rnd_conf_listitem_t *n;
		char *cell[4];

		rnd_dad_tree_clear(tree);
		for (n = rnd_conflist_first(&nat->val.list[idx]); n != NULL; n = rnd_conflist_next(n)) {
			const char *strval;
			rolename = rnd_conf_role_name(rnd_conf_lookup_role(n->prop.src));
			if (nat->type == RND_CFN_HLIST)
				strval = n->name;
			else
				strval = print_conf_val(n->type, &n->val, buf, sizeof(buf));

			cell[0] = rolename == NULL ? rnd_strdup("") : rnd_strdup(rolename);
			cell[1] = rnd_strdup_printf("%ld", n->prop.prio);
			cell[2] = rnd_strdup(strval);
			cell[3] = 0;
			rnd_dad_tree_append(attr, NULL, cell);
		}
		return;
	}

	/* default: set the value of the given node from hv loaded above */
	hv.str = print_conf_val(nat->type, &nat->val, buf, sizeof(buf));
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wnatval[nat->type], &hv);

	src = nat->prop[idx].src;
	if (src != NULL) {
		rolename = rnd_conf_role_name(rnd_conf_lookup_role(nat->prop[idx].src));
		hv.str = tmp = rnd_strdup_printf("prio: %d role: %s\nsource: %s:%d.%d", nat->prop[idx].prio, rolename, src->file_name, src->line, src->col);
	}
	else
		hv.str = tmp = rnd_strdup_printf("prio: %d\nsource: <not saved>", nat->prop[idx].prio);
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wsrc[nat->type], &hv);
	free(tmp);

	return;
}

static void dlg_conf_select_node_cb(rnd_hid_attribute_t *attrib, void *hid_ctx, rnd_hid_row_t *row)
{
	rnd_hid_tree_t *tree = attrib->wdata;
	char *end, *end2;
	rnd_conf_native_t *nat;

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
				rnd_message(RND_MSG_WARNING, "Warning: can't show array item %s: path too long\n", row->path);
				return;
			}
			memcpy(tmp, row->path, len);
			tmp[len] = '\0';
			dlg_conf_select_node((pref_ctx_t *)tree->user_ctx, tmp, NULL, idx);
		}
		return;
	}

	/* non-array selection */
	nat = rnd_conf_get_field(row->path);
	if ((nat != NULL) && (nat->array_size > 1)) { /* array head: do not display for now */
		dlg_conf_select_node((pref_ctx_t *)tree->user_ctx, NULL, NULL, 0);
		return;
	}
	dlg_conf_select_node((pref_ctx_t *)tree->user_ctx, row->path, nat, 0);
}


void pcb_pref_dlg_conf_changed_cb(pref_ctx_t *ctx, rnd_conf_native_t *cfg, int arr_idx)
{
	if (ctx->conf.selected_nat == cfg)
		dlg_conf_select_node(ctx, NULL, cfg, ctx->conf.selected_idx);
}


static void pcb_pref_dlg_conf_filter_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr_inp)
{
	pref_ctx_t *ctx = caller_data;
	rnd_hid_attribute_t *attr;
	rnd_hid_tree_t *tree;
	const char *text;
	int have_filter_text;

	attr = &ctx->dlg[ctx->conf.wtree];
	tree = attr->wdata;
	text = attr_inp->val.str;
	have_filter_text = (*text != '\0');

	/* hide or unhide everything */
	rnd_dad_tree_hide_all(tree, &tree->rows, have_filter_text);

	if (have_filter_text) /* unhide hits and all their parents */
		rnd_dad_tree_unhide_filter(tree, &tree->rows, 0, text);

	rnd_dad_tree_update_hide(attr);
}


static void build_natval(pref_ctx_t *ctx)
{
	static const char *hdr_nat[] = {"role", "prio", "value", NULL};

	RND_DAD_BEGIN_TABBED(pref_ctx.dlg, type_tabs);
		RND_DAD_COMPFLAG(pref_ctx.dlg, RND_HATF_EXPFILL | RND_HATF_HIDE_TABLAB);
		ctx->conf.wnattype = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: string");
			RND_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[0] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[0] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: boolean");
			RND_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[1] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[1] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: integer");
			RND_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[2] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[2] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: real");
			RND_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[3] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[3] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: coord");
			RND_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[4] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[4] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: unit");
			RND_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[5] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[5] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "Data type: color");
			RND_DAD_LABEL(ctx->dlg, "role/prio");
				ctx->conf.wsrc[6] = RND_DAD_CURRENT(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(data)");
				ctx->conf.wnatval[6] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_LABEL(ctx->dlg, "Data type: list of strings");
			ctx->conf.wsrc[7] = -1;
			RND_DAD_TREE(ctx->dlg, 3, 0, hdr_nat); /* input state */
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
				ctx->conf.wnatval[7] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_LABEL(ctx->dlg, "Data type: list of hash subtrees");
			ctx->conf.wsrc[8] = -1;
			RND_DAD_TREE(ctx->dlg, 3, 0, hdr_nat); /* input state */
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
				ctx->conf.wnatval[8] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_LABEL(ctx->dlg, "(no conf node selected)");
			ctx->conf.wnatval[9] = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);
}

void pcb_dlg_pref_conf_create(pref_ctx_t *ctx)
{
	static const char *hdr_intree[] = {"role", "prio", "policy", "value", NULL};

	RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
	RND_DAD_BEGIN_HPANE(ctx->dlg);
		RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
		ctx->conf.wmainp = RND_DAD_CURRENT(ctx->dlg);

		/* left: tree */
		RND_DAD_BEGIN_VBOX(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			RND_DAD_TREE(ctx->dlg, 1, 1, NULL);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_SCROLL);
				ctx->conf.wtree = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_TREE_SET_CB(ctx->dlg, selected_cb, dlg_conf_select_node_cb);
				RND_DAD_TREE_SET_CB(ctx->dlg, ctx, ctx);
			RND_DAD_STRING(ctx->dlg);
				RND_DAD_HELP(ctx->dlg, "Filter text:\nlist conf nodes with\nmatching name only");
				RND_DAD_CHANGE_CB(ctx->dlg, pcb_pref_dlg_conf_filter_cb);
				ctx->conf.wfilter = RND_DAD_CURRENT(ctx->dlg);
		RND_DAD_END(ctx->dlg);

		/* right: details */
		RND_DAD_BEGIN_VPANE(ctx->dlg);
			RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL);
			
			/* right/top: conf file */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_FRAME);
				RND_DAD_LABEL(ctx->dlg, "");
					ctx->conf.wname = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "");
					ctx->conf.wdesc = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_LABEL(ctx->dlg, "INPUT: configuration node (\"file\" version)");
				RND_DAD_TREE(ctx->dlg, 4, 0, hdr_intree); /* input state */
					ctx->conf.wintree = RND_DAD_CURRENT(ctx->dlg);
				RND_DAD_BEGIN_HBOX(ctx->dlg);
					RND_DAD_BUTTON(ctx->dlg, "Edit selected...");
						RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_edit_cb);
					RND_DAD_BUTTON(ctx->dlg, "Remove selected");
						RND_DAD_CHANGE_CB(ctx->dlg, pref_conf_del_cb);
				RND_DAD_END(ctx->dlg);
			RND_DAD_END(ctx->dlg);

			/* right/bottom: native file */
			RND_DAD_BEGIN_VBOX(ctx->dlg);
				RND_DAD_COMPFLAG(ctx->dlg, RND_HATF_EXPFILL | RND_HATF_FRAME);
				RND_DAD_LABEL(ctx->dlg, "NATIVE: in-memory conf node after the merge");
				build_natval(ctx);

			RND_DAD_END(ctx->dlg);
		RND_DAD_END(ctx->dlg);
	RND_DAD_END(ctx->dlg);

	setup_tree(ctx);
	setup_intree(ctx, NULL, 0);
}

void pcb_dlg_pref_conf_open(pref_ctx_t *ctx, const char *tabarg)
{
	rnd_hid_attr_val_t hv;
	hv.dbl = 0.25;
	rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wmainp, &hv);

	if (tabarg != NULL) {
		rnd_hid_attr_val_t hv;
		hv.str = rnd_strdup(tabarg);
		rnd_gui->attr_dlg_set_value(ctx->dlg_hid_ctx, ctx->conf.wfilter, &hv);
		pcb_pref_dlg_conf_filter_cb(ctx->dlg_hid_ctx, ctx, &ctx->dlg[ctx->conf.wfilter]);
		rnd_dad_tree_expcoll(&ctx->dlg[ctx->conf.wtree], NULL, 1, 1);
	}
}


