/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, layer_ui.c, was written and is Copyright (C) 2016 by
 *  Tibor 'Igor2' Palinkas.
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

/* Virtual layers for UI and debug */
#include "config.h"
#define GVT_DONT_UNDEF
#include "layer_ui.h"
#include "event.h"
#include <genvector/genvector_impl.c>

vtlayer_t pcb_uilayer;

pcb_layer_t *pcb_uilayer_alloc(const char *cookie, const char *name, const char *color)
{
	int n;
	pcb_layer_t *l;

	if (cookie == NULL)
		return NULL;

	for(n = 0; n < vtlayer_len(&pcb_uilayer); n++) {
		l = &pcb_uilayer.array[n];
		if (l->cookie == NULL) {
			l->cookie = cookie;
			goto found;
		}
	}

	l = vtlayer_alloc_append(&pcb_uilayer, 1);
found:;
	l->cookie = cookie;
	l->Color = color;
	l->Name = name;
	l->On = 1;
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return l;
}

void pcb_uilayer_free_all_cookie(const char *cookie)
{
	int n;
	for(n = 0; n < vtlayer_len(&pcb_uilayer); n++) {
		pcb_layer_t *l = &pcb_uilayer.array[n];
		if (l->cookie == cookie) {
#warning TODO: free all objects
			l->cookie = NULL;
			l->Color = l->Name = NULL;
			l->On = 0;
			pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
		}
	}
}

