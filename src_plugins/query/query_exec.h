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

/* Query language - execution */

#ifndef PCB_QUERY_EXEC_H
#define PCB_QUERY_EXEC_H

typedef struct pcb_qry_exec_s {
	pcb_qry_node_t *root;
	pcb_qry_val_t all;       /* a list of all objects */
	pcb_query_iter_t *iter;  /* current iterator */
} pcb_qry_exec_t;

void pcb_qry_init(pcb_qry_exec_t *ctx, pcb_qry_node_t *root);
void pcb_qry_uninit(pcb_qry_exec_t *ctx);

int pcb_qry_run(pcb_qry_node_t *prg, void (*cb)(void *user_ctx, pcb_qry_val_t *res, pcb_any_obj_t *current), void *user_ctx);

int pcb_qry_is_true(pcb_qry_val_t *val);

int pcb_qry_eval(pcb_qry_exec_t *ctx, pcb_qry_node_t *node, pcb_qry_val_t *res);

int pcb_qry_it_reset(pcb_qry_exec_t *ctx, pcb_qry_node_t *node);

/* Returns 1 if context iterator is valid, 0 if the loop is over */
int pcb_qry_it_next(pcb_qry_exec_t *ctx);



/* Helper macros: load value o and return 0 */
#define PCB_QRY_RET_INT(o, value) \
do { \
	o->type = PCBQ_VT_LONG; \
	o->data.lng = value; \
	return 0; \
} while(0)

#define PCB_QRY_RET_DBL(o, value) \
do { \
	o->type = PCBQ_VT_DOUBLE; \
	o->data.dbl = value; \
	return 0; \
} while(0)

#define PCB_QRY_RET_COORD(o, value) \
do { \
	o->type = PCBQ_VT_COORD; \
	o->data.crd = value; \
	return 0; \
} while(0)

#define PCB_QRY_RET_STR(o, value) \
do { \
	o->type = PCBQ_VT_STRING; \
	o->data.str = value; \
	return 0; \
} while(0)

#define PCB_QRY_RET_SIDE(o, on_bottom) \
do { \
	o->type = PCBQ_VT_STRING; \
	o->data.str = on_bottom ? "BOTTOM" : "TOP"; \
	return 0; \
} while(0)

/* The case when the operation couldn't be carried out, sort of NaN */
#define PCB_QRY_RET_INV(o) \
do { \
	o->type = PCBQ_VT_VOID; \
	return 0; \
} while(0)


#endif
