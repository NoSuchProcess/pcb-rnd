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
#include "compat_misc.h"

const char *pcb_layer_type_to_file_name(int idx, int style)
{
	int group;
	int nlayers;
	const char *single_name;

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
		group = GetLayerGroupNumberByNumber(idx);
		nlayers = PCB->LayerGroups.Number[group];
		single_name = PCB->Data->Layer[idx].Name;
		if (group == GetLayerGroupNumberByNumber(pcb_component_silk_layer)) {
			if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
				return single_name;
			return "top";
		}
		else if (group == GetLayerGroupNumberByNumber(pcb_solder_silk_layer)) {
			if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 2))
				return single_name;
			return "bottom";
		}
		else if (nlayers == 1
						 && (strcmp(PCB->Data->Layer[idx].Name, "route") == 0 || strcmp(PCB->Data->Layer[idx].Name, "outline") == 0)) {
			return "outline";
		}
		else {
			static char buf[20];
			if (style == PCB_FNS_first || (style == PCB_FNS_single && nlayers == 1))
				return single_name;
			sprintf(buf, "group%d", group);
			return buf;
		}
		break;
	}
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
