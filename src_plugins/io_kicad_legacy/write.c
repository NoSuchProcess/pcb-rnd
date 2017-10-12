/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *     Copyright (C) 2016 Erich S. Heinzle
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
 *
 */
#include <math.h>
#include "config.h"
#include "compat_misc.h"
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "../io_kicad/uniq_name.h"
#include "data.h"
#include "write.h"
#include "layer.h"
#include "const.h"
#include "netlist.h"
#include "obj_all.h"

/* layer "0" is first copper layer = "0. Back - Solder"
 * and layer "15" is "15. Front - Component"
 * and layer "20" SilkScreen Back
 * and layer "21" SilkScreen Front
 */

/* generates a line by line listing of the elements being saved */
static int io_kicad_legacy_write_element_index(FILE * FP, pcb_data_t *Data);

/* generates a default via drill size for the layout */
static int write_kicad_legacy_layout_via_drill_size(FILE * FP);

/* writes the buffer to file */
int io_kicad_legacy_write_buffer(pcb_plug_io_t *ctx, FILE * FP, pcb_buffer_t *buff, pcb_bool elem_only)
{
	/*fputs("io_kicad_legacy_write_buffer()", FP); */

	if (elementlist_length(&buff->Data->Element) == 0) {
		pcb_message(PCB_MSG_ERROR, "Buffer has no elements!\n");
		return -1;
	}

	fputs("PCBNEW-LibModule-V1	jan 01 jan 2016 00:00:01 CET\n",FP);
	fputs("Units mm\n",FP);
	fputs("$INDEX\n",FP);
	io_kicad_legacy_write_element_index(FP, buff->Data);
	fputs("$EndINDEX\n",FP);

	/* WriteViaData(FP, buff->Data); */

	pcb_write_element_data(FP, buff->Data, "kicadl");

	/*
		for (i = 0; i < pcb_max_layer; i++)
		WriteLayerData(FP, i, &(buff->Data->Layer[i]));
	*/
	return (0);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int io_kicad_legacy_write_pcb(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	/* this is the first step in exporting a layout;
	 * creating a kicd module containing the elements used in the layout
	 */

	/*fputs("io_kicad_legacy_write_pcb()", FP);*/

	pcb_cardinal_t i;
	int kicadLayerCount = 0;
	int physicalLayerCount = 0;
	int layer = 0;
	int currentKicadLayer = 0;
	int currentGroup = 0;
	pcb_coord_t outlineThickness = PCB_MIL_TO_COORD(10); 

	int bottomCount;
	pcb_layer_id_t *bottomLayers;
	int innerCount;
	pcb_layer_id_t *innerLayers;
	int topCount;
	pcb_layer_id_t *topLayers;
	int bottomSilkCount;
	pcb_layer_id_t *bottomSilk;
	int topSilkCount;
	pcb_layer_id_t *topSilk;
	int outlineCount;
	pcb_layer_id_t *outlineLayers;

	pcb_coord_t LayoutXOffset;
	pcb_coord_t LayoutYOffset;

	/* Kicad expects a layout "sheet" size to be specified in mils, and A4, A3 etc... */
	int A4HeightMil = 8267;
	int A4WidthMil = 11700;
	int sheetHeight = A4HeightMil;
	int sheetWidth = A4WidthMil;
	int paperSize = 4; /* default paper size is A4 */

	fputs("PCBNEW-BOARD Version 1 jan 01 jan 2016 00:00:01 CET\n",FP);

	fputs("$GENERAL\n",FP);
	fputs("Ly 1FFF8001\n",FP); /* obsolete, needed for old pcbnew */
	/*puts("Units mm\n",FP);*/ /*decimils most universal legacy format */
	fputs("$EndGENERAL\n",FP);

	fputs("$SHEETDESCR\n",FP);


	/* we sort out the needed kicad sheet size here, using A4, A3, A2, A1 or A0 size as needed */
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > A4WidthMil ||
			PCB_COORD_TO_MIL(PCB->MaxHeight) > A4HeightMil) {
		sheetHeight = A4WidthMil;		/* 11.7" */
		sheetWidth = 2*A4HeightMil; /* 16.5" */
		paperSize = 3; /* this is A3 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth ||
			PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 2*A4HeightMil; /* 16.5" */
		sheetWidth = 2*A4WidthMil;	 /* 23.4" */
		paperSize = 2; /* this is A2 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth ||
			PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 2*A4WidthMil; /* 23.4" */
		sheetWidth = 4*A4HeightMil; /* 33.1" */
		paperSize = 1; /* this is A1 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth ||
			PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 4*A4HeightMil; /* 33.1" */
		sheetWidth = 4*A4WidthMil; /* 46.8"	 */
		paperSize = 0; /* this is A0 size; where would you get it made ?!?! */
	}
	fprintf(FP, "Sheet A%d ", paperSize);
	/* we now sort out the offsets for centring the layout in the chosen sheet size here */
	if (sheetWidth > PCB_COORD_TO_MIL(PCB->MaxWidth)) {	 /* usually A4, bigger if needed */
		fprintf(FP, "%d ", sheetWidth); /* legacy kicad: elements decimils, sheet size mils*/
		LayoutXOffset = PCB_MIL_TO_COORD(sheetWidth)/2 - PCB->MaxWidth/2;
	} else { /* the layout is bigger than A0; most unlikely, but... */
		pcb_fprintf(FP, "%.0ml ", PCB->MaxWidth);
		LayoutXOffset = 0;
	}
	if (sheetHeight > PCB_COORD_TO_MIL(PCB->MaxHeight)) {
		fprintf(FP, "%d", sheetHeight);
		LayoutYOffset = PCB_MIL_TO_COORD(sheetHeight)/2 - PCB->MaxHeight/2;
	} else { /* the layout is bigger than A0; most unlikely, but... */
		pcb_fprintf(FP, "%.0ml", PCB->MaxHeight);
		LayoutYOffset = 0;
	}
	fputs("\n", FP);
	fputs("$EndSHEETDESCR\n",FP);

	fputs("$SETUP\n",FP);
	fputs("InternalUnit 0.000100 INCH\n",FP); /* decimil is the default v1 kicad legacy unit */

	/* here we define the copper layers in the exported kicad file */
	physicalLayerCount = pcb_layergrp_list(PCB, PCB_LYT_COPPER, NULL, 0);

	fputs("Layers ",FP);
	kicadLayerCount = physicalLayerCount;
	if (kicadLayerCount%2 == 1) {
		kicadLayerCount++; /* kicad doesn't like odd numbers of layers, has been deprecated for some time apparently */
	}
	
	fprintf(FP, "%d\n", kicadLayerCount); 
	layer = 0;
	if (physicalLayerCount >= 1) {
		fprintf(FP, "Layer[%d] COPPER_LAYER_0 signal\n", layer);
	}
	if (physicalLayerCount > 1) { /* seems we need to ignore layers > 16 due to kicad limitation */
		for (layer = 1; (layer < (kicadLayerCount - 1)) && (layer < 15); layer++ ) {
			fprintf(FP, "Layer[%d] Inner%d.Cu signal\n", layer, layer);
		} 
		fputs("Layer[15] COPPER_LAYER_15 signal\n",FP);	
	}

	write_kicad_legacy_layout_via_drill_size(FP);

	fputs("$EndSETUP\n",FP);

	/* now come the netlist "equipotential" descriptors */

	write_kicad_legacy_equipotential_netlists(FP, PCB);

	/* module descriptions come next */

	write_kicad_legacy_layout_elements(FP, PCB, PCB->Data, LayoutXOffset, LayoutYOffset);

	/* we now need to map pcb's layer groups onto the kicad layer numbers */
	currentKicadLayer = 0;
	currentGroup = 0;

	/* figure out which pcb layers are bottom copper and make a list */
	bottomCount = pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, NULL, 0);
	if (bottomCount > 0 ) {
		bottomLayers = malloc(sizeof(pcb_layer_id_t) * bottomCount);
		pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, bottomLayers, bottomCount);
	} else {
		bottomLayers = NULL;
	}

	/* figure out which pcb layers are internal copper layers and make a list */
	innerCount = pcb_layer_list(PCB_LYT_INTERN | PCB_LYT_COPPER, NULL, 0);
	if (innerCount > 0 ) {
		innerLayers = malloc(sizeof(pcb_layer_id_t) * innerCount);
		pcb_layer_list(PCB_LYT_INTERN | PCB_LYT_COPPER, innerLayers, innerCount);
	} else {
		innerLayers = NULL;
	}

	/* figure out which pcb layers are top copper and make a list */
	topCount = pcb_layer_list(PCB_LYT_TOP | PCB_LYT_COPPER, NULL, 0);
	if (topCount > 0 ) {
		topLayers = malloc(sizeof(pcb_layer_id_t) * topCount);
		pcb_layer_list(PCB_LYT_TOP | PCB_LYT_COPPER, topLayers, topCount);
	} else {
		topLayers = NULL;
	}

	/* figure out which pcb layers are bottom silk and make a list */
	bottomSilkCount = pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, NULL, 0);
	if (bottomSilkCount > 0 ) {
		bottomSilk = malloc(sizeof(pcb_layer_id_t) * bottomSilkCount);
		pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, bottomSilk, bottomSilkCount);
	} else {
		bottomSilk = NULL;
	}

	/* figure out which pcb layers are top silk and make a list */
	topSilkCount = pcb_layer_list(PCB_LYT_TOP | PCB_LYT_SILK, NULL, 0);
	if (topSilkCount > 0) {
		topSilk = malloc(sizeof(pcb_layer_id_t) * topSilkCount);
		pcb_layer_list(PCB_LYT_TOP | PCB_LYT_SILK, topSilk, topSilkCount);
	} else {
		topSilk = NULL;
	}

	/* figure out which pcb layers are outlines and make a list */
 	outlineCount = pcb_layer_list(PCB_LYT_OUTLINE, NULL, 0);
	if (outlineCount > 0) {
		outlineLayers = malloc(sizeof(pcb_layer_id_t) * outlineCount);
		pcb_layer_list(PCB_LYT_OUTLINE, outlineLayers, outlineCount);
	} else {
		outlineLayers = NULL;
	}

	/* we now proceed to write the outline tracks to the kicad file, layer by layer */
	currentKicadLayer = 28; /* 28 is the edge cuts layer in kicad */
	if (outlineCount > 0 )  {
		for (i = 0; i < outlineCount; i++) /* write top copper tracks, if any */
			{
				write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]),
										LayoutXOffset, LayoutYOffset);
				write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]),
										LayoutXOffset, LayoutYOffset);
			}
	} else { /* no outline layer per se, export the board margins instead  - obviously some scope to reduce redundant code...*/
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										outlineThickness);
										pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
				fputs("$EndDRAWSEGMENT\n", FP);
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										outlineThickness);
										pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
				fputs("$EndDRAWSEGMENT\n", FP);
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										outlineThickness);
										pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
				fputs("$EndDRAWSEGMENT\n", FP);
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										outlineThickness);
										pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
				fputs("$EndDRAWSEGMENT\n", FP);
	}


	/* we now proceed to write the bottom silk lines, arcs, text to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for (i = 0; i < bottomSilkCount; i++) /* write bottom silk lines, if any */
		{
			write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the bottom copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for (i = 0; i < bottomCount; i++) /* write bottom copper tracks, if any */
		{
			write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}	/* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper text to the kicad file, layer by layer */
	if (innerCount > 0) {
		currentGroup = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for (i = 0, currentKicadLayer = 1; i < innerCount; i++) /* write inner copper text, group by group */
		{
			if (currentGroup != pcb_layer_get_group(PCB, innerLayers[i])) {
				currentGroup = pcb_layer_get_group(PCB, innerLayers[i]);
				currentKicadLayer++;
				if (currentKicadLayer > 14) {
					currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
				}
			}
			write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the top copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for (i = 0; i < topCount; i++) /* write top copper tracks, if any */
		{
			write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the top silk lines, arcs, text to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for (i = 0; i < topSilkCount; i++) /* write top silk lines, if any */
		{
			write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* having done the graphical elements, we move onto tracks and vias */ 

	fputs("$TRACK\n",FP);
	write_kicad_legacy_layout_vias(FP, PCB->Data, LayoutXOffset, LayoutYOffset);

	/* we now proceed to write the bottom copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for (i = 0; i < bottomCount; i++) /* write bottom copper tracks, if any */
		{
			write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}	/* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper tracks to the kicad file, layer by layer */
	if (innerCount > 0) {
		currentGroup = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for (i = 0, currentKicadLayer = 1; i < innerCount; i++) /* write inner copper tracks, group by group */
		{
			if (currentGroup != pcb_layer_get_group(PCB, innerLayers[i])) {
				currentGroup = pcb_layer_get_group(PCB, innerLayers[i]);
				currentKicadLayer++;
				if (currentKicadLayer > 14) {
					currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
				}
			}
			write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the top copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for (i = 0; i < topCount; i++) /* write top copper tracks, if any */
		{
			write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}
	fputs("$EndTRACK\n",FP);

	/*
	  * now we proceed to write polygons for each layer, and iterate much like we did for tracks
	 */

	/* we now proceed to write the bottom silk polygons  to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for (i = 0; i < bottomSilkCount; i++) /* write bottom silk polygons, if any */
		{
			write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the bottom copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for (i = 0; i < bottomCount; i++) /* write bottom copper polygons, if any */
		{
			write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}	/* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper polygons to the kicad file, layer by layer */
	if (innerCount > 0) {
		currentGroup = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for (i = 0, currentKicadLayer = 1; i < innerCount; i++) /* write inner copper polygons, group by group */
		{
			if (currentGroup != pcb_layer_get_group(PCB, innerLayers[i])) {
				currentGroup = pcb_layer_get_group(PCB, innerLayers[i]);
				currentKicadLayer++;
				if (currentKicadLayer > 14) {
					currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
				}
			}
			write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the top copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for (i = 0; i < topCount; i++) /* write top copper polygons, if any */
		{
			write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the top silk polygons to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for (i = 0; i < topSilkCount; i++) /* write top silk polygons, if any */
		{
			write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset);
		}


	fputs("$EndBOARD\n",FP);

	/* now free memory from arrays that were used */
	if (bottomCount > 0) {
		free(bottomLayers);
	}
	if (innerCount > 0) {
		free(innerLayers);
	}
	if (topCount > 0) {
		free(topLayers);
	}
	if (topSilkCount > 0) {
		free(topSilk);
	}
	if (bottomSilkCount > 0) {
		free(bottomSilk);
	}
	if (outlineCount > 0) {
		free(outlineLayers);
	}
	return (0);
}

/* ---------------------------------------------------------------------------
 * writes (eventually) de-duplicated list of element names in kicad legacy format module $INDEX
 */
static int io_kicad_legacy_write_element_index(FILE * FP, pcb_data_t *Data)
{
	gdl_iterator_t eit;
	pcb_element_t *element;
	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */

	elementlist_dedup_initializer(ededup);

	/* Now initialize the group with defaults */
	unm_init(&group1);

	elementlist_foreach(&Data->Element, &eit, element) {
		elementlist_dedup_skip(ededup, element);
		/* skip duplicate elements */
		/* only non empty elements */

		if (!linelist_length(&element->Line)
				&& !pinlist_length(&element->Pin)
				&& !arclist_length(&element->Arc)
				&& !padlist_length(&element->Pad))
			continue;

		fprintf(FP, "%s\n", unm_name(&group1, element->Name[0].TextString, element));

	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);
	return 0;
}


/* ---------------------------------------------------------------------------
 * writes kicad format via data
 For a track segment:
 Position shape Xstart Ystart Xend Yend width
 Description layer 0 netcode timestamp status
 Shape parameter is set to 0 (reserved for futu
*/


int write_kicad_legacy_layout_vias(FILE * FP, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		/*		pcb_fprintf(FP, "Po 3 %.3mm %.3mm %.3mm %.3mm %.3mm\n",
					via->X, via->Y, via->X, via->Y, via->Thickness);
					pcb_fprintf(FP, "De F0 1 0 0 0\n"); */
		pcb_fprintf(FP, "Po 3 %.0mk %.0mk %.0mk %.0mk %.0mk\n", /* testing kicad printf */
								via->X + xOffset, via->Y + yOffset,
								via->X + xOffset, via->Y + yOffset, via->Thickness);
		pcb_fprintf(FP, "De 15 1 0 0 0\n"); /* this is equivalent to 0F, via from 15 -> 0 */
	}
	return 0;
}

static int write_kicad_legacy_layout_via_drill_size(FILE * FP)
{
	pcb_fprintf(FP, "ViaDrill 250\n"); /* decimil format, default for now, ~= 0.635mm */
	return 0;
}

int write_kicad_legacy_layout_tracks(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		/*
			fprintf(FP, "Layer(%i ", (int) Number + 1);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->name));
			fputs(")\n(\n", FP);
			WriteAttributeList(FP, &layer->Attributes, "\t");
		*/
		int localFlag = 0;
		linelist_foreach(&layer->Line, &it, line) {
			if (currentLayer < 16) { /* a copper line i.e. track */
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										line->Point1.X + xOffset, line->Point1.Y + yOffset,
										line->Point2.X + xOffset, line->Point2.Y + yOffset,
										line->Thickness);
				pcb_fprintf(FP, "De %d 0 0 0 0\n", currentLayer); /* omitting net info */
			} else if ((currentLayer == 20) || (currentLayer == 21) || (currentLayer == 28) ) { /* a silk line or outline */
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										line->Point1.X + xOffset, line->Point1.Y + yOffset,
										line->Point2.X + xOffset, line->Point2.Y + yOffset,
										line->Thickness);
				pcb_fprintf(FP, "De %d 0 0 0 0\n", currentLayer); /* omitting net info */
				fputs("$EndDRAWSEGMENT\n", FP);
			}
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

int write_kicad_legacy_layout_arcs(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_arc_t *arc;
	pcb_arc_t localArc; /* for converting ellipses to circular arcs */
	pcb_cardinal_t currentLayer = number;
	pcb_coord_t radius, xStart, yStart, xEnd, yEnd;
	int copperStartX; /* used for mapping geda copper arcs onto kicad copper lines */
	int copperStartY; /* used for mapping geda copper arcs onto kicad copper lines */

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		/*
			fprintf(FP, "Layer(%i ", (int) Number + 1);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->name));
			fputs(")\n(\n", FP);
			WriteAttributeList(FP, &layer->Attributes, "\t");
		*/
		int localFlag = 0;
		int kicadArcShape = 2; /* 3 = circle, and 2 = arc, 1= rectangle used in eeschema only */ 
		arclist_foreach(&layer->Arc, &it, arc) {
			localArc = *arc;
			if (arc->Width > arc->Height) {
				radius = arc->Height;
				localArc.Width = radius;
			} else {
				radius = arc->Width;
				localArc.Height = radius;
			}
			if (arc->Delta == 360.0 || arc->Delta == -360.0 ) { /* it's a circle */
				kicadArcShape = 3;
			} else { /* it's an arc */
				kicadArcShape = 2;
			}

			xStart = localArc.X + xOffset;
			yStart = localArc.Y + yOffset;
			pcb_arc_get_end(&localArc, 1, &xEnd, &yEnd);
			xEnd += xOffset; 
			yEnd += yOffset; 
			pcb_arc_get_end(&localArc, 0, &copperStartX, &copperStartY);
			copperStartX += xOffset;
			copperStartY += yOffset;

			if (currentLayer < 16) { /* a copper arc, i.e. track, is unsupported by kicad, and will be exported as a line */
				kicadArcShape = 0; /* make it a line for copper layers - kicad doesn't do arcs on copper */ 
				pcb_fprintf(FP, "Po %d %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										kicadArcShape, copperStartX, copperStartY, xEnd, yEnd,
										arc->Thickness);
				pcb_fprintf(FP, "De %d 0 0 0 0\n", currentLayer); /* in theory, copper arcs unsupported by kicad, make angle = 0 */
			} else if ((currentLayer == 20) || (currentLayer == 21) || (currentLayer == 28) ) { /* a silk arc or outline */
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po %d %.0mk %.0mk %.0mk %.0mk %.0mk\n",
										kicadArcShape, xStart, yStart, xEnd, yEnd,
										arc->Thickness);
				pcb_fprintf(FP, "De %d 0 %mA 0 0\n", currentLayer, arc->Delta); /* in theory, decidegrees != 900 unsupported by older kicad*/
				fputs("$EndDRAWSEGMENT\n", FP);
			}
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

int write_kicad_legacy_layout_text(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mWidth = myfont->MaxWidth; /* kicad needs the width of the widest letter */
	pcb_coord_t defaultStrokeThickness = 100*2540; /* use 100 mil as default 100% stroked font line thickness */
	int kicadMirrored = 1; /* 1 is not mirrored, 0  is mirrored */ 

	pcb_coord_t defaultXSize;
	pcb_coord_t defaultYSize;
	pcb_coord_t strokeThickness;
	int rotation;	
	pcb_coord_t textOffsetX;
	pcb_coord_t textOffsetY;
	pcb_coord_t halfStringWidth;
	pcb_coord_t halfStringHeight;
	int localFlag;

	gdl_iterator_t it;
	pcb_text_t *text;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		/*
			fprintf(FP, "Layer(%i ", (int) Number + 1);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->name));
			fputs(")\n(\n", FP);
			WriteAttributeList(FP, &layer->Attributes, "\t");
		*/
		localFlag = 0;
		textlist_foreach(&layer->Text, &it, text) {
			if ((currentLayer < 16) || (currentLayer == 20) || (currentLayer == 21) ) { /* copper or silk layer text */
				fputs("$TEXTPCB\nTe \"", FP);
				fputs(text->TextString,FP);
				fputs("\"\n", FP);
				defaultXSize = 5*PCB_SCALE_TEXT(mWidth, text->Scale)/6; /* IIRC kicad treats this as kerned width of upper case m */
				defaultYSize = defaultXSize;
				strokeThickness = PCB_SCALE_TEXT(defaultStrokeThickness, text->Scale /2);
				rotation = 0;	
				textOffsetX = 0;
				textOffsetY = 0;
				halfStringWidth = (text->BoundingBox.X2 - text->BoundingBox.X1)/2;
				if (halfStringWidth < 0) {
					halfStringWidth = -halfStringWidth;
				}
				halfStringHeight = (text->BoundingBox.Y2 - text->BoundingBox.Y1)/2;
				if (halfStringHeight < 0) {
					halfStringHeight = -halfStringHeight;
				}
				if (text->Direction == 3) { /*vertical down*/
					if (currentLayer == 0 || currentLayer == 20) {  /* back copper or silk */ 
						rotation = 2700;
						kicadMirrored = 0; /* mirrored */
						textOffsetY -= halfStringHeight;
						textOffsetX -= 2*halfStringWidth; /* was 1*hsw */
					} else {    /* front copper or silk */
						rotation = 2700;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY = halfStringHeight;
						textOffsetX -= halfStringWidth;
					}
				} else if (text->Direction == 2)  { /*upside down*/
					if (currentLayer == 0 || currentLayer == 20) {  /* back copper or silk */ 
						rotation = 0;
						kicadMirrored = 0; /* mirrored */
						textOffsetY += halfStringHeight;
					} else {    /* front copper or silk */
						rotation = 1800;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY -= halfStringHeight;
					}
					textOffsetX = -halfStringWidth;
				} else if (text->Direction == 1) { /*vertical up*/
					if (currentLayer == 0 || currentLayer == 20) {  /* back copper or silk */ 
						rotation = 900;
						kicadMirrored = 0; /* mirrored */
						textOffsetY = halfStringHeight;
						textOffsetX += halfStringWidth; 
					} else {    /* front copper or silk */
						rotation = 900;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY = -halfStringHeight;
						textOffsetX = 0; /* += halfStringWidth; */
					}
				} else if (text->Direction == 0)  { /*normal text*/
					if (currentLayer == 0 || currentLayer == 20) {  /* back copper or silk */
						rotation = 1800;
						kicadMirrored = 0; /* mirrored */
						textOffsetY -= halfStringHeight;
					} else {    /* front copper or silk */
						rotation = 0;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY += halfStringHeight;
					}
					textOffsetX = halfStringWidth;
				}
/*				printf("\"%s\" direction field: %d\n", text->TextString, text->Direction);
				printf("textOffsetX: %d,  textOffsetY: %d\n", textOffsetX, textOffsetY); */
				pcb_fprintf(FP, "Po %.0mk %.0mk %.0mk %.0mk %.0mk %d\n",
										text->X + xOffset + textOffsetX, text->Y + yOffset + textOffsetY,
										defaultXSize, defaultYSize, strokeThickness, rotation);
				pcb_fprintf(FP, "De %d %d B98C Normal\n", currentLayer, kicadMirrored); /* timestamp made up B98C  */
				fputs("$EndTEXTPCB\n", FP);
			}
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a .mod library
 */
int io_kicad_legacy_write_element(pcb_plug_io_t *ctx, FILE * FP, pcb_data_t *Data)
{


	/*
	write_kicad_legacy_module_header(FP);
	fputs("io_kicad_legacy_write_element()", FP);
	return 0;
	*/


	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_element_t *element;

	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc exporting */

	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	const char * currentElementName;

	elementlist_dedup_initializer(ededup);
	/* Now initialize the group with defaults */
	unm_init(&group1);

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		elementlist_dedup_skip(ededup, element); /* skip duplicate elements */

		/* TOOD: Footprint name element->Name[0].TextString */

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;
		/* the coordinates and text-flags are the same for
		 * both names of an element
		 */
		/* the following element summary is not used
			 in kicad; the module's header contains this
			 information

			 fprintf(FP, "\nDS %s ", F2S(element, PCB_TYPE_ELEMENT));
			 pcb_print_quoted_string(FP, (char *) PCB_EMPTY(PCB_ELEM_NAME_DESCRIPTION(element)));
			 fputc(' ', FP);
			 pcb_print_quoted_string(FP, (char *) PCB_EMPTY(PCB_ELEM_NAME_REFDES(element)));
			 fputc(' ', FP);
			 pcb_print_quoted_string(FP, (char *) PCB_EMPTY(PCB_ELEM_NAME_VALUE(element)));
			 pcb_fprintf(FP, " %mm %mm %mm %mm %d %d %s]\n(\n",
			 element->MarkX, element->MarkY,
			 PCB_ELEM_TEXT_DESCRIPTION(element).X - element->MarkX,
			 PCB_ELEM_TEXT_DESCRIPTION(element).Y - element->MarkY,
			 PCB_ELEM_TEXT_DESCRIPTION(element).Direction,
			 PCB_ELEM_TEXT_DESCRIPTION(element).Scale, F2S(&(PCB_ELEM_TEXT_DESCRIPTION(element)), PCB_TYPE_ELEMENT_NAME));

		*/

		/*		//WriteAttributeList(FP, &element->Attributes, "\t");
		 */

		currentElementName = unm_name(&group1, element->Name[0].TextString, element);
		fprintf(FP, "$MODULE %s\n", currentElementName);
		fputs("Po 0 0 0 15 51534DFF 00000000 ~~\n",FP);
		fprintf(FP, "Li %s\n", currentElementName);
		fprintf(FP, "Cd %s\n", currentElementName);
		fputs("Sc 0\n",FP);
		fputs("AR\n",FP);
		fputs("Op 0 0 0\n",FP);
		fputs("T0 0 -6.000 1.524 1.524 0 0.305 N V 21 N \"S***\"\n",FP); /*1.524 is basically 600 decimil, 0.305 is ~= 120 decimil */

		linelist_foreach(&element->Line, &it, line) {
			pcb_fprintf(FP, "DS %.3mm %.3mm %.3mm %.3mm %.3mm ",
									line->Point1.X - element->MarkX,
									line->Point1.Y - element->MarkY,
									line->Point2.X - element->MarkX,
									line->Point2.Y - element->MarkY,
									line->Thickness);
			fputs("21\n",FP); /* an arbitrary Kicad layer, front silk, need to refine this */
		}

		arclist_foreach(&element->Arc, &it, arc) {
			pcb_arc_get_end(arc, 0, &arcStartX, &arcStartY);
			pcb_arc_get_end(arc, 1, &arcEndX, &arcEndY);
			if ((arc->Delta == 360.0) || (arc->Delta == -360.0)) { /* it's a circle */
				pcb_fprintf(FP, "DC %.3mm %.3mm %.3mm %.3mm %.3mm ",
										arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcStartX - element->MarkX, /* x on circle */
										arcStartY - element->MarkY, /* y on circle */
										arc->Thickness); /* stroke thickness */
			} else {
				/*
				   as far as can be determined from the Kicad documentation,
				   http://en.wikibooks.org/wiki/Kicad/file_formats#Drawings

				   the origin for rotation is the positive x direction, and going CW

				   whereas in gEDA, the gEDA origin for rotation is the negative x axis,
				   with rotation CCW, so we need to reverse delta angle

				   deltaAngle is CW in Kicad in deci-degrees, and CCW in degrees in gEDA
				   NB it is in degrees in the newer s-file kicad module/footprint format
				*/
				pcb_fprintf(FP, "DA %.3mm %.3mm %.3mm %.3mm %mA %.3mm ",
										arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcEndX - element->MarkX, /* x on arc */
										arcEndY - element->MarkY, /* y on arc */
										arc->Delta, /* CW delta angle in decidegrees */
										arc->Thickness); /* stroke thickness */
			}
			fputs("21\n",FP); /* and now append a suitable Kicad layer, front silk = 21 */
		}

		pinlist_foreach(&element->Pin, &it, pin) {
			fputs("$PAD\n",FP);	 /* start pad descriptor for a pin */

			pcb_fprintf(FP, "Po %.3mm %.3mm\n", /* positions of pad */
									pin->X - element->MarkX,
									pin->Y - element->MarkY);

			fputs("Sh ",FP); /* pin shape descriptor */
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number));

			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
				fputs(" R ",FP); /* square */
			} else {
				fputs(" C ",FP); /* circular */
			}

			pcb_fprintf(FP, "%.3mm %.3mm ", pin->Thickness, pin->Thickness); /* height = width */
			fputs("0 0 0\n",FP); /* deltaX deltaY Orientation as float in decidegrees */

			fputs("Dr ",FP); /* drill details; size and x,y pos relative to pad location */
			pcb_fprintf(FP, "%mm 0 0\n", pin->DrillingHole);

			fputs("At STD N 00E0FFFF\n", FP); /* through hole STD pin, all copper layers */

			fputs("Ne 0 \"\"\n",FP); /* library parts have empty net descriptors */
			/*
				pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
				fprintf(FP, " %s\n", F2S(pin, PCB_TYPE_PIN));
			*/
			fputs("$EndPAD\n",FP);
		}
		padlist_foreach(&element->Pad, &it, pad) {
			fputs("$PAD\n",FP);	 /* start pad descriptor for an smd pad */

			pcb_fprintf(FP, "Po %.3mm %.3mm\n", /* positions of pad */
									(pad->Point1.X + pad->Point2.X)/2- element->MarkX,
									(pad->Point1.Y + pad->Point2.Y)/2- element->MarkY);

			fputs("Sh ",FP); /* pin shape descriptor */
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pad->Number));
			fputs(" R ",FP); /* rectangular, not a pin */

			if ((pad->Point1.X-pad->Point2.X) <= 0
					&& (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
										pad->Point2.Y-pad->Point1.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) <= 0
								 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
										pad->Point1.Y-pad->Point2.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
								 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
										pad->Point1.Y-pad->Point2.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
								 && (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
										pad->Point2.Y-pad->Point1.Y + pad->Thickness); /* height */
			}

			fputs("0 0 0\n",FP); /* deltaX deltaY Orientation as float in decidegrees */

			fputs("Dr 0 0 0\n",FP); /* drill details; zero size; x,y pos vs pad location */

			fputs("At SMD N 00888000\n", FP); /* SMD pin, need to use right layer mask */

			fputs("Ne 0 \"\"\n",FP); /* library parts have empty net descriptors */
			fputs("$EndPAD\n",FP);
		}
		fprintf(FP, "$EndMODULE %s\n", currentElementName);		
	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);

	return 0;
}


/* ---------------------------------------------------------------------------
 * writes netlist data in kicad legacy format for use in a layout .brd file
 */

int write_kicad_legacy_equipotential_netlists(FILE * FP, pcb_board_t *Layout)
{
	int n; /* code mostly lifted from netlist.c */ 
	int netNumber;
	pcb_lib_menu_t *menu;
	pcb_lib_entry_t *netlist;
	
	/* first we write a default netlist for the 0 net, which is for unconnected pads in pcbnew */
	fputs("$EQUIPOT\n",FP);
	fputs("Na 0 \"\"\n", FP);
	fputs("St ~\n", FP);
	fputs("$EndEQUIPOT\n",FP);

	/* now we step through any available netlists and generate descriptors */
	for (n = 0, netNumber = 1; n < Layout->NetlistLib[PCB_NETLIST_EDITED].MenuN; n++, netNumber ++) {
		menu = &Layout->NetlistLib[PCB_NETLIST_EDITED].Menu[n];
		netlist = &menu->Entry[0];
		if (netlist != NULL) {
			fputs("$EQUIPOT\n",FP);
			fprintf(FP, "Na %d \"%s\"\n", netNumber, pcb_netlist_name(menu));  /* netlist 0 was used for unconnected pads  */
			fputs("St ~\n", FP);
			fputs("$EndEQUIPOT\n",FP);
		}
	}
	return 0;
}


/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a layout .brd file
 */
int write_kicad_legacy_layout_elements(FILE * FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{

	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc rendering */
	pcb_coord_t xPos, yPos;

	pcb_element_t *element;
	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	const char * currentElementName;
	pcb_lib_menu_t *current_pin_menu;
	pcb_lib_menu_t *current_pad_menu;

	int silkLayer = 21;  /* hard coded default, 20 is bottom silk */ 
	int copperLayer = 15; /* hard coded default, 0 is bottom copper */

	elementlist_dedup_initializer(ededup);
	/* Now initialize the group with defaults */
	unm_init(&group1);

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		/* elementlist_dedup_skip(ededup, element);  */
		/* let's not skip duplicate elements for layout export*/

		/* TOOD: Footprint name element->Name[0].TextString */

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;
		/* the coordinates and text-flags are the same for
		 * both names of an element
		 */

		xPos = element->MarkX + xOffset;
		yPos = element->MarkY + yOffset;
		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element)) {
			silkLayer = 20;
			copperLayer = 0;
		} else {
			silkLayer = 21;
			copperLayer = 15;
		}

		currentElementName = unm_name(&group1, element->Name[0].TextString, element);
		fprintf(FP, "$MODULE %s\n", currentElementName);
		pcb_fprintf(FP, "Po %.0mk %.0mk 0 %d 51534DFF 00000000 ~~\n", xPos, yPos, copperLayer);
		fprintf(FP, "Li %s\n", currentElementName);
		fprintf(FP, "Cd %s\n", currentElementName);
		fputs("Sc 0\n",FP);
		fputs("AR\n",FP);
		fputs("Op 0 0 0\n",FP);
		fprintf(FP, "T0 0 -4000 600 600 0 120 N V %d N \"%s\"\n", silkLayer, element->Name[PCB_ELEMNAME_IDX_REFDES].TextString);
		fprintf(FP, "T1 0 -5000 600 600 0 120 N V %d N \"%s\"\n", silkLayer, element->Name[PCB_ELEMNAME_IDX_VALUE].TextString);
		fprintf(FP, "T2 0 -6000 600 600 0 120 N V %d N \"%s\"\n", silkLayer, element->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString);
		pinlist_foreach(&element->Pin, &it, pin) {
			fputs("$PAD\n",FP);	 /* start pad descriptor for a pin */

			pcb_fprintf(FP, "Po %.0mk %.0mk\n", /* positions of pad */
									pin->X - element->MarkX,
									pin->Y - element->MarkY);

			fputs("Sh ",FP); /* pin shape descriptor */
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number));

			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
				fputs(" R ",FP); /* square */
			} else {
				fputs(" C ",FP); /* circular */
			}

			pcb_fprintf(FP, "%.0mk %.0mk ", pin->Thickness, pin->Thickness); /* height = width */
			fputs("0 0 0\n",FP); /* deltaX deltaY Orientation as float in decidegrees */

			fputs("Dr ",FP); /* drill details; size and x,y pos relative to pad location */
			pcb_fprintf(FP, "%.0mk 0 0\n", pin->DrillingHole);

			fputs("At STD N 00E0FFFF\n", FP); /* through hole STD pin, all copper layers */

			current_pin_menu = pcb_netlist_find_net4pin(Layout, pin);
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				fprintf(FP, "Ne %d \"%s\"\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			} else {
				fprintf(FP, "Ne 0 \"\"\n"); /* unconnected pads have zero for net */
			} 
			/*
				pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
				fprintf(FP, " %s\n", F2S(pin, PCB_TYPE_PIN));
			*/
			fputs("$EndPAD\n",FP);
		}
		padlist_foreach(&element->Pad, &it, pad) {
			fputs("$PAD\n",FP);	 /* start pad descriptor for an smd pad */

			pcb_fprintf(FP, "Po %.0mk %.0mk\n", /* positions of pad */
									(pad->Point1.X + pad->Point2.X)/2- element->MarkX,
									(pad->Point1.Y + pad->Point2.Y)/2- element->MarkY);

			fputs("Sh ",FP); /* pin shape descriptor */
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pad->Number));
			fputs(" R ",FP); /* rectangular, not a pin */

			if ((pad->Point1.X-pad->Point2.X) <= 0
					&& (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.0mk %.0mk ",
										pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
										pad->Point2.Y-pad->Point1.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) <= 0
								 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.0mk %.0mk ",
										pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
										pad->Point1.Y-pad->Point2.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
								 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.0mk %.0mk ",
										pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
										pad->Point1.Y-pad->Point2.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
								 && (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.0mk %.0mk ",
										pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
										pad->Point2.Y-pad->Point1.Y + pad->Thickness); /* height */
			}

			fputs("0 0 0\n",FP); /* deltaX deltaY Orientation as float in decidegrees */

			fputs("Dr 0 0 0\n",FP); /* drill details; zero size; x,y pos vs pad location */

			fputs("At SMD N 00888000\n", FP); /* SMD pin, need to use right layer mask */

			current_pad_menu = pcb_netlist_find_net4pad(Layout, pad);
			if ((current_pad_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pad_menu) != PCB_NETLIST_INVALID_INDEX)) {
				fprintf(FP, "Ne %d \"%s\"\n", (1 + pcb_netlist_net_idx(Layout, current_pad_menu)), pcb_netlist_name(current_pad_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			} else {
				fprintf(FP, "Ne 0 \"\"\n"); /* a net number of 0 indicates an unconnected pad in pcbnew */
			} 

			fputs("$EndPAD\n",FP);

		}
		linelist_foreach(&element->Line, &it, line) {
			pcb_fprintf(FP, "DS %.0mk %.0mk %.0mk %.0mk %.0mk ",
									line->Point1.X - element->MarkX,
									line->Point1.Y - element->MarkY,
									line->Point2.X - element->MarkX,
									line->Point2.Y - element->MarkY,
									line->Thickness);
			fprintf(FP, "%d\n", silkLayer); /* an arbitrary Kicad layer, front silk, need to refine this */
		}

		arclist_foreach(&element->Arc, &it, arc) {

			pcb_arc_get_end(arc, 0, &arcStartX, &arcStartY);
			pcb_arc_get_end(arc, 1, &arcEndX, &arcEndY);

			if ((arc->Delta == 360.0) || (arc->Delta == -360.0)) { /* it's a circle */
				pcb_fprintf(FP, "DC %.0mk %.0mk %.0mk %.0mk %.0mk ",
										arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcStartX - element->MarkX, /* x on circle */
										arcStartY - element->MarkY, /* y on circle */
										arc->Thickness); /* stroke thickness */
			} else {
				pcb_fprintf(FP, "DA %.0mk %.0mk %.0mk %.0mk %mA %.0mk ",
										arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcEndX - element->MarkX, /* x on arc */
										arcEndY - element->MarkY, /* y on arc */
										arc->Delta, /* CW delta angle in decidegrees */
										arc->Thickness); /* stroke thickness */
			}
			fprintf(FP, "%d\n", silkLayer); /* and now append a suitable Kicad layer, front silk = 21, back silk 20 */
		}

		fprintf(FP, "$EndMODULE %s\n", currentElementName);
	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);

	return 0;
}


/* ---------------------------------------------------------------------------
 * writes polygon data in kicad legacy format for use in a layout .brd file
 */

int write_kicad_legacy_layout_polygons(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	int i, j;
	gdl_iterator_t it;
	pcb_polygon_t *polygon;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		int localFlag = 0;
		polylist_foreach(&layer->Polygon, &it, polygon) {
			if (polygon->HoleIndexN == 0) { /* no holes defined within polygon, which we implement support for first */

				/* preliminaries for zone settings */
				fputs("$CZONE_OUTLINE\n", FP);
				fputs("ZInfo 478E3FC8 0 \"\"\n", FP); /* use default empty netname, net 0, not connected */
				fprintf(FP,"ZLayer %d\n", currentLayer); 
				fprintf(FP,"ZAux %d E\n", polygon->PointN); /* corner count, use edge hatching for displaying the zone in pcbnew */
				fputs("ZClearance 200 X\n", FP); /* set pads/pins to not be connected to pours in the zone by default */
				fputs("ZMinThickness 190\n", FP); /* minimum copper thickness in zone, default setting */
				fputs("ZOptions 0 32 F 200 200\n", FP); /* solid fill, 32 segments per arc, antipad thickness, thermal stubs width(s) */

				/* now the zone outline is defined */

				for (i = 0, j=0; i < polygon->PointN; i++) {
					if (i == (polygon->PointN - 1) ) {
						j = 1; /* flags that this is the last vertex of the outline */
					}
					pcb_fprintf(FP, "ZCorner %.0mk %.0mk %d\n",
											polygon->Points[i].X + xOffset, polygon->Points[i].Y + yOffset, j);
				}

				/*
				  *   in here could go additional plolygon descriptors for holes removed from  the previously defined outer polygon
				  */ 

				fputs("$endCZONE_OUTLINE\n",  FP);

			} 
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}
