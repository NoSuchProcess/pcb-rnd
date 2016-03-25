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
#include "global.h"
#include "data.h"
#include "misc.h"
#include "hid_helper.h"

const char *layer_type_to_file_name(int idx, int style)
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
		if (group == GetLayerGroupNumberByNumber(component_silk_layer)) {
			if (style == FNS_first || (style == FNS_single && nlayers == 2))
				return single_name;
			return "top";
		}
		else if (group == GetLayerGroupNumberByNumber(solder_silk_layer)) {
			if (style == FNS_first || (style == FNS_single && nlayers == 2))
				return single_name;
			return "bottom";
		}
		else if (nlayers == 1
						 && (strcmp(PCB->Data->Layer[idx].Name, "route") == 0 || strcmp(PCB->Data->Layer[idx].Name, "outline") == 0)) {
			return "outline";
		}
		else {
			static char buf[20];
			if (style == FNS_first || (style == FNS_single && nlayers == 1))
				return single_name;
			sprintf(buf, "group%d", group);
			return buf;
		}
		break;
	}
}
