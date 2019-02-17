/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"
#include <genht/htsp.h>
#include <genht/hash.h>
#include <ctype.h>

#include "board.h"
#include "data.h"
#include "compat_misc.h"
#include "layer_grp.h"
#include "find.h"
#include "obj_term.h"
#include "conf_core.h"
#include "undo.h"
#include "obj_rat_draw.h"
#include "obj_subc_parent.h"

#define TDL_DONT_UNDEF
#include "netlist2.h"
#include <genlist/gentdlist_impl.c>

void pcb_net_term_free_fields(pcb_net_term_t *term)
{
	pcb_attribute_free(&term->Attributes);
	free(term->refdes);
	free(term->term);
}

void pcb_net_term_free(pcb_net_term_t *term)
{
	pcb_net_term_free_fields(term);
	free(term);
}

pcb_net_term_t *pcb_net_term_get(pcb_net_t *net, const char *refdes, const char *term, pcb_bool alloc)
{
	pcb_net_term_t *t;

	/* for allocation this is slow, O(N^2) algorithm, but other than a few
	   biggish networks like GND, there won't be too many connections anyway) */
	for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
		if ((strcmp(t->refdes, refdes) == 0) && (strcmp(t->term, term) == 0))
			return t;
	}

	if (alloc) {
		t = calloc(sizeof(pcb_net_term_t), 1);
		t->type = PCB_OBJ_NET_TERM;
		t->parent_type = PCB_PARENT_NET;
		t->parent.net = net;
		t->refdes = pcb_strdup(refdes);
		t->term = pcb_strdup(term);
		pcb_termlist_append(&net->conns, t);
		return t;
	}

	return NULL;
}

pcb_net_term_t *pcb_net_term_get_by_obj(pcb_net_t *net, const pcb_any_obj_t *obj)
{
	pcb_data_t *data;
	pcb_subc_t *sc;

	if (obj->term == NULL)
		return NULL;

	if (obj->parent_type == PCB_PARENT_LAYER)
		data = obj->parent.layer->parent.data;
	else if (obj->parent_type == PCB_PARENT_DATA)
		data = obj->parent.data;
	else
		return NULL;

	if (data->parent_type != PCB_PARENT_SUBC)
		return NULL;

	sc = data->parent.subc;
	if (sc->refdes == NULL)
		return NULL;

	return pcb_net_term_get(net, sc->refdes, obj->term, pcb_false);
}

pcb_net_term_t *pcb_net_term_get_by_pinname(pcb_net_t *net, const char *pinname, pcb_bool alloc)
{
	char tmp[256];
	char *pn, *refdes, *term;
	int len = strlen(pinname)+1;
	pcb_net_term_t *t = NULL;

	if (len <= sizeof(tmp)) {
		pn = tmp;
		memcpy(pn, pinname, len);
	}
	else
		pn = pcb_strdup(pinname);
	

	refdes = pn;
	term = strchr(refdes, '-');
	if (term != NULL) {
		*term = '\0';
		term++;
		t = pcb_net_term_get(net, refdes, term, alloc);
	}

	if (pn != tmp)
		free(pn);
	return t;

}


int pcb_net_term_del(pcb_net_t *net, pcb_net_term_t *term)
{
	pcb_termlist_remove(term);
	pcb_net_term_free(term);
	return 0;
}


int pcb_net_term_del_by_name(pcb_net_t *net, const char *refdes, const char *term)
{
	pcb_net_term_t *t;

	for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t))
		if ((strcmp(t->refdes, refdes) == 0) && (strcmp(t->term, term) == 0))
			return pcb_net_term_del(net, t);

	return -1;
}

pcb_bool pcb_net_name_valid(const char *netname)
{
	for(;*netname != '\0'; netname++) {
		if (isalnum(*netname)) continue;
		switch(*netname) {
			case '_':
				break;
			return pcb_false;
		}
	}
	return pcb_true;
}

pcb_net_t *pcb_net_get(pcb_board_t *pcb, pcb_netlist_t *nl, const char *netname, pcb_bool alloc)
{
	pcb_net_t *net;

	if (nl == NULL)
		return NULL;

	if (!pcb_net_name_valid(netname))
		return NULL;

	net = htsp_get(nl, netname);
	if (net != NULL)
		return net;

	if (alloc) {
		net = calloc(sizeof(pcb_net_t), 1);
		net->type = PCB_OBJ_NET;
		net->parent_type = PCB_PARENT_BOARD;
		net->parent.board = pcb;
		net->name = pcb_strdup(netname);
		htsp_set(nl, net->name, net);
		return net;
	}
	return NULL;
}

void pcb_net_free_fields(pcb_net_t *net)
{
	pcb_attribute_free(&net->Attributes);
	free(net->name);
	for(;;) {
		pcb_net_term_t *term = pcb_termlist_first(&net->conns);
		if (term == NULL)
			break;
		pcb_termlist_remove(term);
		pcb_net_term_free(term);
	}
}

void pcb_net_free(pcb_net_t *net)
{
	pcb_net_free_fields(net);
	free(net);
}

int pcb_net_del(pcb_netlist_t *nl, const char *netname)
{
	htsp_entry_t *e;

	if (nl == NULL)
		return -1;

	e = htsp_getentry(nl, netname);
	if (e == NULL)
		return -1;

	pcb_net_free(e->value);

	htsp_delentry(nl, e);
	return 0;
}

static pcb_cardinal_t pcb_net_term_crawl(const pcb_board_t *pcb, pcb_net_term_t *term, pcb_find_t *fctx, int first)
{
	pcb_any_obj_t *o;

/* there can be multiple terminals with the same ID, but it is enough to run find from the first: find.c will consider them all */
	o = pcb_term_find_name(pcb, pcb->Data, PCB_LYT_COPPER, term->refdes, term->term, 0, NULL, NULL);
	if (o == NULL)
		return 0;

	if (first)
		return pcb_find_from_obj(fctx, PCB->Data, o);

	if (PCB_FIND_IS_MARKED(fctx, o))
		return 0; /* already visited, no need to run 'find' again */

	return pcb_find_from_obj_next(fctx, PCB->Data, o);
}

/* Short circuit found between net and an offender object that should not
   be part of the net but is connected to the net */
static void net_found_short(pcb_board_t *pcb, pcb_net_t *net, pcb_any_obj_t *offender)
{
	pcb_subc_t *sc = pcb_obj_parent_subc(offender);
TODO("This should be PCB_NETLIST_EDITED - revise the rest too")
	pcb_net_term_t *offt = pcb_net_find_by_refdes_term(&pcb->netlist[PCB_NETLIST_INPUT], sc->refdes, offender->term);
	pcb_net_t *offn;
	const char *offnn = "<unknown>";
	
	if (offt != NULL) {
		offn = offt->parent.net;
		offnn = offn->name;
	}

	pcb_trace("short: net=%s offender=%s-%s (of net %s)\n", net->name, sc->refdes, offender->term, offnn);
}

static int net_short_check(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	if (new_obj->term != NULL) {
		pcb_net_term_t *t;
		pcb_net_t *net = fctx->user_data;
		pcb_subc_t *sc = pcb_obj_parent_subc(new_obj);
		if (sc == NULL)
			return 0;

		/* if new_obj is a terminal on our net, return */
		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t))
			if ((strcmp(t->refdes, sc->refdes) == 0) && (strcmp(t->term, new_obj->term) == 0))
				return 0;

		/* new_obj is not on our net but has a refdes-term -> must be a short */
		net_found_short(PCB, net, new_obj);
	}

	return 0;
}

pcb_cardinal_t pcb_net_crawl_flag(pcb_board_t *pcb, pcb_net_t *net, unsigned long setf, unsigned long clrf)
{
	pcb_find_t fctx;
	pcb_net_term_t *t;
	pcb_cardinal_t res = 0, n;

	memset(&fctx, 0, sizeof(fctx));
	fctx.flag_set = setf;
	fctx.flag_clr = clrf;
	fctx.flag_chg_undoable = 1;
	fctx.only_mark_rats = 1; /* do not trust rats, but do mark them */
	fctx.user_data = net;
	fctx.found_cb = net_short_check;

	for(t = pcb_termlist_first(&net->conns), n = 0; t != NULL; t = pcb_termlist_next(t), n++) {
		res += pcb_net_term_crawl(pcb, t, &fctx, (n == 0));
	}

	pcb_find_free(&fctx);
	return res;
}

pcb_net_term_t *pcb_net_find_by_refdes_term(pcb_netlist_t *nl, const char *refdes, const char *term)
{
	htsp_entry_t *e;

	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_t *net = (pcb_net_t *)e->value;
		pcb_net_term_t *t;

		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t))
			if ((strcmp(t->refdes, refdes) == 0) && (strcmp(t->term, term) == 0))
				return t;
	}

	return NULL;
}

pcb_net_term_t *pcb_net_find_by_pinname(pcb_netlist_t *nl, const char *pinname)
{
	char tmp[256];
	char *pn, *refdes, *term;
	int len = strlen(pinname)+1;
	pcb_net_term_t *t = NULL;

	if (len <= sizeof(tmp)) {
		pn = tmp;
		memcpy(pn, pinname, len);
	}
	else
		pn = pcb_strdup(pinname);

	refdes = pn;
	term = strchr(refdes, '-');
	if (term != NULL) {
		*term = '\0';
		term++;
		t = pcb_net_find_by_refdes_term(nl, refdes, term);
	}

	if (pn != tmp)
		free(pn);
	return t;
}

static int netname_sort(const void *va, const void *vb)
{
	const pcb_net_t **a = (const pcb_net_t **)va;
	const pcb_net_t **b = (const pcb_net_t **)vb;
	return strcmp((*a)->name, (*b)->name);
}

pcb_net_t **pcb_netlist_sort(pcb_netlist_t *nl)
{
	pcb_net_t **arr;
	htsp_entry_t *e;
	long n;

	if (nl->used == 0)
		return NULL;
	arr = malloc(nl->used * sizeof(pcb_net_t));

	for(e = htsp_first(nl), n = 0; e != NULL; e = htsp_next(nl, e), n++)
		arr[n] = e->value;
	qsort(arr, nl->used, sizeof(pcb_net_t *), netname_sort);
	return arr;
}

#include "netlist_geo.c"

pcb_cardinal_t pcb_net_add_rats(const pcb_board_t *pcb, pcb_net_t *net, pcb_rat_accuracy_t acc)
{
	pcb_find_t fctx;
	pcb_net_term_t *t;
	pcb_cardinal_t drawn = 0, r, n, s1, s2, su, sd;
	vtp0_t subnets;
	pcb_subnet_dist_t *connmx;
	char *done;
	int left;
	pcb_rat_t *line;

	memset(&fctx, 0, sizeof(fctx));
	fctx.consider_rats = 1; /* keep existing rats and their connections */
	fctx.list_found = 1;
	fctx.user_data = net;
	fctx.found_cb = net_short_check;
	vtp0_init(&subnets);

	/* each component of a desired network is called a submnet; already connected
	   objects of each subnet is collected on a vtp0_t; object-lists per submnet
	   is saved in variable "subnets" */
	for(t = pcb_termlist_first(&net->conns), n = 0; t != NULL; t = pcb_termlist_next(t), n++) {
		r = pcb_net_term_crawl(pcb, t, &fctx, (n == 0));
		if (r > 0) {
			vtp0_t *objs = malloc(sizeof(vtp0_t));
			memcpy(objs, &fctx.found, sizeof(vtp0_t));
			vtp0_append(&subnets, objs);
			memset(&fctx.found, 0, sizeof(vtp0_t));
		}
	}

	/* find the shortest connection between any two subnets and save the info
	   in connmx */
	connmx = calloc(sizeof(pcb_subnet_dist_t), vtp0_len(&subnets) * vtp0_len(&subnets));
	for(s1 = 0; s1 < vtp0_len(&subnets); s1++) {
		for(s2 = s1+1; s2 < vtp0_len(&subnets); s2++) {
			connmx[s2 * vtp0_len(&subnets) + s1] = connmx[s1 * vtp0_len(&subnets) + s2] = pcb_subnet_dist(pcb, subnets.array[s1], subnets.array[s2], acc);
		}
	}

	/* Start collecting subnets into one bug snowball of newly connected
	   subnets. done[subnet] is 1 if a subnet is already in the snowball.
	   Use a greedy algorithm: mark the first subnet as dine, then always
	   add the shortest from any 'undone' subnet to any 'done' */
	done = calloc(vtp0_len(&subnets), 1);
	done[0] = 1;
	for(left = vtp0_len(&subnets)-1; left > 0; left--) {
		double best_dist = HUGE_VAL;
		int bestu;
		pcb_subnet_dist_t *best = NULL, *curr;

		for(su = 1; su < vtp0_len(&subnets); su++) {
			if (done[su]) continue;
			for(sd = 0; sd < vtp0_len(&subnets); sd++) {
				curr = &connmx[su * vtp0_len(&subnets) + sd];
				if ((done[sd]) && (curr->dist2 < best_dist)) {
					bestu = su;
					best_dist = curr->dist2;
					best = curr;
				}
			}
		}

		if (best == NULL) {
			/* Unlikely: if there are enough restrictions on the search, e.g.
			   PCB_RATACC_ONLY_MANHATTAN for the old autorouter is on and some
			   subnets have only heavy terminals made of non-manhattan-lines,
			   we will not find a connection. When best is NULL, that means
			   no connection found between any undone subnet to any done subnet
			   found, so some subnets will remain disconnected (there is no point
			   in looping more, this won't improve) */
			break;
		}

		/* best connection is 'best' between from 'undone' network bestu; draw the rat */
		line = pcb_rat_new(pcb->Data, -1,
			best->o1x, best->o1y, best->o2x, best->o2y, best->o1g, best->o2g,
			conf_core.appearance.rat_thickness, pcb_no_flags());
		if (line  != NULL) {
			if (best->dist2 == 0)
				PCB_FLAG_SET(PCB_FLAG_VIA, line);
			pcb_undo_add_obj_to_create(PCB_OBJ_RAT, line, line, line);
			pcb_rat_invalidate_draw(line);
			drawn++;
		}
		done[bestu] = 1;
	}

	/* cleanup */
	for(n = 0; n < vtp0_len(&subnets); n++)
		vtp0_uninit(subnets.array[n]);
	free(connmx);
	free(done);
	pcb_find_free(&fctx);
	return drawn;
}


pcb_cardinal_t pcb_net_add_all_rats(const pcb_board_t *pcb, pcb_rat_accuracy_t acc)
{
	htsp_entry_t *e;
	pcb_cardinal_t drawn  =0;

	for(e = htsp_first(&pcb->netlist[PCB_NETLIST_INPUT]); e != NULL; e = htsp_next(&pcb->netlist[PCB_NETLIST_INPUT], e))
		drawn += pcb_net_add_rats(pcb, (pcb_net_t *)e->value, acc);

	return drawn;
}

void pcb_netlist_init(pcb_netlist_t *nl)
{
	htsp_init(nl, strhash, strkeyeq);
}


void pcb_netlist_uninit(pcb_netlist_t *nl)
{
	htsp_entry_t *e;

	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e))
		pcb_net_free(e->value);

	htsp_uninit(nl);
}

