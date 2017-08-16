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

/* generic netlist operations
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "error.h"
#include "plug_io.h"
#include "find.h"
#include "rats.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "netlist.h"
#include "event.h"

#define STEP_POINT 100

/*
  int    PCB->NetlistLib[n].MenuN
  char * PCB->NetlistLib[n].Menu[i].Name
     [0] == '*' (ok for rats) or ' ' (skip for rats)
     [1] == unused
     [2..] actual name
  char * PCB->NetlistLib[n].Menu[i].Style
  int    PCB->NetlistLib[n].Menu[i].EntryN
  char * PCB->NetlistLib[n].Menu[i].Entry[j].ListEntry
*/

void pcb_netlist_changed(int force_unfreeze)
{
	if (force_unfreeze)
		PCB->netlist_frozen = 0;
	if (PCB->netlist_frozen)
		PCB->netlist_needs_update = 1;
	else {
		PCB->netlist_needs_update = 0;
		pcb_event(PCB_EVENT_NETLIST_CHANGED, NULL);
	}
}

pcb_lib_menu_t *pcb_netnode_to_netname(const char *nodename)
{
	int i, j;
	/*printf("nodename [%s]\n", nodename); */
	for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; i++) {
		for (j = 0; j < PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i].EntryN; j++) {
			if (strcmp(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i].Entry[j].ListEntry, nodename) == 0) {
				/*printf(" in [%s]\n", PCB->NetlistLib.Menu[i].Name); */
				return &(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i]);
			}
		}
	}
	return 0;
}

pcb_lib_menu_t *pcb_netname_to_netname(const char *netname)
{
	int i;

	if ((netname[0] == '*' || netname[0] == ' ') && netname[1] == ' ') {
		/* Looks like we were passed an internal netname, skip the prefix */
		netname += 2;
	}
	for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; i++) {
		if (strcmp(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i].Name + 2, netname) == 0) {
			return &(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i]);
		}
	}
	return 0;
}

int pcb_pin_name_to_xy(pcb_lib_entry_t * pin, pcb_coord_t *x, pcb_coord_t *y)
{
	pcb_connection_t conn;
	if (!pcb_rat_seek_pad(pin, &conn, pcb_false))
		return 1;
	switch (conn.obj->type) {
	case PCB_OBJ_PIN:
		*x = ((pcb_pin_t *) (conn.obj))->X;
		*y = ((pcb_pin_t *) (conn.obj))->Y;
		return 0;
	case PCB_OBJ_PAD:
		*x = ((pcb_pad_t *) (conn.obj))->Point1.X;
		*y = ((pcb_pad_t *) (conn.obj))->Point1.Y;
		return 0;
	}
	return 1;
}

void pcb_netlist_find(pcb_lib_menu_t * net, pcb_lib_entry_t * pin)
{
	pcb_coord_t x, y;
	if (pcb_pin_name_to_xy(net->Entry, &x, &y))
		return;
	pcb_lookup_conn(x, y, 1, 1, PCB_FLAG_FOUND);
}

void pcb_netlist_select(pcb_lib_menu_t * net, pcb_lib_entry_t * pin)
{
	pcb_coord_t x, y;
	if (pcb_pin_name_to_xy(net->Entry, &x, &y))
		return;
	pcb_lookup_conn(x, y, 1, 1, PCB_FLAG_SELECTED);
}

void pcb_netlist_rats(pcb_lib_menu_t * net, pcb_lib_entry_t * pin)
{
	net->Name[0] = ' ';
	net->flag = 1;
	pcb_netlist_changed(0);
}

void pcb_netlist_norats(pcb_lib_menu_t * net, pcb_lib_entry_t * pin)
{
	net->Name[0] = '*';
	net->flag = 0;
	pcb_netlist_changed(0);
}

/* The primary purpose of this action is to remove the netlist
   completely so that a new one can be loaded, usually via a gsch2pcb
   style script.  */
void pcb_netlist_clear(pcb_lib_menu_t * net, pcb_lib_entry_t * pin)
{
	pcb_lib_t *netlist = (pcb_lib_t *) & PCB->NetlistLib;
	int ni, pi;

	if (net == 0) {
		/* Clear the entire netlist. */
		for (ni = 0; ni < PCB_NUM_NETLISTS; ni++)
			pcb_lib_free(&(PCB->NetlistLib[ni]));
	}
	else if (pin == 0) {
		/* Remove a net from the netlist. */
		ni = net - netlist->Menu;
		if (ni >= 0 && ni < netlist->MenuN) {
			/* if there is exactly one item, MenuN is 1 and ni is 0 */
			if (netlist->MenuN - ni > 1)
				memmove(net, net + 1, (netlist->MenuN - ni - 1) * sizeof(*net));
			netlist->MenuN--;
		}
	}
	else {
		/* Remove a pin from the given net.  Note that this may leave an
		   empty net, which is different than removing the net
		   (above).  */
		pi = pin - net->Entry;
		if (pi >= 0 && pi < net->EntryN) {
			/* if there is exactly one item, MenuN is 1 and ni is 0 */
			if (net->EntryN - pi > 1)
				memmove(pin, pin + 1, (net->EntryN - pi - 1) * sizeof(*pin));
			net->EntryN--;
		}
	}
	pcb_netlist_changed(0);
}

void pcb_netlist_style(pcb_lib_menu_t * net, const char *style)
{
	free(net->Style);
	net->Style = pcb_strdup_null((char *) style);
}

pcb_lib_menu_t *pcb_netlist_find_net4pinname(pcb_board_t *pcb, const char *pin)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[PCB_NETLIST_EDITED].MenuN; n++) {
		pcb_lib_menu_t *menu = &pcb->NetlistLib[PCB_NETLIST_EDITED].Menu[n];
		int p;
		for (p = 0; p < menu->EntryN; p++) {
			pcb_lib_entry_t *entry = &menu->Entry[p];
			if (strcmp(entry->ListEntry, pin) == 0)
				return menu;
		}
	}
	return NULL;
}

static pcb_lib_menu_t *pcb_netlist_find_net4pin_any(pcb_board_t *pcb, const char *ename, const char *pname)
{
	char pinname[256];
	int len;

	if ((ename == NULL) || (pname == NULL))
		return NULL;

	len = pcb_snprintf(pinname, sizeof(pinname), "%s-%s", ename, pname);
	if (len >= sizeof(pinname))
		return NULL;

	return pcb_netlist_find_net4pinname(pcb, pinname);
}

pcb_lib_menu_t *pcb_netlist_find_net4pin(pcb_board_t *pcb, const pcb_pin_t *pin)
{
	const pcb_element_t *e = pin->Element;

	if (e == NULL)
		return NULL;

	return pcb_netlist_find_net4pin_any(pcb, e->Name[PCB_ELEMNAME_IDX_REFDES].TextString, pin->Number);
}


pcb_lib_menu_t *pcb_netlist_find_net4pad(pcb_board_t *pcb, const pcb_pad_t *pad)
{
	const pcb_element_t *e = pad->Element;

	if (e == NULL)
		return NULL;

	return pcb_netlist_find_net4pin_any(pcb, e->Name[PCB_ELEMNAME_IDX_REFDES].TextString, pad->Number);
}

pcb_cardinal_t pcb_netlist_net_idx(pcb_board_t *pcb, pcb_lib_menu_t *net)
{
	pcb_lib_menu_t *first = &pcb->NetlistLib[PCB_NETLIST_EDITED].Menu[0];
	pcb_lib_menu_t *last  = &pcb->NetlistLib[PCB_NETLIST_EDITED].Menu[pcb->NetlistLib[PCB_NETLIST_EDITED].MenuN-1];
	
	if ((net < first) || (net > last))
		return PCB_NETLIST_INVALID_INDEX;

	return net - first;
}

/* ---------------------------------------------------------------------------
 * get next slot for a subnet, allocates memory if necessary
 */
pcb_net_t *pcb_net_new(pcb_netlist_t *Netlist)
{
	pcb_net_t *net = Netlist->Net;

	/* realloc new memory if necessary and clear it */
	if (Netlist->NetN >= Netlist->NetMax) {
		Netlist->NetMax += STEP_POINT;
		net = (pcb_net_t *) realloc(net, Netlist->NetMax * sizeof(pcb_net_t));
		Netlist->Net = net;
		memset(net + Netlist->NetN, 0, STEP_POINT * sizeof(pcb_net_t));
	}
	return (net + Netlist->NetN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a net list, allocates memory if necessary
 */
pcb_netlist_t *pcb_netlist_new(pcb_netlist_list_t *Netlistlist)
{
	pcb_netlist_t *netlist = Netlistlist->NetList;

	/* realloc new memory if necessary and clear it */
	if (Netlistlist->NetListN >= Netlistlist->NetListMax) {
		Netlistlist->NetListMax += STEP_POINT;
		netlist = (pcb_netlist_t *) realloc(netlist, Netlistlist->NetListMax * sizeof(pcb_netlist_t));
		Netlistlist->NetList = netlist;
		memset(netlist + Netlistlist->NetListN, 0, STEP_POINT * sizeof(pcb_netlist_t));
	}
	return (netlist + Netlistlist->NetListN++);
}

/* ---------------------------------------------------------------------------
 * frees memory used by a net
 */
void pcb_netlist_free(pcb_netlist_t *Netlist)
{
	if (Netlist) {
		PCB_NET_LOOP(Netlist);
		{
			pcb_net_free(net);
		}
		PCB_END_LOOP;
		free(Netlist->Net);
		memset(Netlist, 0, sizeof(pcb_netlist_t));
	}
}

/* ---------------------------------------------------------------------------
 * frees memory used by a net list
 */
void pcb_netlist_list_free(pcb_netlist_list_t *Netlistlist)
{
	if (Netlistlist) {
		PCB_NETLIST_LOOP(Netlistlist);
		{
			pcb_netlist_free(netlist);
		}
		PCB_END_LOOP;
		free(Netlistlist->NetList);
		memset(Netlistlist, 0, sizeof(pcb_netlist_list_t));
	}
}

/* ---------------------------------------------------------------------------
 * frees memory used by a subnet
 */
void pcb_net_free(pcb_net_t *Net)
{
	if (Net) {
		free(Net->Connection);
		memset(Net, 0, sizeof(pcb_net_t));
	}
}
