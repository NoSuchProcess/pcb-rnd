/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
#include "data_it.h"
#include "board.h"
#include <librnd/core/error.h>
#include "plug_io.h"
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include "netlist.h"
#include "data_it.h"
#include "find.h"
#include "obj_term.h"
#include "search.h"
#include "obj_subc_parent.h"

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
					rnd_message(RND_MSG_ERROR, "Exactly two terminals should be selected for swap (more than 2 selected at the moment)\n");
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
		rnd_message(RND_MSG_ERROR, "Exactly two terminals should be selected for swap (less than 2 selected at the moment)\n");
		goto quit;
	}


	t1 = pcb_net_find_by_pinname(&PCB->netlist[PCB_NETLIST_EDITED], pins[0]);
	t2 = pcb_net_find_by_pinname(&PCB->netlist[PCB_NETLIST_EDITED], pins[1]);
	if ((t1 == NULL) || (t2 == NULL)) {
		rnd_message(RND_MSG_ERROR, "That terminal is not on a net.\n");
		goto quit;
	}

	n1 = t1->parent.net;
	n2 = t2->parent.net;
	if (n1 == n2) {
		rnd_message(RND_MSG_ERROR, "Those two terminals are on the same net, can't swap them.\n");
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

	n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_INPUT+(!!patch)], netname, PCB_NETA_ALLOC);
	t = pcb_net_term_get_by_pinname(n, pinname, PCB_NETA_NOALLOC);
	if (t == NULL) {
		if (!patch) {
			t = pcb_net_term_get_by_pinname(n, pinname, PCB_NETA_ALLOC);
			PCB->netlist_needs_update=1;
		}
		else {
			t = pcb_net_term_get_by_pinname(n, pinname, PCB_NETA_ALLOC);
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

static void pcb_netlist_autolen(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], new_net->name, 0);
	if (n != NULL)
		n->auto_len = 1;
	pcb_netlist_changed(0);
}

static void pcb_netlist_noautolen(pcb_net_t *new_net, pcb_net_term_t *term)
{
	pcb_net_t *n = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], new_net->name, 0);
	if (n != NULL)
		n->auto_len = 0;
	pcb_netlist_changed(0);
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

static void allrats(int val)
{
	htsp_entry_t *e;
	int chg = 0;
	for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
		pcb_net_t *n = e->value;
		if ((n != NULL) && (n->inhibit_rats != val)) {
			n->inhibit_rats = val;
			chg = 1;
		}
	}
	if (chg)
		pcb_netlist_changed(0);
}

static int pcb_netlist_allrats()
{
	allrats(0);
	return 0;
}

static int pcb_netlist_noallrats()
{
	allrats(1);
	return 0;
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
	"Net(find|select|rats|norats|ripup|addrats|clear[,net[,pin]])\n"
	"Net(freeze|thaw|forcethaw)\n"
	"Net(swap)\n"
	"Net(add,net,pin)\n"
	"Net([rename|merge],srcnet,dstnet)\n"
	"Net(remove,netname)"
	;
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
			refdes = rnd_strdup(a2);
			termid = strchr(refdes, '-');
			if (termid != NULL) {
				*termid = '\0';
				termid++;
			}
			else
				termid = "";
			for(term = pcb_termlist_first(&net->conns); term != NULL; term = pcb_termlist_next(term)) {
				if ((rnd_strcasecmp(refdes, term->refdes) == 0) && (rnd_strcasecmp(termid, term->term) == 0)) {
					pin_found = 1;
					func(net, term);
				}
			}
			
			free(refdes);
		}
		if (rnd_gui != NULL)
			rnd_gui->invalidate_all(rnd_gui);
	}
	else if (argc > 2) {
		pin_found = 1;
		{
			for(term = pcb_termlist_first(&net->conns); term != NULL; term = pcb_termlist_next(term))
				func(net, term);
		}
		if (rnd_gui != NULL)
			rnd_gui->invalidate_all(rnd_gui);
	}
	else {
		func(net, NULL);
		if (rnd_gui != NULL)
			rnd_gui->invalidate_all(rnd_gui);
	}

	return pin_found;
}

static void netlist_freeze(pcb_board_t *pcb)
{
	pcb->netlist_frozen++;
}

static void netlist_unfreeze(pcb_board_t *pcb)
{
	if (pcb->netlist_frozen > 0) {
		pcb->netlist_frozen--;
		if (pcb->netlist_needs_update)
			pcb_netlist_changed(0);
	}
}

/* Remove a net (back annotated): remove all connections so the net is empty */
static int netlist_remove(pcb_board_t *pcb, const char *netname)
{
	pcb_net_t *net;
	pcb_net_term_t *t, *tnext;

	net = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], netname, 0);
	if (net == NULL) {
		rnd_message(RND_MSG_ERROR, "No such net: '%s'\n", netname);
		return 1;
	}

	/* remove all terminals from net*/
	for(t = pcb_termlist_first(&net->conns); t != NULL; t = tnext) {
		char *pinname = rnd_strdup_printf("%s-%s", t->refdes, t->term);

		tnext = pcb_termlist_next(t);
		pcb_net_term_del(net, t);
		pcb_ratspatch_append_optimize(PCB, RATP_DEL_CONN, pinname, net->name, NULL);

		free(pinname);
	}
	pcb_net_del(&PCB->netlist[PCB_NETLIST_EDITED], netname);
	pcb_netlist_changed(0);

	return 0;
}


/* Rename or merge networks: move all conns from 'from' to 'to'. If merge,
   'to' shall be an existing network, else it shall be a non-existing network. */
static int netlist_merge(pcb_board_t *pcb, const char *from, const char *to, int merge)
{
	pcb_net_t *nfrom, *nto;
	pcb_net_term_t *t, *tnext;

	nfrom = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], from, 0);
	if (nfrom == NULL) {
		rnd_message(RND_MSG_ERROR, "No such net: '%s'\n", from);
		return 1;
	}

	nto = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], to, 0);
	if (merge) {
		if (nto == NULL) {
			rnd_message(RND_MSG_ERROR, "No such net: '%s'\n", to);
			return 1;
		}
	}
	else {
		if (nto != NULL) {
			rnd_message(RND_MSG_ERROR, "Net name '%s' already in use\n", to);
			return 1;
		}
		nto = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], to, PCB_NETA_ALLOC);
	}

	/* move over all terminals from nfrom to nto */
	for(t = pcb_termlist_first(&nfrom->conns); t != NULL; t = tnext) {
		char *pinname = rnd_strdup_printf("%s-%s", t->refdes, t->term);

		tnext = pcb_termlist_next(t);
		pcb_net_term_del(nfrom, t);
		pcb_ratspatch_append_optimize(PCB, RATP_DEL_CONN, pinname, nfrom->name, NULL);

		pcb_net_term_get_by_pinname(nto, pinname, PCB_NETA_ALLOC);
		pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pinname, nto->name, NULL);

		free(pinname);
	}
	pcb_net_del(&PCB->netlist[PCB_NETLIST_EDITED], from);
	pcb_netlist_changed(0);
	return 0;
}

static fgw_error_t pcb_act_Netlist(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	NFunc func;
	const char *a1 = NULL, *a2 = NULL;
	char *a2free = NULL;
	int op;
	pcb_net_t *net = NULL;
	unsigned net_found = 0;
	unsigned pin_found = 0;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Netlist, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_STR, Netlist, a1 = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, Netlist, a2 = argv[3].val.str);
	RND_ACT_IRES(0);

	if (!PCB)
		return 1;

	switch(op) {
		case F_Remove:
			if (a1 == NULL)
				RND_ACT_FAIL(Netlist);
			RND_ACT_IRES(netlist_remove(PCB, a1));
			return 0;
		case F_Rename:
			if (a1 == NULL)
				RND_ACT_FAIL(Netlist);
			if (a2 == NULL) {
				a2 = a2free = rnd_hid_prompt_for(RND_ACT_DESIGN, "New name of the network", NULL, "net rename");
				if (a2 == NULL) {
					RND_ACT_IRES(1);
					return 0;
				}
			}
			RND_ACT_IRES(netlist_merge(PCB, a1, a2, 0));
			free(a2free);
			return 0;
		case F_Merge:
			if (a1 == NULL)
				RND_ACT_FAIL(Netlist);
			if (a2 == NULL) {
				a2 = a2free = rnd_hid_prompt_for(RND_ACT_DESIGN, "Network name to merge into", NULL, "net merge");
				if (a2 == NULL) {
					RND_ACT_IRES(1);
					return 0;
				}
			}
			RND_ACT_IRES(netlist_merge(PCB, a1, a2, 1));
			free(a2free);
			return 0;
		case F_Find: func = pcb_netlist_find; break;
		case F_Select: func = pcb_netlist_select; break;
		case F_Unselect: func = pcb_netlist_unselect; break;
		case F_AutoLen: func = pcb_netlist_autolen; break;
		case F_NoAutoLen: func = pcb_netlist_noautolen; break;
		case F_Rats: func = pcb_netlist_rats; break;
		case F_NoRats: func = pcb_netlist_norats; break;
		case F_AllRats: return pcb_netlist_allrats();
		case F_NoAllRats: return pcb_netlist_noallrats();
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
			netlist_freeze(PCB);
			return 0;
		case F_Thaw:
			netlist_unfreeze(PCB);
			return 0;
		case F_ForceThaw:
			PCB->netlist_frozen = 0;
			if (PCB->netlist_needs_update)
				pcb_netlist_changed(0);
			return 0;
		default:
			RND_ACT_FAIL(Netlist);
	}

	netlist_freeze(PCB);
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
	netlist_unfreeze(PCB);

	if (argc > 3 && !pin_found) {
		rnd_message(RND_MSG_ERROR, "Net %s has no pin %s\n", a1, a2);
		return 1;
	}
	else if (!net_found) {
		rnd_message(RND_MSG_ERROR, "No net named %s\n", a1);
	}


	return 0;
}

static void claim_import_by_flag(pcb_data_t *data, vtp0_t *termlist, unsigned long flg)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;

	for(o = pcb_data_first(&it, data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		pcb_subc_t *subc;

		if (o->type == PCB_OBJ_SUBC)
			claim_import_by_flag(((pcb_subc_t *)o)->data, termlist, flg);

		if ((o->term == NULL) || (!PCB_FLAG_TEST(flg, o)))
			continue;

		subc = pcb_obj_parent_subc(o);
		if ((subc == NULL) || (subc->refdes == NULL))
			continue;

		vtp0_append(termlist, o);
	}
}

static int claim_net_cb(pcb_find_t *ctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	pcb_subc_t *subc;
	vtp0_t *termlist = ctx->user_data;

	if (new_obj->term == NULL)
		return 0;

	subc = pcb_obj_parent_subc(new_obj);
	if ((subc == NULL) || (subc->refdes == NULL))
		return 0;

	vtp0_append(termlist, new_obj);
	return 0;
}

static void pcb_net_claim_from_list(pcb_board_t *pcb, pcb_net_t *net, vtp0_t *termlist)
{
	size_t n;

	for(n = 0; n < vtp0_len(termlist); n++) {
		pcb_any_obj_t *obj = termlist->array[n];
		pcb_subc_t *subc = pcb_obj_parent_subc(obj);
		pcb_net_term_t *t = pcb_net_find_by_obj(&pcb->netlist[PCB_NETLIST_EDITED], obj);
		char *pinname;

		if (t != NULL) {
			/* remove term from its original net */
			pinname = rnd_strdup_printf("%s-%s", t->refdes, t->term);
			pcb_ratspatch_append_optimize(PCB, RATP_DEL_CONN, pinname, net->name, NULL);
			free(pinname);
		}

		t = pcb_net_term_get(net, subc->refdes, obj->term, PCB_NETA_ALLOC);
		pinname = rnd_strdup_printf("%s-%s", t->refdes, t->term);
		pcb_ratspatch_append_optimize(PCB, RATP_ADD_CONN, pinname, net->name, NULL);
		free(pinname);
	}
}

static const char pcb_acts_ClaimNet[] = "ClaimNet(object|selected|found,[netname])\n";
static const char pcb_acth_ClaimNet[] = "Claim existing connections and create a new net";
/* DOC: claimnet.html */
static fgw_error_t pcb_act_ClaimNet(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_find_t fctx;
	int op;
	vtp0_t termlist;
	pcb_net_t *net;
	char *netname = NULL, *free_netname = NULL;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Netlist, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_STR, Netlist, netname = argv[2].val.str);
	RND_ACT_IRES(0);

	vtp0_init(&termlist);
	switch(op) {
		case F_Object:
			{
				rnd_coord_t x, y;
				void *r1, *r2, *r3;

				rnd_hid_get_coords("Select an object to claim network from", &x, &y, 0);
				if (pcb_search_screen(x, y, PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_PSTK, &r1, &r2, &r3) <= 0)
					return 0;

				memset(&fctx, 0, sizeof(fctx));
				fctx.consider_rats = 1;
				fctx.found_cb = claim_net_cb;
				fctx.user_data = &termlist;
				pcb_find_from_obj(&fctx, PCB->Data, (pcb_any_obj_t *)r2);
			}
			break;
		case F_Found:
			claim_import_by_flag(PCB->Data, &termlist, PCB_FLAG_FOUND);
			break;
		case F_Selected:
			claim_import_by_flag(PCB->Data, &termlist, PCB_FLAG_SELECTED);
			break;
		default:
			vtp0_uninit(&termlist);
			RND_ACT_FAIL(ClaimNet);
	}

	if (termlist.used < 1) {
		vtp0_uninit(&termlist);
		rnd_message(RND_MSG_ERROR, "Can not claim network: no terminal found.\nPlease pick objects that have terminals with refdes-termID.\n");
		RND_ACT_IRES(1);
		return 0;
	}

	if (netname == NULL) {
		free_netname = netname = rnd_hid_prompt_for(RND_ACT_DESIGN, "Name of the new network", NULL, "net name");
		if (netname == NULL) {
			vtp0_uninit(&termlist);
			RND_ACT_IRES(1);
			return 0;
		}
	}

	if (pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], netname, 0) != NULL) {
		free(free_netname);
		vtp0_uninit(&termlist);
		rnd_message(RND_MSG_ERROR, "Can not claim network: '%s' is an existing network\n", netname);
		RND_ACT_IRES(1);
		return 0;
	}

	net = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], netname, PCB_NETA_ALLOC);
	free(free_netname);

	if (net == NULL) {
		vtp0_uninit(&termlist);
		rnd_message(RND_MSG_ERROR, "Can not claim network: failed to create net '%s'\n", netname);
		RND_ACT_IRES(1);
		return 0;
	}

	pcb_net_claim_from_list(PCB, net, &termlist);
	pcb_netlist_changed(0);
	vtp0_uninit(&termlist);
	return 0;
}

static int ba_subc_(pcb_board_t *pcb, pcb_subc_t *subc, int op2, int undoable)
{
	int res = -1;

	switch(op2) {
		case F_Add:
			res = rats_patch_add_subc(pcb, subc, undoable);
			if (res == 0) {
				char *val;
				val = pcb_attribute_get(&subc->Attributes, "footprint");
				if (val != NULL)
					pcb_ratspatch_append(pcb, RATP_CHANGE_COMP_ATTRIB, subc->refdes, "footprint", val, 1);
				val = pcb_attribute_get(&subc->Attributes, "value");
				if (val != NULL)
					pcb_ratspatch_append(pcb, RATP_CHANGE_COMP_ATTRIB, subc->refdes, "value", val, 1);
			}
			break;
		case F_Remove:
			if (!rats_patch_is_subc_referenced(pcb, subc->refdes))
				return 0; /* already removed */
			rats_patch_break_subc_conns(pcb, subc, undoable);
			return rats_patch_del_subc(pcb, subc, 1, undoable);
		default:
			rnd_message(RND_MSG_ERROR, "BaSubc(): invalid second argument\n");
	}
	return res;
}

static int ba_subc_object(pcb_board_t *pcb, int op2, int undoable)
{
	rnd_coord_t x, y;
	void *r1, *r2, *r3;

	rnd_hid_get_coords("Select a subcircuit for back annotation", &x, &y, 0);
	if ((pcb_search_screen(x, y, PCB_OBJ_SUBC, &r1, &r2, &r3) <= 0) || (r2 == NULL)) {
		rnd_message(RND_MSG_ERROR, "No subcircuit under the cursor\n");
		return -1;
	}

	return ba_subc_(pcb, r2, op2, undoable);
}

static int ba_subc_selected(pcb_board_t *pcb, pcb_data_t *data, int op2, int undoable)
{
	int res = 0;

	PCB_SUBC_LOOP(data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc))
			res |= ba_subc_(pcb, subc, op2, undoable);
		ba_subc_selected(pcb, subc->data, op2, undoable); /* recurse for subc in subc */
	}
	PCB_END_LOOP;

	return res;
}

static const char pcb_acts_BaSubc[] = "BaSubc(object|selected, add|remove)\n";
static const char pcb_acth_BaSubc[] = "Add new subcircuits to the back annotation list or remove and back annotate removal of a subcircuit";
/* DOC: basubc.html */
static fgw_error_t pcb_act_BaSubc(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op1, op2;
	pcb_board_t *pcb = PCB_ACT_BOARD;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Netlist, op1 = fgw_keyword(&argv[1]));
	RND_ACT_CONVARG(2, FGW_KEYWORD, Netlist, op2 = fgw_keyword(&argv[2]));
	RND_ACT_IRES(-1);

	switch(op1) {
		case F_Object:   RND_ACT_IRES(ba_subc_object(pcb, op2, 1)); break;
		case F_Selected: RND_ACT_IRES(ba_subc_selected(pcb, pcb->Data, op2, 1)); break;
		default:
			rnd_message(RND_MSG_ERROR, "Invalid first argument for BaSubc()\n");
	}
	return 0;
}

static pcb_any_obj_t *find_term(pcb_board_t *pcb)
{
	int rv;
	rnd_coord_t x, y;
	void *r1, *r2, *r3;
	pcb_any_obj_t *obj;

	rnd_hid_get_coords("Select a termina to connect", &x, &y, 0);
	rv = pcb_search_screen(x, y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &r1, &r2, &r3);
	obj = r2;
	if ((rv <= 0) || (obj == NULL) || (obj->term == NULL))
		rv = pcb_search_screen(x, y, PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_PSTK| PCB_OBJ_SUBC_PART, &r1, &r2, &r3);
	obj = r2;
	if ((rv <= 0) || (obj == NULL) || (obj->term == NULL)) {
		rnd_message(RND_MSG_ERROR, "No terminal for BaConn()\n");
		return NULL;
	}

	return obj;
}

static const char pcb_acts_BaConn[] =
	"BaConn(object|selected, add, [netname])\n"
	"BaConn(object|selected, remove)\n";
static const char pcb_acth_BaConn[] = "Add a terminal-net connection to the back annotation list or add a terminal-net connection removal to the back annotation list";
/* DOC: basubc.html */
static fgw_error_t pcb_act_BaConn(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op1, op2;
	pcb_board_t *pcb = PCB_ACT_BOARD;
	char *netname = NULL, *freeme = NULL;
	pcb_net_t *net;
	pcb_any_obj_t *obj;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Netlist, op1 = fgw_keyword(&argv[1]));
	RND_ACT_CONVARG(2, FGW_KEYWORD, Netlist, op2 = fgw_keyword(&argv[2]));
	RND_ACT_IRES(-1);

	switch(op2) {

		case F_Add:

			RND_ACT_MAY_CONVARG(3, FGW_STR, BaConn, netname = argv[3].val.str);
			if (netname == NULL)
				freeme = netname = rnd_hid_prompt_for(RND_ACT_DESIGN, "Name of the network to connecto to", NULL, "connect term to net: netname");
			if (netname == NULL)
				return 0; /* cancel */
			net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_EDITED], netname, 0);
			if (net == NULL) {
				rnd_message(RND_MSG_ERROR, "BaConn(): net '%s' not found\n", netname);
				free(freeme);
				return 0;
			}
			free(freeme);

			switch(op1) {
				case F_Object:
					obj = find_term(pcb);
					if (obj == NULL)
						return 0;
					if (pcb_ratspatch_addconn_term(pcb, net, obj) > 0)
						RND_ACT_IRES(0);
					break;
				case F_Selected:
					if (pcb_ratspatch_addconn_selected(pcb, pcb->Data, net) > 0)
						RND_ACT_IRES(0);
					break;
				default:
					rnd_message(RND_MSG_ERROR, "Invalid first argument for BaConn()\n");
			}
		break;
		case F_Remove:
			switch(op1) {
				case F_Object:
					obj = find_term(pcb);
					if (obj == NULL)
						return 0;
					if (pcb_ratspatch_delconn_term(pcb, obj) > 0)
						RND_ACT_IRES(0);
					break;
				case F_Selected:
					if (pcb_ratspatch_delconn_selected(pcb, pcb->Data) > 0)
						RND_ACT_IRES(0);
					break;
				default:
					rnd_message(RND_MSG_ERROR, "Invalid first argument for BaConn()\n");
			}
			break;

		default:
			rnd_message(RND_MSG_ERROR, "Invalid second argument for BaConn()\n");
	}
	return 0;
}

static rnd_action_t netlist_action_list[] = {
	{"net", pcb_act_Netlist, pcb_acth_Netlist, pcb_acts_Netlist},
	{"netlist", pcb_act_Netlist, pcb_acth_Netlist, pcb_acts_Netlist},
	{"claimnet", pcb_act_ClaimNet, pcb_acth_ClaimNet, pcb_acts_ClaimNet},
	{"basubc", pcb_act_BaSubc, pcb_acth_BaSubc, pcb_acts_BaSubc},
	{"baconn", pcb_act_BaConn, pcb_acth_BaConn, pcb_acts_BaConn}
};

void pcb_netlist_act_init2(void)
{
	RND_REGISTER_ACTIONS(netlist_action_list, NULL);
}
