/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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

typedef struct plugin_info_s plugin_info_t;

typedef void (*pcb_uninit_t)(void);

struct plugin_info_s {
	char *name;
	char *path;
	void *handle;
	int dynamic_loaded;
	pcb_uninit_t uninit;
	plugin_info_t *next;
};

extern plugin_info_t *plugins;

/* Init the plugin system */
void plugins_init(void);

/* Uninit each plugin then uninit the plugin system */
void plugins_uninit(void);

/* Register a new plugin (or buildin) */
void plugin_register(const char *name, const char *path, void *handle, int dynamic, pcb_uninit_t uninit);


/* Hook based plugin generics; plugins that implement a common API should use
   HOOK_REGISTER with an api struct. The core should run the plugins using
   HOOK_CALL */

#define HOOK_CALL(chain_type, chain, func, res, accept, ...) \
do { \
	chain_type *__ch__; \
	for(__ch__ = (chain); __ch__ != NULL; __ch__ = __ch__->next) { \
		if (__ch__->func == NULL) \
			continue; \
		res = __ch__->func(__ch__, __VA_ARGS__); \
		if (res accept) \
			break; \
	} \
} while(0)

#define HOOK_REGISTER(chain_type, chain, hstruct) \
do { \
	(hstruct)->next = chain; \
	chain = (hstruct); \
} while(0)

