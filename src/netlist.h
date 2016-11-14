/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_NETLIST_H
#define PCB_NETLIST_H

/* generic netlist operations */
#include "config.h"
#include "library.h"
#include "route_style.h"

struct pcb_net_s {								/* holds a net of connections */
	pcb_cardinal_t ConnectionN,					/* the number of connections contained */
	  ConnectionMax;							/* max connections from malloc */
	pcb_connection_t *Connection;
	pcb_route_style_t *Style;
};

typedef struct {								/* holds a list of nets */
	pcb_cardinal_t NetN,								/* the number of subnets contained */
	  NetMax;											/* max subnets from malloc */
	pcb_net_t *Net;
} pcb_netlist_t;

typedef struct {								/* holds a list of net lists */
	pcb_cardinal_t NetListN,						/* the number of net lists contained */
	  NetListMax;									/* max net lists from malloc */
	pcb_netlist_t *NetList;
} pcb_netlist_list_t;


void pcb_netlist_changed(int force_unfreeze);
pcb_lib_menu_t *pcb_netnode_to_netname(const char *nodename);
pcb_lib_menu_t *pcb_netname_to_netname(const char *netname);

int pcb_pin_name_to_xy(pcb_lib_entry_t *pin, pcb_coord_t *x, pcb_coord_t *y);
void pcb_netlist_find(pcb_lib_menu_t *net, pcb_lib_entry_t *pin);
void pcb_netlist_select(pcb_lib_menu_t *net, pcb_lib_entry_t *pin);
void pcb_netlist_rats(pcb_lib_menu_t *net, pcb_lib_entry_t *pin);
void pcb_netlist_norats(pcb_lib_menu_t *net, pcb_lib_entry_t *pin);
void pcb_netlist_clear(pcb_lib_menu_t *net, pcb_lib_entry_t *pin);
void pcb_netlist_style(pcb_lib_menu_t *net, const char *style);

/* Return the net entry for a pin name (slow search). The pin name is
   like "U101-5", so element's refdes, dash, pin number */
pcb_lib_menu_t *pcb_netlist_find_net4pinname(pcb_board_t *pcb, const char *pinname);

/* Same as pcb_netlist_find_net4pinname but with pin pointer */
pcb_lib_menu_t *pcb_netlist_find_net4pin(pcb_board_t *pcb, const pcb_pin_t *pin);
pcb_lib_menu_t *pcb_netlist_find_net4pad(pcb_board_t *pcb, const pcb_pad_t *pad);


/* Evaluate to const char * name of the network; lmt is (pcb_lib_menu_t *) */
#define pcb_netlist_name(lmt) ((const char *)((lmt)->Name+2))

/* Evaluate to 0 or 1 depending on whether the net is marked with *; lmt is (pcb_lib_menu_t *) */
#define pcb_netlist_is_bad(lmt) (((lmt)->Name[0]) == '*')


/* Return the index of the net or PCB_NETLIST_INVALID_INDEX if the net is not
   on the netlist. NOTE: indices returned are valid only until the first
   netlist change! */
pcb_cardinal_t pcb_netlist_net_idx(pcb_board_t *pcb, pcb_lib_menu_t *net);

#define PCB_NETLIST_INVALID_INDEX ((pcb_cardinal_t)(-1))

pcb_net_t *pcb_net_new(pcb_netlist_t *);
pcb_netlist_t *pcb_netlist_new(pcb_netlist_list_t *);
void pcb_netlist_list_free(pcb_netlist_list_t *);
void pcb_netlist_free(pcb_netlist_t *);
void pcb_net_free(pcb_net_t *);

#define PCB_NETLIST_LOOP(top) do   {                            \
        pcb_cardinal_t        n;                                \
        pcb_netlist_t *  netlist;                               \
        for (n = (top)->NetListN-1; n != -1; n--)               \
        {                                                       \
                netlist = &(top)->NetList[n]

#define PCB_NET_LOOP(top) do   {                                \
        pcb_cardinal_t        n;                                \
        pcb_net_t *  net;                                       \
        for (n = (top)->NetN-1; n != -1; n--)                   \
        {                                                       \
                net = &(top)->Net[n]


#endif
