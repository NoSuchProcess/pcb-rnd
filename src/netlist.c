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
#include <ctype.h>
#include <genregex/regex_sei.h>

#include "action_helper.h"
#include "data.h"
#include "error.h"
#include "plug_io.h"
#include "find.h"
#include "mymem.h"
#include "rats.h"
#include "create.h"
#include "rats_patch.h"
#include "hid_actions.h"
#include "compat_misc.h"

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

#warning TODO: move these in PCB
int netlist_frozen = 0;
int netlist_needs_update = 0;

void pcb_netlist_changed(int force_unfreeze)
{
	if (force_unfreeze)
		netlist_frozen = 0;
	if (netlist_frozen)
		netlist_needs_update = 1;
	else {
		netlist_needs_update = 0;
		hid_action("NetlistChanged");
	}
}

LibraryMenuTypePtr pcb_netnode_to_netname(const char *nodename)
{
	int i, j;
	/*printf("nodename [%s]\n", nodename); */
	for (i = 0; i < PCB->NetlistLib[NETLIST_EDITED].MenuN; i++) {
		for (j = 0; j < PCB->NetlistLib[NETLIST_EDITED].Menu[i].EntryN; j++) {
			if (strcmp(PCB->NetlistLib[NETLIST_EDITED].Menu[i].Entry[j].ListEntry, nodename) == 0) {
				/*printf(" in [%s]\n", PCB->NetlistLib.Menu[i].Name); */
				return &(PCB->NetlistLib[NETLIST_EDITED].Menu[i]);
			}
		}
	}
	return 0;
}

LibraryMenuTypePtr pcb_netname_to_netname(const char *netname)
{
	int i;

	if ((netname[0] == '*' || netname[0] == ' ') && netname[1] == ' ') {
		/* Looks like we were passed an internal netname, skip the prefix */
		netname += 2;
	}
	for (i = 0; i < PCB->NetlistLib[NETLIST_EDITED].MenuN; i++) {
		if (strcmp(PCB->NetlistLib[NETLIST_EDITED].Menu[i].Name + 2, netname) == 0) {
			return &(PCB->NetlistLib[NETLIST_EDITED].Menu[i]);
		}
	}
	return 0;
}

int pcb_pin_name_to_xy(LibraryEntryType * pin, Coord *x, Coord *y)
{
	ConnectionType conn;
	if (!SeekPad(pin, &conn, pcb_false))
		return 1;
	switch (conn.type) {
	case PCB_TYPE_PIN:
		*x = ((PinType *) (conn.ptr2))->X;
		*y = ((PinType *) (conn.ptr2))->Y;
		return 0;
	case PCB_TYPE_PAD:
		*x = ((PadType *) (conn.ptr2))->Point1.X;
		*y = ((PadType *) (conn.ptr2))->Point1.Y;
		return 0;
	}
	return 1;
}

void pcb_netlist_find(LibraryMenuType * net, LibraryEntryType * pin)
{
	Coord x, y;
	if (pcb_pin_name_to_xy(net->Entry, &x, &y))
		return;
	LookupConnection(x, y, 1, 1, PCB_FLAG_FOUND);
}

void pcb_netlist_select(LibraryMenuType * net, LibraryEntryType * pin)
{
	Coord x, y;
	if (pcb_pin_name_to_xy(net->Entry, &x, &y))
		return;
	LookupConnection(x, y, 1, 1, PCB_FLAG_SELECTED);
}

void pcb_netlist_rats(LibraryMenuType * net, LibraryEntryType * pin)
{
	net->Name[0] = ' ';
	net->flag = 1;
	pcb_netlist_changed(0);
}

void pcb_netlist_norats(LibraryMenuType * net, LibraryEntryType * pin)
{
	net->Name[0] = '*';
	net->flag = 0;
	pcb_netlist_changed(0);
}

/* The primary purpose of this action is to remove the netlist
   completely so that a new one can be loaded, usually via a gsch2pcb
   style script.  */
void pcb_netlist_clear(LibraryMenuType * net, LibraryEntryType * pin)
{
	LibraryType *netlist = (LibraryType *) & PCB->NetlistLib;
	int ni, pi;

	if (net == 0) {
		/* Clear the entire netlist. */
		for (ni = 0; ni < NUM_NETLISTS; ni++)
			FreeLibraryMemory(&(PCB->NetlistLib[ni]));
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

void pcb_netlist_style(LibraryMenuType * net, const char *style)
{
	free(net->Style);
	net->Style = pcb_strdup_null((char *) style);
}

LibraryMenuTypePtr pcb_netlist_find_net4pinname(PCBTypePtr pcb, const char *pin)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[NETLIST_EDITED].MenuN; n++) {
		LibraryMenuTypePtr menu = &pcb->NetlistLib[NETLIST_EDITED].Menu[n];
		int p;
		for (p = 0; p < menu->EntryN; p++) {
			LibraryEntryTypePtr entry = &menu->Entry[p];
			if (strcmp(entry->ListEntry, pin) == 0)
				return menu;
		}
	}
	return NULL;
}
