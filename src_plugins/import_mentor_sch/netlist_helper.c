/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  netlist build helper
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <genht/hash.h>

#include "netlist_helper.h"
#include "compat_misc.h"


nethlp_ctx_t *nethlp_new(nethlp_ctx_t *prealloc)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;
	htsp_init(&prealloc->id2refdes, strhash, strkeyeq);
	return prealloc;
}

void nethlp_destroy(nethlp_ctx_t *nhctx)
{
	htsp_entry_t *e;
	for (e = htsp_first(&nhctx->id2refdes); e; e = htsp_next(&nhctx->id2refdes, e)) {
		free(e->key);
		free(e->value);
	}
	htsp_uninit(&nhctx->id2refdes);
	if (nhctx->alloced)
		free(nhctx);
}


nethlp_elem_ctx_t *nethlp_elem_new(nethlp_ctx_t *nhctx, nethlp_elem_ctx_t *prealloc, const char *id)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_elem_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;
	prealloc->nhctx = nhctx;
	prealloc->id = pcb_strdup(id);
	htsp_init(&prealloc->attr, strhash, strkeyeq);
	return prealloc;
}

void nethlp_elem_refdes(nethlp_elem_ctx_t *ectx, const char *refdes)
{
	htsp_set(&ectx->nhctx->id2refdes, pcb_strdup(ectx->id), pcb_strdup(refdes));
}

void nethlp_elem_attr(nethlp_elem_ctx_t *ectx, const char *key, const char *val)
{
	htsp_set(&ectx->attr, pcb_strdup(key), pcb_strdup(val));
}

void nethlp_elem_done(nethlp_elem_ctx_t *ectx)
{
	htsp_entry_t *e;

	printf("Elem '%s' -> '%s'\n", ectx->id, (char *)htsp_get(&ectx->nhctx->id2refdes, ectx->id));

	for (e = htsp_first(&ectx->attr); e; e = htsp_next(&ectx->attr, e)) {
		free(e->key);
		free(e->value);
	}
	htsp_uninit(&ectx->attr);
	free(ectx->id);
	if (ectx->alloced)
		free(ectx);
}


nethlp_net_ctx_t *nethlp_net_new(nethlp_ctx_t *nhctx, nethlp_net_ctx_t *prealloc, const char *netname)
{
	if (prealloc == NULL) {
		prealloc = malloc(sizeof(nethlp_net_ctx_t));
		prealloc->alloced = 1;
	}
	else
		prealloc->alloced = 0;
	prealloc->nhctx = nhctx;

	prealloc->netname = pcb_strdup(netname);
	return prealloc;
}

void nethlp_net_add_term(nethlp_net_ctx_t *nctx, const char *part, const char *pin)
{
	char *refdes = htsp_get(&nctx->nhctx->id2refdes, part);
	if (refdes == NULL)
		refdes = "n/a";
	printf("net: %s %s (%s) %s\n", nctx->netname, refdes, part, pin);
}

void nethlp_net_destroy(nethlp_net_ctx_t *nctx)
{
	free(nctx->netname);
	if (nctx->alloced)
		free(nctx);
}

