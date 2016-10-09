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

#ifndef PCB_QUERY_H
#define PCB_QUERY_H

#include "obj_any.h"
#include <genht/htsi.h>

typedef struct pcb_qry_val_s pcb_qry_val_t;
typedef struct pcb_query_iter_s  pcb_query_iter_t;

typedef int (*pcb_qry_fnc_t)(int argc, pcb_qry_val_t *argv[], pcb_qry_val_t *res);

/* value of an expression */
typedef enum pcb_qry_valtype_e {
	PCBQ_VT_VOID,
	PCBQ_VT_OBJ,
	PCBQ_VT_LST,
	PCBQ_VT_COORD,   /* considered int */
	PCBQ_VT_DOUBLE,
	PCBQ_VT_STRING
} pcb_qry_valtype_t;

struct pcb_qry_val_s {
	pcb_qry_valtype_t type;
	union {
		pcb_obj_t obj;
		pcb_objlist_t lst;
		Coord crd;
		double dbl;
		const char *str;
	} data;
};

/* Script parsed into a tree */
typedef struct pcb_qry_node_s pcb_qry_node_t;
typedef enum {
	PCBQ_RULE,
	PCBQ_RNAME,
	PCBQ_EXPR_PROG,
	PCBQ_EXPR,
	PCBQ_ITER_CTX,

	PCBQ_OP_AND,
	PCBQ_OP_OR,
	PCBQ_OP_EQ,
	PCBQ_OP_NEQ,
	PCBQ_OP_GTEQ,
	PCBQ_OP_LTEQ,
	PCBQ_OP_GT,
	PCBQ_OP_LT,
	PCBQ_OP_ADD,
	PCBQ_OP_SUB,
	PCBQ_OP_MUL,
	PCBQ_OP_DIV,

	PCBQ_OP_NOT,
	PCBQ_FIELD,
	PCBQ_FIELD_OF,
	PCBQ_VAR,
	PCBQ_FNAME,
	PCBQ_FCALL,

	PCBQ_DATA_COORD,   /* leaf */
	PCBQ_DATA_DOUBLE,  /* leaf */
	PCBQ_DATA_STRING,  /* leaf */

	PCBQ_nodetype_max
} pcb_qry_nodetype_t;


struct pcb_qry_node_s {
	pcb_qry_nodetype_t type;
	pcb_qry_node_t *next;       /* sibling on this level of the tree (or NULL) */
	pcb_qry_node_t *parent;
	union {
		Coord crd;
		double dbl;
		const char *str;
		pcb_query_iter_t *iter_ctx;
		pcb_qry_node_t *children;   /* first child (NULL for a leaf node) */
		pcb_qry_fnc_t fnc;
	} data;
	pcb_qry_val_t result;
};


pcb_qry_node_t *pcb_qry_n_alloc(pcb_qry_nodetype_t ntype);
pcb_qry_node_t *pcb_qry_n_insert(pcb_qry_node_t *parent, pcb_qry_node_t *ch);

void pcb_qry_dump_tree(const char *prefix, pcb_qry_node_t *top);

void pcb_qry_set_input(const char *script);


struct pcb_query_iter_s {
	htsi_t names;         /* name->index hash */
	int num_vars;

	const char **vn;      /* pointers to the hash names so they can be indexed */
	pcb_qry_val_t *lst;
	pcb_obj_t **it;       /* iterator state for each variable - point into the correspoinding lst[] */

	int all_idx;          /* index of the "@" variable or -1 */
};

pcb_query_iter_t *pcb_qry_iter_alloc(void);
int pcb_qry_iter_var(pcb_query_iter_t *it, const char *varname);
void pcb_qry_iter_init(pcb_query_iter_t *it);

pcb_query_iter_t *pcb_qry_find_iter(pcb_qry_node_t *node);

char *pcb_query_sprint_val(pcb_qry_val_t *val);

/* functions */
int pcb_qry_fnc_reg(const char *name, pcb_qry_fnc_t fnc);
pcb_qry_fnc_t pcb_qry_fnc_lookup(const char *name);
const char *pcb_qry_fnc_name(pcb_qry_fnc_t fnc);

#endif
