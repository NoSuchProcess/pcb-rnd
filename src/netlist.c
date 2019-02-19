/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* generic netlist operations */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "brave.h"
#include "error.h"
#include "plug_io.h"
#include "find.h"
#include "rats.h"
#include "actions.h"
#include "compat_misc.h"
#include "netlist.h"
#include "event.h"
#include "data.h"
#include "netlist2.h"

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

pcb_lib_menu_t *pcb_netlist_find_net4term(pcb_board_t *pcb, const pcb_any_obj_t *term)
{
	pcb_data_t *data;
	pcb_subc_t *sc;
	char pinname[256];
	int len;

	if (term->term == NULL)
		return NULL;

	if (term->parent_type == PCB_PARENT_LAYER)
		data = term->parent.layer->parent.data;
	else if (term->parent_type == PCB_PARENT_DATA)
		data = term->parent.data;
	else
		return NULL;

	if (data->parent_type != PCB_PARENT_SUBC)
		return NULL;

	sc = data->parent.subc;
	if (sc->refdes == NULL)
		return NULL;

	len = pcb_snprintf(pinname, sizeof(pinname), "%s-%s", sc->refdes, term->term);
	if (len >= sizeof(pinname))
		return NULL;

	return pcb_netlist_find_net4pinname(pcb, pinname);
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
pcb_oldnet_t *pcb_net_new(pcb_board_t *pcb, pcb_oldnetlist_t *Netlist)
{
	pcb_oldnet_t *net = Netlist->Net;

	/* realloc new memory if necessary and clear it */
	if (Netlist->NetN >= Netlist->NetMax) {
		Netlist->NetMax += STEP_POINT;
		net = (pcb_oldnet_t *) realloc(net, Netlist->NetMax * sizeof(pcb_oldnet_t));
		Netlist->Net = net;
		memset(net + Netlist->NetN, 0, STEP_POINT * sizeof(pcb_oldnet_t));
	}

	net->type = PCB_OBJ_NET;
	net->parent_type = PCB_PARENT_BOARD;
	net->parent.board = pcb;
	return (net + Netlist->NetN++);
}

/* ---------------------------------------------------------------------------
 * get next slot for a net list, allocates memory if necessary
 */
pcb_oldnetlist_t *pcb_netlist_new(pcb_netlist_list_t *Netlistlist)
{
	pcb_oldnetlist_t *netlist = Netlistlist->NetList;

	/* realloc new memory if necessary and clear it */
	if (Netlistlist->NetListN >= Netlistlist->NetListMax) {
		Netlistlist->NetListMax += STEP_POINT;
		netlist = (pcb_oldnetlist_t *) realloc(netlist, Netlistlist->NetListMax * sizeof(pcb_oldnetlist_t));
		Netlistlist->NetList = netlist;
		memset(netlist + Netlistlist->NetListN, 0, STEP_POINT * sizeof(pcb_oldnetlist_t));
	}
	return (netlist + Netlistlist->NetListN++);
}

/* ---------------------------------------------------------------------------
 * frees memory used by a net
 */
void pcb_netlist_free(pcb_oldnetlist_t *Netlist)
{
	if (Netlist) {
		PCB_NET_LOOP(Netlist);
		{
			pcb_oldnet_free(net);
		}
		PCB_END_LOOP;
		free(Netlist->Net);
		memset(Netlist, 0, sizeof(pcb_oldnetlist_t));
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
void pcb_oldnet_free(pcb_oldnet_t *Net)
{
	if (Net) {
		free(Net->Connection);
		memset(Net, 0, sizeof(pcb_oldnet_t));
	}
}

pcb_lib_menu_t *pcb_netlist_lookup(int patch, const char *netname, pcb_bool alloc)
{
	pcb_lib_t *netlist = patch ? &PCB->NetlistLib[PCB_NETLIST_EDITED] : &PCB->NetlistLib[PCB_NETLIST_INPUT];
	long ni;

	if ((netname == NULL) || (*netname == '\0'))
		return NULL;

	for (ni = 0; ni < netlist->MenuN; ni++)
		if (strcmp(netlist->Menu[ni].Name + 2, netname) == 0)
			return &(netlist->Menu[ni]);

	if (!alloc)
		return NULL;

	if (!patch) {
		pcb_lib_menu_t *net = pcb_lib_menu_new(netlist, NULL);
		net->Name = pcb_strdup_printf("  %s", netname);
		net->flag = 1;
		PCB->netlist_needs_update=1;
		return net;
	}

	return pcb_lib_net_new(netlist, (char *)netname, NULL);
}
