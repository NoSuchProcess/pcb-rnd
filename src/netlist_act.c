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

/* generic netlist operations
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <genregex/regex_sei.h>

#include "funchash_core.h"
#include "data.h"
#include "board.h"
#include "error.h"
#include "plug_io.h"
#include "actions.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "netlist.h"
#include "data_it.h"

static int pcb_netlist_swap()
{
	char *pins[3] = { NULL, NULL, NULL };
	int next = 0, n;
	int ret = -1;
	pcb_lib_menu_t *nets[2];

	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_any_obj_t *o;
		pcb_data_it_t it;

		for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o) && (o->term != NULL)) {
				int le, lp;

				if (next > 2) {
					pcb_message(PCB_MSG_ERROR, "Exactly two terminals should be selected for swap (more than 2 selected at the moment)\n");
					goto quit;
				}

				le = strlen(subc->refdes);
				lp = strlen(o->term);
				pins[next] = malloc(le + lp + 2);
				sprintf(pins[next], "%s-%s", subc->refdes, o->term);
				next++;
			}
		}
	}
	PCB_END_LOOP;

	if (next < 2) {
		pcb_message(PCB_MSG_ERROR, "Exactly two terminals should be selected for swap (less than 2 selected at the moment)\n");
		goto quit;
	}


	nets[0] = pcb_netlist_find_net4pinname(PCB, pins[0]);
	nets[1] = pcb_netlist_find_net4pinname(PCB, pins[1]);
	if ((nets[0] == NULL) || (nets[1] == NULL)) {
		pcb_message(PCB_MSG_ERROR, "That terminal is not on a net.\n");
		goto quit;
	}
	if (nets[0] == nets[1]) {
		pcb_message(PCB_MSG_ERROR, "Those two terminals are on the same net, can't swap them.\n");
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
static int pcb_netlist_add(int patch, const char *netname, const char *pinname)
{
	int pi;
	pcb_lib_menu_t *net = NULL;
	pcb_lib_entry_t *pin = NULL;

	if ((netname == NULL) || (pinname == NULL))
		return -1;

	if ((*netname == '\0') || (*pinname == '\0'))
		return -1;

	net = pcb_netlist_lookup(patch, netname, pcb_true);

	for (pi = 0; pi < net->EntryN; pi++) {
		if (strcmp(net->Entry[pi].ListEntry, pinname) == 0) {
			pin = &(net->Entry[pi]);
			break;
		}
	}


	if (pin == NULL) {
		if (!patch) {
			pcb_lib_entry_t *entry = pcb_lib_entry_new(net);
			entry->ListEntry = pcb_strdup_printf("%s", pinname);
			entry->ListEntry_dontfree = 0;
			PCB->netlist_needs_update=1;
		}
		else {
			pin = pcb_lib_conn_new(net, (char *) pinname);
			pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pin->ListEntry, net->Name + 2, NULL);
		}
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

static fgw_error_t pcb_act_Netlist(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	NFunc func;
	const char *a1 = NULL, *a2 = NULL;
	int op, i, j;
	pcb_lib_menu_t *net;
	pcb_lib_entry_t *pin;
	int net_found = 0;
	int pin_found = 0;
	int use_re = 0;
	re_sei_t *regex;
	PCB_ACT_CONVARG(1, FGW_KEYWORD, Netlist, op = fgw_keyword(&argv[1]));
	PCB_ACT_MAY_CONVARG(2, FGW_STR, Netlist, a1 = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, Netlist, a2 = argv[3].val.str);
	PCB_ACT_IRES(0);

	if (!PCB)
		return 1;

	switch(op) {
		case F_Find: func = pcb_netlist_find; break;
		case F_Select: func = pcb_netlist_select; break;
		case F_Rats: func = pcb_netlist_rats; break;
		case F_NoRats: func = pcb_netlist_norats; break;
		case F_Style: func = (NFunc) pcb_netlist_style; break;
		case F_Swap: return pcb_netlist_swap(); break;
		case F_Clear:
			func = pcb_netlist_clear;
			if (argc == 2) {
				pcb_netlist_clear(NULL, NULL);
				return 0;
			}
			break;
		case F_Add:
			/* Add is different, because the net/pin won't already exist.  */
			return pcb_netlist_add(0, a1, a2);
		case F_Sort:
			pcb_sort_netlist();
			pcb_ratspatch_make_edited(PCB);
			return 0;
		case F_Freeze:
			PCB->netlist_frozen++;
			return 0;
		case F_Thaw:
			if (PCB->netlist_frozen > 0) {
				PCB->netlist_frozen--;
				if (PCB->netlist_needs_update)
					pcb_netlist_changed(0);
			}
			return 0;
		case F_ForceThaw:
			PCB->netlist_frozen = 0;
			if (PCB->netlist_needs_update)
				pcb_netlist_changed(0);
			return 0;
		default:
			PCB_ACT_FAIL(Netlist);
	}

	if (argc > 2) {
		use_re = 1;
		for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN; i++) {
			net = PCB->NetlistLib[PCB_NETLIST_INPUT].Menu + i;
			if (pcb_strcasecmp(a1, net->Name + 2) == 0)
				use_re = 0;
		}
		if (use_re) {
			regex = re_sei_comp(a1);
			if (re_sei_errno(regex) != 0) {
				pcb_message(PCB_MSG_ERROR, _("regexp error: %s\n"), re_error_str(re_sei_errno(regex)));
				re_sei_free(regex);
				return 1;
			}
		}
	}

/* This code is for changing the netlist style */
	for (i = PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN - 1; i >= 0; i--) {
		net = PCB->NetlistLib[PCB_NETLIST_INPUT].Menu + i;

		if (argc > 1) {
			if (use_re) {
				if (re_sei_exec(regex, net->Name + 2) == 0)
					continue;
			}
			else {
				if (pcb_strcasecmp(net->Name + 2, a1))
					continue;
			}
		}
		net_found = 1;

		pin = 0;
		if (func == (NFunc) pcb_netlist_style) {
			pcb_netlist_style(net, a2);
		}
		else if (argc > 3) {
			int l = strlen(a2);
			for (j = net->EntryN - 1; j >= 0; j--)
				if (pcb_strcasecmp(net->Entry[j].ListEntry, a2) == 0
						|| (pcb_strncasecmp(net->Entry[j].ListEntry, a2, l) == 0 && net->Entry[j].ListEntry[l] == '-')) {
					pin = net->Entry + j;
					pin_found = 1;
					func(net, pin);
				}
		}
		else
			func(net, 0);
	}

	if (argc > 3 && !pin_found) {
		pcb_gui->log("Net %s has no pin %s\n", a1, a2);
		return 1;
	}
	else if (!net_found) {
		pcb_gui->log("No net named %s\n", a1);
	}

	if (use_re)
		re_sei_free(regex);

	return 0;
}

pcb_action_t netlist_action_list[] = {
	{"net", pcb_act_Netlist, pcb_acth_Netlist, pcb_acts_Netlist},
	{"netlist", pcb_act_Netlist, pcb_acth_Netlist, pcb_acts_Netlist}
};

PCB_REGISTER_ACTIONS(netlist_action_list, NULL)
