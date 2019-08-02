/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#ifndef PCB_COLOR_CACHE_H
#define PCB_COLOR_CACHE_H

#include <stdlib.h>
#include <genht/htip.h>
#include <genht/hash.h>
#include "global_typedefs.h"
#include "color.h"

typedef void (*pcb_clrcache_free_t)(pcb_clrcache_t *cache, void *hidclr);

struct pcb_clrcache_s {
	htip_t ht;
	int hidsize;
	pcb_clrcache_free_t hidfree;

	void *user_data; /* the caller can set this up after pcb_clrcache_init and use it in hidfree() */
};

PCB_INLINE void pcb_clrcache_init(pcb_clrcache_t *cache, int hidsize, pcb_clrcache_free_t hidfree)
{
	htip_init(&cache->ht, longhash, longkeyeq);
	cache->hidsize = hidsize;
	cache->hidfree = hidfree;
	cache->user_data = NULL;
}

PCB_INLINE void pcb_clrcache_del(pcb_clrcache_t *cache, const pcb_color_t *color)
{
	void *old = htip_get(&cache->ht, color->packed);
	if (old == NULL)
		return;
	if (cache->hidfree != NULL)
		cache->hidfree(cache, old);
	free(old);
}

PCB_INLINE void *pcb_clrcache_get(pcb_clrcache_t *cache, const pcb_color_t *color, int alloc)
{
	void *clr = htip_get(&cache->ht, color->packed);
	if (clr != NULL)
		return clr;

	if (!alloc)
		return NULL;

	clr = calloc(cache->hidsize, 1);
	htip_set(&cache->ht, color->packed, clr);
	return clr;
}

PCB_INLINE void pcb_clrcache_uninit(pcb_clrcache_t *cache)
{
	htip_entry_t *e;

	for(e = htip_first(&cache->ht); e != NULL; e = htip_next(&cache->ht, e)) {
		if (cache->hidfree != NULL)
			cache->hidfree(cache, e->value);
		free(e->value);
	}
	htip_uninit(&cache->ht);
}

#endif
