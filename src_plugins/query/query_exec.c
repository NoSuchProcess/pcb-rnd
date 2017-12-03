/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 */

/* Query language - execution */

#include <stdio.h>
#include "config.h"
#include "data.h"
#include "query.h"
#include "query_exec.h"
#include "query_access.h"
#include "pcb-printf.h"

void pcb_qry_init(pcb_qry_exec_t *ctx, pcb_qry_node_t *root)
{
	memset(ctx, 0, sizeof(pcb_qry_exec_t));
	ctx->all.type = PCBQ_VT_LST;
	pcb_qry_list_all(&ctx->all, PCB_OBJ_ANY);
	ctx->root = root;
	ctx->iter = NULL;
}

void pcb_qry_uninit(pcb_qry_exec_t *ctx)
{
	pcb_qry_list_free(&ctx->all);
#warning TODO: free the iterator
}

int pcb_qry_run(pcb_qry_node_t *prg, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_obj_t *current), void *user_ctx)
{
	pcb_qry_exec_t ec;
	pcb_qry_val_t res;
	int errs = 0;

	pcb_qry_init(&ec, prg);
	if (pcb_qry_it_reset(&ec, prg) != 0)
		return -1;

	do {
		if (pcb_qry_eval(&ec, prg, &res) == 0) {
			pcb_obj_t *current = NULL;
			if (ec.iter->all_idx >= 0)
				current = ec.iter->it[ec.iter->all_idx];
			cb(user_ctx, &res, current);
		}
		else
			errs++;
	} while(pcb_qry_it_next(&ec));
	pcb_qry_uninit(&ec);
	return errs;
}


/* load unary operand to o1 */
#define UNOP() \
do { \
	if ((node->data.children == NULL) || (node->data.children->next != NULL)) \
		return -1; \
	if (pcb_qry_eval(ctx, node->data.children, &o1) < 0) \
		return -1; \
} while(0)

/* load 1st binary operand to o1 */
#define BINOPS1() \
do { \
	if ((node->data.children == NULL) || (node->data.children->next == NULL) || (node->data.children->next->next != NULL)) \
		return -1; \
	if (pcb_qry_eval(ctx, node->data.children, &o1) < 0) \
		return -1; \
} while(0)

/* load 2nd binary operand to o2 */
#define BINOPS2() \
do { \
	if (pcb_qry_eval(ctx, node->data.children->next, &o2) < 0) \
		return -1; \
} while(0)

/* load binary operands to o1 and o2 */
#define BINOPS() \
do { \
	BINOPS1(); \
	BINOPS2(); \
} while(0)

static int promote(pcb_qry_val_t *a, pcb_qry_val_t *b)
{
	if ((a->type == PCBQ_VT_VOID) || (b->type == PCBQ_VT_VOID)) {
		a->type = b->type = PCBQ_VT_VOID;
		return 0;
	}
	if (a->type == b->type)
		return 0;
	switch(a->type) {
		case PCBQ_VT_OBJ:   return -1;
		case PCBQ_VT_LST:   return -1;
		case PCBQ_VT_COORD:
			if (b->type == PCBQ_VT_DOUBLE)  PCB_QRY_RET_DBL(a, a->data.crd);
			return -1;
		case PCBQ_VT_DOUBLE:
			if (b->type == PCBQ_VT_COORD)  PCB_QRY_RET_DBL(b, b->data.crd);
			return -1;
		case PCBQ_VT_STRING:
		case PCBQ_VT_VOID:
			return -1;
	}
	return -1;
}

static const char *op2str(char *buff, int buff_size, pcb_qry_val_t *val)
{
	switch(val->type) {
		case PCBQ_VT_COORD:
			pcb_snprintf(buff, buff_size, "%mI", val->data.crd);
			return buff;
		case PCBQ_VT_DOUBLE:
			pcb_snprintf(buff, buff_size, "%f", val->data.crd);
			return buff;
		case PCBQ_VT_STRING:
			return val->data.str;
		case PCBQ_VT_VOID:
		case PCBQ_VT_OBJ:
		case PCBQ_VT_LST:   return NULL;
	}
	return NULL;
}

int pcb_qry_is_true(pcb_qry_val_t *val)
{
	switch(val->type) {
		case PCBQ_VT_VOID:     return 0;
		case PCBQ_VT_OBJ:      return val->data.obj.type != PCB_OBJ_VOID;
		case PCBQ_VT_LST:      return pcb_objlist_first(&val->data.lst) != NULL;
		case PCBQ_VT_COORD:    return val->data.crd;
		case PCBQ_VT_DOUBLE:   return val->data.dbl;
		case PCBQ_VT_STRING:   return (val->data.str != NULL) && (*val->data.str != '\0');
	}
	return 0;
}

static void setup_iter_list(pcb_qry_exec_t *ctx, int var_id, pcb_qry_val_t *listval)
{
	ctx->iter->lst[var_id] = *listval;
	assert(listval->type == PCBQ_VT_LST);
	ctx->iter->it[var_id] = pcb_objlist_first(&listval->data.lst);
}

int pcb_qry_it_reset(pcb_qry_exec_t *ctx, pcb_qry_node_t *node)
{
	int n;
	ctx->iter = pcb_qry_find_iter(node);
	if (ctx->iter == NULL)
		return -1;

	pcb_qry_iter_init(ctx->iter);

	for(n = 0; n < ctx->iter->num_vars; n++) {
		if (strcmp(ctx->iter->vn[n], "@") == 0) {
			setup_iter_list(ctx, n, &ctx->all);
			ctx->iter->all_idx = n;
			break;
		}
	}

	return 0;
}

int pcb_qry_it_next(pcb_qry_exec_t *ctx)
{
	int i;
	for(i = 0; i < ctx->iter->num_vars; i++) {
		ctx->iter->it[i] = pcb_objlist_next(ctx->iter->it[i]);
		if (ctx->iter->it[i] != NULL)
			return 1;
		ctx->iter->it[i] = pcb_objlist_first(&ctx->iter->lst[i].data.lst);
	}
	return 0;
}


int pcb_qry_eval(pcb_qry_exec_t *ctx, pcb_qry_node_t *node, pcb_qry_val_t *res)
{
	pcb_qry_val_t o1, o2;

	switch(node->type) {
		case PCBQ_EXPR:
			return pcb_qry_eval(ctx, node->data.children, res);

		case PCBQ_EXPR_PROG: {
			pcb_qry_node_t *itn, *exprn;
			itn = node->data.children;
			if (itn == NULL)
				return -1;
			exprn = itn->next;
			if (exprn == NULL)
				return -1;
			if (itn->type != PCBQ_ITER_CTX)
				return -1;
			if (ctx->iter != itn->data.iter_ctx)
				return -1;
			return pcb_qry_eval(ctx, exprn, res);
		}


		case PCBQ_OP_AND: /* lazy */
			BINOPS1();
			if (!pcb_qry_is_true(&o1))
				PCB_QRY_RET_INT(res, 0);
			BINOPS2();
			if (!pcb_qry_is_true(&o2))
				PCB_QRY_RET_INT(res, 0);
			PCB_QRY_RET_INT(res, 1);

		case PCBQ_OP_OR: /* lazy */
			BINOPS1();
			if (pcb_qry_is_true(&o1))
				PCB_QRY_RET_INT(res, 1);
			BINOPS2();
			if (pcb_qry_is_true(&o2))
				PCB_QRY_RET_INT(res, 1);
			PCB_QRY_RET_INT(res, 0);

		case PCBQ_OP_EQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_OBJ:      PCB_QRY_RET_INT(res, ((o1.data.obj.type) == (o2.data.obj.type)) && ((o1.data.obj.data.any) == (o2.data.obj.data.any)));
				case PCBQ_VT_LST:      PCB_QRY_RET_INT(res, pcb_qry_list_cmp(&o1, &o2));
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd == o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_INT(res, o1.data.dbl == o2.data.dbl);
				case PCBQ_VT_STRING:
					if (o1.data.str == o2.data.str)
						PCB_QRY_RET_INT(res, 1);
					if ((o1.data.str == NULL) || (o2.data.str == NULL))
						PCB_QRY_RET_INT(res, 0);
					PCB_QRY_RET_INT(res, strcmp(o1.data.str, o2.data.str) == 0);
			}
			return -1;

		case PCBQ_OP_NEQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_OBJ:      PCB_QRY_RET_INT(res, ((o1.data.obj.type) != (o2.data.obj.type)) || ((o1.data.obj.data.any) != (o2.data.obj.data.any)));
				case PCBQ_VT_LST:      PCB_QRY_RET_INT(res, !pcb_qry_list_cmp(&o1, &o2));
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd != o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_INT(res, o1.data.dbl != o2.data.dbl);
				case PCBQ_VT_STRING:
					if (o1.data.str == o2.data.str)
						PCB_QRY_RET_INT(res, 0);
					if ((o1.data.str == NULL) || (o2.data.str == NULL))
						PCB_QRY_RET_INT(res, 1);
					PCB_QRY_RET_INT(res, strcmp(o1.data.str, o2.data.str) != 0);
			}
			return -1;

		case PCBQ_OP_GTEQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd >= o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_INT(res, o1.data.dbl >= o2.data.dbl);
				case PCBQ_VT_STRING:   PCB_QRY_RET_INT(res, strcmp(o1.data.str, o2.data.str) >= 0);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_LTEQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd <= o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_INT(res, o1.data.dbl <= o2.data.dbl);
				case PCBQ_VT_STRING:   PCB_QRY_RET_INT(res, strcmp(o1.data.str, o2.data.str) <= 0);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_GT:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd > o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_INT(res, o1.data.dbl > o2.data.dbl);
				case PCBQ_VT_STRING:   PCB_QRY_RET_INT(res, strcmp(o1.data.str, o2.data.str) > 0);
				default:               return -1;
			}
			return -1;
		case PCBQ_OP_LT:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd < o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_INT(res, o1.data.dbl < o2.data.dbl);
				case PCBQ_VT_STRING:   PCB_QRY_RET_INT(res, strcmp(o1.data.str, o2.data.str) < 0);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_ADD:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd + o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_DBL(res, o1.data.dbl + o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_SUB:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd - o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_DBL(res, o1.data.dbl - o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_MUL:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd * o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_DBL(res, o1.data.dbl * o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_DIV:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_VOID:     PCB_QRY_RET_INV(res);
				case PCBQ_VT_COORD:    PCB_QRY_RET_INT(res, o1.data.crd / o2.data.crd);
				case PCBQ_VT_DOUBLE:   PCB_QRY_RET_DBL(res, o1.data.dbl / o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_MATCH:
			BINOPS1();
			{
				pcb_qry_node_t *o2n =node->data.children->next;
				char buff[128];
				const char *s;
				if (o2n->type != PCBQ_DATA_REGEX)
					return -1;
				s = op2str(buff, sizeof(buff), &o1);
				if (s != NULL)
					PCB_QRY_RET_INT(res, re_se_exec(o2n->precomp.regex, s));
			}
			PCB_QRY_RET_INV(res);

		case PCBQ_OP_NOT:
			UNOP();
			PCB_QRY_RET_INT(res, !pcb_qry_is_true(&o1));

		case PCBQ_FIELD_OF:
			if ((node->data.children == NULL) || (node->data.children->next == NULL))
				return -1;
			if (pcb_qry_eval(ctx, node->data.children, &o1) < 0)
				return -1;
			return pcb_qry_obj_field(&o1, node->data.children->next, res);

		case PCBQ_VAR:
			assert((node->data.crd >= 0) && (node->data.crd < ctx->iter->num_vars));
			res->type = PCBQ_VT_OBJ;
			if (ctx->iter->it[node->data.crd] == NULL)
				return -1;
			res->data.obj = *ctx->iter->it[node->data.crd];
			return 0;

		case PCBQ_LISTVAR: {
			int vi = pcb_qry_iter_var(ctx->iter, node->data.str, 0);
			if ((vi < 0) && (strcmp(node->data.str, "@") == 0)) {
				res->type = PCBQ_VT_LST;
				res->data.lst = ctx->all.data.lst;
				return 0;
			}
			if (vi >= 0) {
				res->type = PCBQ_VT_LST;
				res->data.lst = ctx->iter->lst[vi].data.lst;
				return 0;
			}
		}
		return -1;

		case PCBQ_FNAME:
			return -1; /* shall not eval such a node */

		case PCBQ_FCALL: {
			pcb_qry_val_t args[64];
			int n;
			pcb_qry_node_t *farg, *fname = node->data.children;
			if (fname == NULL)
				return -1;
			farg = fname->next;
			if ((fname->type !=  PCBQ_FNAME) || (fname->data.fnc == NULL))
				return -1;
			memset(args, 0, sizeof(args));
			for(n = 0; (n < 64) && (farg != NULL); n++, farg = farg->next)
				if (pcb_qry_eval(ctx, farg, &args[n]) < 0)
					return -1;

			if (farg != NULL) {
				pcb_message(PCB_MSG_ERROR, "too many function arguments\n");
				return -1;
			}
			printf("CALL n=%d\n", n);
			return fname->data.fnc(n, args, res);
		}

		case PCBQ_DATA_COORD:       PCB_QRY_RET_INT(res, node->data.crd);
		case PCBQ_DATA_DOUBLE:      PCB_QRY_RET_DBL(res, node->data.dbl);
		case PCBQ_DATA_STRING:      PCB_QRY_RET_STR(res, node->data.str);
		case PCBQ_DATA_CONST:       PCB_QRY_RET_INT(res, node->precomp.cnst);
		case PCBQ_DATA_INVALID:     PCB_QRY_RET_INV(res);

		/* not yet implemented: */
		case PCBQ_RULE:

		/* must not meet these while executing a node */
		case PCBQ_DATA_REGEX:
		case PCBQ_nodetype_max:
		case PCBQ_FIELD:
		case PCBQ_RNAME:
		case PCBQ_ITER_CTX:
			return -1;
	}
	return -1;
}
