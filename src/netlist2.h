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

#ifndef PCB_NETLIST2_H
#define PCB_NETLIST2_H

#include <genht/htsp.h>
#include <genlist/gendlist.h>
#include <genvector/vtp0.h>
#include "board.h"
#include "obj_common.h"

struct pcb_net_term_s {
	PCB_ANY_OBJ_FIELDS;
	char *refdes;
	char *term;
	gdl_elem_t link; /* a net is mainly an ordered list of terminals */
};

typedef enum { /* bitfield */
	PCB_RATACC_PRECISE = 1,            /* find the shortest rats, precisely (expensive); if unset, use a simplified algo e.g. considering only endpoints of lines */
	PCB_RATACC_ONLY_MANHATTAN = 2,     /* the old autorouter doesn't like non-manhattan lines and arcs */
	PCB_RATACC_ONLY_SELECTED = 4,
	PCB_RATACC_INFO = 8                /* print INFO messages in the log about how many rats are to go */
} pcb_rat_accuracy_t;

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
	pcb_cardinal_t export_tmp; /* filled in and used by export code; valid only until the end of exporting */
	unsigned inhibit_rats:1;
	pcb_termlist_t conns;
};


/* Initialize an empty netlist */
void pcb_netlist_init(pcb_netlist_t *nl);

/* Free all memory (including nets and terminals) of a netlist */
void pcb_netlist_uninit(pcb_netlist_t *nl);

/* Copy all fields from src to dst, assuming dst is empty */
void pcb_netlist_copy(pcb_board_t *pcb, pcb_netlist_t *dst, pcb_netlist_t *src);

/* Look up (or allocate) a net by name within a netlist. Returns NULL on error */
pcb_net_t *pcb_net_get(pcb_board_t *pcb, pcb_netlist_t *nl, const char *netname, pcb_bool alloc);
pcb_net_t *pcb_net_get_icase(pcb_board_t *pcb, pcb_netlist_t *nl, const char *name); /* read-only, case-insnensitive */
pcb_net_t *pcb_net_get_regex(pcb_board_t *pcb, pcb_netlist_t *nl, const char *regex);
pcb_net_t *pcb_net_get_user(pcb_board_t *pcb, pcb_netlist_t *nl, const char *name_or_rx); /* run all three above in order, until one succeeds */


/* Remove a net from a netlist by namel returns 0 on removal, -1 on error */
int pcb_net_del(pcb_netlist_t *nl, const char *netname);

/* Look up (or allocate) a terminal within a net. Pinname is "refdes-termid".
   Returns NULL on error */
pcb_net_term_t *pcb_net_term_get(pcb_net_t *net, const char *refdes, const char *term, pcb_bool alloc);
pcb_net_term_t *pcb_net_term_get_by_obj(pcb_net_t *net, const pcb_any_obj_t *obj);
pcb_net_term_t *pcb_net_term_get_by_pinname(pcb_net_t *net, const char *pinname, pcb_bool alloc);

/* Remove term from its net and free all fields and term itself */
int pcb_net_term_del(pcb_net_t *net, pcb_net_term_t *term);
int pcb_net_term_del_by_name(pcb_net_t *net, const char *refdes, const char *term);


/* Crawl a net and clear&set flags on each object belonging to the net
   and. Return the number of objects found */
pcb_cardinal_t pcb_net_crawl_flag(pcb_board_t *pcb, pcb_net_t *net, unsigned long setf, unsigned long clrf);


/* Slow, linear search for a terminal, by pinname ("refdes-pinnumber") or
   separate refdes and terminal ID. */
pcb_net_term_t *pcb_net_find_by_pinname(const pcb_netlist_t *nl, const char *pinname);
pcb_net_term_t *pcb_net_find_by_refdes_term(const pcb_netlist_t *nl, const char *refdes, const char *term);
pcb_net_term_t *pcb_net_find_by_obj(const pcb_netlist_t *nl, const pcb_any_obj_t *obj);

/* Create an alphabetic sorted, NULL terminated array from the nets;
   the return value is valid until any change to nl and should be free'd
   by the caller. Pointers in the array are the same as in the has table,
   should not be free'd. */
pcb_net_t **pcb_netlist_sort(pcb_netlist_t *nl);

/* Create missing rat lines */
pcb_cardinal_t pcb_net_add_rats(const pcb_board_t *pcb, pcb_net_t *net, pcb_rat_accuracy_t acc);
pcb_cardinal_t pcb_net_add_all_rats(const pcb_board_t *pcb, pcb_rat_accuracy_t acc);

/* Create a new network or a new net connection by drawing a rat line between two terminals */
pcb_rat_t *pcb_net_create_by_rat_coords(pcb_board_t *pcb, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_bool interactive);

/* Undoably remove all non-subc-part copper objects that are connected to net.
   Return the number of removals. */
pcb_cardinal_t pcb_net_ripup(pcb_board_t *pcb, pcb_net_t *net);

void pcb_netlist_changed(int force_unfreeze);

pcb_bool pcb_net_name_valid(const char *netname);

/*** subnet mapping ***/

typedef struct {
	const pcb_board_t *pcb;
	pcb_net_t *current_net;
	htsp_t found;
	pcb_cardinal_t changed, missing, num_shorts;
} pcb_short_ctx_t;

void pcb_net_short_ctx_init(pcb_short_ctx_t *sctx, const pcb_board_t *pcb, pcb_net_t *net);
void pcb_net_short_ctx_uninit(pcb_short_ctx_t *sctx);

/* Search and collect all subnets of a net, adding rat lines in between them.
   Caller provided subnets is a vector of vtp0_t items that each contain
   (pcb_any_obj_t *) pointers to subnet objects */
pcb_cardinal_t pcb_net_map_subnets(pcb_short_ctx_t *sctx, pcb_rat_accuracy_t acc, vtp0_t *subnets);

void pcb_net_reset_subnets(vtp0_t *subnets); /* clear the subnet list to zero items, but don't free the array ("malloc cache") */
void pcb_net_free_subnets(vtp0_t *subnets);  /* same as reset but also free the array */



/*** looping ***/

typedef struct pcb_net_it_s {
	pcb_netlist_t *nl;
	htsp_entry_t *next;
} pcb_net_it_t;

PCB_INLINE pcb_net_t *pcb_net_next(pcb_net_it_t *it)
{
	pcb_net_t *res;
	if (it->next == NULL)
		return NULL;
	res = it->next->value;
	it->next = htsp_next(it->nl, it->next);
	return res;
}

PCB_INLINE pcb_net_t *pcb_net_first(pcb_net_it_t *it, pcb_netlist_t *nl)
{
	it->nl = nl;
	it->next = htsp_first(nl);
	return pcb_net_next(it);
}

/*** Internal ***/
void pcb_net_free_fields(pcb_net_t *net);
void pcb_net_free(pcb_net_t *net);
void pcb_net_term_free_fields(pcb_net_term_t *term);
void pcb_net_term_free(pcb_net_term_t *term);

void pcb_netlist_geo_init(void);

#endif
