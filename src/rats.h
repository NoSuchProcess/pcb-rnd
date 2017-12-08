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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_RATS_H
#define PCB_RATS_H

#include "config.h"
#include "netlist.h"
#include "layer.h"
#include "layer_grp.h"

struct pcb_connection_s {  /* holds a connection (rat) */
	pcb_coord_t X, Y;        /* coordinate of connection */
	void *ptr1;              /* parent of ptr2??? */
	pcb_any_obj_t *obj;      /* the object of the connection */
	pcb_layergrp_id_t group; /* the layer group of the connection */
	pcb_lib_menu_t *menu;    /* the netmenu this *SHOULD* belong too */
};

pcb_rat_t *pcb_rat_add_net(void);
char *pcb_connection_name(pcb_any_obj_t *obj);

pcb_bool pcb_rat_add_all(pcb_bool SelectedOnly, void (*funcp) (register pcb_connection_t *, register pcb_connection_t *, register pcb_route_style_t *));

pcb_bool pcb_rat_seek_pad(pcb_lib_entry_t * entry, pcb_connection_t * conn, pcb_bool Same);


pcb_netlist_t *pcb_rat_proc_netlist(pcb_lib_t *net_menu);

pcb_netlist_list_t pcb_rat_collect_subnets(pcb_bool SelectedOnly);

pcb_connection_t *pcb_rat_connection_alloc(pcb_net_t *Net);

#define PCB_CONNECTION_LOOP(net) do {                         \
        pcb_cardinal_t        n;                                      \
        pcb_connection_t *      connection;                     \
        for (n = (net)->ConnectionN-1; n != -1; n--)            \
        {                                                       \
                connection = & (net)->Connection[n]

#define PCB_RAT_LOOP(top) do {                                          \
  pcb_rat_t *line;                                                    \
  gdl_iterator_t __it__;                                            \
  ratlist_foreach(&(top)->Rat, &__it__, line) {


#endif
