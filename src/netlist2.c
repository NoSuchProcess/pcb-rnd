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
#include "compat_misc.h"

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
		t->refdes = pcb_strdup(refdes);
		t->term = pcb_strdup(term);
		pcb_termlist_append(&net->conns, t);
		return t;
	}

	return NULL;
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
			case '.':
				break;
			return pcb_false;
		}
	}
	return pcb_true;
}

pcb_net_t *pcb_net_get(pcb_netlist_t *nl, const char *netname, pcb_bool alloc)
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
		net = calloc(sizeof(pcb_net_t *), 1);
		net->name = pcb_strdup(netname);
		net->type = PCB_OBJ_NET;
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

