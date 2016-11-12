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
	ConnectionTypePtr Connection;
	RouteStyleTypePtr Style;
};

typedef struct {								/* holds a list of nets */
	pcb_cardinal_t NetN,								/* the number of subnets contained */
	  NetMax;											/* max subnets from malloc */
	pcb_net_t *Net;
} NetListType, *NetListTypePtr;

typedef struct {								/* holds a list of net lists */
	pcb_cardinal_t NetListN,						/* the number of net lists contained */
	  NetListMax;									/* max net lists from malloc */
	NetListTypePtr NetList;
} NetListListType, *NetListListTypePtr;


void pcb_netlist_changed(int force_unfreeze);
LibraryMenuTypePtr pcb_netnode_to_netname(const char *nodename);
LibraryMenuTypePtr pcb_netname_to_netname(const char *netname);

int pcb_pin_name_to_xy(LibraryEntryType *pin, Coord *x, Coord *y);
void pcb_netlist_find(LibraryMenuType *net, LibraryEntryType *pin);
void pcb_netlist_select(LibraryMenuType *net, LibraryEntryType *pin);
void pcb_netlist_rats(LibraryMenuType *net, LibraryEntryType *pin);
void pcb_netlist_norats(LibraryMenuType *net, LibraryEntryType *pin);
void pcb_netlist_clear(LibraryMenuType *net, LibraryEntryType *pin);
void pcb_netlist_style(LibraryMenuType *net, const char *style);

/* Return the net entry for a pin name (slow search). The pin name is
   like "U101-5", so element's refdes, dash, pin number */
LibraryMenuTypePtr pcb_netlist_find_net4pinname(pcb_board_t *pcb, const char *pinname);

/* Same as pcb_netlist_find_net4pinname but with pin pointer */
LibraryMenuTypePtr pcb_netlist_find_net4pin(pcb_board_t *pcb, const PinType *pin);
LibraryMenuTypePtr pcb_netlist_find_net4pad(pcb_board_t *pcb, const PadType *pad);


/* Evaluate to const char * name of the network; lmt is (LibraryMenuType *) */
#define pcb_netlist_name(lmt) ((const char *)((lmt)->Name+2))

/* Evaluate to 0 or 1 depending on whether the net is marked with *; lmt is (LibraryMenuType *) */
#define pcb_netlist_is_bad(lmt) (((lmt)->Name[0]) == '*')


/* Return the index of the net or PCB_NETLIST_INVALID_INDEX if the net is not
   on the netlist. NOTE: indices returned are valid only until the first
   netlist change! */
pcb_cardinal_t pcb_netlist_net_idx(pcb_board_t *pcb, LibraryMenuType *net);

#define PCB_NETLIST_INVALID_INDEX ((pcb_cardinal_t)(-1))

pcb_net_t *GetNetMemory(NetListTypePtr);
NetListTypePtr GetNetListMemory(NetListListTypePtr);
void FreeNetListListMemory(NetListListTypePtr);
void FreeNetListMemory(NetListTypePtr);
void FreeNetMemory(pcb_net_t *);

#define NETLIST_LOOP(top) do   {                         \
        pcb_cardinal_t        n;                                      \
        NetListTypePtr   netlist;                               \
        for (n = (top)->NetListN-1; n != -1; n--)               \
        {                                                       \
                netlist = &(top)->NetList[n]

#define NET_LOOP(top) do   {                             \
        pcb_cardinal_t        n;                                      \
        pcb_net_t *  net;                                       \
        for (n = (top)->NetN-1; n != -1; n--)                   \
        {                                                       \
                net = &(top)->Net[n]


#endif
