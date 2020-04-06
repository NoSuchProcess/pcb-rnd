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
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/error.h>
#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include <librnd/core/list_conf.h>
#include <librnd/core/compat_misc.h>

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

static const char *drc_query_cookie = "drc_query";

const conf_drc_query_t conf_drc_query;
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

pcb_coord_t load_obj_const(pcb_obj_qry_const_t *cnst)
{
	switch(cnst->val.type) {
		case PCBQ_VT_COORD: return cnst->val.data.crd;
		case PCBQ_VT_LONG: return cnst->val.data.lng;
		case PCBQ_VT_DOUBLE: return cnst->val.data.dbl;
		
		case PCBQ_VT_VOID:
		case PCBQ_VT_OBJ:
		case PCBQ_VT_LST:
		case PCBQ_VT_STRING:
			break;
	}
	return 0;
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
		pcb_coord_t *expv = NULL, expv_, *mesv = NULL, mesv_;
		for(i = 0; i < res->data.lst.used-1; i+=2) {
			pcb_any_obj_t *cmd = res->data.lst.array[i], *obj = res->data.lst.array[i+1];
			pcb_qry_drc_ctrl_t ctrl = pcb_qry_drc_ctrl_decode(cmd);
			pcb_obj_qry_const_t *cnst = res->data.lst.array[i+1];
			
			switch(ctrl) {
				case PCB_QRY_DRC_GRP1:     pcb_view_append_obj(violation, 0, obj); break;
				case PCB_QRY_DRC_GRP2:     pcb_view_append_obj(violation, 1, obj); break;
				case PCB_QRY_DRC_EXPECT:   expv = &expv_; expv_ = load_obj_const(cnst); break;
				case PCB_QRY_DRC_MEASURE:  mesv = &mesv_; mesv_ = load_obj_const(cnst); break;
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

static long drc_qry_exec(pcb_qry_exec_t *ec, pcb_board_t *pcb, pcb_view_list_t *lst, const char *name, const char *type, const char *title, const char *desc, const char *query)
{
	const char *scope = NULL;
	drc_qry_ctx_t qctx;
	pcb_drcq_stat_t *st;
	double ts, te;

	if (query == NULL) {
		pcb_message(PCB_MSG_ERROR, "drc_query: igoring rule with no query string:%s\n", name);
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

	st = pcb_drcq_stat_get(name);

	ts = pcb_dtime();
	pcb_qry_run_script(ec, pcb, query, scope, drc_qry_exec_cb, &qctx);
	te = pcb_dtime();

	st->last_run_time = te - ts;
	st->sum_run_time += te - ts;
	st->run_cnt++;
	st->last_hit_cnt = qctx.hit_cnt;
	st->sum_hit_cnt += qctx.hit_cnt;

	return 0;
}

static const char *load_str(lht_node_t *rule, pcb_conf_listitem_t *i, const char *name)
{
	lht_node_t *n = lht_dom_hash_get(rule, name);
	if (n == NULL)
		return NULL;
	if (n->type != LHT_TEXT) {
		pcb_message(PCB_MSG_ERROR, "drc_query: igoring non-text node %s of rule %s \n", name, i->name);
		return NULL;
	}
	return n->data.text.value;
}

static int *drc_get_disable(const char *name)
{
	char *path = pcb_concat(DRC_CONF_PATH_DISABLE, name, NULL);
	conf_native_t *nat = pcb_conf_get_field(path);
	free(path);
	if ((nat == NULL) || (nat->type != CFN_BOOLEAN))
		return NULL;
	return nat->val.boolean;
}

static void pcb_drc_query(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	gdl_iterator_t it;
	pcb_conf_listitem_t *i;
	long cnt = 0;
	int bufno = -1;
	pcb_qry_exec_t ec;


	if (conf_drc_query.plugins.drc_query.disable)
		return;

	pcb_qry_init(&ec, pcb, NULL, bufno);

	pcb_conflist_foreach(&conf_drc_query.plugins.drc_query.rules, &it, i) {
		lht_node_t *rule = i->prop.src;
		int *dis;
		if (rule->type != LHT_HASH) {
			pcb_message(PCB_MSG_ERROR, "drc_query: rule %s is not a hash\n", i->name);
			continue;
		}

		dis = drc_get_disable(i->name);
		if ((dis != NULL) && (*dis != 0))
			continue;

		cnt += drc_qry_exec(&ec, pcb, &pcb_drc_lst, i->name,
			load_str(rule, i, "type"), load_str(rule, i, "title"), load_str(rule, i, "desc"),
			load_str(rule, i, "query")
		);
	}

	pcb_qry_uninit(&ec);
	drc_rlist_pcb2dlg(); /* for the run time */
}

static vtp0_t free_drc_conf_nodes;
static conf_native_t *nat_defs = NULL;
static conf_native_t *nat_rules = NULL;
static void drc_query_newconf(conf_native_t *cfg, pcb_conf_listitem_t *i)
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
		char *path = pcb_concat(DRC_CONF_PATH_DISABLE, nd->name, NULL);

		if (pcb_conf_get_field(path) == NULL) {
			const char *sdesc;
			conf_native_t *nat;
			pcb_bool_t *b;
			lht_node_t *ndesc;

			ndesc = lht_dom_hash_get(nd, "desc");
			if ((ndesc != NULL) && (ndesc->type == LHT_TEXT)) sdesc = ndesc->data.text.value;

			b = calloc(sizeof(pcb_bool_t), 1);
			nat = pcb_conf_reg_field_(b, 1, CFN_BOOLEAN, path, pcb_strdup(sdesc), 0);
			if (nat == NULL) {
				free(b);
				pcb_message(PCB_MSG_ERROR, "drc_query: failed to register conf node '%s'\n", path);
				goto fail;
			}

			nat->random_flags.dyn_hash_path = 1;
			nat->random_flags.dyn_desc = 1;
			nat->random_flags.dyn_val = 1;
			vtp0_append(&free_drc_conf_nodes, nat);
			pcb_conf_set(CFR_INTERNAL, path, -1, "0", POL_OVERWRITE);
		}
		else
			free(path);
	}
	else if (nat_defs == cfg) {
		lht_node_t *nd = i->prop.src;
		char *path = pcb_concat(DRC_CONF_PATH_CONST, nd->name, NULL);
		pcb_coord_t *c;

		if (pcb_conf_get_field(path) == NULL) {
			union {
				pcb_coord_t c;
				double d;
				void *ptr;
				char *str;
			} anyval;
			conf_native_t *nat;
			lht_node_t *ndesc = lht_dom_hash_get(nd, "desc");
			lht_node_t *ntype = lht_dom_hash_get(nd, "type");
			lht_node_t *ndefault = lht_dom_hash_get(nd, "default");
			lht_node_t *nlegacy = lht_dom_hash_get(nd, "legacy");
			const char *sdesc = "n/a", *stype = NULL, *sdefault = NULL, *slegacy = NULL;
			conf_native_type_t type;
			if ((ndesc != NULL) && (ndesc->type == LHT_TEXT)) sdesc = ndesc->data.text.value;
			if ((ntype != NULL) && (ntype->type == LHT_TEXT)) stype = ntype->data.text.value;
			if ((ndefault != NULL) && (ndefault->type == LHT_TEXT)) sdefault = ndefault->data.text.value;
			if ((nlegacy != NULL) && (nlegacy->type == LHT_TEXT)) slegacy = nlegacy->data.text.value;


			if (stype == NULL) {
				pcb_message(PCB_MSG_ERROR, "drc_query: missing type field for constant %s\n", nd->name);
				goto fail;
			}

			type = pcb_conf_native_type_parse(stype);
			if (type >= CFN_LIST) {
				pcb_message(PCB_MSG_ERROR, "drc_query: invalid type '%s' for %s\n", stype, nd->name);
				goto fail;
			}

			c = calloc(sizeof(anyval), 1);
			nat = pcb_conf_reg_field_(c, 1, type, path, pcb_strdup(sdesc), 0);
			if (nat == NULL) {
				free(c);
				pcb_message(PCB_MSG_ERROR, "drc_query: failed to register conf node '%s'\n", path);
				goto fail;
			}

			nat->random_flags.dyn_hash_path = 1;
			nat->random_flags.dyn_desc = 1;
			nat->random_flags.dyn_val = 1;
			vtp0_append(&free_drc_conf_nodes, nat);

			if (slegacy != NULL)
				pcb_conf_legacy(path, slegacy);
			else if (sdefault != NULL)
				pcb_conf_set(CFR_INTERNAL, path, -1, sdefault, POL_OVERWRITE);
			path = NULL; /* hash key shall not be free'd */
		}
		fail:;
		free(path);
	}
}

#include "dlg.c"

static pcb_drc_impl_t drc_query_impl = {"drc_query", "query() based DRC", "drcquerylistrules"};

static pcb_action_t drc_query_action_list[] = {
	{"DrcQueryListRules", pcb_act_DrcQueryListRules, pcb_acth_DrcQueryListRules, pcb_acts_DrcQueryListRules},
	{"DrcQueryEditRule", pcb_act_DrcQueryEditRule, pcb_acth_DrcQueryEditRule, pcb_acts_DrcQueryEditRule}
};

int pplg_check_ver_drc_query(int ver_needed) { return 0; }

void pplg_uninit_drc_query(void)
{
	long n;

	pcb_drc_impl_unreg(&drc_query_impl);
	pcb_event_unbind_allcookie(drc_query_cookie);
	pcb_conf_unreg_file(DRC_QUERY_CONF_FN, drc_query_conf_internal);
	pcb_conf_unreg_fields(DRC_CONF_PATH_PLUGIN);
	pcb_conf_hid_unreg(drc_query_cookie);

	for(n = 0; n < free_drc_conf_nodes.used; n++)
		pcb_conf_unreg_field(free_drc_conf_nodes.array[n]);
	vtp0_uninit(&free_drc_conf_nodes);

	pcb_remove_actions_by_cookie(drc_query_cookie);
	pcb_drcq_stat_uninit();
}

static conf_hid_callbacks_t cbs;

int pplg_init_drc_query(void)
{
	PCB_API_CHK_VER;

	pcb_drcq_stat_init();

	pcb_event_bind(PCB_EVENT_DRC_RUN, pcb_drc_query, NULL, drc_query_cookie);

	vtp0_init(&free_drc_conf_nodes);
	cbs.new_hlist_item_post = drc_query_newconf;
	pcb_conf_hid_reg(drc_query_cookie, &cbs);

	pcb_conf_reg_file(DRC_QUERY_CONF_FN, drc_query_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_drc_query, field,isarray,type_name,cpath,cname,desc,flags);
#include "drc_query_conf_fields.h"

	PCB_REGISTER_ACTIONS(drc_query_action_list, drc_query_cookie)
	pcb_drc_impl_reg(&drc_query_impl);

	return 0;
}

