/*
 *				COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016, 2017 Erich S. Heinzle
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
#include "data.h"
#include "write.h"
#include "layer.h"
#include "const.h"
#include "obj_all.h"

#define F2S(OBJ, TYPE) pcb_strflg_f2s((OBJ)->Flags, TYPE)

/* ---------------------------------------------------------------------------
 * writes autotrax vias to file
 */
int write_autotrax_layout_vias(FILE * FP, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	int viaDrillMil = 25; /* a reasonable default */
	/* write information about via */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "FV\n%.0ml %.0ml %.0ml %d\n", 
				via->X + xOffset, PCB->MaxHeight - (via->Y + yOffset),
				via->Thickness, viaDrillMil);
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes generic autotrax track descriptor line for components and layouts 
 */
int write_autotrax_track(FILE * FP, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_line_t *line, pcb_cardinal_t layer)
{
	int userRouted = 1;
			pcb_fprintf(FP, "%.0ml %.0ml %.0ml %.0ml %.0ml %d %d\n",
				line->Point1.X + xOffset, PCB->MaxHeight - (line->Point1.Y + yOffset),
				line->Point2.X + xOffset, PCB->MaxHeight - (line->Point2.Y + yOffset),
				line->Thickness, layer, userRouted);
	return 0;
}

/* -------------------------------------------------------------------------------
 * generates an autotrax arc "segments" value to approximate an arc being exported  
 */
int pcb_rnd_arc_to_autotrax_segments(pcb_angle_t arcStart, pcb_angle_t arcDelta)
{
	int arcSegments = 0; /* start with no arc segments */
	/* 15 = circle, bit 1 = LUQ, bit 2 = LLQ, bit 3 = LRQ, bit 4 = URQ */
	if (arcDelta == -360 ) { /* it's a circle */	
		arcDelta = 360;
	}
	if (arcDelta < 0 ) {
		arcDelta = -arcDelta;
		arcStart -= arcDelta;
	}

	while (arcStart < 0) {
		arcStart += 360;
	}
	while (arcStart > 360) {
		arcStart -= 360;
	}
	/* pcb_printf("Arc start: %ma, Arc delta: %ma\n", arcStart, arcDelta); */
	if (arcDelta >= 360) { /* it's a circle */
		arcSegments |= 0x0F;
	} else {
		if (arcStart <= 0.0 && (arcStart + arcDelta) >= 90.0 ) {
			arcSegments |= 0x04; /* LLQ */
		}
		if (arcStart <= 90.0 && (arcStart + arcDelta) >= 180.0 ) {
			arcSegments |= 0x08; /* LRQ */
		}
		if (arcStart <= 180.0 && (arcStart + arcDelta) >= 270.0 ) {
			arcSegments |= 0x01; /* URQ */
		}
		if (arcStart <= 270.0 && (arcStart + arcDelta) >= 360.0 ) {
			arcSegments |= 0x02; /* ULQ */
		}
	}	
	return arcSegments;
}

/* ---------------------------------------------------------------------------
 * writes generic autotrax arc descriptor line for components and layouts 
 */
int write_autotrax_arc(FILE *FP, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_arc_t * arc, int currentLayer) 
{
	pcb_coord_t radius;
			if (arc->Width > arc->Height) {
				radius = arc->Height;
			} else {
				radius = arc->Width;
			}
			pcb_fprintf(FP, "%.0ml %.0ml %.0ml %d %.0ml %d\n",
				arc->X + xOffset, PCB->MaxHeight - (arc->Y + yOffset), radius,
				pcb_rnd_arc_to_autotrax_segments(arc->StartAngle, arc->Delta),
				arc->Thickness, currentLayer);
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes netlist data in autotrax format
 */
int write_autotrax_equipotential_netlists(FILE * FP, pcb_board_t *Layout)
{
	int showStatus = 0;
	/* show status can be 0 or 1 for a net:
	   0 hide rats nest
	   1 show rats nest */
	/* now we step through any available netlists and generate descriptors */

	if (PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN) {
		int n, p;

		for (n = 0; n < PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN; n++) {
			pcb_lib_menu_t *menu = &PCB->NetlistLib[PCB_NETLIST_INPUT].Menu[n];
			fprintf(FP, "NETDEF\n");
			pcb_fprintf(FP, "%s\n", &menu->Name[2]);
			pcb_fprintf(FP, "%d\n", showStatus);
			fprintf(FP, "(\n");
			for (p = 0; p < menu->EntryN; p++) {
				pcb_lib_entry_t *entry = &menu->Entry[p];
				pcb_fprintf(FP, "%s\n", entry->ListEntry);
			}
			fprintf(FP, ")\n");
		}
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes autotrax PCB to file
 */
int io_autotrax_write_pcb(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	pcb_cardinal_t i;
	int physicalLayerCount = 0;
	int currentAutotraxLayer = 0;
	int currentGroup = 0;

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

	pcb_coord_t LayoutXOffset = 0;
	pcb_coord_t LayoutYOffset = 0;

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

	if (physicalLayerCount > 8) {
		pcb_message(PCB_MSG_ERROR, "Warning: Physical layer count exceeds protel autotrax layer support for 6 layers plus GND and PWR planes.\n");
		/*return -1;*/
	}

	/* component "COMP" descriptions come next */

	pcb_trace("About to write layout elements to Protel Autotrax file.\n");
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
		pcb_message(PCB_MSG_ERROR, "Warning: Inner layer count exceeds protel autotrax maximum of 4 inner copper layers.\n");
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

	pcb_trace("About to write outline tracks to Protel Autotrax file.\n");
	/* we now proceed to write the outline tracks to the autotrax file, layer by layer */
	currentAutotraxLayer = 8; /* 11 is the "board layer" in autotrax, and 12 the keepout */
	if (outlineCount > 0 )  {
		for (i = 0; i < outlineCount; i++) /* write top copper tracks, if any */
			{
				write_autotrax_layout_tracks(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[outlineLayers[i]]),
						LayoutXOffset, LayoutYOffset);
				write_autotrax_layout_arcs(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[outlineLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			}
	}

	pcb_trace("About to write bottom silk elements to Protel Autotrax file.\n");
	/* we now proceed to write the bottom silk lines, arcs, text to the autotrax file, using layer 8 */
	currentAutotraxLayer = 8; /* 8 is the "bottom overlay" layer in autotrax */
	for (i = 0; i < bottomSilkCount; i++) /* write bottom silk lines, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomSilk[i]]),
						LayoutXOffset, LayoutYOffset);
		}

	pcb_trace("About to write bottom copper features to Protel Autotrax file.\n");
	/* we now proceed to write the bottom copper features to the autorax file, layer by layer */
	currentAutotraxLayer = 6; /* 6 is the bottom layer in autotrax */
	for (i = 0; i < bottomCount; i++) /* write bottom copper tracks, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[bottomLayers[i]]),
						LayoutXOffset, LayoutYOffset);
		}

	pcb_trace("About to write internal copper features to Protel Autotrax file.\n");
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
			write_autotrax_layout_tracks(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[innerLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[innerLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[innerLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[innerLayers[i]]),
						LayoutXOffset, LayoutYOffset);
		}

	pcb_trace("About to write top copper features to Protel Autotrax file.\n");
	/* we now proceed to write the top copper features to the autotrax file, layer by layer */
	currentAutotraxLayer = 1; /* 1 is the top most copper layer in autotrax */
	for (i = 0; i < topCount; i++) /* write top copper features, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[topLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer, 
						&(PCB->Data->Layer[topLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[topLayers[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer, 
						&(PCB->Data->Layer[topLayers[i]]),
						LayoutXOffset, LayoutYOffset);
		}

	pcb_trace("About to write top silk features to Protel Autotrax file.\n");
	/* we now proceed to write the top silk lines, arcs, text to the autotrax file, using layer 7*/
	currentAutotraxLayer = 7; /* 7 is the top silk layer in autotrax */
	for (i = 0; i < topSilkCount; i++) /* write top silk features, if any */
		{
			write_autotrax_layout_tracks(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[topSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_arcs(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[topSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_text(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[topSilk[i]]),
						LayoutXOffset, LayoutYOffset);
			write_autotrax_layout_polygons(FP, currentAutotraxLayer,
						&(PCB->Data->Layer[topSilk[i]]),
						LayoutXOffset, LayoutYOffset);
		}

	/* having done the graphical elements, we move onto vias */ 
	pcb_trace("About to write vias to Protel Autotrax file.\n");
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

	pcb_trace("About to write nestlists to Protel Autotrax file.\n");
	/* last are the autotrax netlist descriptors */
	write_autotrax_equipotential_netlists(FP, PCB);

	fputs("ENDPCB\n",FP); /*autotrax footer*/

	return (0);
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
		linelist_foreach(&layer->Line, &it, line) {
			pcb_fprintf(FP, "FT\n");
			write_autotrax_track(FP, xOffset, yOffset, line, currentLayer);
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

/* ---------------------------------------------------------------------------
 * writes autotrax arcs for layouts 
 */
int write_autotrax_layout_arcs(FILE * FP, pcb_cardinal_t number,
		 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_arc_t *arc;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) ){ /*|| (layer->meta.real.name && *layer->meta.real.name)) { */
		int localFlag = 0;
		arclist_foreach(&layer->Arc, &it, arc) {
			pcb_fprintf(FP, "FA\n");
			write_autotrax_arc(FP, xOffset, yOffset, arc, currentLayer);
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}

/* ---------------------------------------------------------------------------
 * writes generic autotrax text descriptor line layouts onl, since no text in .fp 
 */
int write_autotrax_layout_text(FILE * FP, pcb_cardinal_t number,
			 pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mHeight = myfont->MaxHeight; /* autotrax needs the width of the widest letter */
	int autotraxMirrored = 0; /* 0 is not mirrored, +16  is mirrored */ 

	pcb_coord_t defaultStrokeThickness, strokeThickness, textHeight;
	int rotation;
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
				fputs("FS\n", FP);
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
					text->X + xOffset, PCB->MaxHeight - (text->Y + yOffset), textHeight,
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
 * writes element data in autotrax format for use in a layout .PCB file
 */
int write_autotrax_layout_elements(FILE * FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{

	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t xPos, yPos, yPos2, yPos3, textOffset;

	pcb_element_t *element;

	int silkLayer = 7;  /* hard coded default, 7 is bottom silk */ 
	int copperLayer = 1; /* hard coded default, 1 is bottom copper */
	int padShape = 1; /* 1=circle, 2=Rectangle, 3=Octagonal, 4=Rounded Rectangle, 
			5=Cross Hair Target, 6=Moiro Target*/ 
	int drillHole = 0; /* for SMD */

	pcb_box_t *box;

	textOffset = PCB_MIL_TO_COORD(400); /* this gives good placement of refdes relative to element */
	
	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;

		box = &element->BoundingBox;
		xPos = (box->X1 + box->X2)/2 + xOffset;
		yPos = PCB->MaxHeight - (box->Y1 + yOffset - textOffset);
		yPos2 = yPos - PCB_MIL_TO_COORD(200);
		yPos3 = yPos2 - PCB_MIL_TO_COORD(200);

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
			xPos, yPos2, silkLayer);
		pcb_fprintf(FP, "%.0ml %.0ml 100 0 10 %d\n", /* comment field */
			xPos, yPos3, silkLayer);

		pinlist_foreach(&element->Pin, &it, pin) {

			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
 				padShape = 2;
			} else if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, pin)) {
				padShape = 3;
			} else {
				padShape = 1; /* circular */
			}

			pcb_fprintf(FP, "CP\n%.0ml %.0ml %.0ml %.0ml %d %.0ml 1 %d\n%s\n",
				pin->X - element->MarkX,
				PCB->MaxHeight -  (pin->Y),/* - element->MarkY), */
				pin->Thickness, pin->Thickness, padShape,
				pin->DrillingHole, copperLayer,
				(char *) PCB_EMPTY(pin->Number)); /* or ->Name? */
		}
		padlist_foreach(&element->Pad, &it, pad) {
			padShape = 2; /* rectangular */

			pcb_fprintf(FP, "CP\n%.0ml %.0ml ", /* positions of pad */
				(pad->Point1.X + pad->Point2.X)/2- element->MarkX,
				PCB->MaxHeight - ((pad->Point1.Y + pad->Point2.Y)/2- element->MarkY));

			if ((pad->Point1.X-pad->Point2.X) <= 0
					&& (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
					PCB->MaxHeight - (pad->Point2.Y-pad->Point1.Y + pad->Thickness)); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) <= 0
					 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
					PCB->MaxHeight - (pad->Point1.Y-pad->Point2.Y + pad->Thickness)); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
					 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
					PCB->MaxHeight - (pad->Point1.Y-pad->Point2.Y + pad->Thickness)); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
					 && (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
					PCB->MaxHeight - (pad->Point2.Y-pad->Point1.Y + pad->Thickness)); /* height */
			}

			pcb_fprintf(FP, "%d %d 1 %d\n%s\n", padShape, drillHole,
					copperLayer, (char *) PCB_EMPTY(pad->Number)); /*or Name?*/ 

		}
		linelist_foreach(&element->Line, &it, line) { /* autotrax supports tracks in COMPs */
			pcb_fprintf(FP, "CT\n");
			write_autotrax_track(FP, element->MarkX, PCB->MaxHeight - element->MarkY, line, silkLayer);
		}

		arclist_foreach(&element->Arc, &it, arc) {
			pcb_fprintf(FP, "CA\n");
			write_autotrax_arc(FP, element->MarkX, PCB->MaxHeight - element->MarkY, arc, silkLayer);
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
			/* hmm. or, could use a bounding box... */
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
					minx + xOffset, PCB->MaxHeight - (miny + yOffset),
					maxx + xOffset, PCB->MaxHeight - (maxy + yOffset),
					currentLayer);

			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}
