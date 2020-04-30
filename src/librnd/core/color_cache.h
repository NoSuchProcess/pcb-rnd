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

#ifndef RND_COLOR_CACHE_H
#define RND_COLOR_CACHE_H

#include <stdlib.h>
#include <genht/htip.h>
#include <genht/hash.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/color.h>

typedef void (*rnd_clrcache_free_t)(rnd_clrcache_t *cache, void *hidclr);

struct rnd_clrcache_s {
	htip_t ht;
	int hidsize;
	rnd_clrcache_free_t hidfree;

	void *user_data; /* the caller can set this up after rnd_clrcache_init and use it in hidfree() */
};

RND_INLINE void rnd_clrcache_init(rnd_clrcache_t *cache, int hidsize, rnd_clrcache_free_t hidfree)
{
	htip_init(&cache->ht, longhash, longkeyeq);
	cache->hidsize = hidsize;
	cache->hidfree = hidfree;
	cache->user_data = NULL;
}

RND_INLINE void rnd_clrcache_del(rnd_clrcache_t *cache, const rnd_color_t *color)
{
	void *old = htip_get(&cache->ht, color->packed);
	if (old == NULL)
		return;
	if (cache->hidfree != NULL)
		cache->hidfree(cache, old);
	free(old);
}

RND_INLINE void *rnd_clrcache_get(rnd_clrcache_t *cache, const rnd_color_t *color, int alloc)
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

RND_INLINE void rnd_clrcache_uninit(rnd_clrcache_t *cache)
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
