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
#include <librnd/core/conf.h>
#include "data.h"
#include "change.h"
#include <librnd/core/error.h>
#include "undo.h"
#include <librnd/core/plugins.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include "query.h"
#include <librnd/core/fptr_cast.h>

#define PCB dontuse

/******** tree helper ********/

const char *type_name[PCBQ_nodetype_max] = {
	"PCBQ_RULE",
	"PCBQ_RNAME",
	"PCBQ_EXPR_PROG",
	"PCBQ_EXPR",
	"PCBQ_ASSERT",
	"PCBQ_ITER_CTX",
	"PCBQ_OP_THUS",
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
	"PCBQ_LET",
	"PCBQ_VAR",
	"PCBQ_FNAME",
	"PCBQ_FCALL",
	"PCBQ_FLAG",
	"PCBQ_DATA_COORD",
	"PCBQ_DATA_DOUBLE",
	"PCBQ_DATA_STRING",
	"PCBQ_DATA_REGEX",
	"PCBQ_DATA_CONST",
	"PCBQ_DATA_OBJ",
	"PCBQ_DATA_INVALID",
	"PCBQ_DATA_LYTC"
};

char *pcb_query_sprint_val(pcb_qry_val_t *val)
{
	switch(val->type) {
		case PCBQ_VT_VOID:   return pcb_strdup("<void>");
		case PCBQ_VT_COORD:  return pcb_strdup_printf("%mI=%$mH", val->data.crd, val->data.crd);
		case PCBQ_VT_LONG:   return pcb_strdup_printf("%ld", val->data.lng, val->data.lng);
		case PCBQ_VT_DOUBLE: return pcb_strdup_printf("%f", val->data.dbl);
		case PCBQ_VT_STRING: return pcb_strdup_printf("\"%s\"", val->data.str);
		case PCBQ_VT_OBJ:    return pcb_strdup_printf("<obj ID=%ld>", val->data.obj->ID);
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

void pcb_qry_n_free(pcb_qry_node_t *nd)
{
	pcb_qry_node_t *ch, *chn;

	switch(nd->type) {
		case PCBQ_RNAME:
		case PCBQ_FIELD:
		case PCBQ_LISTVAR:
		case PCBQ_DATA_STRING:
		case PCBQ_DATA_CONST:
		case PCBQ_DATA_OBJ:
			free((char *)nd->data.str);
			break;

		case PCBQ_DATA_REGEX:
			free((char *)nd->data.str);
			re_se_free(nd->precomp.regex);
			break;

		case PCBQ_ITER_CTX:
			pcb_qry_iter_free(nd->data.iter_ctx);
			break;

		case PCBQ_FNAME:
		case PCBQ_FLAG:
		case PCBQ_DATA_COORD:
		case PCBQ_DATA_DOUBLE:
		case PCBQ_DATA_INVALID:
		case PCBQ_DATA_LYTC:
		case PCBQ_VAR:
			/* no allocated field */
			break;

		case PCBQ_LET:
		case PCBQ_ASSERT:
			if (nd->precomp.it_active != NULL) {
				vts0_uninit(nd->precomp.it_active);
				free(nd->precomp.it_active);
			}
			goto free_children;

		case PCBQ_OP_NOT:
		case PCBQ_FIELD_OF:
		case PCBQ_FCALL:
		case PCBQ_OP_THUS:
		case PCBQ_OP_AND:
		case PCBQ_OP_OR:
		case PCBQ_OP_EQ:
		case PCBQ_OP_NEQ:
		case PCBQ_OP_GTEQ:
		case PCBQ_OP_LTEQ:
		case PCBQ_OP_GT:
		case PCBQ_OP_LT:
		case PCBQ_OP_ADD:
		case PCBQ_OP_SUB:
		case PCBQ_OP_MUL:
		case PCBQ_OP_DIV:
		case PCBQ_OP_MATCH:
		case PCBQ_RULE:
		case PCBQ_EXPR_PROG:
		case PCBQ_EXPR:
			free_children:;
			for(ch = nd->data.children; ch != NULL; ch = chn) {
				chn = ch->next;
				pcb_qry_n_free(ch);
			}
			break;

		case PCBQ_nodetype_max: break;
	}
	free(nd);
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
		case PCBQ_DATA_OBJ:    pcb_printf("%s%s %s\n", prefix, ind, nd->data.str); break;
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
		case PCBQ_RULE:
			pcb_printf("%s%s%s\n", prefix, ind, nd->data.children->next->data.str);
			n = nd->data.children->next->next;
			if (n != NULL) {
				for(; n != NULL; n = n->next)
					pcb_qry_dump_tree_(prefix, level+1, n, it_ctx);
			}
			else
				pcb_printf("%s%s<empty>\n", prefix, ind);
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
		if ((node->type == PCBQ_EXPR_PROG) || (node->type == PCBQ_RULE)) {
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

void pcb_qry_iter_free(pcb_query_iter_t *it)
{
	htsi_entry_t *e;
	for(e = htsi_first(&it->names); e != NULL; e = htsi_next(&it->names, e))
		free(e->key);
	htsi_uninit(&it->names);
	free(it->vects);
	free(it->idx);
	free(it->lst);
	free(it->vn);
	free(it);
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
	it->last_obj = NULL;
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
	qry_lex_destroy();
}

void pcb_qry_basic_fnc_init(void);

int pplg_init_query(void)
{
	PCB_API_CHK_VER;
	pcb_qry_basic_fnc_init();
	query_action_reg(query_cookie);
	return 0;
}
