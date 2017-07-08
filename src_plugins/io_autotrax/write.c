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
#include "../lib_polyhelp/polyhelp.h"

#define F2S(OBJ, TYPE) pcb_strflg_f2s((OBJ)->Flags, TYPE)

/* ---------------------------------------------------------------------------
 * writes autotrax vias to file
 */
int write_autotrax_layout_vias(FILE * FP, pcb_data_t *Data)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	int via_drill_mil = 25; /* a reasonable default */
	/* write information about via */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "FV\r\n%.0ml %.0ml %.0ml %d\r\n", 
				via->X, PCB->MaxHeight - via->Y,
				via->Thickness, via_drill_mil);
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes generic autotrax track descriptor line for components and layouts 
 */
int write_autotrax_track(FILE * FP, pcb_line_t *line, pcb_cardinal_t layer)
{
	int user_routed = 1;
			pcb_fprintf(FP, "%.0ml %.0ml %.0ml %.0ml %.0ml %d %d\r\n",
				line->Point1.X, PCB->MaxHeight - line->Point1.Y,
				line->Point2.X, PCB->MaxHeight - line->Point2.Y,
				line->Thickness, layer, user_routed);
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes autotrax track descriptor for a pair of polyline vertices 
 */
int write_autotrax_pline_segment(FILE * FP, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t Thickness, pcb_cardinal_t layer)
{
	int user_routed = 1;
		pcb_fprintf(FP, "FT\r\n%.0ml %.0ml %.0ml %.0ml %.0ml %d %d\r\n",
			x1, PCB->MaxHeight - y1,
			x2, PCB->MaxHeight - y2,
			Thickness, layer, user_routed);
	return 0;
}

typedef struct {
	FILE * file;
	pcb_cardinal_t layer;
	pcb_coord_t thickness;
} autotrax_hatch_ctx_t;


static void autotrax_hatch_cb(void *ctx_, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	autotrax_hatch_ctx_t *ctx = (autotrax_hatch_ctx_t *)ctx_;
	write_autotrax_pline_segment(ctx->file, x1, y1, x2, y2, ctx->thickness, ctx->layer);
}

/* ---------------------------------------------------------------------------
 * generates autotrax tracks to cross hatch a complex polygon being exported 
 */
void autotrax_cpoly_hatch_lines(FILE * FP, const pcb_polygon_t *src, pcb_cpoly_hatchdir_t dir, pcb_coord_t period, pcb_coord_t thickness, pcb_cardinal_t layer)
{
	autotrax_hatch_ctx_t ctx;

	ctx.file = FP;
	ctx.layer = layer;
	ctx.thickness = thickness;
	pcb_cpoly_hatch(src, dir, (thickness/2)+1, period, &ctx, autotrax_hatch_cb);
}

/* -------------------------------------------------------------------------------
 * generates an autotrax arc "segments" value to approximate an arc being exported  
 */
int pcb_rnd_arc_to_autotrax_segments(pcb_angle_t arc_start, pcb_angle_t arc_delta)
{
	int arc_segments = 0; /* start with no arc segments */
	/* 15 = circle, bit 1 = LUQ, bit 2 = LLQ, bit 3 = LRQ, bit 4 = URQ */
	if (arc_delta == -360 ) { /* it's a circle */	
		arc_delta = 360;
	}
	if (arc_delta < 0 ) {
		arc_delta = -arc_delta;
		arc_start -= arc_delta;
	}

	while (arc_start < 0) {
		arc_start += 360;
	}
	while (arc_start > 360) {
		arc_start -= 360;
	}
	/* pcb_printf("Arc start: %ma, Arc delta: %ma\r\n", arc_start, arc_delta); */
	if (arc_delta >= 360) { /* it's a circle */
		arc_segments |= 0x0F;
	} else {
		if (arc_start <= 0.0 && (arc_start + arc_delta) >= 90.0 ) {
			arc_segments |= 0x04; /* LLQ */
		}
		if (arc_start <= 90.0 && (arc_start + arc_delta) >= 180.0 ) {
			arc_segments |= 0x08; /* LRQ */
		}
		if (arc_start <= 180.0 && (arc_start + arc_delta) >= 270.0 ) {
			arc_segments |= 0x01; /* URQ */
		}
		if (arc_start <= 270.0 && (arc_start + arc_delta) >= 360.0 ) {
			arc_segments |= 0x02; /* ULQ */
		}
		if (arc_start <= 360.0 && (arc_start + arc_delta) >= 450.0 ) {
			arc_segments |= 0x04; /* LLQ */
		}
		if (arc_start <= 360.0 && (arc_start + arc_delta) >= 540.0 ) {
			arc_segments |= 0x08; /* LRQ */
		}
		if (arc_start <= 360.0 && (arc_start + arc_delta) >= 630.0 ) {
			arc_segments |= 0x01; /* URQ */
		}
	}
	return arc_segments;
}

/* ---------------------------------------------------------------------------
 * writes generic autotrax arc descriptor line for components and layouts 
 */
int write_autotrax_arc(FILE *FP, pcb_arc_t * arc, int current_layer) 
{
	pcb_coord_t radius;
	if (arc->Width > arc->Height) {
		radius = arc->Height;
	} else {
		radius = arc->Width;
	}
	pcb_fprintf(FP, "%.0ml %.0ml %.0ml %d %.0ml %d\r\n",
		arc->X, PCB->MaxHeight - arc->Y, radius,
		pcb_rnd_arc_to_autotrax_segments(arc->StartAngle, arc->Delta),
		arc->Thickness, current_layer);
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes netlist data in autotrax format
 */
int write_autotrax_equipotential_netlists(FILE * FP, pcb_board_t *Layout)
{
	int show_status = 0;
	/* show status can be 0 or 1 for a net:
	   0 hide rats nest
	   1 show rats nest */
	/* now we step through any available netlists and generate descriptors */

	if (PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN) {
		int n, p;

		for (n = 0; n < PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN; n++) {
			pcb_lib_menu_t *menu = &PCB->NetlistLib[PCB_NETLIST_INPUT].Menu[n];
			fprintf(FP, "NETDEF\r\n");
			pcb_fprintf(FP, "%s\r\n", &menu->Name[2]);
			pcb_fprintf(FP, "%d\r\n", show_status);
			fprintf(FP, "(\r\n");
			for (p = 0; p < menu->EntryN; p++) {
				pcb_lib_entry_t *entry = &menu->Entry[p];
				pcb_fprintf(FP, "%s\r\n", entry->ListEntry);
			}
			fprintf(FP, ")\r\n");
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
	int physical_layer_count = 0;
	int current_autotrax_layer = 0;
	int current_group = 0;

	int bottom_count;
	pcb_layer_id_t *bottomLayers;
	int inner_count;
	pcb_layer_id_t *innerLayers;
	int top_count;
	pcb_layer_id_t *topLayers;
	int bottom_silk_count;
	pcb_layer_id_t *bottomSilk;
	int top_silk_count;
	pcb_layer_id_t *topSilk;
	int outline_count;
	pcb_layer_id_t *outlineLayers;

	/* autotrax expects layout dimensions to be specified in mils */
	int max_width_mil = 32000;
	int max_height_mil = 32000;

	if (pcb_board_normalize(PCB) < 0) {
		pcb_message(PCB_MSG_ERROR, "Unable to normalise layout prior to attempting export.\n");
		return -1;
	}

	fputs("PCB FILE 4\r\n",FP); /*autotrax header*/

	/* we sort out if the layout dimensions exceed the autotrax maxima */
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > max_width_mil ||
			PCB_COORD_TO_MIL(PCB->MaxHeight) > max_height_mil) {
		pcb_message(PCB_MSG_ERROR, "Layout size exceeds protel autotrax 32000 mil x 32000 mil maximum.");
		return -1;
	}

	/* here we count the copper layers to be exported to the autotrax file */
	physical_layer_count = pcb_layergrp_list(PCB, PCB_LYT_COPPER, NULL, 0);

	if (physical_layer_count > 8) {
		pcb_message(PCB_MSG_ERROR, "Warning: Physical layer count exceeds protel autotrax layer support for 6 layers plus GND and PWR planes.\n");
		/*return -1;*/
	}

	/* component "COMP" descriptions come next */

	write_autotrax_layout_elements(FP, PCB, PCB->Data);

	/* we now need to map pcb's layer groups onto the kicad layer numbers */
	current_autotrax_layer = 0;
	current_group = 0;

	/* figure out which pcb layers are bottom copper and make a list */
	bottom_count = pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, NULL, 0);
	if (bottom_count > 0 ) {
		bottomLayers = malloc(sizeof(pcb_layer_id_t) * bottom_count);
		pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, bottomLayers, bottom_count);
	} else {
		bottomLayers = NULL;
	}

	/* figure out which pcb layers are internal copper layers and make a list */
	inner_count = pcb_layer_list(PCB_LYT_INTERN | PCB_LYT_COPPER, NULL, 0);
	if (inner_count > 0 ) {
		innerLayers = malloc(sizeof(pcb_layer_id_t) * inner_count);
		pcb_layer_list(PCB_LYT_INTERN | PCB_LYT_COPPER, innerLayers, inner_count);
	} else {
		innerLayers = NULL;
	}

	if (inner_count > 4) {
		pcb_message(PCB_MSG_ERROR, "Warning: Inner layer count exceeds protel autotrax maximum of 4 inner copper layers.\n");
		/*return -1;*/
	}

	/* figure out which pcb layers are top copper and make a list */
	top_count = pcb_layer_list(PCB_LYT_TOP | PCB_LYT_COPPER, NULL, 0);
	if (top_count > 0 ) {
		topLayers = malloc(sizeof(pcb_layer_id_t) * top_count);
		pcb_layer_list(PCB_LYT_TOP | PCB_LYT_COPPER, topLayers, top_count);
	} else {
		topLayers = NULL;
	}

	/* figure out which pcb layers are bottom silk and make a list */
	bottom_silk_count = pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, NULL, 0);
	if (bottom_silk_count > 0 ) {
		bottomSilk = malloc(sizeof(pcb_layer_id_t) * bottom_silk_count);
		pcb_layer_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, bottomSilk, bottom_silk_count);
	} else {
		bottomSilk = NULL;
	}

	/* figure out which pcb layers are top silk and make a list */
	top_silk_count = pcb_layer_list(PCB_LYT_TOP | PCB_LYT_SILK, NULL, 0);
	if (top_silk_count > 0) {
		topSilk = malloc(sizeof(pcb_layer_id_t) * top_silk_count);
		pcb_layer_list(PCB_LYT_TOP | PCB_LYT_SILK, topSilk, top_silk_count);
	} else {
		topSilk = NULL;
	}

	/* figure out which pcb layers are outlines and make a list */
	outline_count = pcb_layer_list(PCB_LYT_OUTLINE, NULL, 0);
	if (outline_count > 0) {
		outlineLayers = malloc(sizeof(pcb_layer_id_t) * outline_count);
		pcb_layer_list(PCB_LYT_OUTLINE, outlineLayers, outline_count);
	} else {
		outlineLayers = NULL;
	}

	/* we now proceed to write the outline tracks to the autotrax file, layer by layer */
	current_autotrax_layer = 8; /* 11 is the "board layer" in autotrax, and 12 the keepout */
	if (outline_count > 0 )  {
		for (i = 0; i < outline_count; i++) /* write top copper tracks, if any */
			{
				write_autotrax_layout_tracks(FP, current_autotrax_layer,
						&(PCB->Data->Layer[outlineLayers[i]]));
				write_autotrax_layout_arcs(FP, current_autotrax_layer,
						&(PCB->Data->Layer[outlineLayers[i]]));
			}
	}

	/* we now proceed to write the bottom silk lines, arcs, text to the autotrax file, using layer 8 */
	current_autotrax_layer = 8; /* 8 is the "bottom overlay" layer in autotrax */
	for (i = 0; i < bottom_silk_count; i++) /* write bottom silk lines, if any */
		{
			write_autotrax_layout_tracks(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomSilk[i]]));
			write_autotrax_layout_arcs(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomSilk[i]]));
			write_autotrax_layout_text(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomSilk[i]]));
			write_autotrax_layout_polygons(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomSilk[i]]));
		}

	/* we now proceed to write the bottom copper features to the autorax file, layer by layer */
	current_autotrax_layer = 6; /* 6 is the bottom layer in autotrax */
	for (i = 0; i < bottom_count; i++) /* write bottom copper tracks, if any */
		{
			write_autotrax_layout_tracks(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomLayers[i]]));
			write_autotrax_layout_arcs(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomLayers[i]]));
			write_autotrax_layout_text(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomLayers[i]]));
			write_autotrax_layout_polygons(FP, current_autotrax_layer,
						&(PCB->Data->Layer[bottomLayers[i]]));
		}

	/* we now proceed to write the internal copper features to the autotrax file, layer by layer */
	if (inner_count > 0) {
		current_group = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for (i = 0, current_autotrax_layer = 2; i < inner_count; i++) /* write inner copper text, group by group */
		{
			if (current_group != pcb_layer_get_group(PCB, innerLayers[i])) {
				current_group = pcb_layer_get_group(PCB, innerLayers[i]);
				current_autotrax_layer++;
			} /* autotrax inner layers are layers 2 to 5 inclusive */
			write_autotrax_layout_tracks(FP, current_autotrax_layer,
						&(PCB->Data->Layer[innerLayers[i]]));
			write_autotrax_layout_arcs(FP, current_autotrax_layer,
						&(PCB->Data->Layer[innerLayers[i]]));
			write_autotrax_layout_text(FP, current_autotrax_layer,
						&(PCB->Data->Layer[innerLayers[i]]));
			write_autotrax_layout_polygons(FP, current_autotrax_layer,
						&(PCB->Data->Layer[innerLayers[i]]));
		}

	/* we now proceed to write the top copper features to the autotrax file, layer by layer */
	current_autotrax_layer = 1; /* 1 is the top most copper layer in autotrax */
	for (i = 0; i < top_count; i++) /* write top copper features, if any */
		{
			write_autotrax_layout_tracks(FP, current_autotrax_layer,
						&(PCB->Data->Layer[topLayers[i]]));
			write_autotrax_layout_arcs(FP, current_autotrax_layer, 
						&(PCB->Data->Layer[topLayers[i]]));
			write_autotrax_layout_text(FP, current_autotrax_layer,
						&(PCB->Data->Layer[topLayers[i]]));
			write_autotrax_layout_polygons(FP, current_autotrax_layer, 
						&(PCB->Data->Layer[topLayers[i]]));
		}

	/* we now proceed to write the top silk lines, arcs, text to the autotrax file, using layer 7*/
	current_autotrax_layer = 7; /* 7 is the top silk layer in autotrax */
	for (i = 0; i < top_silk_count; i++) /* write top silk features, if any */
		{
			write_autotrax_layout_tracks(FP, current_autotrax_layer,
						&(PCB->Data->Layer[topSilk[i]]));
			write_autotrax_layout_arcs(FP, current_autotrax_layer,
						&(PCB->Data->Layer[topSilk[i]]));
			write_autotrax_layout_text(FP, current_autotrax_layer,
						&(PCB->Data->Layer[topSilk[i]]));
			write_autotrax_layout_polygons(FP, current_autotrax_layer,
						&(PCB->Data->Layer[topSilk[i]]));
		}

	/* having done the graphical elements, we move onto vias */ 
	write_autotrax_layout_vias(FP, PCB->Data);

	/* now free memory from arrays that were used */
	if (bottom_count > 0) {
		free(bottomLayers);
	}
	if (inner_count > 0) {
		free(innerLayers);
	}
	if (top_count > 0) {
		free(topLayers);
	}
	if (top_silk_count > 0) {
		free(topSilk);
	}
	if (bottom_silk_count > 0) {
		free(bottomSilk);
	}
	if (outline_count > 0) {
		free(outlineLayers);
	}

	/* last are the autotrax netlist descriptors */
	write_autotrax_equipotential_netlists(FP, PCB);

	fputs("ENDPCB\r\n",FP); /*autotrax footer*/

	return (0);
}

int write_autotrax_layout_tracks(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_cardinal_t current_layer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->meta.real.name && *layer->meta.real.name)) {
		int local_flag = 0;
		linelist_foreach(&layer->Line, &it, line) {
			pcb_fprintf(FP, "FT\r\n");
			write_autotrax_track(FP, line, current_layer);
			local_flag |= 1;
		}
		return local_flag;
	} else {
		return 0;
	}
}

/* ---------------------------------------------------------------------------
 * writes autotrax arcs for layouts 
 */
int write_autotrax_layout_arcs(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer)
{
	gdl_iterator_t it;
	pcb_arc_t *arc;
	pcb_cardinal_t current_layer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) ){ /*|| (layer->meta.real.name && *layer->meta.real.name)) { */
		int local_flag = 0;
		arclist_foreach(&layer->Arc, &it, arc) {
			pcb_fprintf(FP, "FA\r\n");
			write_autotrax_arc(FP, arc, current_layer);
			local_flag |= 1;
		}
		return local_flag;
	} else {
		return 0;
	}
}

/* ---------------------------------------------------------------------------
 * writes generic autotrax text descriptor line layouts onl, since no text in .fp 
 */
int write_autotrax_layout_text(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mHeight = myfont->MaxHeight; /* autotrax needs the width of the widest letter */
	int autotrax_mirrored = 0; /* 0 is not mirrored, +16  is mirrored */ 

	pcb_coord_t default_stroke_thickness, strokeThickness, textHeight;
	int rotation;
	int local_flag;

	gdl_iterator_t it;
	pcb_text_t *text;
	pcb_cardinal_t current_layer = number;

	int index = 0;

	default_stroke_thickness = 200000;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) ) { /*|| (layer->meta.real.name && *layer->meta.real.name)) {*/
		local_flag = 0;
		textlist_foreach(&layer->Text, &it, text) {
			if (current_layer < 9)  { /* copper or silk layer text */
				fputs("FS\r\n", FP);
				strokeThickness = PCB_SCALE_TEXT(default_stroke_thickness, text->Scale /2);
				textHeight = PCB_SCALE_TEXT(mHeight, text->Scale);
				rotation = 0;
				if (current_layer == 6 || current_layer == 8) {/* back copper or silk */
					autotrax_mirrored = 16; /* mirrored */
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
				pcb_fprintf(FP, "%.0ml %.0ml %.0ml %d %.0ml %d\r\n",
					text->X, PCB->MaxHeight - text->Y, textHeight,
					rotation + autotrax_mirrored, strokeThickness, current_layer);
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
				pcb_fprintf(FP, "\r\n");
			}
			local_flag |= 1;
		}
		return local_flag;
	} else {
		return 0;
	}
}

/* ---------------------------------------------------------------------------
 * writes element data in autotrax format for use in a layout .PCB file
 */
int write_autotrax_layout_elements(FILE * FP, pcb_board_t *Layout, pcb_data_t *Data)
{

	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t xPos, yPos, yPos2, yPos3, text_offset;

	pcb_element_t *element;

	int silk_layer = 7;  /* hard coded default, 7 is bottom silk */ 
	int copper_layer = 1; /* hard coded default, 1 is bottom copper */
	int pad_shape = 1; /* 1=circle, 2=Rectangle, 3=Octagonal, 4=Rounded Rectangle, 
			5=Cross Hair Target, 6=Moiro Target*/ 
	int drill_hole = 0; /* for SMD */

	pcb_box_t *box;

	text_offset = PCB_MIL_TO_COORD(400); /* this gives good placement of refdes relative to element */
	
	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;

		box = &element->BoundingBox;
		xPos = (box->X1 + box->X2)/2;
		yPos = PCB->MaxHeight - (box->Y1 - text_offset);
		yPos2 = yPos - PCB_MIL_TO_COORD(200);
		yPos3 = yPos2 - PCB_MIL_TO_COORD(200);

		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element)) {
			silk_layer = 8;
			copper_layer = 6;
		} else {
			silk_layer = 7;
			copper_layer = 1;
		}

		fprintf(FP, "COMP\r\n%s\r\n", element->Name[PCB_ELEMNAME_IDX_REFDES].TextString);/* designator */
		fprintf(FP, "%s\r\n", element->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString);/* designator */
		fprintf(FP, "%s\r\n", element->Name[PCB_ELEMNAME_IDX_VALUE].TextString);/* designator */
		pcb_fprintf(FP, "%.0ml %.0ml 100 0 10 %d\r\n", /* designator */
			xPos, yPos, silk_layer);
		pcb_fprintf(FP, "%.0ml %.0ml 100 0 10 %d\r\n", /* pattern */
			xPos, yPos2, silk_layer);
		pcb_fprintf(FP, "%.0ml %.0ml 100 0 10 %d\r\n", /* comment field */
			xPos, yPos3, silk_layer);

		pinlist_foreach(&element->Pin, &it, pin) {

			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
				pad_shape = 2;
			} else if (PCB_FLAG_TEST(PCB_FLAG_OCTAGON, pin)) {
				pad_shape = 3;
			} else {
				pad_shape = 1; /* circular */
			}

			pcb_fprintf(FP, "CP\r\n%.0ml %.0ml %.0ml %.0ml %d %.0ml 1 %d\r\n%s\r\n",
				pin->X ,
				PCB->MaxHeight -  (pin->Y),
				pin->Thickness, pin->Thickness, pad_shape,
				pin->DrillingHole, copper_layer,
				(char *) PCB_EMPTY(pin->Number)); /* or ->Name? */
		}
		padlist_foreach(&element->Pad, &it, pad) {
			pad_shape = 2; /* rectangular */

			pcb_fprintf(FP, "CP\r\n%.0ml %.0ml ", /* positions of pad */
				(pad->Point1.X + pad->Point2.X)/2, 
				PCB->MaxHeight - ((pad->Point1.Y + pad->Point2.Y)/2));

			if ((pad->Point1.X-pad->Point2.X) <= 0
					&& (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
					(pad->Point2.Y-pad->Point1.Y + pad->Thickness)); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) <= 0
					 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point2.X-pad->Point1.X + pad->Thickness,	 /* width */
					(pad->Point1.Y-pad->Point2.Y + pad->Thickness)); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
					 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
					(pad->Point1.Y-pad->Point2.Y + pad->Thickness)); /* height */
			} else if ((pad->Point1.X-pad->Point2.X) > 0
					 && (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.0ml %.0ml ",
					pad->Point1.X-pad->Point2.X + pad->Thickness,	 /* width */
					(pad->Point2.Y-pad->Point1.Y + pad->Thickness)); /* height */
			}

			pcb_fprintf(FP, "%d %d 1 %d\r\n%s\r\n", pad_shape, drill_hole,
					copper_layer, (char *) PCB_EMPTY(pad->Number)); /*or Name?*/ 

		}
		linelist_foreach(&element->Line, &it, line) { /* autotrax supports tracks in COMPs */
			pcb_fprintf(FP, "CT\r\n");
			write_autotrax_track(FP, line, silk_layer);
		}

		arclist_foreach(&element->Arc, &it, arc) {
			pcb_fprintf(FP, "CA\r\n");
			write_autotrax_arc(FP, arc, silk_layer);
		}

		fprintf(FP, "ENDCOMP\r\n");
	}
	return 0;
}


/* ---------------------------------------------------------------------------
 * writes polygon data in autotrax fill (rectangle) format for use in a layout .PCB file
 */

int write_autotrax_layout_polygons(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer)
{
	int i;
	gdl_iterator_t it;
	pcb_polygon_t *polygon;
	pcb_cardinal_t current_layer = number;

	pcb_poly_it_t poly_it;
	pcb_polyarea_t *pa;

	pcb_coord_t minx, miny, maxx, maxy;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) ) { /*|| (layer->meta.real.name && *layer->meta.real.name)) {*/
		int local_flag = 0;

		polylist_foreach(&layer->Polygon, &it, polygon) {
			if (pcb_cpoly_is_simple_rect(polygon)) {
				pcb_trace(" simple rectangular polyogon\n");

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
				pcb_fprintf(FP, "FF\r\n%.0ml %.0ml %.0ml %.0ml %d\r\n",
						minx, PCB->MaxHeight - miny,
						maxx, PCB->MaxHeight - maxy,
						current_layer);

				local_flag |= 1;
/* here we need to test for non rectangular polygons to flag imperfect export to easy/autotrax

			if (helper_clipped_polygon_type_function(clipped_thing)) {
				pcb_message(PCB_MSG_ERROR, "Polygon exported as a bounding box only.\n");
			}*/
			} else {
				pcb_coord_t Thickness;
				Thickness = PCB_MIL_TO_COORD(10);
				autotrax_cpoly_hatch_lines(FP, polygon, PCB_CPOLY_HATCH_HORIZONTAL | PCB_CPOLY_HATCH_VERTICAL, Thickness*3, Thickness, current_layer);
				for(pa = pcb_poly_island_first(polygon, &poly_it); pa != NULL; pa = pcb_poly_island_next(&poly_it)) {
					/* now generate cross hatch lines for polygon island export */
					pcb_pline_t *pl, *track;
					/* check if we have a contour for the given island */
					pl = pcb_poly_contour(&poly_it);
					if (pl != NULL) {
						const pcb_vnode_t *v, *n;
						track = pcb_pline_dup_offset(pl, -((Thickness/2)+1));
						v = &track->head;
						do {
							n = v->next;
							write_autotrax_pline_segment(FP, v->point[0], v->point[1], n->point[0], n->point[1], Thickness, current_layer);
						} while ((v = v->next) != &track->head);
						pcb_poly_contour_del(&track);

						/* iterate over all holes within this island */
						for(pl = pcb_poly_hole_first(&poly_it);
							pl != NULL; pl = pcb_poly_hole_next(&poly_it)) {
							const pcb_vnode_t *v, *n;
							track = pcb_pline_dup_offset(pl, -((Thickness/2)+1));
							v = &track->head;
							do {
								n = v->next;
								write_autotrax_pline_segment(FP, v->point[0], v->point[1], n->point[0], n->point[1], Thickness, current_layer);
							} while ((v = v->next) != &track->head);
							pcb_poly_contour_del(&track);
						}
					}
				}
			}
		}
		return local_flag;
	} else {
		return 0;
	}
}
