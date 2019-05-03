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

#include "board.h"
#include "conf_core.h"
#include "funchash_core.h"
#include "layer.h"

static int (*gui_set_layer_group)(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform);

static int common_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	int idx = group;
	if (idx >= 0 && idx < pcb_max_group(PCB)) {
		int n = PCB->LayerGroups.grp[group].len;
		for (idx = 0; idx < n - 1; idx++) {
			int ni = PCB->LayerGroups.grp[group].lid[idx];
			if (ni >= 0 && ni < pcb_max_layer && PCB->Data->Layer[ni].meta.real.vis)
				break;
		}
		idx = PCB->LayerGroups.grp[group].lid[idx];
	}

	/* non-virtual layers with group visibility */
	switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_MASK:
		case PCB_LYT_PASTE:
			return (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags) && PCB->LayerGroups.grp[group].vis);
	}

	if (idx >= 0 && idx < pcb_max_layer && ((flags & PCB_LYT_ANYTHING) != PCB_LYT_SILK))
		return PCB->Data->Layer[idx].meta.real.vis;

	/* virtual layers */
	{
		if (PCB_LAYER_IS_DRILL(flags, purpi))
			return 1;

		switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_INVIS:
			return PCB->InvisibleObjectsOn;
		case PCB_LYT_SILK:
			if (PCB_LAYERFLG_ON_VISIBLE_SIDE(flags))
				return pcb_silk_on(PCB);
			return 0;
		case PCB_LYT_UI:
			return 1;
		case PCB_LYT_RAT:
			return PCB->RatOn;
		}
	}
	return 0;
}

static int pcbui_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty, pcb_xform_t **xform)
{
	int res;

	res = gui_set_layer_group(group, purpose, purpi, layer, flags, is_empty, xform);

	/* if the HID doesn't want it, don't even bother running the above heuristics */
	if (res == 0)
		return 0;

	return common_set_layer_group(group, purpose, purpi, layer, flags, is_empty, xform);
}

static void pcb_rendering_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* hook in our dispatcher */
	gui_set_layer_group = pcb_gui->set_layer_group;
	pcb_gui->set_layer_group = pcbui_set_layer_group;
}

