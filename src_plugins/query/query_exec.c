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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Query language - execution */

#include "global.h"
#include "data.h"
#include "query.h"
#include "query_exec.h"
#include "query_access.h"

void pcb_qry_init(pcb_qry_exec_t *ctx, pcb_qry_node_t *root)
{
	memset(ctx, 0, sizeof(pcb_qry_exec_t));
	ctx->all.type = PCBQ_VT_LST;
	pcb_qry_list_all(&ctx->all, PCB_OBJ_ANY);
	ctx->root = root;
}

void pcb_qry_uninit(pcb_qry_exec_t *ctx)
{
	pcb_qry_list_free(&ctx->all);
}

#define BINOPS1() \
do { \
	if ((node->data.children == NULL) || (node->data.children->next == NULL) || (node->data.children->next->next != NULL)) \
		return -1; \
	if (pcb_qry_eval(ctx, node->data.children, &o1) < 0) \
		return -1; \
} while(0)

#define BINOPS2() \
do { \
	if (pcb_qry_eval(ctx, node->data.children->next, &o2) < 0) \
		return -1; \
} while(0)

#define BINOPS() \
do { \
	BINOPS1(); \
	BINOPS2(); \
} while(0)

#define RET_INT(o, value) \
do { \
	o->type = PCBQ_VT_COORD; \
	o->data.crd = value; \
	return 0; \
} while(0)

#define RET_DBL(o, value) \
do { \
	o->type = PCBQ_VT_DOUBLE; \
	o->data.dbl = value; \
	return 0; \
} while(0)

#define RET_STR(o, value) \
do { \
	o->type = PCBQ_VT_STRING; \
	o->data.str = value; \
	return 0; \
} while(0)

static int promote(pcb_qry_val_t *a, pcb_qry_val_t *b)
{
	if ((a->type == PCBQ_VT_VOID) || (b->type == PCBQ_VT_VOID))
		return -1;
	if (a->type == b->type)
		return 0;
	switch(a->type) {
		case PCBQ_VT_OBJ:   return -1;
		case PCBQ_VT_LST:   return -1;
		case PCBQ_VT_COORD:
			if (b->type == PCBQ_VT_DOUBLE)  RET_DBL(a, a->data.crd);
			return -1;
		case PCBQ_VT_DOUBLE:
			if (b->type == PCBQ_VT_COORD)  RET_DBL(b, b->data.crd);
			return -1;
		case PCBQ_VT_STRING:
		case PCBQ_VT_VOID:
			return -1;
	}
	return -1;
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

int pcb_qry_eval(pcb_qry_exec_t *ctx, pcb_qry_node_t *node, pcb_qry_val_t *res)
{
	pcb_qry_val_t o1, o2;

	switch(node->type) {
		case PCBQ_EXPR:
			return pcb_qry_eval(ctx, node->data.children, res);

		case PCBQ_OP_AND: /* lazy */
			BINOPS1();
			if (!pcb_qry_is_true(&o1))
				RET_INT(res, 0);
			BINOPS2();
			if (!pcb_qry_is_true(&o2))
				RET_INT(res, 0);
			RET_INT(res, 1);

		case PCBQ_OP_OR: /* lazy */
			BINOPS1();
			if (pcb_qry_is_true(&o1))
				RET_INT(res, 1);
			BINOPS2();
			if (pcb_qry_is_true(&o2))
				RET_INT(res, 1);
			RET_INT(res, 0);

		case PCBQ_OP_EQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_OBJ:      RET_INT(res, ((o1.data.obj.type) == (o2.data.obj.type)) && ((o1.data.obj.data.any) == (o2.data.obj.data.any)));
				case PCBQ_VT_LST:      RET_INT(res, pcb_qry_list_cmp(&o1, &o2));
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd == o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_INT(res, o1.data.dbl == o2.data.dbl);
				case PCBQ_VT_STRING:   RET_INT(res, strcmp(o1.data.str, o2.data.str) == 0);
				case PCBQ_VT_VOID:     return -1;
			}
			return -1;

		case PCBQ_OP_NEQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_OBJ:      RET_INT(res, ((o1.data.obj.type) != (o2.data.obj.type)) || ((o1.data.obj.data.any) != (o2.data.obj.data.any)));
				case PCBQ_VT_LST:      RET_INT(res, !pcb_qry_list_cmp(&o1, &o2));
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd != o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_INT(res, o1.data.dbl != o2.data.dbl);
				case PCBQ_VT_STRING:   RET_INT(res, strcmp(o1.data.str, o2.data.str) != 0);
				case PCBQ_VT_VOID:     return -1;
			}
			return -1;

		case PCBQ_OP_GTEQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd >= o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_INT(res, o1.data.dbl >= o2.data.dbl);
				case PCBQ_VT_STRING:   RET_INT(res, strcmp(o1.data.str, o2.data.str) >= 0);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_LTEQ:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd <= o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_INT(res, o1.data.dbl <= o2.data.dbl);
				case PCBQ_VT_STRING:   RET_INT(res, strcmp(o1.data.str, o2.data.str) <= 0);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_GT:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd > o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_INT(res, o1.data.dbl > o2.data.dbl);
				case PCBQ_VT_STRING:   RET_INT(res, strcmp(o1.data.str, o2.data.str) > 0);
				default:               return -1;
			}
			return -1;
		case PCBQ_OP_LT:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd < o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_INT(res, o1.data.dbl < o2.data.dbl);
				case PCBQ_VT_STRING:   RET_INT(res, strcmp(o1.data.str, o2.data.str) < 0);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_ADD:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd + o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_DBL(res, o1.data.dbl + o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_SUB:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd - o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_DBL(res, o1.data.dbl - o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_MUL:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd * o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_DBL(res, o1.data.dbl * o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_DIV:
			BINOPS();
			if (promote(&o1, &o2) != 0)
				return -1;
			switch(o1.type) {
				case PCBQ_VT_COORD:    RET_INT(res, o1.data.crd / o2.data.crd);
				case PCBQ_VT_DOUBLE:   RET_DBL(res, o1.data.dbl / o2.data.dbl);
				default:               return -1;
			}
			return -1;

		case PCBQ_OP_NOT:
		case PCBQ_FIELD:
		case PCBQ_OBJ:
		case PCBQ_VAR:
		case PCBQ_FNAME:
		case PCBQ_FCALL:
			return -1;

		case PCBQ_DATA_COORD:       RET_INT(res, node->data.crd);
		case PCBQ_DATA_DOUBLE:      RET_DBL(res, node->data.dbl);
		case PCBQ_DATA_STRING:      RET_STR(res, node->data.str);

		case PCBQ_nodetype_max:
			return -1;
	}
	return -1;
}

