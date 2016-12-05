/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid_helper.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "compat_misc.h"

#warning TODO: layer support: kill this
const char *pcb_layer_type_to_file_name(int idx, int style)
{
	pcb_layergrp_id_t group;
	int nlayers;
	const char *single_name;
	unsigned int flags = pcb_layer_flags(idx);

	switch (idx) {
	case SL(SILK, TOP):
		return "topsilk";
	case SL(SILK, BOTTOM):
		return "bottomsilk";
	case SL(MASK, TOP):
		return "topmask";
	case SL(MASK, BOTTOM):
		return "bottommask";
	case SL(PDRILL, 0):
		return "plated-drill";
	case SL(UDRILL, 0):
		return "unplated-drill";
	case SL(PASTE, TOP):
		return "toppaste";
	case SL(PASTE, BOTTOM):
		return "bottompaste";
	case SL(INVISIBLE, 0):
		return "invisible";
	case SL(FAB, 0):
		return "fab";
	case SL(ASSY, TOP):
		return "topassembly";
	case SL(ASSY, BOTTOM):
		return "bottomassembly";
	default:
		group = pcb_layer_get_group(idx);
		nlayers = PCB->LayerGroups.Number[group];
		single_name = PCB->Data->Layer[idx].Name;
		if (flags & PCB_LYT_TOP) {
			if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
				return single_name;
			return "top";
		}
		else if (flags & PCB_LYT_BOTTOM) {
			if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
				return single_name;
			return "bottom";
		}
		else if (flags & PCB_LYT_OUTLINE) {
			return "outline";
		}
		else {
			static char buf[20];
			if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 1))
				return single_name;
			sprintf(buf, "group%ld", group);
			return buf;
		}
		break;
	}
}

char *pcb_layer_to_file_name(char *dest, pcb_layer_id_t lid, unsigned int flags, pcb_file_name_style_t style)
{
	pcb_virt_layer_t *v;
	pcb_layergrp_id_t group;
	int nlayers;
	const char *single_name, *res = NULL;

	if (flags == 0)
		flags = pcb_layer_flags(lid);

	if (flags & PCB_LYT_OUTLINE) {
		strcpy(dest, "outline");
		return dest;
	}

	v = pcb_vlayer_get_first(flags);
	if (v != NULL) {
		strcpy(dest, v->name);
		return dest;
	}

	
	group = pcb_layer_get_group(lid);
	nlayers = PCB->LayerGroups.Number[group];
	single_name = pcb_layer_name(lid);

	if (flags & PCB_LYT_TOP) {
		if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
			res = single_name;
		res = "top";
	}
	else if (flags & PCB_LYT_BOTTOM) {
		if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
			res = single_name;
		res = "bottom";
	}
	else {
		static char buf[20];
		if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 1))
			res = single_name;
		sprintf(buf, "group%ld", group);
		res = buf;
	}

	assert(res != NULL);
	strcpy(dest, res);
	return dest;
}


void pcb_derive_default_filename(const char *pcbfile, pcb_hid_attribute_t * filename_attrib, const char *suffix, char **memory)
{
	char *buf;
	char *pf;

	if (pcbfile == NULL)
		pf = pcb_strdup("unknown.pcb");
	else
		pf = pcb_strdup(pcbfile);

	if (!pf || (memory && filename_attrib->default_val.str_value != *memory))
		return;

	buf = (char *) malloc(strlen(pf) + strlen(suffix) + 1);
	if (memory)
		*memory = buf;
	if (buf) {
		size_t bl;
		strcpy(buf, pf);
		bl = strlen(buf);
		if (bl > 4 && strcmp(buf + bl - 4, ".pcb") == 0)
			buf[bl - 4] = 0;
		strcat(buf, suffix);
		if (filename_attrib->default_val.str_value)
			free((void *) filename_attrib->default_val.str_value);
		filename_attrib->default_val.str_value = buf;
	}

	free(pf);
}
