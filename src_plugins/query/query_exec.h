/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

/* Query language - execution */

#ifndef PCB_QUERY_EXEC_H
#define PCB_QUERY_EXEC_H

#include "query.h"
#include <genht/htpp.h>
#include <genht/htpi.h>
#include <time.h>

#define PCB_QRY_MAX_FUNC_ARGS 64

struct pcb_qry_exec_s {
	pcb_board_t *pcb;
	pcb_qry_node_t *root;
	pcb_qry_val_t all;       /* a list of all objects */
	pcb_query_iter_t *iter;  /* current iterator */
	vtp0_t autofree;

	int (*progress_cb)(pcb_qry_exec_t *ec, long at, long total);
	void *progress_ctx;

	/* data/call cache */
	htpp_t obj2netterm;   /* (pcb_any_obj_t *) object -> (pcb_any_obj_t *) terminal with the lowest ID on the net segment; for floating segments any object with the lowest ID will be accepted */
	htpp_t obj2lenseg;    /* (pcb_any_obj_t *) object -> (pcb_any_obj_t *) length-segment head: each length-segment is a junction-free part of the network */
	htpi_t obj2lenseg_cc; /* (pcb_any_obj_t *) object -> connection-counter (how many different objects are connected) */
	vtp0_t obj2lenseg_free; /* delayed free of obj2lenseg_cc objects */
	vtp0_t tmplst;        /* may be reused in a function call to save on allocation; always clear it at the end of the fnc */
	time_t last_prog_cb;

	unsigned obj2netterm_inited:1;
	unsigned obj2lenseg_inited:1;
};

/* if bufno is -1, scope is the board, else scope is the buffer addressed by bufno */
void pcb_qry_init(pcb_qry_exec_t *ctx, pcb_board_t *pcb, pcb_qry_node_t *root, int bufno);
void pcb_qry_uninit(pcb_qry_exec_t *ctx);

/* parts of init/uninit for the case a single pcb_qry_exec_t is used for
   multiple queries for sharing the cache */
void pcb_qry_setup(pcb_qry_exec_t *ctx, pcb_board_t *pcb, pcb_qry_node_t *root);
void pcb_qry_autofree(pcb_qry_exec_t *ctx);

/* Execute an expression or a rule; if ec is NULL, use a temporary context and
   free it before returning; else ec is the shared context for multiple queries
   with persistent cache */
int pcb_qry_run(pcb_qry_exec_t *ec, pcb_board_t *pcb, pcb_qry_node_t *prg, int bufno, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current), void *user_ctx);

int pcb_qry_is_true(pcb_qry_val_t *val);

int pcb_qry_eval(pcb_qry_exec_t *ctx, pcb_qry_node_t *node, pcb_qry_val_t *res);

int pcb_qry_it_reset(pcb_qry_exec_t *ctx, pcb_qry_node_t *node);

/* Returns 1 if context iterator is valid, 0 if the loop is over */
int pcb_qry_it_next(pcb_qry_exec_t *ctx);



/* Helper macros: load value o and return 0 */
#define PCB_QRY_RET_INT_SRC(o, value, node) \
do { \
	o->source = node; \
	o->type = PCBQ_VT_LONG; \
	o->data.lng = value; \
	return 0; \
} while(0)
#define PCB_QRY_RET_INT(o, value) PCB_QRY_RET_INT_SRC(o, value, NULL)


#define PCB_QRY_RET_OBJ_SRC(o, value, node) \
do { \
	o->source = node; \
	o->type = PCBQ_VT_OBJ; \
	o->data.obj = value; \
	return 0; \
} while(0)
#define PCB_QRY_RET_OBJ(o, value) PCB_QRY_RET_OBJ_SRC(o, value, NULL)

#define PCB_QRY_RET_DBL_SRC(o, value, node) \
do { \
	o->source = node; \
	o->type = PCBQ_VT_DOUBLE; \
	o->data.dbl = value; \
	return 0; \
} while(0)
#define PCB_QRY_RET_DBL(o, value) PCB_QRY_RET_DBL_SRC(o, value, NULL)

#define PCB_QRY_RET_COORD_SRC(o, value, node) \
do { \
	o->source = node; \
	o->type = PCBQ_VT_COORD; \
	o->data.crd = value; \
	return 0; \
} while(0)
#define PCB_QRY_RET_COORD(o, value) PCB_QRY_RET_COORD_SRC(o, value, NULL)

#define PCB_QRY_RET_STR_SRC(o, value, node) \
do { \
	o->source = node; \
	o->type = PCBQ_VT_STRING; \
	o->data.str = value; \
	return 0; \
} while(0)
#define PCB_QRY_RET_STR(o, value) PCB_QRY_RET_STR_SRC(o, value, NULL)

#define PCB_QRY_RET_SIDE_SRC(o, on_bottom, node) \
do { \
	o->source = node; \
	o->type = PCBQ_VT_STRING; \
	o->data.str = on_bottom ? "BOTTOM" : "TOP"; \
	return 0; \
} while(0)
#define PCB_QRY_RET_SIDE(o, value) PCB_QRY_RET_SIDE_SRC(o, value, NULL)

/* The case when the operation couldn't be carried out, sort of NaN */
#define PCB_QRY_RET_INV_SRC(o, node) \
do { \
	o->source = node; \
	o->type = PCBQ_VT_VOID; \
	return 0; \
} while(0)
#define PCB_QRY_RET_INV(o) PCB_QRY_RET_INV_SRC(o, NULL)

/* Convert src_arg to coordinate and cache the result if src_arg is tied
   to a tree node */
#define PCB_QRY_ARG_CONV_TO_COORD(dst_crd, src_arg, err_inst) \
do { \
	rnd_bool succ; \
	if ((src_arg)->type == PCBQ_VT_COORD)  { dst_crd = (src_arg)->data.crd; break; } \
	if ((src_arg)->type == PCBQ_VT_LONG)   { dst_crd = (src_arg)->data.lng; break; } \
	if ((src_arg)->type == PCBQ_VT_DOUBLE) { dst_crd = rnd_round((src_arg)->data.dbl); break; } \
	if ((src_arg)->type == PCBQ_VT_STRING) { \
		dst_crd = rnd_get_value((src_arg)->data.str, NULL, NULL, &succ); \
		if (succ) break; \
	} \
	err_inst; \
} while(0)

#endif
