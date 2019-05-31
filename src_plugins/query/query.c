/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
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

/* Query language - common code for the compiled tree and plugin administration */

#include "config.h"
#include <genht/hash.h>
#include <genht/htsi.h>
#include "conf.h"
#include "data.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "hid_init.h"
#include "actions.h"
#include "compat_misc.h"
#include "query.h"
#include "fptr_cast.h"

/******** tree helper ********/

const char *type_name[PCBQ_nodetype_max] = {
	"PCBQ_RULE",
	"PCBQ_RNAME",
	"PCBQ_EXPR_PROG",
	"PCBQ_EXPR",
	"PCBQ_ITER_CTX",
	"PCBQ_OP_AND",
	"PCBQ_OP_OR",
	"PCBQ_OP_EQ",
	"PCBQ_OP_NEQ",
	"PCBQ_OP_GTEQ",
	"PCBQ_OP_LTEQ",
	"PCBQ_OP_GT",
	"PCBQ_OP_LT",
	"PCBQ_OP_ADD",
	"PCBQ_OP_SUB",
	"PCBQ_OP_MUL",
	"PCBQ_OP_DIV",
	"PCBQ_OP_MATCH",
	"PCBQ_OP_NOT",
	"PCBQ_FIELD",
	"PCBQ_FIELD_OF",
	"PCBQ_LISTVAR",
	"PCBQ_VAR",
	"PCBQ_FNAME",
	"PCBQ_FCALL",
	"PCBQ_FLAG",
	"PCBQ_DATA_COORD",
	"PCBQ_DATA_DOUBLE",
	"PCBQ_DATA_STRING",
	"PCBQ_DATA_REGEX",
	"PCBQ_DATA_CONST",
	"PCBQ_DATA_INVALID",
	"PCBQ_DATA_LYTC"
};

char *pcb_query_sprint_val(pcb_qry_val_t *val)
{
	switch(val->type) {
		case PCBQ_VT_VOID:   return pcb_strdup("<void>");
		case PCBQ_VT_COORD:  return pcb_strdup_printf("%mI=%$mH", val->data.crd, val->data.crd);
		case PCBQ_VT_DOUBLE: return pcb_strdup_printf("%f", val->data.dbl);
		case PCBQ_VT_STRING: return pcb_strdup_printf("\"%s\"", val->data.str);
		case PCBQ_VT_OBJ:    return pcb_strdup("<obj>");
		case PCBQ_VT_LST:    return pcb_strdup("<lst>");
	}
	return pcb_strdup("<invalid>");
}

const char *pcb_qry_nodetype_name(pcb_qry_nodetype_t ntype)
{
	int type = ntype;
	if ((type < 0) || (type >= PCBQ_nodetype_max))
		return "<invalid>";
	return type_name[type];
}

pcb_qry_node_t *pcb_qry_n_alloc(pcb_qry_nodetype_t ntype)
{
	pcb_qry_node_t *nd = calloc(sizeof(pcb_qry_node_t), 1);
	nd->type = ntype;
	return nd;
}

pcb_qry_node_t *pcb_qry_n_insert(pcb_qry_node_t *parent, pcb_qry_node_t *ch)
{
	ch->next = parent->data.children;
	parent->data.children = ch;
	ch->parent = parent;
	return parent;
}

static char ind[] = "                                                                                ";
void pcb_qry_dump_tree_(const char *prefix, int level, pcb_qry_node_t *nd, pcb_query_iter_t *it_ctx)
{
	pcb_qry_node_t *n;
	if (level < sizeof(ind))  ind[level] = '\0';
	printf("%s%s%s    ", prefix, ind, pcb_qry_nodetype_name(nd->type));
	switch(nd->type) {
		case PCBQ_DATA_INVALID:pcb_printf("%s%s invalid (literal)\n", prefix, ind); break;
		case PCBQ_DATA_COORD:  pcb_printf("%s%s %mI (%$mm)\n", prefix, ind, nd->data.crd, nd->data.crd); break;
		case PCBQ_DATA_DOUBLE: pcb_printf("%s%s %f\n", prefix, ind, nd->data.dbl); break;
		case PCBQ_DATA_CONST:  pcb_printf("%s%s %s\n", prefix, ind, nd->data.str); break;
		case PCBQ_ITER_CTX:    pcb_printf("%s%s vars=%d\n", prefix, ind, nd->data.iter_ctx->num_vars); break;
		case PCBQ_FLAG:        pcb_printf("%s%s %s\n", prefix, ind, nd->precomp.flg->name); break;
		case PCBQ_VAR:
			pcb_printf("%s%s ", prefix, ind);
			if ((it_ctx != NULL) && (nd->data.crd < it_ctx->num_vars)) {
				if (it_ctx->vects == NULL)
					pcb_qry_iter_init(it_ctx);
				printf("%s\n", it_ctx->vn[nd->data.crd]);
			}
			else
				printf("<invalid:%d>\n", nd->data.crd);
			break;
		case PCBQ_FNAME:
			{
				const char *name = pcb_qry_fnc_name(nd->data.fnc);
				if (name == NULL)
					pcb_printf("%s%s <unknown>\n", prefix, ind);
				else
					pcb_printf("%s%s %s()\n", prefix, ind, name);
			}
			break;
		case PCBQ_FIELD:
		case PCBQ_LISTVAR:
		case PCBQ_DATA_REGEX:
		case PCBQ_DATA_STRING: pcb_printf("%s%s '%s'\n", prefix, ind, nd->data.str); break;
		default:
			printf("\n");
			if (level < sizeof(ind))  ind[level] = ' ';
			for(n = nd->data.children; n != NULL; n = n->next) {
				if (n->parent != nd)
					printf("#parent# ");
				pcb_qry_dump_tree_(prefix, level+1, n, it_ctx);
			}
			return;
	}
	if (level < sizeof(ind))  ind[level] = ' ';
}

pcb_query_iter_t *pcb_qry_find_iter(pcb_qry_node_t *node)
{
	for(; node != NULL;node = node->parent) {
		if (node->type == PCBQ_EXPR_PROG) {
			if (node->data.children->type == PCBQ_ITER_CTX)
				return node->data.children->data.iter_ctx;
		}
		if (node->type == PCBQ_EXPR_PROG) {
			if (node->data.children->type == PCBQ_ITER_CTX)
				return node->data.children->data.iter_ctx;
		}
	}

	return NULL;
}

void pcb_qry_dump_tree(const char *prefix, pcb_qry_node_t *top)
{
	pcb_query_iter_t *iter_ctx = pcb_qry_find_iter(top);

	if (iter_ctx == NULL)
		printf("<can't find iter context>\n");

	for(; top != NULL; top = top->next)
		pcb_qry_dump_tree_(prefix, 0, top, iter_ctx);
}

/******** iter admin ********/
pcb_query_iter_t *pcb_qry_iter_alloc(void)
{
	pcb_query_iter_t *it = calloc(1, sizeof(pcb_query_iter_t));
	htsi_init(&it->names, strhash, strkeyeq);
	return it;
}


int pcb_qry_iter_var(pcb_query_iter_t *it, const char *varname, int alloc)
{
	htsi_entry_t *e = htsi_getentry(&it->names, varname);

	if (e != NULL)
		return e->value;

	if (!alloc)
		return -1;

	htsi_set(&it->names, pcb_strdup(varname), it->num_vars);
	return it->num_vars++;
}

void pcb_qry_iter_init(pcb_query_iter_t *it)
{
	htsi_entry_t *e;

	if (it->vn != NULL)
		return;
	it->vects  = calloc(sizeof(vtp0_t *), it->num_vars);
	it->idx  = calloc(sizeof(pcb_cardinal_t), it->num_vars);
	it->lst = calloc(sizeof(pcb_qry_val_t), it->num_vars);

	it->vn = malloc(sizeof(char *) * it->num_vars);
	for (e = htsi_first(&it->names); e; e = htsi_next(&it->names, e))
		it->vn[e->value] = e->key;
	it->all_idx = -1;
}

/******** functions ********/
static htsp_t *qfnc = NULL;

static void pcb_qry_fnc_destroy(void)
{
	htsp_entry_t *e;
	for(e = htsp_first(qfnc); e != NULL; e = htsp_next(qfnc, e))
		free(e->key);
	htsp_free(qfnc);
	qfnc = NULL;
}

int pcb_qry_fnc_reg(const char *name, pcb_qry_fnc_t fnc)
{
	if (qfnc == NULL)
		qfnc = htsp_alloc(strhash, strkeyeq);
	if (htsp_get(qfnc, name) != NULL)
		return -1;

	htsp_set(qfnc, pcb_strdup(name), pcb_cast_f2d((pcb_fptr_t)fnc));

	return 0;
}

pcb_qry_fnc_t pcb_qry_fnc_lookup(const char *name)
{
	if (qfnc == NULL)
		return NULL;

	return (pcb_qry_fnc_t)pcb_cast_d2f(htsp_get(qfnc, name));
}

/* slow linear search: it's only for the dump */
const char *pcb_qry_fnc_name(pcb_qry_fnc_t fnc)
{
	htsp_entry_t *e;
	void *target = pcb_cast_f2d((pcb_fptr_t)fnc);

	if (qfnc == NULL)
		return NULL;

	for(e = htsp_first(qfnc); e != NULL; e = htsp_next(qfnc, e))
		if (e->value == target)
			return e->key;
	return NULL;
}

/******** parser helper ********/
void qry_error(void *prog, const char *err)
{
	pcb_trace("qry_error: %s\n", err);
}

int qry_wrap()
{
	return 1;
}

/******** plugin helper ********/
void query_action_reg(const char *cookie);

static const char *query_cookie = "query plugin";

int pplg_check_ver_query(int ver_needed) { return 0; }

void pplg_uninit_query(void)
{
	pcb_remove_actions_by_cookie(query_cookie);
	pcb_qry_fnc_destroy();
}

void pcb_qry_basic_fnc_init(void);

int pplg_init_query(void)
{
	PCB_API_CHK_VER;
	pcb_qry_basic_fnc_init();
	query_action_reg(query_cookie);
	return 0;
}
