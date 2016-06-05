/* $Id$ */

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
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

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

#ifdef HAVE_REGCOMP
#undef HAVE_RE_COMP
#endif

#if defined(HAVE_REGCOMP) || defined(HAVE_RE_COMP)
#define USE_RE
#endif


RCSID("$Id$");

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

typedef void (*NFunc) (LibraryMenuType *, LibraryEntryType *);

int netlist_frozen = 0;
static int netlist_needs_update = 0;

void NetlistChanged(int force_unfreeze)
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

LibraryMenuTypePtr netnode_to_netname(char *nodename)
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

LibraryMenuTypePtr netname_to_netname(char *netname)
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

static int pin_name_to_xy(LibraryEntryType * pin, int *x, int *y)
{
	ConnectionType conn;
	if (!SeekPad(pin, &conn, false))
		return 1;
	switch (conn.type) {
	case PIN_TYPE:
		*x = ((PinType *) (conn.ptr2))->X;
		*y = ((PinType *) (conn.ptr2))->Y;
		return 0;
	case PAD_TYPE:
		*x = ((PadType *) (conn.ptr2))->Point1.X;
		*y = ((PadType *) (conn.ptr2))->Point1.Y;
		return 0;
	}
	return 1;
}

static void netlist_find(LibraryMenuType * net, LibraryEntryType * pin)
{
	int x, y;
	if (pin_name_to_xy(net->Entry, &x, &y))
		return;
	LookupConnection(x, y, 1, 1, FOUNDFLAG);
}

static void netlist_select(LibraryMenuType * net, LibraryEntryType * pin)
{
	int x, y;
	if (pin_name_to_xy(net->Entry, &x, &y))
		return;
	LookupConnection(x, y, 1, 1, SELECTEDFLAG);
}

static void netlist_rats(LibraryMenuType * net, LibraryEntryType * pin)
{
	net->Name[0] = ' ';
	net->flag = 1;
	NetlistChanged(0);
}

static void netlist_norats(LibraryMenuType * net, LibraryEntryType * pin)
{
	net->Name[0] = '*';
	net->flag = 0;
	NetlistChanged(0);
}

/* The primary purpose of this action is to remove the netlist
   completely so that a new one can be loaded, usually via a gsch2pcb
   style script.  */
static void netlist_clear(LibraryMenuType * net, LibraryEntryType * pin)
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
	NetlistChanged(0);
}

static void netlist_style(LibraryMenuType * net, const char *style)
{
	free(net->Style);
	net->Style = STRDUP((char *) style);
}


static int netlist_swap()
{
	char *pins[3] = { NULL, NULL, NULL };
	int next = 0, n;
	int ret = -1;
	LibraryMenuTypePtr nets[2];

	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (TEST_FLAG(SELECTEDFLAG, pin)) {
				int le, lp;

				if (next > 2) {
					Message("Exactly two pins should be selected for swap (more than 2 selected at the moment)\n");
					goto quit;
				}

				le = strlen(element->Name[1].TextString);
				lp = strlen(pin->Number);
				pins[next] = malloc(le + lp + 2);
				sprintf(pins[next], "%s-%s", element->Name[1].TextString, pin->Number);
				next++;
			}
		}
		END_LOOP;
	}
	END_LOOP;

	if (next < 2) {
		Message("Exactly two pins should be selected for swap (less than 2 selected at the moment)\n");
		goto quit;
	}


	nets[0] = rats_patch_find_net4pin(PCB, pins[0]);
	nets[1] = rats_patch_find_net4pin(PCB, pins[1]);
	if ((nets[0] == NULL) || (nets[1] == NULL)) {
		Message("That pin is not on a net.\n");
		goto quit;
	}
	if (nets[0] == nets[1]) {
		Message("Those two pins are on the same net, can't swap them.\n");
		goto quit;
	}


	rats_patch_append_optimize(PCB, RATP_DEL_CONN, pins[0], nets[0]->Name + 2, NULL);
	rats_patch_append_optimize(PCB, RATP_DEL_CONN, pins[1], nets[1]->Name + 2, NULL);
	rats_patch_append_optimize(PCB, RATP_ADD_CONN, pins[0], nets[1]->Name + 2, NULL);
	rats_patch_append_optimize(PCB, RATP_ADD_CONN, pins[1], nets[0]->Name + 2, NULL);

	/* TODO: not very efficient to regenerate the whole list... */
	rats_patch_make_edited(PCB);
	ret = 0;

quit:;
	for (n = 0; n < 3; n++)
		if (pins[n] != NULL)
			free(pins[n]);
	return ret;
}


/* The primary purpose of this action is to rebuild a netlist from a
   script, in conjunction with the clear action above.  */
static int netlist_add(const char *netname, const char *pinname)
{
	int ni, pi;
	LibraryType *netlist = &PCB->NetlistLib[NETLIST_EDITED];
	LibraryMenuType *net = NULL;
	LibraryEntryType *pin = NULL;

	for (ni = 0; ni < netlist->MenuN; ni++)
		if (strcmp(netlist->Menu[ni].Name + 2, netname) == 0) {
			net = &(netlist->Menu[ni]);
			break;
		}
	if (net == NULL) {
		net = CreateNewNet(netlist, (char *) netname, NULL);
	}

	for (pi = 0; pi < net->EntryN; pi++)
		if (strcmp(net->Entry[pi].ListEntry, pinname) == 0) {
			pin = &(net->Entry[pi]);
			break;
		}
	if (pin == NULL) {
		pin = CreateNewConnection(net, (char *) pinname);
		rats_patch_append_optimize(PCB, RATP_ADD_CONN, pin->ListEntry, net->Name + 2, NULL);
	}

	NetlistChanged(0);
	return 0;
}

static const char netlist_syntax[] =
	"Net(find|select|rats|norats|clear[,net[,pin]])\n" "Net(freeze|thaw|forcethaw)\n" "Net(swap)\n" "Net(add,net,pin)";

static const char netlist_help[] = "Perform various actions on netlists.";

/* %start-doc actions Netlist

Each of these actions apply to a specified set of nets.  @var{net} and
@var{pin} are patterns which match one or more nets or pins; these
patterns may be full names or regular expressions.  If an exact match
is found, it is the only match; if no exact match is found,
@emph{then} the pattern is tried as a regular expression.

If neither @var{net} nor @var{pin} are specified, all nets apply.  If
@var{net} is specified but not @var{pin}, all nets matching @var{net}
apply.  If both are specified, nets which match @var{net} and contain
a pin matching @var{pin} apply.

@table @code

@item find
Nets which apply are marked @emph{found} and are drawn in the
@code{connected-color} color.

@item select
Nets which apply are selected.

@item rats
Nets which apply are marked as available for the rats nest.

@item norats
Nets which apply are marked as not available for the rats nest.

@item clear
Clears the netlist.

@item add
Add the given pin to the given netlist, creating either if needed.

@item swap
Swap the connections one end of two selected rats and pins.

@item sort
Called after a list of add's, this sorts the netlist.

@item freeze
@itemx thaw
@itemx forcethaw
Temporarily prevents changes to the netlist from being reflected in
the GUI.  For example, if you need to make multiple changes, you
freeze the netlist, make the changes, then thaw it.  Note that
freeze/thaw requests may nest, with the netlist being fully thawed
only when all pending freezes are thawed.  You can bypass the nesting
by using forcethaw, which resets the freeze count and immediately
updates the GUI.

@end table

%end-doc */

static int Netlist(int argc, char **argv, Coord x, Coord y)
{
	NFunc func;
	int i, j;
	LibraryMenuType *net;
	LibraryEntryType *pin;
	int net_found = 0;
	int pin_found = 0;
#if defined(USE_RE)
	int use_re = 0;
#endif
#if defined(HAVE_REGCOMP)
	regex_t elt_pattern;
	regmatch_t match;
#endif
#if defined(HAVE_RE_COMP)
	char *elt_pattern;
#endif

	if (!PCB)
		return 1;
	if (argc == 0) {
		Message(netlist_syntax);
		return 1;
	}
	if (strcasecmp(argv[0], "find") == 0)
		func = netlist_find;
	else if (strcasecmp(argv[0], "select") == 0)
		func = netlist_select;
	else if (strcasecmp(argv[0], "rats") == 0)
		func = netlist_rats;
	else if (strcasecmp(argv[0], "norats") == 0)
		func = netlist_norats;
	else if (strcasecmp(argv[0], "clear") == 0) {
		func = netlist_clear;
		if (argc == 1) {
			netlist_clear(NULL, NULL);
			return 0;
		}
	}
	else if (strcasecmp(argv[0], "style") == 0)
		func = (NFunc) netlist_style;
	else if (strcasecmp(argv[0], "swap") == 0)
		return netlist_swap();
	else if (strcasecmp(argv[0], "add") == 0) {
		/* Add is different, because the net/pin won't already exist.  */
		return netlist_add(ACTION_ARG(1), ACTION_ARG(2));
	}
	else if (strcasecmp(argv[0], "sort") == 0) {
		sort_netlist();
		return 0;
	}
	else if (strcasecmp(argv[0], "freeze") == 0) {
		netlist_frozen++;
		return 0;
	}
	else if (strcasecmp(argv[0], "thaw") == 0) {
		if (netlist_frozen > 0) {
			netlist_frozen--;
			if (netlist_needs_update)
				NetlistChanged(0);
		}
		return 0;
	}
	else if (strcasecmp(argv[0], "forcethaw") == 0) {
		netlist_frozen = 0;
		if (netlist_needs_update)
			NetlistChanged(0);
		return 0;
	}
	else {
		Message(netlist_syntax);
		return 1;
	}

#if defined(USE_RE)
	if (argc > 1) {
		int result;
		use_re = 1;
		for (i = 0; i < PCB->NetlistLib[NETLIST_EDITED].MenuN; i++) {
			net = PCB->NetlistLib[NETLIST_EDITED].Menu + i;
			if (strcasecmp(argv[1], net->Name + 2) == 0)
				use_re = 0;
		}
		if (use_re) {
#if defined(HAVE_REGCOMP)
			result = regcomp(&elt_pattern, argv[1], REG_EXTENDED | REG_ICASE | REG_NOSUB);
			if (result) {
				char errorstring[128];

				regerror(result, &elt_pattern, errorstring, 128);
				Message(_("regexp error: %s\n"), errorstring);
				regfree(&elt_pattern);
				return (1);
			}
#endif
#if defined(HAVE_RE_COMP)
			if ((elt_pattern = re_comp(argv[1])) != NULL) {
				Message(_("re_comp error: %s\n"), elt_pattern);
				return (false);
			}
#endif
		}
	}
#endif

	for (i = PCB->NetlistLib[NETLIST_EDITED].MenuN - 1; i >= 0; i--) {
		net = PCB->NetlistLib[NETLIST_EDITED].Menu + i;

		if (argc > 1) {
#if defined(USE_RE)
			if (use_re) {
#if defined(HAVE_REGCOMP)
				if (regexec(&elt_pattern, net->Name + 2, 1, &match, 0) != 0)
					continue;
#endif
#if defined(HAVE_RE_COMP)
				if (re_exec(net->Name + 2) != 1)
					continue;
#endif
			}
			else
#endif
			if (strcasecmp(net->Name + 2, argv[1]))
				continue;
		}
		net_found = 1;

		pin = 0;
		if (func == (void *) netlist_style) {
			netlist_style(net, ACTION_ARG(2));
		}
		else if (argc > 2) {
			int l = strlen(argv[2]);
			for (j = net->EntryN - 1; j >= 0; j--)
				if (strcasecmp(net->Entry[j].ListEntry, argv[2]) == 0
						|| (strncasecmp(net->Entry[j].ListEntry, argv[2], l) == 0 && net->Entry[j].ListEntry[l] == '-')) {
					pin = net->Entry + j;
					pin_found = 1;
					func(net, pin);
				}
		}
		else
			func(net, 0);
	}

	if (argc > 2 && !pin_found) {
		gui->log("Net %s has no pin %s\n", argv[1], argv[2]);
		return 1;
	}
	else if (!net_found) {
		gui->log("No net named %s\n", argv[1]);
	}
#ifdef HAVE_REGCOMP
	if (use_re)
		regfree(&elt_pattern);
#endif

	return 0;
}

HID_Action netlist_action_list[] = {
	{"net", 0, Netlist,
	 netlist_help, netlist_syntax}
	,
	{"netlist", 0, Netlist,
	 netlist_help, netlist_syntax}
};

REGISTER_ACTIONS(netlist_action_list, NULL)
