/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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

#ifndef RND_RND_PLUGINS_H
#define RND_RND_PLUGINS_H

#include <puplug/puplug.h>
#include <puplug/error.h>

/* core's version stored in plugins.o */
extern unsigned long rnd_api_ver;

#define RND_API_VER_MATCH (PCB_API_VER == rnd_api_ver)
#define RND_API_CHK_VER \
do { \
	if (!RND_API_VER_MATCH) {\
		fprintf(stderr, "pcb-rnd API version incompatibility: " __FILE__ "=%lu core=%lu\n(not loading this plugin)\n", (unsigned long)PCB_API_VER, rnd_api_ver); \
		return 1; \
	} \
} while(0)

extern pup_context_t rnd_pup;
extern char **rnd_pup_paths;

void rnd_plugin_add_dir(const char *dir);
void rnd_plugin_uninit(void);

/* Hook based plugin generics; plugins that implement a common API should use
   HOOK_REGISTER with an api struct. The core should run the plugins using
   HOOK_CALL */

#define RND_HOOK_CALL_DO(chain_type, chain, func, res, accept, funcargs, do_on_success) \
do { \
	chain_type *self; \
	for(self = (chain); self != NULL; self = self->next) { \
		if (self->func == NULL) \
			continue; \
		res = self->func funcargs; \
		if (res accept) {\
			do_on_success; \
			break; \
		} \
	} \
} while(0)

#define RND_HOOK_CALL(chain_type, chain, func, res, accept, funcargs) \
	RND_HOOK_CALL_DO(chain_type, chain, func, res, accept, funcargs, (void)0)

#define RND_HOOK_CALL_ALL(chain_type, chain, func, cb, funcargs) \
do { \
	chain_type *self; \
	for(self = (chain); self != NULL; self = self->next) { \
		int res; \
		if (self->func == NULL) \
			continue; \
		res = self->func funcargs; \
		cb(self, res); \
	} \
} while(0)

#define RND_HOOK_REGISTER(chain_type, chain, hstruct) \
do { \
	(hstruct)->next = chain; \
	chain = (hstruct); \
} while(0)


#define RND_HOOK_UNREGISTER(chain_type, chain, hstruct) \
do { \
	chain_type *__n__, *__prev__ = NULL, *__h__ = (hstruct); \
	while (chain == __h__) \
		chain = chain->next; \
	for(__n__ = chain; __n__ != NULL; __n__ = __n__->next) { \
		if ((__n__ == __h__) && (__prev__ != NULL)) \
			__prev__->next = __n__->next; \
		__prev__ = __n__; \
	} \
} while(0)

#endif
