/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include "obj_common.h"

typedef htsp_t pcb_netlist_t;

typedef struct pcb_net_term_s pcb_net_term_t;


struct pcb_net_term_s {
	PCB_ANY_OBJ_FIELDS;
	char *refdes;
	char *term;
	gdl_elem_t link; /* a net is mainly an ordered list of terminals */
};

/* List of refdes-terminals */
#define TDL(x)      pcb_termlist_ ## x
#define TDL_LIST_T  pcb_termlist_t
#define TDL_ITEM_T  pcb_net_term_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define pcb_termlist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))


#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>


struct pcb_net_s {
	PCB_ANY_OBJ_FIELDS;
	char *name;
	pcb_termlist_t conns;
};


/* Initialize an empty netlist */
void pcb_netlist_init(pcb_netlist_t *nl);

/* Free all memory (including nets and terminals) of a netlist */
void pcb_netlist_uninit(pcb_netlist_t *nl);

/* Look up (or allocate) a net by name within a netlist. Returns NULL on error */
pcb_net_t *pcb_net_get(pcb_board_t *pcb, pcb_netlist_t *nl, const char *netname, pcb_bool alloc);

/* Remove a net from a netlist by namel returns 0 on removal, -1 on error */
int pcb_net_del(pcb_netlist_t *nl, const char *netname);

/* Look up (or allocate) a terminal within a net. Returns NULL on error */
pcb_net_term_t *pcb_net_term_get(pcb_net_t *net, const char *refdes, const char *term, pcb_bool alloc);

/* Remove term from its net and free all fields and term itself */
int pcb_net_term_del(pcb_net_t *net, pcb_net_term_t *term);
int pcb_net_term_del_by_name(pcb_net_t *net, const char *refdes, const char *term);


/* Crawl a net and clear&set flags on each object belonging to the net
   and. Return the number of objects found */
pcb_cardinal_t pcb_net_crawl_flag(pcb_board_t *pcb, pcb_net_t *net, unsigned long setf, unsigned long clrf);


/* Slow, linear search for a terminal, by pinname ("refdes-pinnumber") or
   separate refdes and terminal ID. */
pcb_net_term_t *pcb_net_find_by_pinname(pcb_netlist_t *nl, const char *pinname);
pcb_net_term_t *pcb_net_find_by_refdes_term(pcb_netlist_t *nl, const char *refdes, const char *term);

pcb_bool pcb_net_name_valid(const char *netname);


/*** Internal ***/
void pcb_net_free_fields(pcb_net_t *net);
void pcb_net_free(pcb_net_t *net);
void pcb_net_term_free_fields(pcb_net_term_t *term);
void pcb_net_term_free(pcb_net_term_t *term);
