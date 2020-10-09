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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_QUERY_H
#define PCB_QUERY_H

#include <genvector/vtp0.h>
#include <genvector/vti0.h>
#include <genht/htsi.h>
#include <genregex/regex_se.h>
#include "fields_sphash.h"
#include "obj_common.h"
#include "layer.h"
#include "flag_str.h"

typedef struct pcb_qry_val_s pcb_qry_val_t;
typedef struct pcb_query_iter_s  pcb_query_iter_t;
typedef struct pcb_qry_exec_s pcb_qry_exec_t;

typedef int (*pcb_qry_fnc_t)(pcb_qry_exec_t *ectx, int argc, pcb_qry_val_t *argv, pcb_qry_val_t *res);

/* value of an expression */
typedef enum pcb_qry_valtype_e {
	PCBQ_VT_VOID,
	PCBQ_VT_OBJ,
	PCBQ_VT_LST,
	PCBQ_VT_COORD,
	PCBQ_VT_LONG,
	PCBQ_VT_DOUBLE,
	PCBQ_VT_STRING
} pcb_qry_valtype_t;

typedef struct pcb_qry_node_s pcb_qry_node_t;

struct pcb_qry_val_s {
	pcb_qry_valtype_t type;
	union {
		pcb_any_obj_t *obj;
		vtp0_t lst;
		rnd_coord_t crd;
		double dbl;
		long lng;
		const char *str;
		struct {
			const char *str;
			char tmp[4];
		} local_str;
	} data;
	void *source; /* TODO: for the cache */
};

/* Script parsed into a tree */
typedef enum {
	PCBQ_RULE,
	PCBQ_RNAME,
	PCBQ_EXPR_PROG,
	PCBQ_EXPR,
	PCBQ_ASSERT,
	PCBQ_ITER_CTX,

	PCBQ_OP_THUS,
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
	PCBQ_OP_MATCH,
	
	PCBQ_OP_NOT,
	PCBQ_FIELD,
	PCBQ_FIELD_OF,
	PCBQ_LISTVAR,
	PCBQ_LET,
	PCBQ_VAR,
	PCBQ_FNAME,
	PCBQ_FCALL,
	PCBQ_FUNCTION,
	PCBQ_RETURN,
	PCBQ_ARG,

	PCBQ_FLAG,         /* leaf */

	PCBQ_DATA_COORD,   /* leaf */
	PCBQ_DATA_DOUBLE,  /* leaf */
	PCBQ_DATA_STRING,  /* leaf */
	PCBQ_DATA_REGEX,   /* leaf */
	PCBQ_DATA_CONST,   /* leaf */
	PCBQ_DATA_OBJ,     /* leaf */
	PCBQ_DATA_INVALID, /* leaf */
	PCBQ_DATA_LYTC,    /* leaf */

	PCBQ_nodetype_max
} pcb_qry_nodetype_t;

struct pcb_qry_node_s {
	pcb_qry_nodetype_t type;
	pcb_qry_node_t *next;         /* sibling on this level of the tree (or NULL) */
	pcb_qry_node_t *parent;
	union {                       /* field selection depends on ->type */
		rnd_coord_t crd;
		double dbl;
		const char *str;
		pcb_query_iter_t *iter_ctx;
		pcb_qry_node_t *children;   /* first child (NULL for a leaf node) */
		struct pcb_qry_lytc_s {
			pcb_layer_type_t lyt;
			pcb_layer_combining_t lyc;
		} lytc;
	} data;
	union {                       /* field selection depends on ->type */
		query_fields_keys_t fld;    /* field_sphash value from str */
		pcb_qry_val_t result;       /* of pure functions and subtrees and constant values converted e.g. to coord */
		re_se_t *regex;
		long cnst;                  /* named constant */
		pcb_any_obj_t *obj;
		const pcb_flag_bits_t *flg;
		vti0_t *it_active;
		struct {                    /* FCALL's FNAME*/
			pcb_qry_fnc_t bui;        /* if not NULL, then data.str is NULL and this is a builtin call; if NULL, this is an user function call to node .uf */
			pcb_qry_node_t *uf;
		} fnc;
	} precomp;
};


pcb_qry_node_t *pcb_qry_n_alloc(pcb_qry_nodetype_t ntype);
pcb_qry_node_t *pcb_qry_n_insert(pcb_qry_node_t *parent, pcb_qry_node_t *ch);
pcb_qry_node_t *pcb_qry_n_append(pcb_qry_node_t *parent, pcb_qry_node_t *ch);

void pcb_qry_dump_tree(const char *prefix, pcb_qry_node_t *top);

void pcb_qry_set_input(const char *script);

struct pcb_query_iter_s {
	htsi_t names;         /* name->index hash */
	int num_vars, last_active;

	const char **vn;      /* pointers to the hash names so they can be indexed */
	pcb_qry_val_t *lst;
	vti0_t *it_active;    /* points to a char array of which iterators are active */

	/* iterator state for each variable - point into the corresponding lst[] */
	vtp0_t **vects;
	rnd_cardinal_t *idx;

	pcb_any_obj_t *last_obj; /* last object retrieved by the expression */
};

pcb_query_iter_t *pcb_qry_iter_alloc(void);
void pcb_qry_iter_free(pcb_query_iter_t *it);
int pcb_qry_iter_var(pcb_query_iter_t *it, const char *varname, int alloc);
void pcb_qry_iter_init(pcb_query_iter_t *it);

void pcb_qry_n_free(pcb_qry_node_t *nd);

pcb_query_iter_t *pcb_qry_find_iter(pcb_qry_node_t *node);

char *pcb_query_sprint_val(pcb_qry_val_t *val);

const char *pcb_qry_nodetype_name(pcb_qry_nodetype_t ntype);
int pcb_qry_nodetype_has_children(pcb_qry_nodetype_t ntype);

/* functions */
int pcb_qry_fnc_reg(const char *name, pcb_qry_fnc_t fnc);
pcb_qry_fnc_t pcb_qry_fnc_lookup(const char *name);
const char *pcb_qry_fnc_name(pcb_qry_fnc_t fnc);

/* parser */
void qry_error(void *prog, const char *err);

/* external entry points for convenience */

/* Compile and execute a script, calling cb for each object. Returns the number
   of evaluation errors or -1 if evaluation couldn't start */
int pcb_qry_run_script(pcb_qry_exec_t *ec, pcb_board_t *pcb, const char *script, const char *scope, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current), void *user_ctx);

/* Compile a script; returns NULL on error, else the caller needs to call
  pcb_qry_n_free() on the result to free it after use */
pcb_qry_node_t *pcb_query_compile(const char *script);

/* Extract a list of definitions used by the script (by compiling the script);
   dst key is the definition name, val is the number of uses. Return value
   is total number of definition usage. */
int pcb_qry_extract_defs(htsi_t *dst, const char *script);

/*** drc list-reports ***/

/* dynamic alloced fake objects that store constants */

#define PCB_OBJ_QRY_CONST  0x0100000
typedef struct pcb_obj_qry_const_s {
	PCB_ANY_OBJ_FIELDS;
	pcb_qry_val_t val;
} pcb_obj_qry_const_t;

/* static alloced marker fake objects */
typedef enum {
	PCB_QRY_DRC_GRP1,
	PCB_QRY_DRC_GRP2,
	PCB_QRY_DRC_EXPECT,
	PCB_QRY_DRC_MEASURE,
	PCB_QRY_DRC_TEXT,
	PCB_QRY_DRC_invalid
} pcb_qry_drc_ctrl_t;

extern pcb_any_obj_t pcb_qry_drc_ctrl[PCB_QRY_DRC_invalid];

#define pcb_qry_drc_ctrl_decode(obj) \
((((obj) >= &pcb_qry_drc_ctrl[0]) && ((obj) < &pcb_qry_drc_ctrl[PCB_QRY_DRC_invalid])) \
	? ((obj) - &pcb_qry_drc_ctrl[0]) : PCB_QRY_DRC_invalid)



#endif
