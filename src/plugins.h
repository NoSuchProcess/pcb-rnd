/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#ifndef PCB_RND_PLUGINS_H
#define PCB_RND_PLUGINS_H

#include <puplug/puplug.h>
#include <puplug/error.h>

extern pup_context_t pcb_pup;

/* Hook based plugin generics; plugins that implement a common API should use
   HOOK_REGISTER with an api struct. The core should run the plugins using
   HOOK_CALL */

#define PCB_HOOK_CALL_DO(chain_type, chain, func, res, accept, funcargs, do_on_success) \
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

#define PCB_HOOK_CALL(chain_type, chain, func, res, accept, funcargs) \
	PCB_HOOK_CALL_DO(chain_type, chain, func, res, accept, funcargs, (void)0)

#define PCB_HOOK_CALL_ALL(chain_type, chain, func, cb, funcargs) \
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

#define PCB_HOOK_REGISTER(chain_type, chain, hstruct) \
do { \
	(hstruct)->next = chain; \
	chain = (hstruct); \
} while(0)


#define PCB_HOOK_UNREGISTER(chain_type, chain, hstruct) \
do { \
	chain_type *__n__, *__prev__ = NULL, *__h__ = (hstruct); \
	for(__n__ = chain; __n__ != NULL; __n__ = __n__->next) { \
		if ((__n__ == __h__) && (__prev__ != NULL)) \
			__prev__->next = __n__->next; \
		__prev__ = __n__; \
	} \
	if (chain == __n__) \
		chain = chain->next; \
} while(0)

#endif
