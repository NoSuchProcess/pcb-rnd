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
#include "board.h"
#include "error.h"
#include "plug_io.h"
#include "hid_actions.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "netlist.h"

static int pcb_netlist_swap()
{
	char *pins[3] = { NULL, NULL, NULL };
	int next = 0, n;
	int ret = -1;
	pcb_lib_menu_t *nets[2];

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pin)) {
				int le, lp;

				if (next > 2) {
					pcb_message(PCB_MSG_ERROR, "Exactly two pins should be selected for swap (more than 2 selected at the moment)\n");
					goto quit;
				}

				le = strlen(element->Name[1].TextString);
				lp = strlen(pin->Number);
				pins[next] = malloc(le + lp + 2);
				sprintf(pins[next], "%s-%s", element->Name[1].TextString, pin->Number);
				next++;
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	if (next < 2) {
		pcb_message(PCB_MSG_ERROR, "Exactly two pins should be selected for swap (less than 2 selected at the moment)\n");
		goto quit;
	}


	nets[0] = pcb_netlist_find_net4pinname(PCB, pins[0]);
	nets[1] = pcb_netlist_find_net4pinname(PCB, pins[1]);
	if ((nets[0] == NULL) || (nets[1] == NULL)) {
		pcb_message(PCB_MSG_ERROR, "That pin is not on a net.\n");
		goto quit;
	}
	if (nets[0] == nets[1]) {
		pcb_message(PCB_MSG_ERROR, "Those two pins are on the same net, can't swap them.\n");
		goto quit;
	}


	pcb_ratspatch_append_optimize(PCB, RATP_DEL_CONN, pins[0], nets[0]->Name + 2, NULL);
	pcb_ratspatch_append_optimize(PCB, RATP_DEL_CONN, pins[1], nets[1]->Name + 2, NULL);
	pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pins[0], nets[1]->Name + 2, NULL);
	pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pins[1], nets[0]->Name + 2, NULL);

	/* TODO: not very efficient to regenerate the whole list... */
	pcb_ratspatch_make_edited(PCB);
	ret = 0;

quit:;
	for (n = 0; n < 3; n++)
		if (pins[n] != NULL)
			free(pins[n]);
	return ret;
}


/* The primary purpose of this action is to rebuild a netlist from a
   script, in conjunction with the clear action above.  */
static int pcb_netlist_add(const char *netname, const char *pinname)
{
	int ni, pi;
	pcb_lib_t *netlist = &PCB->NetlistLib[PCB_NETLIST_EDITED];
	pcb_lib_menu_t *net = NULL;
	pcb_lib_entry_t *pin = NULL;

	for (ni = 0; ni < netlist->MenuN; ni++)
		if (strcmp(netlist->Menu[ni].Name + 2, netname) == 0) {
			net = &(netlist->Menu[ni]);
			break;
		}
	if (net == NULL) {
		net = pcb_lib_net_new(netlist, (char *) netname, NULL);
	}

	for (pi = 0; pi < net->EntryN; pi++)
		if (strcmp(net->Entry[pi].ListEntry, pinname) == 0) {
			pin = &(net->Entry[pi]);
			break;
		}
	if (pin == NULL) {
		pin = pcb_lib_conn_new(net, (char *) pinname);
		pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pin->ListEntry, net->Name + 2, NULL);
	}

	pcb_netlist_changed(0);
	return 0;
}

static const char pcb_acts_Netlist[] =
	"Net(find|select|rats|norats|clear[,net[,pin]])\n" "Net(freeze|thaw|forcethaw)\n" "Net(swap)\n" "Net(add,net,pin)";

static const char pcb_acth_Netlist[] = "Perform various actions on netlists.";

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

typedef void (*NFunc) (pcb_lib_menu_t *, pcb_lib_entry_t *);

static int pcb_act_Netlist(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	NFunc func;
	int i, j;
	pcb_lib_menu_t *net;
	pcb_lib_entry_t *pin;
	int net_found = 0;
	int pin_found = 0;
	int use_re = 0;
	re_sei_t *regex;

	if (!PCB)
		return 1;
	if (argc == 0) {
		pcb_message(PCB_MSG_DEFAULT, pcb_acts_Netlist);
		return 1;
	}
	if (pcb_strcasecmp(argv[0], "find") == 0)
		func = pcb_netlist_find;
	else if (pcb_strcasecmp(argv[0], "select") == 0)
		func = pcb_netlist_select;
	else if (pcb_strcasecmp(argv[0], "rats") == 0)
		func = pcb_netlist_rats;
	else if (pcb_strcasecmp(argv[0], "norats") == 0)
		func = pcb_netlist_norats;
	else if (pcb_strcasecmp(argv[0], "clear") == 0) {
		func = pcb_netlist_clear;
		if (argc == 1) {
			pcb_netlist_clear(NULL, NULL);
			return 0;
		}
	}
	else if (pcb_strcasecmp(argv[0], "style") == 0)
		func = (NFunc) pcb_netlist_style;
	else if (pcb_strcasecmp(argv[0], "swap") == 0)
		return pcb_netlist_swap();
	else if (pcb_strcasecmp(argv[0], "add") == 0) {
		/* Add is different, because the net/pin won't already exist.  */
		return pcb_netlist_add(PCB_ACTION_ARG(1), PCB_ACTION_ARG(2));
	}
	else if (pcb_strcasecmp(argv[0], "sort") == 0) {
		pcb_sort_netlist();
		return 0;
	}
	else if (pcb_strcasecmp(argv[0], "freeze") == 0) {
		PCB->netlist_frozen++;
		return 0;
	}
	else if (pcb_strcasecmp(argv[0], "thaw") == 0) {
		if (PCB->netlist_frozen > 0) {
			PCB->netlist_frozen--;
			if (PCB->netlist_needs_update)
				pcb_netlist_changed(0);
		}
		return 0;
	}
	else if (pcb_strcasecmp(argv[0], "forcethaw") == 0) {
		PCB->netlist_frozen = 0;
		if (PCB->netlist_needs_update)
			pcb_netlist_changed(0);
		return 0;
	}
	else {
		pcb_message(PCB_MSG_DEFAULT, pcb_acts_Netlist);
		return 1;
	}

	if (argc > 1) {
		use_re = 1;
		for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; i++) {
			net = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu + i;
			if (pcb_strcasecmp(argv[1], net->Name + 2) == 0)
				use_re = 0;
		}
		if (use_re) {
			regex = re_sei_comp(argv[1]);
			if (re_sei_errno(regex) != 0) {
				pcb_message(PCB_MSG_DEFAULT, _("regexp error: %s\n"), re_error_str(re_sei_errno(regex)));
				re_sei_free(regex);
				return (1);
			}
		}
	}

	for (i = PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN - 1; i >= 0; i--) {
		net = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu + i;

		if (argc > 1) {
			if (use_re) {
				if (re_sei_exec(regex, net->Name + 2) == 0)
					continue;
			}
			else {
				if (pcb_strcasecmp(net->Name + 2, argv[1]))
					continue;
			}
		}
		net_found = 1;

		pin = 0;
		if (func == (NFunc) pcb_netlist_style) {
			pcb_netlist_style(net, PCB_ACTION_ARG(2));
		}
		else if (argc > 2) {
			int l = strlen(argv[2]);
			for (j = net->EntryN - 1; j >= 0; j--)
				if (pcb_strcasecmp(net->Entry[j].ListEntry, argv[2]) == 0
						|| (pcb_strncasecmp(net->Entry[j].ListEntry, argv[2], l) == 0 && net->Entry[j].ListEntry[l] == '-')) {
					pin = net->Entry + j;
					pin_found = 1;
					func(net, pin);
				}
		}
		else
			func(net, 0);
	}

	if (argc > 2 && !pin_found) {
		pcb_gui->log("Net %s has no pin %s\n", argv[1], argv[2]);
		return 1;
	}
	else if (!net_found) {
		pcb_gui->log("No net named %s\n", argv[1]);
	}

	if (use_re)
		re_sei_free(regex);

	return 0;
}

pcb_hid_action_t netlist_action_list[] = {
	{"net", 0, pcb_act_Netlist,
	 pcb_acth_Netlist, pcb_acts_Netlist}
	,
	{"netlist", 0, pcb_act_Netlist,
	 pcb_acth_Netlist, pcb_acts_Netlist}
};

PCB_REGISTER_ACTIONS(netlist_action_list, NULL)
