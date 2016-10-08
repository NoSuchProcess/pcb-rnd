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

#ifndef PCB_QUERY_EXEC_H
#define PCB_QUERY_EXEC_H

typedef struct pcb_qry_exec_s {
	pcb_qry_node_t *root;
	pcb_qry_val_t all;       /* a list of all objects */
	pcb_query_iter_t *iter;  /* current iterator */
} pcb_qry_exec_t;

void pcb_qry_init(pcb_qry_exec_t *ctx, pcb_qry_node_t *root);
void pcb_qry_uninit(pcb_qry_exec_t *ctx);

int pcb_qry_is_true(pcb_qry_val_t *val);

int pcb_qry_eval(pcb_qry_exec_t *ctx, pcb_qry_node_t *node, pcb_qry_val_t *res);

#endif
