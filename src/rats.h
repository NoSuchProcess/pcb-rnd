/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *	rats.c, rats.h Copyright (C) 1997, harry eaton
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

/* prototypes for rats routines */

#ifndef PCB_RATS_H
#define PCB_RATS_H

#include "config.h"
#include "netlist.h"

/* ---------------------------------------------------------------------------
 * structure used by device drivers
 */


struct pcb_connection_s {				/* holds a connection (rat) */
	Coord X, Y;										/* coordinate of connection */
	long int type;								/* type of object in ptr1 - 3 */
	void *ptr1, *ptr2;						/* the object of the connection */
	pcb_cardinal_t group;								/* the layer group of the connection */
	pcb_lib_menu_t *menu;				/* the netmenu this *SHOULD* belong too */
};

pcb_rat_t *AddNet(void);
char *ConnectionName(int, void *, void *);

pcb_bool AddAllRats(pcb_bool, void (*)(register pcb_connection_t *, register pcb_connection_t *, register RouteStyleTypePtr));
pcb_bool SeekPad(pcb_lib_entry_t *, pcb_connection_t *, pcb_bool);

NetListTypePtr ProcNetlist(pcb_lib_t *);
NetListListType CollectSubnets(pcb_bool);
pcb_connection_t *GetConnectionMemory(pcb_net_t *);

#define CONNECTION_LOOP(net) do {                         \
        pcb_cardinal_t        n;                                      \
        pcb_connection_t *      connection;                     \
        for (n = (net)->ConnectionN-1; n != -1; n--)            \
        {                                                       \
                connection = & (net)->Connection[n]

#define RAT_LOOP(top) do {                                          \
  pcb_rat_t *line;                                                    \
  gdl_iterator_t __it__;                                            \
  ratlist_foreach(&(top)->Rat, &__it__, line) {


#endif
