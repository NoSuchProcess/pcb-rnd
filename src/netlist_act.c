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
#include "netlist2.h"
#include "data_it.h"
#include "find.h"

TODO("netlist: remove these all with the old netlist removal")
#include "brave.h"
#include "netlist.h"
#include "rats.h"

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

TODO("netlist: remove with the old netlist code:")
static pcb_any_obj_t *pcb_pin_name_to_obj(pcb_lib_entry_t *pin)
{
	pcb_connection_t conn;
	if (!pcb_rat_seek_pad(pin, &conn, pcb_false))
		return NULL;
	return conn.obj;
}

static unsigned long pcb_netlist_setclrflg(pcb_lib_menu_t *net, pcb_lib_entry_t *pin, pcb_flag_values_t setf, pcb_flag_values_t clrf)
{
	pcb_find_t fctx;
	pcb_any_obj_t *o;
	unsigned long res;

	o = pcb_pin_name_to_obj(pin);
	if (o == NULL)
		return 0;

	memset(&fctx, 0, sizeof(fctx));
	fctx.flag_set = setf;
	fctx.flag_clr = clrf;
	fctx.flag_chg_undoable = 1;
	fctx.consider_rats = 1;
	res = pcb_find_from_obj(&fctx, PCB->Data, o);
	pcb_find_free(&fctx);
	return res;
}

static void pcb_netlist_find(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
{
	if (pcb_brave & PCB_BRAVE_NETLIST2)
		pcb_net_crawl_flag(PCB, pcb_net_get(PCB, &PCB->netlist[0], net->Name+2, 0), PCB_FLAG_FOUND, 0);
	else
		pcb_netlist_setclrflg(net, pin, PCB_FLAG_FOUND, 0);
}

static void pcb_netlist_select(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
{
	if (pcb_brave & PCB_BRAVE_NETLIST2)
		pcb_net_crawl_flag(PCB, pcb_net_get(PCB, &PCB->netlist[0], net->Name+2, 0), PCB_FLAG_SELECTED, 0);
	else
		pcb_netlist_setclrflg(net, pin, PCB_FLAG_SELECTED, 0);
}

static void pcb_netlist_unselect(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
{
	if (pcb_brave & PCB_BRAVE_NETLIST2)
		pcb_net_crawl_flag(PCB, pcb_net_get(PCB, &PCB->netlist[0], net->Name+2, 0), 0, PCB_FLAG_SELECTED);
	else
	pcb_netlist_setclrflg(net, pin, 0, PCB_FLAG_SELECTED);
}

static void pcb_netlist_rats(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
{
	if (pcb_brave & PCB_BRAVE_NETLIST2) {
		pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], net->Name+2, 0);
		if (n != NULL)
			n->inhibit_rats = 0;
	}
	net->Name[0] = ' ';
	net->flag = 1;
	pcb_netlist_changed(0);
}

static void pcb_netlist_norats(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
{
	if (pcb_brave & PCB_BRAVE_NETLIST2) {
		pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], net->Name+2, 0);
		if (n != NULL)
			n->inhibit_rats = 1;
	}
	net->Name[0] = '*';
	net->flag = 0;
	pcb_netlist_changed(0);
}

/* The primary purpose of this action is to remove the netlist
   completely so that a new one can be loaded, usually via a gsch2pcb
   style script.  */
static void pcb_netlist_clear(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
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

static void pcb_netlist_style(pcb_lib_menu_t *net, const char *style)
{
	free(net->Style);
	net->Style = pcb_strdup_null((char *) style);
}

static void pcb_netlist_ripup(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
{
	pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], net->Name+2, 0);
	assert(n != NULL);
	pcb_net_ripup(PCB, n);
}

static void pcb_netlist_addrats(pcb_lib_menu_t *net, pcb_lib_entry_t *pin)
{
	pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], net->Name+2, 0);
	assert(n != NULL);
	pcb_net_add_rats(PCB, n, PCB_RATACC_PRECISE);
}


static const char pcb_acts_Netlist[] =
	"Net(find|select|rats|norats||ripup|addrats|clear[,net[,pin]])\n" "Net(freeze|thaw|forcethaw)\n" "Net(swap)\n" "Net(add,net,pin)";

static const char pcb_acth_Netlist[] = "Perform various actions on netlists.";

/* DOC: netlist.html */

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
		case F_Unselect: func = pcb_netlist_unselect; break;
		case F_Rats: func = pcb_netlist_rats; break;
		case F_NoRats: func = pcb_netlist_norats; break;
		case F_AddRats: func = pcb_netlist_addrats; break;
		case F_Style: func = (NFunc) pcb_netlist_style; break;
		case F_Ripup: func = pcb_netlist_ripup; break;
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
				if (pcb_strcasecmp(net->Name + 2, a1) != 0)
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
			for (j = net->EntryN - 1; j >= 0; j--) {
				if (pcb_strcasecmp(net->Entry[j].ListEntry, a2) == 0
						|| (pcb_strncasecmp(net->Entry[j].ListEntry, a2, l) == 0 && net->Entry[j].ListEntry[l] == '-')) {
					pin = net->Entry + j;
					pin_found = 1;
					func(net, pin);
				}
			}
			if (pcb_gui != NULL)
				pcb_gui->invalidate_all();
		}
		else if (argc > 2) {
			pin_found = 1;
			if (pcb_brave & PCB_BRAVE_NETLIST2) {
				func(net, net->Entry);
			}
			else {
				for (j = net->EntryN - 1; j >= 0; j--)
					func(net, net->Entry + j);
			}
			if (pcb_gui != NULL)
				pcb_gui->invalidate_all();
		}
		else {
			func(net, 0);
			if (pcb_gui != NULL)
				pcb_gui->invalidate_all();
		}
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
