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


static const char *drc_query_cookie = "drc_query";

const conf_drc_query_t conf_drc_query;
#define DRC_QUERY_CONF_FN "drc_query.conf"

typedef struct {
	pcb_board_t *pcb;
	pcb_view_list_t *lst;
	const char *type;
	const char *title;
	const char *desc;
	unsigned disable:1;
} drc_qry_ctx_t;

pcb_coord_t load_obj_const(pcb_obj_qry_const_t *cnst)
{
	switch(cnst->val.type) {
		case PCBQ_VT_COORD: return cnst->val.data.crd;
		case PCBQ_VT_LONG: return cnst->val.data.lng;
		case PCBQ_VT_DOUBLE: return cnst->val.data.dbl;
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
}

static long drc_qry_exec(pcb_board_t *pcb, pcb_view_list_t *lst, pcb_conf_listitem_t *i, const char *type, const char *title, const char *desc, const char *query)
{
	const char *scope = NULL;
	drc_qry_ctx_t qctx;

	if (query == NULL) {
		pcb_message(PCB_MSG_ERROR, "drc_query: igoring rule with no query string:%s\n", i->name);
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

	pcb_qry_run_script(pcb, query, scope, drc_qry_exec_cb, &qctx);

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

static long load_int(lht_node_t *rule, pcb_conf_listitem_t *i, const char *name, long invalid)
{
	lht_node_t *n = lht_dom_hash_get(rule, name);
	long l;
	const char *val;
	char *end;

	if (n == NULL)
		return invalid;
	if (n->type != LHT_TEXT) {
		pcb_message(PCB_MSG_ERROR, "drc_query: igoring non-text node %s of rule %s \n", name, i->name);
		return invalid;
	}

	val = n->data.text.value;

	if ((pcb_strcasecmp(val, "yes") == 0) || (pcb_strcasecmp(val, "true") == 0))
		return 1;
	if ((pcb_strcasecmp(val, "no") == 0) || (pcb_strcasecmp(val, "false") == 0))
		return 0;

	l = strtol(val, &end, 10);
	if (*end != '\0') {
		pcb_message(PCB_MSG_ERROR, "drc_query: ignoring invalid value '%s' for %s of rule %s \n", val, name, i->name);
		return invalid;
	}

	return l;
}

static void pcb_drc_query(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	gdl_iterator_t it;
	pcb_conf_listitem_t *i;
	long cnt = 0, disable;

	if (conf_drc_query.plugins.drc_query.disable)
		return;

	pcb_conflist_foreach(&conf_drc_query.plugins.drc_query.rules, &it, i) {
		lht_node_t *rule = i->prop.src;
		if (rule->type != LHT_HASH) {
			pcb_message(PCB_MSG_ERROR, "drc_query: rule %s is not a hash\n", i->name);
			continue;
		}

		disable = load_int(rule, i, "disable", 0);
		if (disable)
			continue;

		cnt += drc_qry_exec((pcb_board_t *)hidlib, &pcb_drc_lst, i,
			load_str(rule, i, "type"), load_str(rule, i, "title"), load_str(rule, i, "desc"),
			load_str(rule, i, "query")
		);
	}
}

static conf_native_t *nat_defs = NULL;
static void drc_query_newconf(conf_native_t *cfg, pcb_conf_listitem_t *i)
{
	if (nat_defs == NULL) {
		if (strncmp(cfg->hash_path, "plugins/drc_query/definitions", 29) != 0)
			return;
		nat_defs = cfg;
	}


	if (nat_defs == cfg) {
		lht_node_t *nd = i->prop.src;
		char *path = pcb_concat("design/drc/", nd->name, NULL);
		static pcb_coord_t c;

		if (pcb_conf_get_field(path) == NULL) {
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

			nat = pcb_conf_reg_field_(&c, 1, type, path, pcb_strdup(sdesc), 0);
			if (nat == NULL) {
				pcb_message(PCB_MSG_ERROR, "drc_query: failed to register conf node '%s'\n", path);
				goto fail;
			}
			
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


int pplg_check_ver_drc_query(int ver_needed) { return 0; }

void pplg_uninit_drc_query(void)
{
	pcb_event_unbind_allcookie(drc_query_cookie);
	pcb_conf_unreg_file(DRC_QUERY_CONF_FN, drc_query_conf_internal);
	pcb_conf_unreg_fields("plugins/drc_query/");
	pcb_conf_hid_unreg(drc_query_cookie);
}

static conf_hid_callbacks_t cbs;

int pplg_init_drc_query(void)
{
	PCB_API_CHK_VER;
	pcb_event_bind(PCB_EVENT_DRC_RUN, pcb_drc_query, NULL, drc_query_cookie);

	cbs.new_hlist_item_post = drc_query_newconf;
	pcb_conf_hid_reg(drc_query_cookie, &cbs);

	pcb_conf_reg_file(DRC_QUERY_CONF_FN, drc_query_conf_internal);
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	pcb_conf_reg_field(conf_drc_query, field,isarray,type_name,cpath,cname,desc,flags);
#include "drc_query_conf_fields.h"

	return 0;
}

