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
#include <genregex/regex_sei.h>
#include <ctype.h>

#include "board.h"
#include "data.h"
#include "data_it.h"
#include "event.h"
#include "compat_misc.h"
#include "layer_grp.h"
#include "find.h"
#include "obj_term.h"
#include "conf_core.h"
#include "undo.h"
#include "obj_rat_draw.h"
#include "obj_subc_parent.h"
#include "search.h"
#include "remove.h"
#include "draw.h"

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

static pcb_net_term_t *pcb_net_term_alloc(pcb_net_t *net, const char *refdes, const char *term)
{
	pcb_net_term_t *t;

	t = calloc(sizeof(pcb_net_term_t), 1);
	t->type = PCB_OBJ_NET_TERM;
	t->parent_type = PCB_PARENT_NET;
	t->parent.net = net;
	t->refdes = pcb_strdup(refdes);
	t->term = pcb_strdup(term);
	pcb_termlist_append(&net->conns, t);
	return t;
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

	if (alloc)
		return pcb_net_term_alloc(net, refdes, term);
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

static pcb_net_t *pcb_net_alloc(pcb_board_t *pcb, pcb_netlist_t *nl, const char *netname)
{
	pcb_net_t *net;

	net = calloc(sizeof(pcb_net_t), 1);
	net->type = PCB_OBJ_NET;
	net->parent_type = PCB_PARENT_BOARD;
	net->parent.board = pcb;
	net->name = pcb_strdup(netname);
	htsp_set(nl, net->name, net);
	return net;
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

	if (alloc)
		return pcb_net_alloc(pcb, nl, netname);

	return NULL;
}

pcb_net_t *pcb_net_get_icase(pcb_board_t *pcb, pcb_netlist_t *nl, const char *name)
{
	htsp_entry_t *e;

	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_t *net = e->value;
		if (pcb_strcasecmp(name, net->name) == 0)
			break;
	}

	if (e == NULL)
		return NULL;
	return e->value;
}

pcb_net_t *pcb_net_get_regex(pcb_board_t *pcb, pcb_netlist_t *nl, const char *rx)
{
	htsp_entry_t *e;
	re_sei_t *regex;

	regex = re_sei_comp(rx);
	if (re_sei_errno(regex) != 0)
		return NULL;

	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_t *net = e->value;
		if (re_sei_exec(regex, net->name))
			break;
	}
	re_sei_free(regex);

	if (e == NULL)
		return NULL;
	return e->value;
}

pcb_net_t *pcb_net_get_user(pcb_board_t *pcb, pcb_netlist_t *nl, const char *name_or_rx)
{
	pcb_net_t *net = pcb_net_get(pcb, nl, name_or_rx, 0);
	if (net == NULL)
		net = pcb_net_get_icase(pcb, nl, name_or_rx);
	if (net == NULL)
		net = pcb_net_get_regex(pcb, nl, name_or_rx);
	return net;
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

/* crawl from a single terminal; "first" sould be a pointer to an int
   initialized to 0. Returns number of objects found. */
static pcb_cardinal_t pcb_net_term_crawl(const pcb_board_t *pcb, pcb_net_term_t *term, pcb_find_t *fctx, int *first, pcb_cardinal_t *missing)
{
	pcb_any_obj_t *o;

/* there can be multiple terminals with the same ID, but it is enough to run find from the first: find.c will consider them all */
	o = pcb_term_find_name(pcb, pcb->Data, PCB_LYT_COPPER, term->refdes, term->term, NULL, NULL);
	if (o == NULL) {
		if (missing != NULL)
			(*missing)++;
		return 0;
	}

	if ((*first) == 0) {
		*first = 1;
		return pcb_find_from_obj(fctx, PCB->Data, o);
	}

	if (PCB_FIND_IS_MARKED(fctx, o))
		return 0; /* already visited, no need to run 'find' again */

	return pcb_find_from_obj_next(fctx, PCB->Data, o);
}

void pcb_net_short_ctx_init(pcb_short_ctx_t *sctx, const pcb_board_t *pcb, pcb_net_t *net)
{
	sctx->pcb = pcb;
	sctx->current_net = net;
	sctx->changed = 0;
	sctx->missing = 0;
	htsp_init(&sctx->found, strhash, strkeyeq);
}

void pcb_net_short_ctx_uninit(pcb_short_ctx_t *sctx)
{
	htsp_entry_t *e;
	for(e = htsp_first(&sctx->found); e != NULL; e = htsp_next(&sctx->found, e))
		free(e->key);
	htsp_uninit(&sctx->found);
	if (sctx->changed) {
		pcb_gui->invalidate_all();
		conf_core.temp.rat_warn = pcb_true;
	}
}

/* Return 1 if net1-net2 (or net2-net1) is already seen as a short, return 0
   else. Save net1-net2 as seen. */
static int short_ctx_is_dup(pcb_short_ctx_t *sctx, pcb_net_t *net1, pcb_net_t *net2)
{
	char *key;
	int order;

	order = strcmp(net1->name, net2->name);

	if (order == 0) {
		pcb_message(PCB_MSG_ERROR, "netlist internal error: short_ctx_is_dup() net %s shorted with itself?!\n", net1->name);
		return 1;
	}
	if (order > 0)
		key = pcb_concat(net1->name, "-", net2->name, NULL);
	else
		key = pcb_concat(net2->name, "-", net1->name, NULL);

	if (htsp_has(&sctx->found, key)) {
		free(key);
		return 1;
	}

	htsp_set(&sctx->found, key, net1);
	return 0;
}

/* Short circuit found between net and an offender object that should not
   be part of the net but is connected to the net */
static void net_found_short(pcb_short_ctx_t *sctx, pcb_any_obj_t *offender)
{
	pcb_subc_t *sc = pcb_obj_parent_subc(offender);
	int handled = 0;

	pcb_net_term_t *offt = pcb_net_find_by_refdes_term(&sctx->pcb->netlist[PCB_NETLIST_EDITED], sc->refdes, offender->term);
	pcb_net_t *offn;
	const char *offnn = "<unknown>";
	
	if (offt != NULL) {
		offn = offt->parent.net;
		offnn = offn->name;
		if (short_ctx_is_dup(sctx, sctx->current_net, offn))
			return;
	}

	if (offnn != NULL)
		pcb_message(PCB_MSG_WARNING, "SHORT: net \"%s\" is shorted to \"%s\" at terminal %s-%s\n", sctx->current_net->name, offnn, sc->refdes, offender->term);
	else
		pcb_message(PCB_MSG_WARNING, "SHORT: net \"%s\" is shorted to terminal %s-%s\n", sctx->current_net->name, sc->refdes, offender->term);

	pcb_event(PCB_EVENT_NET_INDICATE_SHORT, "pppp", sctx->current_net, offender, offn, &handled);
	if (!handled) {
		pcb_net_term_t *orig_t = pcb_termlist_first(&sctx->current_net->conns);
		pcb_any_obj_t *orig_o = pcb_term_find_name(sctx->pcb, sctx->pcb->Data, PCB_LYT_COPPER, orig_t->refdes, orig_t->term, NULL, NULL);

		/* dummy fallback: warning-highlight the two terminals */
		PCB_FLAG_SET(PCB_FLAG_WARN, offender);
		if (orig_o != NULL)
			PCB_FLAG_SET(PCB_FLAG_WARN, orig_o);
	}
	sctx->changed++;
}

static int net_short_check(pcb_find_t *fctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype)
{
	if (new_obj->term != NULL) {
		pcb_net_term_t *t;
		pcb_short_ctx_t *sctx = fctx->user_data;
		pcb_subc_t *sc = pcb_obj_parent_subc(new_obj);

		if ((sc == NULL) || (sc->refdes == NULL))
			return 0;

		/* if new_obj is a terminal on our net, return */
		for(t = pcb_termlist_first(&sctx->current_net->conns); t != NULL; t = pcb_termlist_next(t))
			if ((strcmp(t->refdes, sc->refdes) == 0) && (strcmp(t->term, new_obj->term) == 0))
				return 0;

		/* new_obj is not on our net but has a refdes-term -> must be a short */
		net_found_short(fctx->user_data, new_obj);
	}

	return 0;
}

pcb_cardinal_t pcb_net_crawl_flag(pcb_board_t *pcb, pcb_net_t *net, unsigned long setf, unsigned long clrf)
{
	pcb_find_t fctx;
	pcb_net_term_t *t;
	pcb_cardinal_t res = 0, n;
	pcb_short_ctx_t sctx;
	int first = 0;

	pcb_net_short_ctx_init(&sctx, pcb, net);

	memset(&fctx, 0, sizeof(fctx));
	fctx.flag_set = setf;
	fctx.flag_clr = clrf;
	fctx.flag_chg_undoable = 1;
	fctx.only_mark_rats = 1; /* do not trust rats, but do mark them */
	fctx.user_data = &sctx;
	fctx.found_cb = net_short_check;

	for(t = pcb_termlist_first(&net->conns), n = 0; t != NULL; t = pcb_termlist_next(t), n++) {
		res += pcb_net_term_crawl(pcb, t, &fctx, &first, NULL);
	}

	pcb_find_free(&fctx);
	pcb_net_short_ctx_uninit(&sctx);
	return res;
}

pcb_net_term_t *pcb_net_find_by_refdes_term(const pcb_netlist_t *nl, const char *refdes, const char *term)
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

pcb_net_term_t *pcb_net_find_by_pinname(const pcb_netlist_t *nl, const char *pinname)
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

pcb_net_term_t *pcb_net_find_by_obj(const pcb_netlist_t *nl, const pcb_any_obj_t *obj)
{
	const pcb_subc_t *sc;

	if (obj->term == NULL)
		return NULL;

	sc = pcb_obj_parent_subc(obj);
	if (sc == NULL)
		return NULL;

	return pcb_net_find_by_refdes_term(nl, sc->refdes, obj->term);
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
	arr = malloc((nl->used+1) * sizeof(pcb_net_t));

	for(e = htsp_first(nl), n = 0; e != NULL; e = htsp_next(nl, e), n++)
		arr[n] = e->value;
	qsort(arr, nl->used, sizeof(pcb_net_t *), netname_sort);
	arr[nl->used] = NULL;
	return arr;
}

#include "netlist_geo.c"

pcb_cardinal_t pcb_net_map_subnets(pcb_short_ctx_t *sctx, pcb_rat_accuracy_t acc, vtp0_t *subnets)
{
	pcb_find_t fctx;
	pcb_net_term_t *t;
	pcb_cardinal_t drawn = 0, r, n, s1, s2, su, sd;
	pcb_subnet_dist_t *connmx;
	char *done;
	int left, first = 0;
	pcb_rat_t *line;


	memset(&fctx, 0, sizeof(fctx));
	fctx.consider_rats = 1; /* keep existing rats and their connections */
	fctx.list_found = 1;
	fctx.user_data = sctx;
	fctx.found_cb = net_short_check;

	/* each component of a desired network is called a subnet; already connected
	   objects of each subnet is collected on a vtp0_t; object-lists per submnet
	   is saved in variable "subnets" */
	for(t = pcb_termlist_first(&sctx->current_net->conns), n = 0; t != NULL; t = pcb_termlist_next(t), n++) {
		r = pcb_net_term_crawl(sctx->pcb, t, &fctx, &first, &sctx->missing);
		if (r > 0) {
			vtp0_t *objs = malloc(sizeof(vtp0_t));
			memcpy(objs, &fctx.found, sizeof(vtp0_t));
			vtp0_append(subnets, objs);
			memset(&fctx.found, 0, sizeof(vtp0_t));
		}
	}

	/* find the shortest connection between any two subnets and save the info
	   in connmx */
	connmx = calloc(sizeof(pcb_subnet_dist_t), vtp0_len(subnets) * vtp0_len(subnets));
	for(s1 = 0; s1 < vtp0_len(subnets); s1++) {
		for(s2 = s1+1; s2 < vtp0_len(subnets); s2++) {
			connmx[s2 * vtp0_len(subnets) + s1] = connmx[s1 * vtp0_len(subnets) + s2] = pcb_subnet_dist(sctx->pcb, subnets->array[s1], subnets->array[s2], acc);
		}
	}

	/* Start collecting subnets into one bug snowball of newly connected
	   subnets. done[subnet] is 1 if a subnet is already in the snowball.
	   Use a greedy algorithm: mark the first subnet as dine, then always
	   add the shortest from any 'undone' subnet to any 'done' */
	done = calloc(vtp0_len(subnets), 1);
	done[0] = 1;
	for(left = vtp0_len(subnets)-1; left > 0; left--) {
		double best_dist = HUGE_VAL;
		int bestu;
		pcb_subnet_dist_t *best = NULL, *curr;

		for(su = 1; su < vtp0_len(subnets); su++) {
			if (done[su]) continue;
			for(sd = 0; sd < vtp0_len(subnets); sd++) {
				curr = &connmx[su * vtp0_len(subnets) + sd];
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
			sctx->missing++;
			break;
		}

		/* best connection is 'best' between from 'undone' network bestu; draw the rat */
		line = pcb_rat_new(sctx->pcb->Data, -1,
			best->o1x, best->o1y, best->o2x, best->o2y, best->o1g, best->o2g,
			conf_core.appearance.rat_thickness, pcb_no_flags(),
			best->o1, best->o2);
		if (line != NULL) {
			if (best->dist2 == 0)
				PCB_FLAG_SET(PCB_FLAG_VIA, line);
			pcb_undo_add_obj_to_create(PCB_OBJ_RAT, line, line, line);
			pcb_rat_invalidate_draw(line);
			drawn++;
		}
		else
			sctx->missing++;
		done[bestu] = 1;
	}

	/* cleanup */
	free(connmx);
	free(done);
	pcb_find_free(&fctx);
	return drawn;
}

void pcb_net_reset_subnets(vtp0_t *subnets)
{
	pcb_cardinal_t n;
	for(n = 0; n < vtp0_len(subnets); n++)
		vtp0_uninit(subnets->array[n]);
	subnets->used = 0;
}

void pcb_net_free_subnets(vtp0_t *subnets)
{
	pcb_net_reset_subnets(subnets);
	vtp0_uninit(subnets);
}


pcb_cardinal_t pcb_net_add_rats(const pcb_board_t *pcb, pcb_net_t *net, pcb_rat_accuracy_t acc)
{
	pcb_cardinal_t res;
	pcb_short_ctx_t sctx;
	vtp0_t subnets;

	vtp0_init(&subnets);
	pcb_net_short_ctx_init(&sctx, pcb, net);
	res = pcb_net_map_subnets(&sctx, acc, &subnets);
	pcb_net_short_ctx_uninit(&sctx);
	pcb_net_free_subnets(&subnets);
	return res;
}


pcb_cardinal_t pcb_net_add_all_rats(const pcb_board_t *pcb, pcb_rat_accuracy_t acc)
{
	htsp_entry_t *e;
	pcb_cardinal_t drawn = 0;
	pcb_short_ctx_t sctx;
	vtp0_t subnets;

	vtp0_init(&subnets);

	pcb_net_short_ctx_init(&sctx, pcb, NULL);

	for(e = htsp_first(&pcb->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&pcb->netlist[PCB_NETLIST_EDITED], e)) {
		pcb_net_t *net = e->value;
		if (acc & PCB_RATACC_ONLY_SELECTED) {
			pcb_net_term_t *t;
			int has_selection = 0;
			for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
				pcb_any_obj_t *o = pcb_term_find_name(pcb, pcb->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
				if ((o != NULL) && (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))) {
					has_selection = 1;
					break;
				}
			}
			if (!has_selection)
				continue;
		}

		sctx.current_net = net;
		if (sctx.current_net->inhibit_rats)
			continue;
		drawn += pcb_net_map_subnets(&sctx, acc, &subnets);
		pcb_net_reset_subnets(&subnets);
	}

	if (acc & PCB_RATACC_INFO) {
		long rem = ratlist_length(&pcb->Data->Rat);
		if (rem > 0)
			pcb_message(PCB_MSG_INFO, "%d rat line%s remaining\n", rem, rem > 1 ? "s" : "");
		else if (acc & PCB_RATACC_ONLY_SELECTED)
			pcb_message(PCB_MSG_WARNING, "No rat for any network that has selected terminal\n");
		else if (sctx.missing > 0)
			pcb_message(PCB_MSG_WARNING, "Nothing more to add, but there are\neither rat-lines in the layout, disabled nets\nin the net-list, or missing components\n");
		else
			pcb_message(PCB_MSG_INFO, "Congratulations!!\n" "The layout is complete and has no shorted nets.\n");
	}

	vtp0_uninit(&subnets);
	pcb_net_short_ctx_uninit(&sctx);
	return drawn;
}

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

void pcb_netlist_copy(pcb_board_t *pcb, pcb_netlist_t *dst, pcb_netlist_t *src)
{
	htsp_entry_t *e;

	assert(dst->used == 0);
	for(e = htsp_first(src); e != NULL; e = htsp_next(src, e)) {
		pcb_net_t *src_net, *dst_net;
		pcb_net_term_t *src_term, *dst_term;

		src_net = e->value;
		dst_net = pcb_net_alloc(pcb, dst, src_net->name);
		dst_net->export_tmp = src_net->export_tmp;
		dst_net->inhibit_rats = src_net->inhibit_rats;
		pcb_attribute_copy_all(&dst_net->Attributes, &src_net->Attributes);

		for(src_term = pcb_termlist_first(&src_net->conns); src_term != NULL; src_term = pcb_termlist_next(src_term)) {
			dst_term = pcb_net_term_alloc(dst_net, src_term->refdes, src_term->term);
			pcb_attribute_copy_all(&dst_term->Attributes, &src_term->Attributes);
		}
	}
}

/* Return the (most natural) copper group the obj is in */
static pcb_layergrp_id_t get_side_group(pcb_board_t *pcb, pcb_any_obj_t *obj)
{
	switch(obj->type) {
		case PCB_OBJ_ARC:
		case PCB_OBJ_LINE:
		case PCB_OBJ_POLY:
		case PCB_OBJ_TEXT:
			return pcb_layer_get_group_(obj->parent.layer);
		case PCB_OBJ_PSTK:
			if (pcb_pstk_shape((pcb_pstk_t *)obj, PCB_LYT_COPPER | PCB_LYT_TOP, 0) != NULL)
				return pcb_layergrp_get_top_copper();
			if (pcb_pstk_shape((pcb_pstk_t *)obj, PCB_LYT_COPPER | PCB_LYT_BOTTOM, 0) != NULL)
				return pcb_layergrp_get_bottom_copper();
		default: return -1;
	}
}

static pcb_rat_t *pcb_net_create_by_rat_(pcb_board_t *pcb, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_any_obj_t *o1, pcb_any_obj_t *o2, pcb_bool interactive)
{
	pcb_subc_t *sc1, *sc2, *sctmp;
	pcb_net_t *net1 = NULL, *net2 = NULL, *ntmp, *target_net = NULL;
	pcb_layergrp_id_t group1, group2;
	pcb_rat_t *res;
	static long netname_cnt = 0;
	char ratname_[32], *ratname, *id;
	long old_len, new_len;

	if ((o1 == o2) || (o1 == NULL) || (o2 == NULL)) {
		pcb_message(PCB_MSG_ERROR, "Missing start or end terminal\n");
		return NULL;
	}

	{ /* make sure at least one of the terminals is not on a net */
		pcb_net_term_t *term1, *term2;

		sc1 = pcb_obj_parent_subc(o1);
		sc2 = pcb_obj_parent_subc(o2);
		if ((sc1 == NULL) || (sc2 == NULL) || (sc1->refdes == NULL) || (sc2->refdes == NULL)) {
			pcb_message(PCB_MSG_ERROR, "Both start or end terminal must be in a subcircuit with refdes\n");
			return NULL;
		}

		term1 = pcb_net_find_by_refdes_term(&pcb->netlist[PCB_NETLIST_EDITED], sc1->refdes, o1->term);
		term2 = pcb_net_find_by_refdes_term(&pcb->netlist[PCB_NETLIST_EDITED], sc2->refdes, o2->term);
		if (term1 != NULL) net1 = term1->parent.net;
		if (term2 != NULL) net1 = term2->parent.net;

		if ((net1 == net2) && (net1 != NULL)) {
			pcb_message(PCB_MSG_ERROR, "Those terminals are already on the same net (%s)\n", net1->name);
			return NULL;
		}
		if ((net1 != NULL) && (net2 != NULL)) {
			pcb_message(PCB_MSG_ERROR, "Can not connect two existing nets with a rat (%s and %s)\n", net1->name, net2->name);
			return NULL;
		}
	}

	/* swap vars so it's always o1 is off-net (and o2 may be in a net) */
	if (net1 != NULL) {
		pcb_any_obj_t *otmp;
		otmp = o1; o1 = o2; o2 = otmp;
		sctmp = sc1; sc1 = sc2; sc2 = sctmp;
		ntmp = net1; net1 = net2; net2 = ntmp;
	}

	group1 = get_side_group(pcb, o1);
	group2 = get_side_group(pcb, o2);
	if ((group1 == -1) && (group2 == -1)) {
		pcb_message(PCB_MSG_ERROR, "Can not determine copper layer group of that terminal\n");
		return NULL;
	}

	/* passed all sanity checks, o1 is off-net; figure the target_net (create it if needed) */
	if ((net1 == NULL) && (net2 == NULL)) {
		do {
			sprintf(ratname_, "pcbrnd%ld", ++netname_cnt);
		} while(htsp_has(&pcb->netlist[PCB_NETLIST_EDITED], ratname_));
		if (interactive) {
			ratname = pcb_hid_prompt_for("Name of the new net", ratname_, "rat net name");
			if (ratname == NULL) /* cancel */
				return NULL;
		}
		else
			ratname = ratname_;
		target_net = pcb_net_get(pcb, &pcb->netlist[PCB_NETLIST_EDITED], ratname, 1);

		assert(target_net != NULL);
		if (ratname != ratname_)
			free(ratname);
	}
	else
		target_net = net2;

	/* create the rat and add terminals in the target_net */
	res = pcb_rat_new(pcb->Data, -1, x1, y1, x2, y2, group1, group2, conf_core.appearance.rat_thickness, pcb_no_flags(), o1, o2);

	old_len = pcb_termlist_length(&target_net->conns);
	pcb_net_term_get(target_net, sc1->refdes, o1->term, 1);
	new_len = pcb_termlist_length(&target_net->conns);
	if (new_len != old_len) {
		id = pcb_concat(sc1->refdes, "-", o1->term, NULL);
		pcb_ratspatch_append(pcb, RATP_ADD_CONN, id, target_net->name, NULL);
		free(id);
	}

	old_len = new_len;
	pcb_net_term_get(target_net, sc2->refdes, o2->term, 1);
	new_len = pcb_termlist_length(&target_net->conns);
	if (new_len != old_len) {
		id = pcb_concat(sc2->refdes, "-", o2->term, NULL);
		pcb_ratspatch_append(pcb, RATP_ADD_CONN, id, target_net->name, NULL);
		free(id);
	}

	pcb_netlist_changed(0);
	return res;
}

static pcb_any_obj_t *find_rat_end(pcb_board_t *pcb, pcb_coord_t x, pcb_coord_t y, const char *loc)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_any_obj_t *o;
	pcb_objtype_t type = pcb_search_obj_by_location(PCB_OBJ_CLASS_TERM | PCB_OBJ_SUBC_PART, &ptr1, &ptr2, &ptr3, x, y, 5);
	pcb_subc_t *sc;

	o = ptr2;
	if ((type == PCB_OBJ_VOID) || (o->term == NULL)) {
		pcb_message(PCB_MSG_ERROR, "Can't find a terminal at %s\n", loc);
		return NULL;
	}

	sc = pcb_obj_parent_subc(o);
	if ((sc == NULL) || (sc->refdes == NULL)) {
		pcb_message(PCB_MSG_ERROR, "The terminal terminal found at %s is not part of a subc with refdes\n", loc);
		return NULL;
	}

	return o;
}

pcb_rat_t *pcb_net_create_by_rat_coords(pcb_board_t *pcb, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_bool interactive)
{
	pcb_any_obj_t *os, *oe;
	if ((x1 == x2) && (y1 == y2))
		return NULL;

	os = find_rat_end(pcb, x1, y1, "rat line start");
	oe = find_rat_end(pcb, x2, y2, "rat line end");

	return pcb_net_create_by_rat_(pcb, x1, y1, x2, y2, os, oe, interactive);
}


pcb_cardinal_t pcb_net_ripup(pcb_board_t *pcb, pcb_net_t *net)
{
	pcb_find_t fctx;
	pcb_net_term_t *t;
	pcb_cardinal_t res, n;
	pcb_any_obj_t *o, *lasto;
	pcb_data_it_t it;
	int first = 0;

	memset(&fctx, 0, sizeof(fctx));
	fctx.only_mark_rats = 1; /* do not trust rats, but do mark them */

	for(t = pcb_termlist_first(&net->conns), n = 0; t != NULL; t = pcb_termlist_next(t), n++)
		pcb_net_term_crawl(pcb, t, &fctx, &first, NULL);

	pcb_undo_save_serial();
	pcb_draw_inhibit_inc();

	/* always remove the (n-1)th object; removing the current iterator object
	   confuses the iteration */
	res = 0;
	lasto = NULL;
	o = pcb_data_first(&it, pcb->Data, PCB_OBJ_CLASS_REAL & (~PCB_OBJ_SUBC));
	for(;;) {
		if ((lasto != NULL) && (PCB_DFLAG_TEST(&lasto->Flags, fctx.mark))) {
			pcb_remove_object(lasto->type, lasto->parent.any, lasto, lasto);
			res++;
		}
		lasto = o;
		if (lasto == NULL)
			break;
		o = pcb_data_next(&it);
	}

	pcb_undo_restore_serial();
	if (res > 0)
		pcb_undo_inc_serial();

	pcb_draw_inhibit_dec();
	pcb_find_free(&fctx);
	return res;
}
