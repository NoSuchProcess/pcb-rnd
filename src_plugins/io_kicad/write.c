/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016 Erich S. Heinzle
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
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "uniq_name.h"
#include "data.h"
#include "write.h"
#include "layer.h"
#include "const.h"
#include "netlist.h"
#include "obj_all.h"

#define F2S(OBJ, TYPE) pcb_strflg_f2s((OBJ)->Flags, TYPE)

/* layer "0" is first copper layer = "0. Back - Solder"
 * and layer "15" is "15. Front - Component"
 * and layer "20" SilkScreen Back
 * and layer "21" SilkScreen Front
 */

/* generates a line by line listing of the elements being saved */
static int io_kicad_write_element_index(FILE * FP, pcb_data_t *Data);

/* generates text for the kicad layer provided  */
static char * kicad_sexpr_layer_to_text(int layer);

/* generates a default via drill size for the layout */
static int write_kicad_layout_via_drill_size(FILE * FP, pcb_cardinal_t indentation);

/* writes the buffer to file */
int io_kicad_write_buffer(pcb_plug_io_t *ctx, FILE * FP, pcb_buffer_t *buff)
{
	/*fputs("io_kicad_legacy_write_buffer()", FP); */

	fputs("PCBNEW-LibModule-V1	jan 01 jan 2016 00:00:01 CET\n",FP);
	fputs("Units mm\n",FP);
	fputs("$INDEX\n",FP);
	io_kicad_write_element_index(FP, buff->Data);
	fputs("$EndINDEX\n",FP);

	/* WriteViaData(FP, buff->Data); */

	pcb_write_element_data(FP, buff->Data, "kicadl");

	/*
		for (i = 0; i < pcb_max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(buff->Data->Layer[i]));
	*/
	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int io_kicad_write_pcb(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	/* this is the first step in exporting a layout;
	 * creating a kicad module containing the elements used in the layout
	 */

	/*fputs("io_kicad_legacy_write_pcb()", FP);*/

	int baseSExprIndent = 2;

	pcb_cardinal_t i;
	int physicalLayerCount = 0;
	int kicadLayerCount = 0;
	int silkLayerCount= 0;
	int outlineLayerCount = 0;
	int layer = 0;
	int currentKicadLayer = 0;
	int currentGroup = 0;
	pcb_coord_t outlineThickness = PCB_MIL_TO_COORD(10); 

	int bottomCount;
	int *bottomLayers;
	int innerCount;
	int *innerLayers;
	int topCount;
	int *topLayers;
	int bottomSilkCount;
	int *bottomSilk;
	int topSilkCount;
	int *topSilk;
	int outlineCount;
	int *outlineLayers;

	pcb_coord_t LayoutXOffset;
	pcb_coord_t LayoutYOffset;

	/* Kicad expects a layout "sheet" size to be specified in mils, and A4, A3 etc... */
	int A4HeightMil = 8267;
	int A4WidthMil = 11700;
	int sheetHeight = A4HeightMil;
	int sheetWidth = A4WidthMil;
	int paperSize = 4; /* default paper size is A4 */

	fputs("(kicad_pcb (version 3) (host pcbnew \"(2013-02-20 BZR 3963)-testing\")",FP);

	fprintf(FP, "\n%*s(general\n", baseSExprIndent, "");
	fprintf(FP, "%*s)\n", baseSExprIndent, "");


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
	fprintf(FP, "\n%*s(page A%d)\n", baseSExprIndent, "", paperSize);


	/* we now sort out the offsets for centring the layout in the chosen sheet size here */
	if (sheetWidth > PCB_COORD_TO_MIL(PCB->MaxWidth)) {	 /* usually A4, bigger if needed */
		/* fprintf(FP, "%d ", sheetWidth);  legacy kicad: elements decimils, sheet size mils */
		LayoutXOffset = PCB_MIL_TO_COORD(sheetWidth)/2 - PCB->MaxWidth/2;
	} else { /* the layout is bigger than A0; most unlikely, but... */
		/* pcb_fprintf(FP, "%.0ml ", PCB->MaxWidth); */
		LayoutXOffset = 0;
	}
	if (sheetHeight > PCB_COORD_TO_MIL(PCB->MaxHeight)) {
		/* fprintf(FP, "%d", sheetHeight); */
		LayoutYOffset = PCB_MIL_TO_COORD(sheetHeight)/2 - PCB->MaxHeight/2;
	} else { /* the layout is bigger than A0; most unlikely, but... */
		/* pcb_fprintf(FP, "%.0ml", PCB->MaxHeight); */
		LayoutYOffset = 0;
	}

	/* here we define the copper layers in the exported kicad file */
	physicalLayerCount = pcb_layer_group_list(PCB_LYT_COPPER, NULL, 0);

	fprintf(FP, "\n%*s(layers\n", baseSExprIndent, "");
	kicadLayerCount = physicalLayerCount;
	if (kicadLayerCount%2 == 1) {
		kicadLayerCount++; /* kicad doesn't like odd numbers of layers, has been deprecated for some time apparently */
	}
	
	layer = 0;
	if (physicalLayerCount >= 1) {
		fprintf(FP, "%*s(%d bottom_side.Cu signal)\n", baseSExprIndent + 2, "", layer);
	}
	if (physicalLayerCount > 1) { /* seems we need to ignore layers > 16 due to kicad limitation */
		for (layer = 1; (layer < (kicadLayerCount - 1)) && (layer < 15); layer++ ) {
			fprintf(FP, "%*s(%d Inner%d.Cu signal)\n", baseSExprIndent + 2, "", layer, layer);
		}
		fprintf(FP, "%*s(15 top_side.Cu signal)\n", baseSExprIndent + 2, "");	
	}
	fprintf(FP, "%*s(20 B.SilkS user)\n", baseSExprIndent + 2, "");
	fprintf(FP, "%*s(21 F.SilkS user)\n", baseSExprIndent + 2, "");
	fprintf(FP, "%*s(28 Edge.Cuts user)\n", baseSExprIndent + 2, "");
	fprintf(FP, "%*s)\n", baseSExprIndent, "");

	/* setup section */
	fprintf(FP, "\n%*s(setup\n", baseSExprIndent, "");
	write_kicad_layout_via_drill_size(FP, baseSExprIndent + 2);
	fprintf(FP, "%*s)\n", baseSExprIndent, "");

	/* now come the netlist "equipotential" descriptors */

	write_kicad_equipotential_netlists(FP, PCB, baseSExprIndent);

	/* module descriptions come next */

	fputs("\n", FP);
	write_kicad_layout_elements(FP, PCB, PCB->Data, LayoutXOffset, LayoutYOffset, baseSExprIndent);

	/* we now need to map pcb's layer groups onto the kicad layer numbers */
	currentKicadLayer = 0;
	currentGroup = 0;

	/* figure out which pcb layers are bottom copper and make a list */
	bottomLayers = malloc(sizeof(int) * physicalLayerCount);
	/*int bottomLayers[physicalLayerCount];*/
	bottomCount = pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, NULL, 0);
	pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, bottomLayers, physicalLayerCount);

	/* figure out which pcb layers are internal copper layers and make a list */
	innerLayers = malloc(sizeof(int) * physicalLayerCount);
	/*int innerLayers[physicalLayerCount];*/
	innerCount = pcb_layer_list(PCB_LYT_INTERN | PCB_LYT_COPPER, NULL, 0);
	pcb_layer_list(PCB_LYT_INTERN | PCB_LYT_COPPER, innerLayers, physicalLayerCount);

	/* figure out which pcb layers are top copper and make a list */
	topLayers = malloc(sizeof(int) * physicalLayerCount);
	/*int topLayers[physicalLayerCount];*/
	topCount = pcb_layer_list(PCB_LYT_TOP | PCB_LYT_COPPER, NULL, 0);
	pcb_layer_list(PCB_LYT_TOP | PCB_LYT_COPPER, topLayers, physicalLayerCount);

	silkLayerCount = pcb_layer_group_list(PCB_LYT_SILK, NULL, 0);

	/* figure out which pcb layers are bottom silk and make a list */
	bottomSilk = malloc(sizeof(int) * silkLayerCount);
	/*int bottomSilk[silkLayerCount];*/
	bottomSilkCount = pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, NULL, 0);
	pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, bottomSilk, silkLayerCount);

	/* figure out which pcb layers are top silk and make a list */
	topSilk = malloc(sizeof(int) * silkLayerCount);
	/*int topSilk[silkLayerCount];*/
	topSilkCount = pcb_layer_list(PCB_LYT_TOP | PCB_LYT_SILK, NULL, 0);
	pcb_layer_list(PCB_LYT_TOP | PCB_LYT_SILK, topSilk, silkLayerCount);

	/* here we count outline layers */
	outlineLayerCount = pcb_layer_group_list(PCB_LYT_OUTLINE, NULL, 0);

	/* figure out which pcb layers are outlines and make a list */
	outlineLayers = malloc(sizeof(int) * outlineLayerCount);
	outlineCount = pcb_layer_list(PCB_LYT_OUTLINE, NULL, 0);
	pcb_layer_list(PCB_LYT_OUTLINE, outlineLayers, outlineCount);


	/* we now proceed to write the bottom silk lines, arcs, text to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for (i = 0; i < bottomSilkCount; i++) /* write bottom silk lines, if any */
		{
			write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the bottom copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for (i = 0; i < bottomCount; i++) /* write bottom copper tracks, if any */
		{
			write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}	/* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper text to the kicad file, layer by layer */
	if (innerCount != 0) {
		currentGroup = pcb_layer_lookup_group(innerLayers[0]);
	}
	for (i = 0, currentKicadLayer = 1; i < innerCount; i++) /* write inner copper text, group by group */
		{
			if (currentGroup != pcb_layer_lookup_group(innerLayers[i])) {
				currentGroup = pcb_layer_lookup_group(innerLayers[i]);
				currentKicadLayer++;
				if (currentKicadLayer > 14) {
					currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
				}
			}
			write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the top copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for (i = 0; i < topCount; i++) /* write top copper tracks, if any */
		{
			write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the top silk lines, arcs, text to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for (i = 0; i < topSilkCount; i++) /* write top silk lines, if any */
		{
			write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* having done the graphical elements, we move onto tracks and vias */ 

	fputs("\n",FP); /* move onto tracks and vias */
	write_kicad_layout_vias(FP, PCB->Data, LayoutXOffset, LayoutYOffset, baseSExprIndent);

	/* we now proceed to write the bottom copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for (i = 0; i < bottomCount; i++) /* write bottom copper tracks, if any */
		{
			write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}	/* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper tracks to the kicad file, layer by layer */
	if (innerCount != 0) {
		currentGroup = pcb_layer_lookup_group(innerLayers[0]);
	}
	for (i = 0, currentKicadLayer = 1; i < innerCount; i++) /* write inner copper tracks, group by group */
		{
			if (currentGroup != pcb_layer_lookup_group(innerLayers[i])) {
				currentGroup = pcb_layer_lookup_group(innerLayers[i]);
				currentKicadLayer++;
				if (currentKicadLayer > 14) {
					currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
				}
			}
			write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the top copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for (i = 0; i < topCount; i++) /* write top copper tracks, if any */
		
		{
			write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the outline tracks to the kicad file, layer by layer */
	currentKicadLayer = 28; /* 28 is the edge cuts layer in kicad */
	if (outlineCount != 0) {
		for (i = 0; i < outlineCount; i++) /* write outline tracks, if any */
			{
				write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]),
										LayoutXOffset, LayoutYOffset, baseSExprIndent);
				write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]),
										LayoutXOffset, LayoutYOffset, baseSExprIndent);
			}
	} else { /* no outline layer per se, export the board margins instead  - obviously some scope to reduce redundant code...*/
				fprintf(FP, "%*s", baseSExprIndent, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
				fprintf(FP, "%*s", baseSExprIndent, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
				fprintf(FP, "%*s", baseSExprIndent, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										PCB->MaxWidth/2 + LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
				fprintf(FP, "%*s", baseSExprIndent, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 + LayoutYOffset,
										PCB->MaxWidth/2 - LayoutXOffset, PCB->MaxHeight/2 - LayoutYOffset,
										kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
	}

	fputs("\n",FP); /* finished  tracks and vias */

	/*
	  * now we proceed to write polygons for each layer, and iterate much like we did for tracks
         */

	/* we now proceed to write the bottom silk polygons  to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for (i = 0; i < bottomSilkCount; i++) /* write bottom silk polygons, if any */
		{
			write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the bottom copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for (i = 0; i < bottomCount; i++) /* write bottom copper polygons, if any */
		{
			write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}	/* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper polygons to the kicad file, layer by layer */
	if (innerCount != 0) {
		currentGroup = pcb_layer_lookup_group(innerLayers[0]);
	}
	for (i = 0, currentKicadLayer = 1; i < innerCount; i++) /* write inner copper polygons, group by group */
		{
			if (currentGroup != pcb_layer_lookup_group(innerLayers[i])) {
				currentGroup = pcb_layer_lookup_group(innerLayers[i]);
				currentKicadLayer++;
				if (currentKicadLayer > 14) {
					currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
				}
			}
			write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the top copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for (i = 0; i < topCount; i++) /* write top copper polygons, if any */
		{
			write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	/* we now proceed to write the top silk polygons to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for (i = 0; i < topSilkCount; i++) /* write top silk polygons, if any */
		{
			write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}

	fputs(")\n",FP); /* finish off the board */

	/* now free memory from arrays that were used */
	free(bottomLayers);
	free(innerLayers);
	free(topLayers);
	free(topSilk);
	free(bottomSilk);
	free(outlineLayers);
	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes (eventually) de-duplicated list of element names in kicad legacy format module $INDEX
 */
static int io_kicad_write_element_index(FILE * FP, pcb_data_t *Data)
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


int write_kicad_layout_vias(FILE * FP, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		fprintf(FP, "%*s", indentation,"");
		pcb_fprintf(FP, "(via (at %.3mm %.3mm) (size %.3mm) (layers %s %s))\n", 
								via->X + xOffset, via->Y + yOffset, via->Thickness,
								kicad_sexpr_layer_to_text(0), kicad_sexpr_layer_to_text(15)); /* skip (net 0) for now */
	}
	return 0;
}

static char * kicad_sexpr_layer_to_text(int layer)
{
	switch (layer) {
		case 0:
			return "bottom_side.Cu";
		case 1:
			return "Inner1.Cu";
		case 2:
			return "Inner2.Cu";
		case 3:
			return "Inner3.Cu";
		case 4:
			return "Inner4.Cu";
		case 5:
			return "Inner5.Cu";
		case 6:
			return "Inner6.Cu";
		case 7:
			return "Inner7.Cu";
		case 8:
			return "Inner8.Cu";
		case 9:
			return "Inner9.Cu";
		case 10:
			return "Inner10.Cu";
		case 11:
			return "Inner11.Cu";
		case 12:
			return "Inner12.Cu";
		case 13:
			return "Inner13.Cu";
		case 14:
			return "Inner14.Cu";
		case 15:
			return "top_side.Cu";
		case 20:
			return "B.SilkS";
		case 21:
			return "F.SilkS";
		case 28:
			return "Edge.Cuts"; /* kicad's outline layer */
		default:
			return "";
	}
}

static int write_kicad_layout_via_drill_size(FILE * FP, pcb_cardinal_t indentation)
{
	fprintf(FP, "%*s", indentation,"");
	pcb_fprintf(FP, "(via_drill 0.635)\n"); /* mm format, default for now, ~= 0.635mm */
	return 0;
}

int write_kicad_layout_tracks(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!LAYER_IS_PCB_EMPTY(layer) || (layer->Name && *layer->Name)) {
		/*
			fprintf(FP, "Layer(%i ", (int) Number + 1);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->Name));
			fputs(")\n(\n", FP);
			WriteAttributeList(FP, &layer->Attributes, "\t");
		*/
		int localFlag = 0;
		linelist_foreach(&layer->Line, &it, line) {
			if ((currentLayer < 16) || (currentLayer == 28)) { /* a copper line i.e. track, or an outline edge cut */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										line->Point1.X + xOffset, line->Point1.Y + yOffset,
										line->Point2.X + xOffset, line->Point2.Y + yOffset,
										kicad_sexpr_layer_to_text(currentLayer), line->Thickness); /* neglect (net ___ ) for now */
			} else if ((currentLayer == 20) || (currentLayer == 21)  || (currentLayer == 28)) { /* a silk line or outline line */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(gr_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										line->Point1.X + xOffset, line->Point1.Y + yOffset,
										line->Point2.X + xOffset, line->Point2.Y + yOffset,
										kicad_sexpr_layer_to_text(currentLayer), line->Thickness);
			}
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

int write_kicad_layout_arcs(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	gdl_iterator_t it;
	pcb_arc_t *arc;
	pcb_arc_t localArc; /* for converting ellipses to circular arcs */
	pcb_box_t *boxResult; /* for figuring out arc ends */
	pcb_cardinal_t currentLayer = number;
	pcb_coord_t radius, xStart, yStart, xEnd, yEnd;
	int copperStartX; /* used for mapping geda copper arcs onto kicad copper lines */
	int copperStartY; /* used for mapping geda copper arcs onto kicad copper lines */

	/* write information about non empty layers */
	if (!LAYER_IS_PCB_EMPTY(layer) || (layer->Name && *layer->Name)) {
		/*
			fprintf(FP, "Layer(%i ", (int) Number + 1);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->Name));
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
		boxResult = pcb_arc_get_ends(&localArc);
			if (arc->Delta == 360.0 || arc->Delta == -360.0 ) { /* it's a circle */
				kicadArcShape = 3;
			} else { /* it's an arc */
				kicadArcShape = 2;
			}
			xStart = localArc.X + xOffset;
			yStart = localArc.Y + yOffset;
			xEnd = boxResult->X2 + xOffset; 
			yEnd = boxResult->Y2 + yOffset; 
			copperStartX = boxResult->X1 + xOffset;
			copperStartY = boxResult->Y1 + yOffset; 
			if ((currentLayer < 16) || (currentLayer == 28)) { /* a copper arc, i.e. track, or edge cut, is unsupported by kicad, and will be exported as a line */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										copperStartX, copperStartY, xEnd, yEnd,
										kicad_sexpr_layer_to_text(currentLayer), arc->Thickness); /* neglect (net ___ ) for now */
			} else if ((currentLayer == 20) || (currentLayer == 21)) { /* a silk arc, or outline */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(gr_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n",
										xStart, yStart, xEnd, yEnd, arc->Delta,
										kicad_sexpr_layer_to_text(currentLayer), arc->Thickness);
			}
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

int write_kicad_layout_text(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	pcb_font_t *myfont = &PCB->Font;
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
	if (!LAYER_IS_PCB_EMPTY(layer) || (layer->Name && *layer->Name)) {
		/*
			fprintf(FP, "Layer(%i ", (int) Number + 1);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->Name));
			fputs(")\n(\n", FP);
			WriteAttributeList(FP, &layer->Attributes, "\t");
		*/
		localFlag = 0;
		textlist_foreach(&layer->Text, &it, text) {
			if ((currentLayer < 16) || (currentLayer == 20) || (currentLayer == 21) ) { /* copper or silk layer text */
				fprintf(FP, "%*s(gr_text \"%s\" ", indentation, "", text->TextString);
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
				printf("textOffsetX: %d,  textOffsetY: %d\n", textOffsetX, textOffsetY);     TODO need to sort out rotation */
				pcb_fprintf(FP, "(at %.3mm %.3mm", text->X + xOffset + textOffsetX, text->Y + yOffset + textOffsetY);
				if (rotation != 0) {
					fprintf(FP, " %d", rotation/10); /* convert decidegrees to degrees */
				}
				pcb_fprintf(FP, ") (layer %s)\n", kicad_sexpr_layer_to_text(currentLayer));
				fprintf(FP, "%*s", indentation +2,"");
				pcb_fprintf(FP, "(effects (font (size %.3mm %.3mm) (thickness %.3mm))", defaultXSize, defaultYSize, strokeThickness); /* , rotation */
				if (kicadMirrored  == 0) {
					fprintf(FP, " (justify mirror)");
				}
				fprintf(FP, ")\n%*s)\n", indentation,"");
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
int io_kicad_write_element(pcb_plug_io_t *ctx, FILE * FP, pcb_data_t *Data)
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
	pcb_box_t *boxResult;

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
			 pcb_print_quoted_string(FP, (char *) PCB_EMPTY(DESCRIPTION_NAME(element)));
			 fputc(' ', FP);
			 pcb_print_quoted_string(FP, (char *) PCB_EMPTY(NAMEONPCB_NAME(element)));
			 fputc(' ', FP);
			 pcb_print_quoted_string(FP, (char *) PCB_EMPTY(VALUE_NAME(element)));
			 pcb_fprintf(FP, " %mm %mm %mm %mm %d %d %s]\n(\n",
			 element->MarkX, element->MarkY,
			 DESCRIPTION_TEXT(element).X - element->MarkX,
			 DESCRIPTION_TEXT(element).Y - element->MarkY,
			 DESCRIPTION_TEXT(element).Direction,
			 DESCRIPTION_TEXT(element).Scale, F2S(&(DESCRIPTION_TEXT(element)), PCB_TYPE_ELEMENT_NAME));

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
			boxResult = pcb_arc_get_ends(arc);
			arcStartX = boxResult->X1;
			arcStartY = boxResult->Y1;
			arcEndX = boxResult->X2; 
			arcEndY = boxResult->Y2; 
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

int write_kicad_equipotential_netlists(FILE * FP, pcb_board_t *Layout, pcb_cardinal_t indentation)
{
        int n; /* code mostly lifted from netlist.c */ 
	int netNumber;
	pcb_lib_menu_t *menu;
	pcb_lib_entry_t *netlist;
	
	/* first we write a default netlist for the 0 net, which is for unconnected pads in pcbnew */
	fprintf(FP, "\n%*s(net 0 \"\")\n", indentation, "");

	/* now we step through any available netlists and generate descriptors */
        for (n = 0, netNumber = 1; n < Layout->NetlistLib[PCB_NETLIST_EDITED].MenuN; n++, netNumber ++) {
                menu = &Layout->NetlistLib[PCB_NETLIST_EDITED].Menu[n];
		netlist = &menu->Entry[0];
		if (netlist != NULL) {
			fprintf(FP, "%*s(net %d %s)\n", indentation, "", netNumber, pcb_netlist_name(menu));  /* netlist 0 was used for unconnected pads  */
                }
        }
	return 0;
}

/* may need to export a netclass or two 
(net_class Default "Ceci est la Netclass par dÃ©faut"
(clearance 0.254)
(trace_width 0.254)
(via_dia 0.889)
(via_drill 0.635)
(uvia_dia 0.508)
(uvia_drill 0.127)
(add_net "")
*/


/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a layout .brd file
 */
int write_kicad_layout_elements(FILE * FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{

	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc rendering */
	pcb_coord_t xPos, yPos;

	pcb_element_t *element;
	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	const char * currentElementName;
	const char * currentElementRef;
	const char * currentElementVal;

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
		if (currentElementName == NULL) {
			currentElementName = "unknown";
		}
		currentElementRef = element->Name[NAMEONPCB_INDEX].TextString;
		if (currentElementRef == NULL) {
			currentElementRef = "unknown";
		}
		currentElementVal = element->Name[VALUE_INDEX].TextString;
		if (currentElementVal == NULL) {
			currentElementVal = "unknown";
		}

		fprintf(FP, "%*s", indentation, "");
		fprintf(FP,  "(module %s (layer %s) (tedit 4E4C0E65) (tstamp 5127A136)\n",
								currentElementName, kicad_sexpr_layer_to_text(copperLayer));
		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(at %.3mm %.3mm)\n", xPos, yPos);

		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(fp_text reference %s (at 0 0.127) (layer %s)\n",
								currentElementRef, kicad_sexpr_layer_to_text(silkLayer));
		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(fp_text value %s (at 0 0.127) (layer %s)\n",
								currentElementVal, kicad_sexpr_layer_to_text(silkLayer));
		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

		linelist_foreach(&element->Line, &it, line) {
			fprintf(FP, "%*s", indentation + 2, "");
			pcb_fprintf(FP, "(fp_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
									line->Point1.X - element->MarkX,
									line->Point1.Y - element->MarkY,
									line->Point2.X - element->MarkX,
									line->Point2.Y - element->MarkY,
									kicad_sexpr_layer_to_text(silkLayer),
									line->Thickness);
		}

		arclist_foreach(&element->Arc, &it, arc) {

			pcb_box_t *boxResult = pcb_arc_get_ends(arc);
			arcStartX = boxResult->X1;
			arcStartY = boxResult->Y1;
			arcEndX = boxResult->X2; 
			arcEndY = boxResult->Y2; 

			if ((arc->Delta == 360.0) || (arc->Delta == -360.0)) { /* it's a circle */
				fprintf(FP, "%*s", indentation +2, "");
				pcb_fprintf(FP, "(fp_circle (center %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcStartX - element->MarkX, /* x on circle */
										arcStartY - element->MarkY, /* y on circle */
										kicad_sexpr_layer_to_text(silkLayer), arc->Thickness);  /* stroke thickness */
			} else {
				fprintf(FP, "%*s", indentation +2, "");
				pcb_fprintf(FP, "(fp_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n",
										arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcEndX - element->MarkX, /* x on arc */
										arcEndY - element->MarkY, /* y on arc */
										arc->Delta, /* CW delta angle in decidegrees */
										kicad_sexpr_layer_to_text(silkLayer), arc->Thickness);  /* stroke thickness */
			}
		}


		pinlist_foreach(&element->Pin, &it, pin) {
			fprintf(FP, "%*s", indentation + 2, "");
			fputs("(pad ", FP);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number));
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
				fputs(" thru_hole rect ",FP); /* square */
			} else {
				fputs(" thru_hole circle ",FP); /* circular */
			}
			pcb_fprintf(FP, "(at %.3mm %.3mm)", /* positions of pad */
									pin->X - element->MarkX,
									pin->Y - element->MarkY);
			/* pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number)); */
			pcb_fprintf(FP, " (size %.3mm %.3mm)", pin->Thickness, pin->Thickness); /* height = width */
			/* drill details; size */
			pcb_fprintf(FP, " (drill %.3mm)\n", pin->DrillingHole);
			fprintf(FP, "%*s", indentation + 4, "");
			fprintf(FP, "(layers *.Cu *.Mask)\n"); /* define included layers for pin */
			current_pin_menu = pcb_netlist_find_net4pin(Layout, pin);
			fprintf(FP, "%*s", indentation + 4, "");
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				fprintf(FP, "(net %d \"%s\")\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			} else {
				fprintf(FP, "(net 0 \"\")\n"); /* unconnected pads have zero for net */
			}
			fprintf(FP, "%*s)\n", indentation + 2, "");
			/*
				pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
				fprintf(FP, " %s\n", F2S(pin, PCB_TYPE_PIN));
			*/
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

			fprintf(FP, "%*s)\n", indentation + 2, "");

		}

		fprintf(FP, "%*s)\n\n", indentation, ""); /*  finish off module */
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

int write_kicad_layout_polygons(FILE * FP, pcb_cardinal_t number,
																		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	int i, j;
	gdl_iterator_t it;
	pcb_polygon_t *polygon;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!LAYER_IS_PCB_EMPTY(layer) || (layer->Name && *layer->Name)) {
		int localFlag = 0;
		polylist_foreach(&layer->Polygon, &it, polygon) {
			if (polygon->HoleIndexN == 0) { /* no holes defined within polygon, which we implement support for first */

				/* preliminaries for zone settings */

				fprintf(FP, "%*s(zone (net 0) (net_name \"\") (layer %s) (tstamp 478E3FC8) (hatch edge 0.508)\n", indentation, "", kicad_sexpr_layer_to_text(currentLayer));
				fprintf(FP, "%*s(connect_pads no (clearance 0.508))\n", indentation+2, "");
				fprintf(FP, "%*s(min_thickness 0.4826)\n", indentation+2, "");
				fprintf(FP, "%*s(fill (arc_segments 32) (thermal_gap 0.508) (thermal_bridge_width 0.508))\n", indentation+2, "");
				fprintf(FP, "%*s(polygon\n", indentation+2, "");
				fprintf(FP, "%*s(pts\n", indentation+4, "");

				/* now the zone outline is defined */

				for (i = 0; i < polygon->PointN; i = i + 5) { /* kicad exports five coords per line in s-expr files */
					fprintf(FP, "%*s", indentation + 6, ""); /* pcb_fprintf does not support %*s   */
					for (j = 0; (j < polygon->PointN) && (j < 5); j++) { 
						pcb_fprintf(FP, "(xy %.3mm %.3mm)", polygon->Points[i + j].X + xOffset, polygon->Points[i+ j].Y + yOffset);
						if ((j < 4) && ((i + j) < (polygon->PointN - 1))) {
							fputs(" ", FP);
						}
					}
					fputs("\n", FP);
				}
				fprintf(FP, "%*s)\n", indentation+4, "");
				fprintf(FP, "%*s)\n", indentation+2, "");
				fprintf(FP, "%*s)\n", indentation, ""); /* end zone */
				/*
				  *   in here could go additional polygon descriptors for holes removed from  the previously defined outer polygon
				  */ 

			} 
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}
