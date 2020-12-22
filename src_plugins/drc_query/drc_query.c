/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#include <stdlib.h>
#include <genht/hash.h>
#include <liblihata/dom.h>
#include <liblihata/tree.h>
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/error.h>
#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/list_conf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>

#include "board.h"
#include "drc.h"
#include "view.h"
#include "conf_core.h"
#include "find.h"
#include "event.h"

#include "drc_query_conf.h"
#include "../src_plugins/drc_query/conf_internal.c"
#include "../src_plugins/query/query.h"
#include "../src_plugins/query/query_exec.h"


static void drc_rlist_pcb2dlg(void);

#include "drc_query_stat.c"

static rnd_conf_hid_id_t cfgid;

static const char *drc_query_cookie = "drc_query";

extern conf_drc_query_t conf_drc_query;
#define DRC_QUERY_CONF_FN "drc_query.conf"

#define DRC_CONF_PATH_PLUGIN "plugins/drc_query/"
#define DRC_CONF_PATH_DISABLE "design/drc_disable/"
#define DRC_CONF_PATH_CONST "design/drc/"
#define DRC_CONF_PATH_RULES "plugins/drc_query/rules/"
#define DRC_CONF_PATH_DEFS "plugins/drc_query/definitions/"

typedef struct {
	pcb_board_t *pcb;
	pcb_view_list_t *lst;
	const char *type;
	const char *title;
	const char *desc;
	long hit_cnt;
} drc_qry_ctx_t;

static fgw_arg_t load_obj_const(pcb_obj_qry_const_t *cnst)
{
	fgw_arg_t a;

	switch(cnst->val.type) {
		case PCBQ_VT_COORD:
			a.type = FGW_COORD;
			fgw_coord(&a) = cnst->val.data.crd;
			return a;
		case PCBQ_VT_LONG:
			a.type = FGW_LONG;
			a.val.nat_long = cnst->val.data.lng;
			return a;
		case PCBQ_VT_DOUBLE:
			a.type = FGW_DOUBLE;
			a.val.nat_double = cnst->val.data.dbl;
			return a;

		case PCBQ_VT_STRING:
			a.type = FGW_STR | FGW_DYN;
			a.val.str = rnd_strdup(cnst->val.data.str);
			return a;

		case PCBQ_VT_VOID:
		case PCBQ_VT_OBJ:
		case PCBQ_VT_LST:
			break;
	}

	a.type = FGW_INVALID;
	return a;
}

void drc_qry_exec_cb(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current)
{
	drc_qry_ctx_t *qctx = user_ctx;
	pcb_view_t *violation;
	int bv;

	if (res->type == PCBQ_VT_COORD)
		bv = res->data.crd != 0;
	else if (res->type == PCBQ_VT_LONG)
		bv = res->data.lng != 0;
	else if (res->type == PCBQ_VT_LST)
		bv = res->data.lst.used > 0;
	else
		return;

	if (!bv)
		return;

	violation = pcb_view_new(&qctx->pcb->hidlib, qctx->type, qctx->title, qctx->desc);
	if (res->type == PCBQ_VT_LST) {
		int i;
		fgw_arg_t *expv = NULL, expv_, *mesv = NULL, mesv_;
		for(i = 0; i < res->data.lst.used-1; i+=2) {
			pcb_any_obj_t *cmd = res->data.lst.array[i], *obj = res->data.lst.array[i+1];
			pcb_qry_drc_ctrl_t ctrl = pcb_qry_drc_ctrl_decode(cmd);
			pcb_obj_qry_const_t *cnst = res->data.lst.array[i+1];
			
			switch(ctrl) {
				case PCB_QRY_DRC_GRP1:     pcb_view_append_obj(violation, 0, obj); break;
				case PCB_QRY_DRC_GRP2:     pcb_view_append_obj(violation, 1, obj); break;
				case PCB_QRY_DRC_EXPECT:   expv = &expv_; expv_ = load_obj_const(cnst); break;
				case PCB_QRY_DRC_MEASURE:  mesv = &mesv_; mesv_ = load_obj_const(cnst); break;
				case PCB_QRY_DRC_TEXT:     pcb_view_append_text(violation, (char *)cnst);
				case PCB_QRY_DRC_invalid:
					break;
			}
		}

		if (expv != NULL)
			pcb_drc_set_data(violation, mesv, *expv);
	}
	else
		pcb_view_append_obj(violation, 0, (pcb_any_obj_t *)current);
	pcb_view_set_bbox_by_objs(qctx->pcb->Data, violation);
	pcb_view_list_append(qctx->lst, violation);
	qctx->hit_cnt++;
}

typedef struct {
	const char *name;
	long script_total, script_at;
	void *dialog;
	drc_qry_ctx_t *qctx;
	unsigned cancel:1;
} drc_query_prog_t;

static long drc_qry_exec(pcb_qry_exec_t *ec, pcb_board_t *pcb, pcb_view_list_t *lst, const char *name, const char *type, const char *title, const char *desc, const char *query)
{
	const char *scope = NULL;
	drc_query_prog_t *prog = ec->progress_ctx;
	drc_qry_ctx_t qctx;
	pcb_drcq_stat_t *st;
	double ts, te;

	if ((prog != 0) && (prog->cancel))
		return 0;

	if (query == NULL) {
		rnd_message(RND_MSG_ERROR, "drc_query: igoring rule with no query string:%s\n", name);
		return 0;
	}
	if (type == NULL) type = "DRC violation";
	if (title == NULL) title = "Unspecified error";
	if (desc == NULL) desc = "n/a";

	qctx.pcb = pcb;
	qctx.lst = lst;
	qctx.type = type;
	qctx.title = title;
	qctx.desc = desc;
	qctx.hit_cnt = 0;
	if (prog != NULL)
		prog->qctx = &qctx;

	st = pcb_drcq_stat_get(name);

	ts = rnd_dtime();
	pcb_qry_run_script(ec, pcb, query, scope, drc_qry_exec_cb, &qctx);
	te = rnd_dtime();

	st->last_run_time = te - ts;
	st->sum_run_time += te - ts;
	st->run_cnt++;
	st->last_hit_cnt = qctx.hit_cnt;
	st->sum_hit_cnt += qctx.hit_cnt;
	if (prog != NULL)
		prog->qctx = 0;
	return 0;
}

static const char *load_str(lht_node_t *rule, rnd_conf_listitem_t *i, const char *name)
{
	lht_node_t *n = lht_dom_hash_get(rule, name);
	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		rnd_message(RND_MSG_ERROR, "drc_query: igoring non-text node %s of rule %s \n", name, i->name);
		return NULL;
	}
	return n->data.text.value;
}

static int *drc_get_disable(const char *name)
{
	char *path = rnd_concat(DRC_CONF_PATH_DISABLE, name, NULL);
	rnd_conf_native_t *nat = rnd_conf_get_field(path);
	free(path);
	if ((nat == NULL) || (nat->type != RND_CFN_BOOLEAN))
		return NULL;
	return nat->val.boolean;
}


static int drc_query_progress(pcb_qry_exec_t *ec, long at, long total);

static void pcb_drc_query(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	gdl_iterator_t it;
	rnd_conf_listitem_t *i;
	long cnt = 0;
	int bufno = -1, *ran;
	pcb_qry_exec_t ec;
	drc_query_prog_t prog;

	if (conf_drc_query.plugins.drc_query.disable)
		return;

	pcb_qry_init(&ec, pcb, NULL, bufno);
	ec.progress_cb = drc_query_progress;
	ec.progress_ctx = &prog;

	prog.script_total = rnd_conflist_length(&conf_drc_query.plugins.drc_query.rules);
	prog.script_at = 0;
	prog.dialog = NULL;
	prog.cancel = 0;

	rnd_conflist_foreach(&conf_drc_query.plugins.drc_query.rules, &it, i) {
		lht_node_t *rule = i->prop.src;
		int *dis;
		if (rule->type != LHT_HASH) {
			rnd_message(RND_MSG_ERROR, "drc_query: rule %s is not a hash\n", i->name);
			continue;
		}

		dis = drc_get_disable(i->name);
		if ((dis != NULL) && (*dis != 0))
			continue;

		assert(argv[1].type == RND_EVARG_PTR);
		ran = argv[1].d.p;
		(*ran)++;

		prog.name = i->name;
		cnt += drc_qry_exec(&ec, pcb, &pcb_drc_lst, i->name,
			load_str(rule, i, "type"), load_str(rule, i, "title"), load_str(rule, i, "desc"),
			load_str(rule, i, "query")
		);
		prog.script_at++;
	}

	drc_query_progress(&ec, -1, -1);

	pcb_qry_uninit(&ec);
	drc_rlist_pcb2dlg(); /* for the run time */
}

static void drc_query_refresh_def(void);
static void drc_query_defchg(rnd_conf_native_t *cfg, int v)
{
	drc_query_refresh_def();
}

static vtp0_t free_drc_conf_nodes;
static rnd_conf_native_t *nat_defs = NULL;
static rnd_conf_native_t *nat_rules = NULL;
static void drc_query_newconf(rnd_conf_native_t *cfg, rnd_conf_listitem_t *i)
{
	if (nat_rules == NULL) {
		if (strncmp(cfg->hash_path, DRC_CONF_PATH_RULES, strlen(DRC_CONF_PATH_RULES)-1) == 0) {
			nat_rules = cfg;
			nat_rules->gui_edit_act = "DrcQueryEditRule";
		}
	}

	if (nat_defs == NULL) {
		if (strncmp(cfg->hash_path, DRC_CONF_PATH_DEFS, strlen(DRC_CONF_PATH_DEFS)-1) != 0)
			return;
		nat_defs = cfg;
	}

	if (nat_rules == cfg) {
		lht_node_t *nd = i->prop.src;
		char *path = rnd_concat(DRC_CONF_PATH_DISABLE, nd->name, NULL);

		if (rnd_conf_get_field(path) == NULL) {
			const char *sdesc;
			rnd_conf_native_t *nat;
			rnd_bool_t *b;
			lht_node_t *ndesc;

			ndesc = lht_dom_hash_get(nd, "desc");
			if ((ndesc != NULL) && (ndesc->type == LHT_TEXT)) sdesc = ndesc->data.text.value;

			b = calloc(sizeof(rnd_bool_t), 1);
			nat = rnd_conf_reg_field_(b, 1, RND_CFN_BOOLEAN, path, rnd_strdup(sdesc), 0);
			if (nat == NULL) {
				free(b);
				rnd_message(RND_MSG_ERROR, "drc_query: failed to register conf node '%s'\n", path);
				goto fail;
			}

			nat->random_flags.dyn_hash_path = 1;
			nat->random_flags.dyn_desc = 1;
			nat->random_flags.dyn_val = 1;
			vtp0_append(&free_drc_conf_nodes, nat);
			rnd_conf_set(RND_CFR_INTERNAL, path, -1, "0", RND_POL_OVERWRITE);
		}
		else
			free(path);
	}
	else if (nat_defs == cfg) {
		lht_node_t *nd = i->prop.src;
		char *path = rnd_concat(DRC_CONF_PATH_CONST, nd->name, NULL);
		rnd_coord_t *c;
		rnd_conf_native_t *nat = rnd_conf_get_field(path);

		if ((nat == NULL) || (nat->used == 0)) { /* create the definition and set default value (but only once) */
			union {
				rnd_coord_t c;
				double d;
				void *ptr;
				char *str;
			} anyval;
			lht_node_t *ndesc = lht_dom_hash_get(nd, "desc");
			lht_node_t *ntype = lht_dom_hash_get(nd, "type");
			lht_node_t *ndefault = lht_dom_hash_get(nd, "default");
			lht_node_t *nlegacy = lht_dom_hash_get(nd, "legacy");
			const char *sdesc = "n/a", *stype = NULL, *sdefault = NULL, *slegacy = NULL;
			rnd_conf_native_type_t type;
			int pathfree = 1;

			if ((ndesc != NULL) && (ndesc->type == LHT_TEXT)) sdesc = ndesc->data.text.value;
			if ((ntype != NULL) && (ntype->type == LHT_TEXT)) stype = ntype->data.text.value;
			if ((ndefault != NULL) && (ndefault->type == LHT_TEXT)) sdefault = ndefault->data.text.value;
			if ((nlegacy != NULL) && (nlegacy->type == LHT_TEXT)) slegacy = nlegacy->data.text.value;

			if (stype == NULL) {
				if ((ndesc != NULL) || (sdefault != NULL))
					rnd_message(RND_MSG_ERROR, "drc_query: missing type field for constant %s\n", nd->name);
				goto fail;
			}

			type = rnd_conf_native_type_parse(stype);
			if (type >= RND_CFN_LIST) {
				rnd_message(RND_MSG_ERROR, "drc_query: invalid type '%s' for %s\n", stype, nd->name);
				goto fail;
			}

			/* create the new conf node for the def if it doesn't already exist */
			if (nat == NULL) {
				c = calloc(sizeof(anyval), 1);
				pathfree = 0;
				nat = rnd_conf_reg_field_(c, 1, type, path, rnd_strdup(sdesc), 0);
				if (nat == NULL) {
					free(c);
					rnd_message(RND_MSG_ERROR, "drc_query: failed to register conf node '%s'\n", path);
					goto fail;
				}

				nat->random_flags.dyn_hash_path = 1;
				nat->random_flags.dyn_desc = 1;
				nat->random_flags.dyn_val = 1;
				vtp0_append(&free_drc_conf_nodes, nat);
			}

			/* set default value */
			if (slegacy != NULL)
				pcb_conf_legacy(path, slegacy);
			else if (sdefault != NULL)
				rnd_conf_set(RND_CFR_INTERNAL, path, -1, sdefault, RND_POL_OVERWRITE);

			/* create the chnage binding so the gui can be updated */
			{
				static rnd_conf_hid_callbacks_t cbs2;
				cbs2.val_change_post = drc_query_defchg;
				rnd_conf_hid_set_cb(rnd_conf_get_field(path), cfgid, &cbs2);
			}

			if (!pathfree)
				path = NULL; /* hash key shall not be free'd */
		}
		fail:;
		free(path);
	}
}

static const char *textval_empty(lht_node_t *nd, const char *fname)
{
	lht_node_t *nt = lht_dom_hash_get(nd, fname);

	if ((nt == NULL) || (nt->type != LHT_TEXT))
		return "";

	return nt->data.text.value;
}

static const char *textval_null(lht_node_t *nd, const char *fname)
{
	lht_node_t *nt = lht_dom_hash_get(nd, fname);

	if ((nt == NULL) || (nt->type != LHT_TEXT))
		return NULL;

	return nt->data.text.value;
}

#define MKDIR_ND(outnode, parent, ntype, nname, errinstr) \
do { \
	lht_node_t *nnew; \
	lht_err_t err; \
	char *nname0 = (char *)nname; \
	if (parent->type == LHT_LIST) nname0 = rnd_concat(nname, ":0", NULL); \
	nnew = lht_tree_path_(parent->doc, parent, nname0, 1, 1, &err); \
	if (parent->type == LHT_LIST) free(nname0); \
	if ((nnew != NULL) && (nnew->type != ntype)) { \
		rnd_message(RND_MSG_ERROR, "Internal error: invalid existing node type for %s: %d, rule is NOT saved\n", nname, nnew->type); \
		errinstr; \
	} \
	else if (nnew == NULL) { \
		nnew = lht_dom_node_alloc(ntype, nname); \
		switch(parent->type) { \
			case LHT_HASH: err = lht_dom_hash_put(parent, nnew); break; \
			case LHT_LIST: err = lht_dom_list_append(parent, nnew); break; \
			default: \
				rnd_message(RND_MSG_ERROR, "Internal error: invalid parent node type for %s: %d, rule is NOT saved\n", parent->name, parent->type); \
				errinstr; \
		} \
	} \
	outnode = nnew; \
} while(0)

#define MKDIR_ND_SET_TEXT(parent, nname, nval, errinstr) \
do { \
	lht_node_t *ntxt; \
	MKDIR_ND(ntxt, parent, LHT_TEXT, nname, errinstr); \
	if (ntxt == NULL) { \
		rnd_message(RND_MSG_ERROR, "Internal error: new text node for %s is NULL, rule is NOT saved\n", nname); \
		errinstr; \
	} \
	free(ntxt->data.text.value); \
	ntxt->data.text.value = rnd_strdup(nval == NULL ? "" : nval); \
} while(0)

#define MKDIR_RULES(nd, errinstr) \
do { \
	MKDIR_ND(nd, nd, LHT_HASH, "plugins", errinstr); \
	MKDIR_ND(nd, nd, LHT_HASH, "drc_query", errinstr); \
	MKDIR_ND(nd, nd, LHT_LIST, "rules", errinstr); \
} while(0)

#define MKDIR_CONSTS(nd, errinstr) \
do { \
	MKDIR_ND(nd, nd, LHT_HASH, "plugins", errinstr); \
	MKDIR_ND(nd, nd, LHT_HASH, "drc_query", errinstr); \
	MKDIR_ND(nd, nd, LHT_LIST, "definitions", errinstr); \
} while(0)

#define MKDIR_RULE_ROOT(nd, role, pol, errinst) \
do { \
	nd = rnd_conf_lht_get_first_crpol(role, pol, 1); \
	if (nd == NULL) { \
		rnd_message(RND_MSG_ERROR, "Internal error: failed to create role root, rule is NOT saved\n"); \
		errinst; \
	} \
} while(0)

#define DRC_QUERY_RULE_OR_DEF(is_rule) \
	((is_rule) ? &conf_drc_query.plugins.drc_query.rules : &conf_drc_query.plugins.drc_query.definitions)

static int pcb_drc_query_any_by_name(const char *name, int is_rule, lht_node_t **nd_out, int do_create)
{
	lht_node_t *n = NULL, *nd = NULL;
	int ret = -1, needs_update = 0;
	const rnd_conflist_t *l = DRC_QUERY_RULE_OR_DEF(is_rule);
	gdl_iterator_t it;
	rnd_conf_listitem_t *i;

	rnd_conflist_foreach(l, &it, i) {
		n = i->prop.src;
		if ((n != NULL) && (strcmp(n->name, name) == 0)) {
			nd = n;
			break;
		}
	}

	if (!do_create) {
		if (nd != NULL)
			ret = 0;
	}
	else {
		if (nd == NULL) { /* allocate new node */
			ret = 0;
			MKDIR_RULE_ROOT(nd, RND_CFR_DESIGN, RND_POL_APPEND, goto skip);
			if (is_rule)
				MKDIR_RULES(nd, goto skip);
			else
				MKDIR_CONSTS(nd, goto skip);
			MKDIR_ND(nd, nd, LHT_HASH, name, goto skip);
			if (nd == NULL) /* failed to create */
				ret = -1;
			else
				needs_update = 1;
		}
		else { /* failed to allocate because it exists: return error, but also set the output */
			ret = -1;
		}
	}

	if (needs_update)
		rnd_conf_update(NULL, -1);

	skip:;
	if (nd_out != NULL)
		*nd_out = nd;

	return ret;
}

static int pcb_drc_query_rule_by_name(const char *name, lht_node_t **nd_out, int do_create)
{
	return pcb_drc_query_any_by_name(name, 1, nd_out, do_create);
}

static int pcb_drc_query_def_by_name(const char *name, lht_node_t **nd_out, int do_create)
{
	return pcb_drc_query_any_by_name(name, 0, nd_out, do_create);
}


static int pcb_drc_query_clear(rnd_hidlib_t *hidlib, int is_rule, const char *src)
{
	const rnd_conflist_t *l = DRC_QUERY_RULE_OR_DEF(is_rule);
	gdl_iterator_t it;
	rnd_conf_listitem_t *i;
	int needs_update = 0;

	rnd_conflist_foreach(l, &it, i) {
		const char *nsrc;
		lht_node_t *n = i->prop.src;
		if (n == NULL)
			continue;
		nsrc = textval_null(n, "src");
		if (nsrc == NULL)
			continue;
		if (strcmp(nsrc, src) == 0) {
			needs_update = 1;
			i->prop.src = NULL;
			lht_tree_del(n);
		}
	}

	if (needs_update)
		rnd_conf_update(NULL, -1);

	return -1;
}

static int pcb_drc_query_create(rnd_hidlib_t *hidlib, int is_rule, const char *rule)
{
	lht_node_t *nd;

	if (pcb_drc_query_any_by_name(rule, is_rule, &nd, 1) != 0)
		return -1; /* do not re-create existing rule, force creating a new */

	return 0;
}

void d2() {}

static int pcb_drc_query_set(rnd_hidlib_t *hidlib, int is_rule, const char *rule, const char *key, const char *val)
{
	lht_node_t *nd;

	if (pcb_drc_query_any_by_name(rule, is_rule, &nd, 0) != 0)
		return -1;

	MKDIR_ND_SET_TEXT(nd, key, val, return -1);

	/* copy default value when set in a separate step, after node creation */
	if (!is_rule && (nat_defs != NULL)) {
		char *path = rnd_concat(DRC_CONF_PATH_CONST, nd->name, NULL);
		rnd_conf_native_t *nat = rnd_conf_get_field(path);

		/* set value only if it's a new node, so existing value is not overwritten */
		if ((nat == NULL) || (nat->used == 0)) {
			rnd_conf_listitem_t i = {0};
			i.prop.src = nd;
			drc_query_newconf(nat_defs, &i);
		}
		free(path);
	}

	return 0;
}

static int pcb_drc_query_get(rnd_hidlib_t *hidlib, int is_rule, const char *rule, const char *key, fgw_arg_t *res)
{
	lht_node_t *nd;

	if (pcb_drc_query_any_by_name(rule, is_rule, &nd, 0) != 0)
		return -1;

	res->type = FGW_STR;
	res->val.cstr = textval_empty(nd, key);

	return 0;
}

static int pcb_drc_query_get_rule_defs(rnd_hidlib_t *hidlib, const char *rule, fgw_arg_t *res)
{
	htsi_t defs;
	htsi_entry_t *e;
	gds_t lst;
	fgw_arg_t tmp;
	if (pcb_drc_query_get(hidlib, 1, rule, "query", &tmp) != 0)
		return -1;

	gds_init(&lst);
	htsi_init(&defs, strhash, strkeyeq);
	pcb_qry_extract_defs(&defs, tmp.val.str);
	for(e = htsi_first(&defs); e != NULL; e = htsi_next(&defs, e)) {
		if (lst.used > 0)
			gds_append(&lst, '\n');
		gds_append_str(&lst, e->key);
		free(e->key);
	}
	htsi_uninit(&defs);

	if (lst.used == 0) {
		gds_uninit(&lst);
		res->type = FGW_STR;
		res->val.cstr = "";
		return 0;
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = lst.array;
	return 0;
}


static const char pcb_acts_DrcQueryRuleMod[] = \
	"DrcQueryRuleMod(clear, source)\n"
	"DrcQueryRuleMod(create, rule_name)\n"
	"DrcQueryRuleMod(get, rule_name, field_name)\n"
	"DrcQueryRuleMod(set, rule_name, field_name, value)\n";
static const char pcb_acth_DrcQueryRuleMod[] = "Automated DRC rule editing (for scripting and import)";
static fgw_error_t pcb_act_DrcQueryRuleMod(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *target, *key = NULL, *val=NULL;
	rnd_hidlib_t *hl = RND_ACT_HIDLIB;
	int resi = -1;

	RND_ACT_CONVARG(1, FGW_STR, DrcQueryRuleMod, cmd = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, DrcQueryRuleMod, target = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, DrcQueryRuleMod, key = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, DrcQueryRuleMod, val = argv[4].val.str);

	if (strcmp(cmd, "clear") == 0) resi = pcb_drc_query_clear(hl, 1, target);
	else if (strcmp(cmd, "create") == 0) resi = pcb_drc_query_create(hl, 1, target);
	else if (strcmp(cmd, "set") == 0) resi = pcb_drc_query_set(hl, 1, target, key, val);
	else if (strcmp(cmd, "get") == 0) {
		if (strcmp(key, "defs") == 0)
			return pcb_drc_query_get_rule_defs(hl, target, res);
		else
			return pcb_drc_query_get(hl, 1, target, key, res);
	}
	else
		RND_ACT_FAIL(DrcQueryRuleMod);

	RND_ACT_IRES(resi);
	return 0;
}

static const char pcb_acts_DrcQueryDefMod[] = \
	"DrcQueryDefMod(clear, source)\n"
	"DrcQueryDefMod(create, rule_name)\n"
	"DrcQueryDefMod(get, rule_name, field_name)\n"
	"DrcQueryDefMod(set, rule_name, field_name, value)\n";
static const char pcb_acth_DrcQueryDefMod[] = "Automated DRC rule editing (for scripting and import)";
static fgw_error_t pcb_act_DrcQueryDefMod(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd, *target, *key = NULL, *val=NULL;
	rnd_hidlib_t *hl = RND_ACT_HIDLIB;
	int resi = -1;

	RND_ACT_CONVARG(1, FGW_STR, DrcQueryDefMod, cmd = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, DrcQueryDefMod, target = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, DrcQueryDefMod, key = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, DrcQueryDefMod, val = argv[4].val.str);

	if (strcmp(cmd, "clear") == 0) resi = pcb_drc_query_clear(hl, 0, target);
	else if (strcmp(cmd, "create") == 0) resi = pcb_drc_query_create(hl, 0, target);
	else if (strcmp(cmd, "set") == 0) resi = pcb_drc_query_set(hl, 0, target, key, val);
	else if (strcmp(cmd, "get") == 0) return pcb_drc_query_get(hl, 0, target, key, res);
	else
		RND_ACT_FAIL(DrcQueryDefMod);

	RND_ACT_IRES(resi);
	return 0;
}

#include "drc_lht.c"

static const rnd_hid_fsd_filter_t *init_flt(const char **fmt)
{
	static const char *pat_tdx[] = {"*.tdx", NULL};
	static const char *pat_lht[] = {"*.lht", NULL};
	static const rnd_hid_fsd_filter_t flt[] = {
		{"tEDAx", "tEDAx", pat_tdx},
		{"lihata", "lihata", pat_lht},
		{NULL, NULL, NULL}
	};

	*fmt = "tEDAx";
	return flt;
}

static const char pcb_acts_DrcQueryExport[] = "DrcQueryExport(ruleID, [filename], [format])\n";
static const char pcb_acth_DrcQueryExport[] = "Export a rule and related definitions to a file.";
static fgw_error_t pcb_act_DrcQueryExport(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ires = 0;
	const char *id, *fn = NULL, *fmt;
	char *autofree = NULL;
	rnd_hidlib_t *hl = RND_ACT_HIDLIB;
	const rnd_hid_fsd_filter_t *flt = init_flt(&fmt);

	RND_ACT_CONVARG(1, FGW_STR, DrcQueryExport, id = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, DrcQueryExport, fn = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, DrcQueryExport, fmt = argv[3].val.str);

	if (fn == NULL) {
		const char *ext = ".tdx", *sep;
		char *fnid = rnd_concat(id, ext, NULL);
TODO("cleanup: fix format selection: generalize dlg_loadsave.c's subfmt code");
		fn = autofree = rnd_gui->fileselect(rnd_gui, "drc_query_rule", "Export a drc_query rule and related definitions",
			fnid, ext, flt, "drc_query", 0, NULL);
		free(fnid);
		if (fn == NULL)
			return -1;

		sep = strrchr(fn, '.');
		if (sep != NULL) {
			sep++;
			switch(*sep) {
				case 't': case 'T': fmt = "tEDAx"; break;
				case 'l': case 'L': fmt = "lihata"; break;
			}
		}
	}

	switch(*fmt) {
		case 't': case 'T': ires = rnd_actionva(hl, "savetedax", "drc_query", fn, id, NULL); break;
		case 'l': case 'L': ires = drc_query_lht_save_rule(hl, fn, id); break;
		default: rnd_message(RND_MSG_ERROR, "Can not save in format '%s'\n", fmt); ires = -1; break;
	}

	free(autofree);
	RND_ACT_IRES(ires);
	return 0;
}

static const char pcb_acts_DrcQueryImport[] = "DrcQueryImport([filename])\n";
static const char pcb_acth_DrcQueryImport[] = "Import a rule and related definitions from a file.";
static fgw_error_t pcb_act_DrcQueryImport(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ires = 0;
	const char *fn = NULL, *fmt;
	char *autofree = NULL;
	rnd_hidlib_t *hl = RND_ACT_HIDLIB;
	FILE *f;
	fgw_arg_t args[2], tpres;
	const rnd_hid_fsd_filter_t *flt = init_flt(&fmt);

	RND_ACT_MAY_CONVARG(1, FGW_STR, DrcQueryExport, fn = argv[1].val.str);

	if (fn == NULL) {
		const char *ext = ".tdx";
		fn = autofree = rnd_gui->fileselect(rnd_gui, "drc_query_rule", "Import a drc_query rule and related definitions",
			NULL, ext, flt, "drc_query", RND_HID_FSD_READ, NULL);
		if (fn == NULL)
			return -1;
	}


	/* figure the format and load */
	f = rnd_fopen(hl, fn, "r");
	if (f == NULL)
		return -1;

	fgw_ptr_reg(&rnd_fgw, &args[1], RND_PTR_DOMAIN_FILE_PTR, FGW_PTR | FGW_STRUCT, f);

	if ((rnd_actionv_bin(hl, "tedaxtestparse", &tpres, 2, args) == 0) && (tpres.type == FGW_INT) && (tpres.val.nat_int == 1)) {
		ires = rnd_actionva(hl, "loadtedaxfrom", "drc_query", fn, "", "0", NULL);
		goto done;
	}

	/* To check another format: rewind(f) and repeat the above if() */

	/* fallback: io_lihata */
	ires = drc_query_lht_load_rules(hl, fn);

	done:;
	fgw_ptr_unreg(&rnd_fgw, &args[1], RND_PTR_DOMAIN_FILE_PTR);
	fclose(f);

	if (ires == 0)
		pcb_board_set_changed_flag(PCB_ACT_BOARD, rnd_true);

	free(autofree);
	RND_ACT_IRES(ires);
	return 0;
}

#include "dlg.c"

static pcb_drc_impl_t drc_query_impl = {"drc_query", "query() based DRC", "drcquerylistrules"};

static rnd_action_t drc_query_action_list[] = {
	{"DrcQueryListRules", pcb_act_DrcQueryListRules, pcb_acth_DrcQueryListRules, pcb_acts_DrcQueryListRules},
	{"DrcQueryEditRule", pcb_act_DrcQueryEditRule, pcb_acth_DrcQueryEditRule, pcb_acts_DrcQueryEditRule},
	{"DrcQueryRuleMod", pcb_act_DrcQueryRuleMod, pcb_acth_DrcQueryRuleMod, pcb_acts_DrcQueryRuleMod},
	{"DrcQueryDefMod", pcb_act_DrcQueryDefMod, pcb_acth_DrcQueryDefMod, pcb_acts_DrcQueryDefMod},
	{"DrcQueryExport", pcb_act_DrcQueryExport, pcb_acth_DrcQueryExport, pcb_acts_DrcQueryExport},
	{"DrcQueryImport", pcb_act_DrcQueryImport, pcb_acth_DrcQueryImport, pcb_acts_DrcQueryImport}
};

int pplg_check_ver_drc_query(int ver_needed) { return 0; }

void pplg_uninit_drc_query(void)
{
	long n;

	pcb_drc_impl_unreg(&drc_query_impl);
	rnd_event_unbind_allcookie(drc_query_cookie);
	rnd_conf_unreg_file(DRC_QUERY_CONF_FN, drc_query_conf_internal);
	rnd_conf_unreg_fields(DRC_CONF_PATH_PLUGIN);
	rnd_conf_hid_unreg(drc_query_cookie);

	for(n = 0; n < free_drc_conf_nodes.used; n++)
		rnd_conf_unreg_field(free_drc_conf_nodes.array[n]);
	vtp0_uninit(&free_drc_conf_nodes);

	rnd_remove_actions_by_cookie(drc_query_cookie);
	pcb_drcq_stat_uninit();
}

static rnd_conf_hid_callbacks_t cbs;

int pplg_init_drc_query(void)
{
	static rnd_conf_hid_callbacks_t cbs2;
	static rnd_conf_hid_id_t cfgid;

	RND_API_CHK_VER;

	pcb_drcq_stat_init();

	rnd_event_bind(PCB_EVENT_DRC_RUN, pcb_drc_query, NULL, drc_query_cookie);

	vtp0_init(&free_drc_conf_nodes);
	cbs.new_hlist_item_post = drc_query_newconf;
	cfgid = rnd_conf_hid_reg(drc_query_cookie, &cbs);

	rnd_conf_reg_file(DRC_QUERY_CONF_FN, drc_query_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_drc_query, field,isarray,type_name,cpath,cname,desc,flags);
#include "drc_query_conf_fields.h"

	cbs2.val_change_post = drc_query_defchg;
	rnd_conf_hid_set_cb(rnd_conf_get_field("plugins/drc_query/definitions"), cfgid, &cbs2);

	RND_REGISTER_ACTIONS(drc_query_action_list, drc_query_cookie)
	pcb_drc_impl_reg(&drc_query_impl);

	return 0;
}


conf_drc_query_t conf_drc_query;
