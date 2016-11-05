/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
#include "conf_core.h"
#include "plug_io.h"
#include "compat_misc.h"
#include "hid_actions.h"
#include "paths.h"
#include "rtree.h"

PCBType *PCB;

/* ---------------------------------------------------------------------------
 * free memory used by PCB
 */
void FreePCBMemory(PCBType * pcb)
{
	int i;

	if (pcb == NULL)
		return;

	free(pcb->Name);
	free(pcb->Filename);
	free(pcb->PrintFilename);
	rats_patch_destroy(pcb);
	FreeDataMemory(pcb->Data);
	free(pcb->Data);
	/* release font symbols */
	for (i = 0; i <= MAX_FONTPOSITION; i++)
		free(pcb->Font.Symbol[i].Line);
	for (i = 0; i < NUM_NETLISTS; i++)
		FreeLibraryMemory(&(pcb->NetlistLib[i]));
	vtroutestyle_uninit(&pcb->RouteStyle);
	FreeAttributeListMemory(&pcb->Attributes);
	/* clear struct */
	memset(pcb, 0, sizeof(PCBType));
}

/* creates a new PCB */
PCBTypePtr CreateNewPCB_(pcb_bool SetDefaultNames)
{
	PCBTypePtr ptr, save;
	int i;

	/* allocate memory, switch all layers on and copy resources */
	ptr = (PCBTypePtr) calloc(1, sizeof(PCBType));
	ptr->Data = CreateNewBuffer();
	ptr->Data->pcb = (PCBTypePtr) ptr;

	ptr->ThermStyle = 4;
	ptr->IsleArea = 2.e8;
	ptr->SilkActive = pcb_false;
	ptr->RatDraw = pcb_false;

	/* NOTE: we used to set all the pcb flags on ptr here, but we don't need to do that anymore due to the new conf system */
						/* this is the most useful starting point for now */

	ptr->Grid = conf_core.editor.grid;
	ParseGroupString(conf_core.design.groups, &ptr->LayerGroups, MAX_LAYER);
	save = PCB;
	PCB = ptr;
	hid_action("RouteStylesChanged");
	PCB = save;

	ptr->Zoom = conf_core.editor.zoom;
	ptr->MaxWidth = conf_core.design.max_width;
	ptr->MaxHeight = conf_core.design.max_height;
	ptr->ID = CreateIDGet();
	ptr->ThermScale = 0.5;

	ptr->Bloat = conf_core.design.bloat;
	ptr->Shrink = conf_core.design.shrink;
	ptr->minWid = conf_core.design.min_wid;
	ptr->minSlk = conf_core.design.min_slk;
	ptr->minDrill = conf_core.design.min_drill;
	ptr->minRing = conf_core.design.min_ring;

	for (i = 0; i < MAX_LAYER; i++)
		ptr->Data->Layer[i].Name = pcb_strdup(conf_core.design.default_layer_name[i]);

	CreateDefaultFont(ptr);

	return (ptr);
}

PCBTypePtr CreateNewPCB(void)
{
	PCBTypePtr old, nw;
	int dpcb;

	old = PCB;

	PCB = NULL;

	dpcb = -1;
	pcb_io_err_inhibit_inc();
	conf_list_foreach_path_first(dpcb, &conf_core.rc.default_pcb_file, LoadPCB(__path__, NULL, pcb_false, 1));
	pcb_io_err_inhibit_dec();

	if (dpcb == 0) {
		nw = PCB;
		if (nw->Filename != NULL) {
			/* make sure the new PCB doesn't inherit the name of the default pcb */
			free(nw->Filename);
			nw->Filename = NULL;
		}
	}
	else {
		nw = NULL;
	}

	PCB = old;
	return nw;
}


/* This post-processing step adds the top and bottom silk layers to a
 * pre-existing PCB.
 */
int CreateNewPCBPost(PCBTypePtr pcb, int use_defaults)
{
	/* copy default settings */
	pcb_colors_from_settings(pcb);

	return 0;
}

/* ---------------------------------------------------------------------------
 * Perhaps PCB should internally just use the Settings colors?  For now,
 * use this to set PCB colors so the config can reassign PCB colors.
 */
#warning TODO: indeed, remove this and all the board *color fields
void pcb_colors_from_settings(PCBTypePtr ptr)
{
	int i;

	/* copy default settings */
	ptr->ConnectedColor = conf_core.appearance.color.connected;
	ptr->ElementColor = conf_core.appearance.color.element;
	ptr->ElementColor_nonetlist = conf_core.appearance.color.element_nonetlist;
	ptr->RatColor = conf_core.appearance.color.rat;
	ptr->InvisibleObjectsColor = conf_core.appearance.color.invisible_objects;
	ptr->InvisibleMarkColor = conf_core.appearance.color.invisible_mark;
	ptr->ElementSelectedColor = conf_core.appearance.color.element_selected;
	ptr->RatSelectedColor = conf_core.appearance.color.rat_selected;
	ptr->PinColor = conf_core.appearance.color.pin;
	ptr->PinSelectedColor = conf_core.appearance.color.pin_selected;
	ptr->PinNameColor = conf_core.appearance.color.pin_name;
	ptr->ViaColor = conf_core.appearance.color.via;
	ptr->ViaSelectedColor = conf_core.appearance.color.via_selected;
	ptr->WarnColor = conf_core.appearance.color.warn;
	ptr->MaskColor = conf_core.appearance.color.mask;
	for (i = 0; i < MAX_LAYER; i++) {
		ptr->Data->Layer[i].Color = conf_core.appearance.color.layer[i];
		ptr->Data->Layer[i].SelectedColor = conf_core.appearance.color.layer_selected[i];
	}
	ptr->Data->Layer[component_silk_layer].Color =
		conf_core.editor.show_solder_side ? conf_core.appearance.color.invisible_objects : conf_core.appearance.color.element;
	ptr->Data->Layer[component_silk_layer].SelectedColor = conf_core.appearance.color.element_selected;
	ptr->Data->Layer[solder_silk_layer].Color = conf_core.editor.show_solder_side ? conf_core.appearance.color.element : conf_core.appearance.color.invisible_objects;
	ptr->Data->Layer[solder_silk_layer].SelectedColor = conf_core.appearance.color.element_selected;
}

typedef struct {
	int nplated;
	int nunplated;
} HoleCountStruct;

static r_dir_t hole_counting_callback(const BoxType * b, void *cl)
{
	PinTypePtr pin = (PinTypePtr) b;
	HoleCountStruct *hcs = (HoleCountStruct *) cl;
	if (TEST_FLAG(PCB_FLAG_HOLE, pin))
		hcs->nunplated++;
	else
		hcs->nplated++;
	return R_DIR_FOUND_CONTINUE;
}


/* ---------------------------------------------------------------------------
 * counts the number of plated and unplated holes in the design within
 * a given area of the board. To count for the whole board, pass NULL
 * within_area.
 */
void CountHoles(int *plated, int *unplated, const BoxType * within_area)
{
	HoleCountStruct hcs = { 0, 0 };

	r_search(PCB->Data->pin_tree, within_area, NULL, hole_counting_callback, &hcs, NULL);
	r_search(PCB->Data->via_tree, within_area, NULL, hole_counting_callback, &hcs, NULL);

	if (plated != NULL)
		*plated = hcs.nplated;
	if (unplated != NULL)
		*unplated = hcs.nunplated;
}
