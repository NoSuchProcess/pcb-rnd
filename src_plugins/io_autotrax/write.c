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
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "../io_kicad/uniq_name.h"
#include "data.h"
#include "write.h"
#include "layer.h"
#include "const.h"
/*#include "netlist.h"*/
#include "obj_all.h"

#define F2S(OBJ, TYPE) pcb_strflg_f2s((OBJ)->Flags, TYPE)

/* writes the buffer to file - this can be abused to generate the library file for autotrax */
int io_autotrax_write_buffer(pcb_plug_io_t *ctx, FILE * FP, pcb_buffer_t *buff)
{
	/*fputs("io_kicad_legacy_write_buffer()", FP); */

	fputs("PCBNEW-LibModule-V1	jan 01 jan 2016 00:00:01 CET\n",FP);
	fputs("Units mm\n",FP);
	fputs("$INDEX\n",FP);
	io_kicad_legacy_write_element_index(FP, buff->Data);
	fputs("$EndINDEX\n",FP);

	pcb_write_element_data(FP, buff->Data, "kicadl");

	return (0);
}

/* ---------------------------------------------------------------------------
 * writes autotrax PCB to file
 */
int io_autotrax_write_pcb(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	/* this is the first step in exporting a layout; */

	/*fputs("io_autotrax_write_pcb()", FP);*/

	pcb_cardinal_t i;
	int autotraxLayerCount = 0;
	int physicalLayerCount = 0;
	int layer = 0;
	int currentAutotraxLayer = 0;
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

	/* autotrax expects layout dimensions to be specified in mils */
	int maxWidthMil = 32000;
	int maxHeightMil = 32000;

	fputs("PCB FILE 4\n",FP); /*autotrax header*/

	/* we sort out if the layout dimensions exceed the autotrax maxima */
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > maxWidthMil ||
			PCB_COORD_TO_MIL(PCB->MaxHeight) > maxHeightMil) {
		pcb_message(PCB_MSG_ERROR, "Layout size exceeds protel autotrax 32000 mil x 32000 mil maximum.");
		return -1;
	}

	/* here we count the copper layers to be exported to the autotrax file */
	physicalLayerCount = pcb_layergrp_list(PCB, PCB_LYT_COPPER, NULL, 0);

	if (physicalLayerCount > 6) {
		pcb_message(PCB_MSG_ERROR, "Warning: Physical layer count exceeds protel autotrax layer support for 6 layers.");
		/*return -1;*/
	}

	/* component "COMP" descriptions come next */

	write_autotrax_layout_elements(FP, PCB, PCB->Data, LayoutXOffset, LayoutYOffset);

	/* we now need to map pcb's layer groups onto the kicad layer numbers */
	currentAutotraxLayer = 0;
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

        if (innerCount > 4) {
                pcb_message(PCB_MSG_ERROR, "Warning: Inner layer count exceeds protel autotrax maximum of 4 inner layers.");
                /*return -1;*/
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

	/* we now proceed to write the outline tracks to the autotrax file, layer by layer */
	currentAutotraxLayer = 11; /* 11 is the "board layer" in autotrax */
	if (outlineCount > 0 )  {
		for (i = 0; i < outlineCount; i++) /* write top copper tracks, if any */
			{
				write_autotrax_layout_tracks(FP, currentAutotraxLayer, &(PCB->Data->Layer[outlineLayers[i]]),
										LayoutXOffset, LayoutYOffset);
				write_autotrax_layout_arcs(FP, currentAutotraxLayer, &(PCB->Data->Layer[outlineLayers[i]]),
										LayoutXOffset, LayoutYOffset);
			}
	} else { /* no outline layer per se, export the board margins instead...*/
				pcb_fprintf(FP, "FT\n%.0ml %.0ml %.0ml %.0ml 10 %d 1",
					0, 0, PCB->MaxWidth, 0);
				pcb_fprintf(FP, "FT\n%.0ml %.0ml %.0ml %.0ml 10 %d 1",
					PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
				pcb_fprintf(FP, "FT\n%.0ml %.0ml %.0ml %.0ml 10 %d 1",
					PCB->MaxWidth, PCB->MaxHeight, 0, PCB->MaxHeight);
				pcb_fprintf(FP, "FT\n%.0ml %.0ml %.0ml %.0ml 10 %d 1",
					0, PCB->MaxHeight, 0, 0);
	}

	/* we now proceed to write the bottom silk lines, arcs, text to the autotrax file, using layer 8 */
	currentAutotraxLayer = 8; /* 8 is the "bottom overlay" layer in autotrax */
	for (i = 0; i < bottomSilkCount; i++) /* write bottom silk lines, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the bottom copper features to the autorax file, layer by layer */
	currentAutotraxLayer = 6; /* 6 is the bottom layer in autotrax */
	for (i = 0; i < bottomCount; i++) /* write bottom copper tracks, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomLayers[i]]),
								LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomLayers[i]]),
								LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomLayers[i]]),
								LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
								LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the internal copper features to the autotrax file, layer by layer */
	if (innerCount > 0) {
		currentGroup = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for (i = 0, currentAutotraxLayer = 2; i < innerCount; i++) /* write inner copper text, group by group */
		{
			if (currentGroup != pcb_layer_get_group(PCB, innerLayers[i])) {
				currentGroup = pcb_layer_get_group(PCB, innerLayers[i]);
				currentAutotraxLayer++;
			} /* autotrax inner layers are layers 2 to 5 inclusive */
			write_autotrax_layout_tracks(FP, currentAutotraxLayer, &(PCB->Data->Layer[innerLayers[i]]),
								LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer, &(PCB->Data->Layer[topSilk[i]]),
								LayoutXOffset, LayoutYOffset);
                        write_autotrax_layout_text(FP, currentAutotraxLayer, &(PCB->Data->Layer[topSilk[i]]),
								LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
								LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the top copper features to the autotrax file, layer by layer */
	currentAutotraxLayer = 1; /* 1 is the top most copper layer in autotrax */
	for (i = 0; i < topCount; i++) /* write top copper features, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer, &(PCB->Data->Layer[topLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer, &(PCB->Data->Layer[topSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer, &(PCB->Data->Layer[topSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
						LayoutXOffset, LayoutYOffset);
		}

	/* we now proceed to write the top silk lines, arcs, text to the autotrax file, using layer 7*/
	currentAutotraxLayer = 7; /* 7 is the top silk layer in autotrax */
	for (i = 0; i < topSilkCount; i++) /* write top silk features, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer, &(PCB->Data->Layer[topSilk[i]]),
									LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer, &(PCB->Data->Layer[bottomSilk[i]]),
									LayoutXOffset, LayoutYOffset);
		}

	/* having done the graphical elements, we move onto vias */ 

	write_autotrax_layout_vias(FP, PCB->Data, LayoutXOffset, LayoutYOffset);

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

	/* last are the autotrax netlist descriptors */
	write_autotrax_equipotential_netlists(FP, PCB);

	return (0);
}

/* ---------------------------------------------------------------------------
 * writes (eventually) de-duplicated list of element names in kicad legacy format module $INDEX
 * which may be useful for a library export for autotrax
 */
int io_kicad_legacy_write_element_index(FILE * FP, pcb_data_t *Data)
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

int write_autotrax_layout_vias(FILE * FP, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	int viaDrillMil = 25; /* a reasonable default */
	/* write information about via */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "FV\n%.0ml %.0ml %.0mk %d\n", /* testing kicad printf */
				via->X + xOffset, via->Y + yOffset, via->Thickness, viaDrillMil);
	}
	return 0;
}

int write_autotrax_layout_tracks(FILE * FP, pcb_cardinal_t number,
		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->meta.real.name && *layer->meta.real.name)) {
		int localFlag = 0;
		int userRouted = 1;
		linelist_foreach(&layer->Line, &it, line) {
			pcb_fprintf(FP, "FT\n%.0ml %.0ml %.0ml %.0ml %ml %d %d\n",
				line->Point1.X + xOffset, line->Point1.Y + yOffset,
				line->Point2.X + xOffset, line->Point2.Y + yOffset,
				line->Thickness, currentLayer, userRouted);
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

int write_autotrax_layout_arcs(FILE * FP, pcb_cardinal_t number,
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
	if (!pcb_layer_is_empty_(PCB, layer) ){ /*|| (layer->meta.real.name && *layer->meta.real.name)) { */
		int localFlag = 0;
		int arcSegments = 0; /* 15 = circle, bit 1 = LUQ, bit 2 = LLQ, bit 3 = LRQ, bit 4 = URQ */ 
		arclist_foreach(&layer->Arc, &it, arc) {
			if (arc->Width > arc->Height) {
				radius = arc->Height;
			} else {
				radius = arc->Width;
			}
			if (arc->Delta == 360.0 || arc->Delta == -360.0 ) { /* it's a circle */
				arcSegments |= 0x1111;
			}
			if (arc->StartAngle == 0.0 && arc->Delta >= 90.0 ) {
				arcSegments |= 0x0010;
			}
                        if (arc->StartAngle == 0.0 && arc->Delta >= 90.0 ) {
				arcSegments |= 0x0010;
			}
			if (arc->StartAngle <= 90.0 && (arc->StartAngle + arc->Delta) >= 180.0 ) {
				arcSegments |= 0x0100;
			}
			if (arc->StartAngle <= 180.0 && (arc->StartAngle + arc->Delta) >= 270.0 ) {
				arcSegments |= 0x1000;
			}
			if (arc->StartAngle <= 270.0 && (arc->StartAngle + arc->Delta) >= 360.0 ) {
				arcSegments |= 0x0001;
			}
			/* may need to think about cases with negative Delta as well */
			/* consider exporting segments for arcs not made up of 90 degree quadrants */
			xStart = arc->X + xOffset;
			yStart = arc->Y + yOffset;

			pcb_fprintf(FP, "FA\n%.0ml %.0ml %.0ml %.0ml %ml %d %d\n",
                                xStart, yStart, radius, arcSegments,
                                arc->Thickness, currentLayer);

			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

int write_autotrax_layout_text(FILE * FP, pcb_cardinal_t number,
			 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mHeight = myfont->MaxHeight; /* autotrax needs the width of the widest letter */
	int autotraxMirrored = 0; /* 0 is not mirrored, +16  is mirrored */ 

	pcb_coord_t defaultStrokeThickness, strokeThickness, textHeight;
	int rotation;
	pcb_coord_t textOffsetX;
	pcb_coord_t textOffsetY;

	int localFlag;

	gdl_iterator_t it;
	pcb_text_t *text;
	pcb_cardinal_t currentLayer = number;

	int index = 0;

	defaultStrokeThickness = 200000;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) ) { /*|| (layer->meta.real.name && *layer->meta.real.name)) {*/
		localFlag = 0;
		textlist_foreach(&layer->Text, &it, text) {
			if (currentLayer < 9)  { /* copper or silk layer text */
				fputs("FT\n", FP);
				strokeThickness = PCB_SCALE_TEXT(defaultStrokeThickness, text->Scale /2);
				textHeight = PCB_SCALE_TEXT(mHeight, text->Scale);
				rotation = 0;
				if (currentLayer == 6 || currentLayer == 8) {/* back copper or silk */
					autotraxMirrored = 16; /* mirrored */
				}
				if (text->Direction == 3) { /*vertical down*/
					rotation = 3;
				} else if (text->Direction == 2)  { /*upside down*/
					rotation = 2;
				} else if (text->Direction == 1) { /*vertical up*/
					rotation = 1;
				} else if (text->Direction == 0)  { /*normal text*/
					rotation = 0;
				}
				pcb_fprintf(FP, "%.0ml %.0ml %.0ml %d %.0ml %d\n",
					text->X + xOffset, text->Y + yOffset, textHeight,
					rotation + autotraxMirrored, strokeThickness, currentLayer);
				for (index = 0; index < 32; index++) {
					if (text->TextString[index] == '\0') {
						index = 32;
					} else if (text->TextString[index] < 32
						|| text->TextString[index] > 126) {
						fputc(' ', FP); /* replace non alphanum with space */ 
					} else { /* need to truncate to 32 alphanumeric chars*/
						fputc(text->TextString[index], FP);
					}
				}
				pcb_fprintf(FP, "\n");
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
int io_autotrax_write_element(pcb_plug_io_t *ctx, FILE * FP, pcb_data_t *Data)
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

int write_autotrax_equipotential_netlists(FILE * FP, pcb_board_t *Layout)
{
	pcb_lib_menu_t *menu;
	pcb_lib_entry_t *netlist;
	
	/* now we step through any available netlists and generate descriptors */

	if (PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN) {
		int n, p;

		for (n = 0; n < PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN; n++) {
			pcb_lib_menu_t *menu = &PCB->NetlistLib[PCB_NETLIST_INPUT].Menu[n];
			fprintf(FP, "NETDEF\n");
			pcb_printf(FP, "%s\n", &menu->Name[2]);
			fprintf(FP, "(\n");
			for (p = 0; p < menu->EntryN; p++) {
				pcb_lib_entry_t *entry = &menu->Entry[p];
				pcb_printf(FP, "%s", entry->ListEntry);
			}
			fprintf(FP, ")\n");
		}
	}
	return 0;
}


/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a layout .brd file
 */
int write_autotrax_layout_elements(FILE * FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{

	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc rendering */
	pcb_coord_t xPos, yPos;

	pcb_element_t *element;
	pcb_lib_menu_t *current_pin_menu;
	pcb_lib_menu_t *current_pad_menu;

	int silkLayer = 7;  /* hard coded default, 7 is bottom silk */ 
	int copperLayer = 1; /* hard coded default, 1 is bottom copper */
	int padShape = 1; /* 1=circle, 2=Rectangle, 3=Octagonal, 4=Rounded Rectangle, 
			5=Cross Hair Target, 6=Moiro Target*/ 
	int drillHole = 0; /* for SMD */
	
	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;

		xPos = element->MarkX + xOffset;
		yPos = element->MarkY + yOffset;
		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element)) {
			silkLayer = 8;
			copperLayer = 6;
		} else {
			silkLayer = 7;
			copperLayer = 1;
		}

		fprintf(FP, "COMP\n%s\n", element->Name[PCB_ELEMNAME_IDX_REFDES].TextString);/* designator */
		fprintf(FP, "%s\n", element->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString);/* designator */
		fprintf(FP, "%s\n", element->Name[PCB_ELEMNAME_IDX_VALUE].TextString);/* designator */
		pcb_fprintf(FP, "%.0ml %.0ml 100 0 10 %d\n", /* designator */
			xPos, yPos, silkLayer);
		pcb_fprintf(FP, "%.0ml %.0ml 100 0 10 %d\n", /* pattern */
			xPos, yPos + 200, silkLayer);
		pcb_fprintf(FP, "%.0ml %.0ml 100 0 10 %d\n", /* comment field */
			xPos, yPos + 400, silkLayer);

		pinlist_foreach(&element->Pin, &it, pin) {

			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
 				padShape = 2;
			} else if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, pin)) {
				padShape = 3;
			} else {
                                padShape = 1; /* circular */
			}

			pcb_fprintf(FP, "CP\t%s\n%ml %ml %ml %ml %d %ml 1 %d\n",
				(char *) PCB_EMPTY(pin->Number), /* or ->Name? */
				pin->X - element->MarkX,
				pin->Y - element->MarkY,
				pin->Thickness, pin->Thickness, padShape,
				pin->DrillingHole, copperLayer);
		}
		padlist_foreach(&element->Pad, &it, pad) {
			padShape = 2; /* rectangular */

			pcb_fprintf(FP, "CP\n%ml %ml ", /* positions of pad */
				(pad->Point1.X + pad->Point2.X)/2- element->MarkX,
				(pad->Point1.Y + pad->Point2.Y)/2- element->MarkY);

			if ((pad->Point1.X-pad->Point2.X) <= 0
					&& (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%ml %ml ",
					pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
					pad->Point2.Y-pad->Point1.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) <= 0
					 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%ml %ml ",
					pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
					pad->Point1.Y-pad->Point2.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
					 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%ml %ml ",
					pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
					pad->Point1.Y-pad->Point2.Y + pad->Thickness); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
					 && (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%ml %ml ",
					pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
					pad->Point2.Y-pad->Point1.Y + pad->Thickness); /* height */
			}

			pcb_fprintf(FP, "%d %d 1 %d\n%s\n", padShape, drillHole,
					copperLayer, (char *) PCB_EMPTY(pad->Number)); /*or Name?*/ 

		}
		linelist_foreach(&element->Line, &it, line) { /* autotrax supports tracks in COMPs */
			pcb_fprintf(FP, "CT\n%.0mk %.0mk %.0mk %.0mk %.0mk %d 1\n",
					line->Point1.X - element->MarkX,
					line->Point1.Y - element->MarkY,
					line->Point2.X - element->MarkX,
					line->Point2.Y - element->MarkY,
					line->Thickness, silkLayer);
		}

		arclist_foreach(&element->Arc, &it, arc) {

			pcb_arc_get_end(arc, 0, &arcStartX, &arcStartY);
			pcb_arc_get_end(arc, 1, &arcEndX, &arcEndY);

			if ((arc->Delta >= 360.0) || (arc->Delta <= -360.0)) { /* it's a circle */
				pcb_fprintf(FP, "CA %.0ml %.0ml %.0ml %d %.0mk %d\n",
						arc->X - element->MarkX, /* x_1 centre */
						arc->Y - element->MarkY, /* y_2 centre */
						(arc->Height + arc->Width)/2, /* average */
						15, /* (1 & 2 & 4 & 8 = 15 = 4 segments = circle */
						arcStartY - element->MarkY, /* y on circle */
						arc->Thickness, silkLayer); /* stroke thickness,layer*/
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

		fprintf(FP, "ENDCOMP\n");
	}
	return 0;
}


/* ---------------------------------------------------------------------------
 * writes polygon data in autotrax fill (rectangle) format for use in a layout .PCB file
 */

int write_autotrax_layout_polygons(FILE * FP, pcb_cardinal_t number,
			pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	int i;
	gdl_iterator_t it;
	pcb_polygon_t *polygon;
	pcb_cardinal_t currentLayer = number;

	pcb_coord_t minx, miny, maxx, maxy;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) ) { /*|| (layer->meta.real.name && *layer->meta.real.name)) {*/
		int localFlag = 0;
		polylist_foreach(&layer->Polygon, &it, polygon) {

			minx = maxx = polygon->Points[0].X;
			miny = maxy = polygon->Points[0].Y;

			/* now the fill zone outline is defined by a rectangle enclosing the poly*/
			for (i = 0; i < polygon->PointN; i++) {
				if (minx > polygon->Points[i].X) {
					minx = polygon->Points[i].X;
				}
				if (maxx < polygon->Points[i].X) {
					maxx = polygon->Points[i].X;
				}
				if (miny > polygon->Points[i].Y) {
					miny = polygon->Points[i].Y;
				}
				if (maxy < polygon->Points[i].Y) {
					maxy = polygon->Points[i].Y;
				}
			}
			pcb_fprintf(FP, "FF\n%.0ml %.0ml %.0ml %.0ml %d\n",
                                        minx + xOffset, miny + yOffset,
					maxx + xOffset, maxy + yOffset, currentLayer);

			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}
