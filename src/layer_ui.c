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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Virtual layers for UI and debug */
#include "config.h"
#include "layer.h"
#include "event.h"
#include "compat_misc.h"
#include "genvector/vtp0.h"
#define GVT_DONT_UNDEF
#include "layer_ui.h"
#include <genvector/genvector_impl.c>

vtlayer_t pcb_uilayer;
vtp0_t pcb_uilayers;


pcb_layer_t *pcb_uilayer_alloc(const char *cookie, const char *name, const char *color)
{
	int n;
	pcb_layer_t *l;
	void **p;

	if (cookie == NULL)
		return NULL;

	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		l = pcb_uilayers.array[n];
		if (l == NULL) {
			l = vtlayer_alloc_append(&pcb_uilayer, 1);
			pcb_uilayers.array[n] = l;
			goto found;
		}
	}

	l = vtlayer_alloc_append(&pcb_uilayer, 1);
	p = vtp0_alloc_append(&pcb_uilayers, 1);
	*p = l;
found:;
	l->meta.real.cookie = cookie;
	l->meta.real.color = pcb_strdup(color);
	l->name = pcb_strdup(name);
	l->meta.real.vis = 1;
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
	return l;
}

static void pcb_uilayer_free_(pcb_layer_t *l)
{
	pcb_layer_free(l);
}

void pcb_uilayer_free(pcb_layer_t *ly)
{
	int n;
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		pcb_layer_t *l = pcb_uilayers.array[n];
		if (l == ly) {
			pcb_uilayer_free_(l);
			break;
		}
	}
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
}

void pcb_uilayer_free_all_cookie(const char *cookie)
{
	int n;
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		pcb_layer_t *l = pcb_uilayers.array[n];
		if (l->meta.real.cookie == cookie)
			pcb_uilayer_free_(l);
	}
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
}

void pcb_uilayer_uninit(void)
{
	vtlayer_uninit(&pcb_uilayer);
	vtp0_uninit(&pcb_uilayers);
}
