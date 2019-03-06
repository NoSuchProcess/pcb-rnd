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
#include "compat_misc.h"
#include "netlist2.h"
#include "data_it.h"
#include "find.h"
#include "obj_term.h"

static int pcb_netlist_swap()
{
	char *pins[3] = { NULL, NULL, NULL };
	int next = 0, n;
	int ret = -1;
	pcb_net_term_t *t1, *t2;
	pcb_net_t *n1, *n2;

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


	t1 = pcb_net_find_by_pinname(&PCB->netlist[PCB_NETLIST_EDITED], pins[0]);
	t2 = pcb_net_find_by_pinname(&PCB->netlist[PCB_NETLIST_EDITED], pins[1]);
	if ((t1 == NULL) || (t2 == NULL)) {
		pcb_message(PCB_MSG_ERROR, "That terminal is not on a net.\n");
		goto quit;
	}

	n1 = t1->parent.net;
	n2 = t2->parent.net;
	if (n1 == n2) {
		pcb_message(PCB_MSG_ERROR, "Those two terminals are on the same net, can't swap them.\n");
		goto quit;
	}


	pcb_ratspatch_append_optimize(PCB, RATP_DEL_CONN, pins[0], n1->name, NULL);
	pcb_ratspatch_append_optimize(PCB, RATP_DEL_CONN, pins[1], n2->name, NULL);
	pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pins[0], n2->name, NULL);
	pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pins[1], n1->name, NULL);

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
	pcb_net_t *n;
	pcb_net_term_t *t;

	if ((netname == NULL) || (pinname == NULL))
		return -1;

	if ((*netname == '\0') || (*pinname == '\0'))
		return -1;

	n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_INPUT+(!!patch)], netname, 1);
	t = pcb_net_term_get_by_pinname(n, pinname, 0);
	if (t == NULL) {
		if (!patch) {
			t = pcb_net_term_get_by_pinname(n, pinname, 1);
			PCB->netlist_needs_update=1;
		}
		else {
			t = pcb_net_term_get_by_pinname(n, pinname, 1);
			pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pinname, netname, NULL);
		}
	}

	pcb_netlist_changed(0);
	return 0;
}

static void pcb_netlist_find(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_crawl_flag(PCB, pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], new_net->name, 0), PCB_FLAG_FOUND, 0);
}

static void pcb_netlist_select(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_crawl_flag(PCB, pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], new_net->name, 0), PCB_FLAG_SELECTED, 0);
}

static void pcb_netlist_unselect(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_crawl_flag(PCB, pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], new_net->name, 0), 0, PCB_FLAG_SELECTED);
}

static void pcb_netlist_rats(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], new_net->name, 0);
	if (n != NULL)
		n->inhibit_rats = 0;
	pcb_netlist_changed(0);
}

static void pcb_netlist_norats(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], new_net->name, 0);
	if (n != NULL)
		n->inhibit_rats = 1;
	pcb_netlist_changed(0);
}

/* The primary purpose of this action is to remove the netlist
   completely so that a new one can be loaded, usually via a gsch2pcb
   style script.  */
static void pcb_netlist_clear(pcb_net_t *net, pcb_net_term_t *term)
{
	int ni;

	if (net == 0) {
		/* Clear the entire netlist. */
		for (ni = 0; ni < PCB_NUM_NETLISTS; ni++) {
			pcb_netlist_uninit(&PCB->netlist[ni]);
			pcb_netlist_init(&PCB->netlist[ni]);
		}
	}
	else if (term == NULL) {
		/* Remove a net from the netlist. */
		pcb_net_del(&PCB->netlist[PCB_NETLIST_EDITED], net->name);
	}
	else {
		/* Remove a pin from the given net.  Note that this may leave an
		   empty net, which is different than removing the net
		   (above).  */
		pcb_net_term_del(net, term);
	}
	pcb_netlist_changed(0);
}

static void pcb_netlist_style(pcb_net_t *new_net, const char *style)
{
	pcb_attribute_put(&new_net->Attributes, "style", style);
}

static void pcb_netlist_ripup(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_ripup(PCB, new_net);
}

static void pcb_netlist_addrats(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_add_rats(PCB, new_net, PCB_RATACC_PRECISE);
}


static const char pcb_acts_Netlist[] =
	"Net(find|select|rats|norats||ripup|addrats|clear[,net[,pin]])\n" "Net(freeze|thaw|forcethaw)\n" "Net(swap)\n" "Net(add,net,pin)";

static const char pcb_acth_Netlist[] = "Perform various actions on netlists.";

/* DOC: netlist.html */
static fgw_error_t pcb_act_Netlist(fgw_arg_t *res, int argc, fgw_arg_t *argv);

typedef void (*NFunc)(pcb_net_t *net, pcb_net_term_t *term);

static unsigned netlist_act_do(pcb_net_t *net, int argc, const char *a1, const char *a2, NFunc func)
{
	pcb_net_term_t *term = NULL;
	int pin_found = 0;

	if (func == (NFunc)pcb_netlist_style) {
		pcb_netlist_style(net, a2);
		return 1;
	}

	if (argc > 3) {
		{
			char *refdes, *termid;
			refdes = pcb_strdup(a2);
			termid = strchr(refdes, '-');
			if (termid != NULL) {
				*termid = '\0';
				termid++;
			}
			else
				termid = "";
			for(term = pcb_termlist_first(&net->conns); term != NULL; term = pcb_termlist_next(term)) {
				if ((pcb_strcasecmp(refdes, term->refdes) == 0) && (pcb_strcasecmp(termid, term->term) == 0)) {
					pin_found = 1;
					func(net, term);
				}
			}
			
			free(refdes);
		}
		if (pcb_gui != NULL)
			pcb_gui->invalidate_all();
	}
	else if (argc > 2) {
		pin_found = 1;
		{
			for(term = pcb_termlist_first(&net->conns); term != NULL; term = pcb_termlist_next(term))
				func(net, term);
		}
		if (pcb_gui != NULL)
			pcb_gui->invalidate_all();
	}
	else {
		func(net, NULL);
		if (pcb_gui != NULL)
			pcb_gui->invalidate_all();
	}

	return pin_found;
}

static fgw_error_t pcb_act_Netlist(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	NFunc func;
	const char *a1 = NULL, *a2 = NULL;
	int op;
	pcb_net_t *net = NULL;
	unsigned net_found = 0;
	unsigned pin_found = 0;

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

	{
		pcb_netlist_t *nl = &PCB->netlist[PCB_NETLIST_EDITED];
		net = pcb_net_get(PCB, nl, a1, 0);
		if (net == NULL)
			net = pcb_net_get_icase(PCB, nl, a1);
		if (net == NULL) {  /* no direct match - have to go for the multi-net regex approach */
			re_sei_t *regex = re_sei_comp(a1);
			if (re_sei_errno(regex) == 0) {
				htsp_entry_t *e;
				for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
					pcb_net_t *net = e->value;
					if (re_sei_exec(regex, net->name)) {
						net_found = 1;
						pin_found |= netlist_act_do(net, argc, a1, a2, func);
					}
				}
			}
			re_sei_free(regex);
		}
		else {
			net_found = 1;
			pin_found |= netlist_act_do(net, argc, a1, a2, func);
		}
	}

	if (argc > 3 && !pin_found) {
		pcb_gui->log("Net %s has no pin %s\n", a1, a2);
		return 1;
	}
	else if (!net_found) {
		pcb_gui->log("No net named %s\n", a1);
	}


	return 0;
}

pcb_action_t netlist_action_list[] = {
	{"net", pcb_act_Netlist, pcb_acth_Netlist, pcb_acts_Netlist},
	{"netlist", pcb_act_Netlist, pcb_acth_Netlist, pcb_acts_Netlist}
};

PCB_REGISTER_ACTIONS(netlist_action_list, NULL)
