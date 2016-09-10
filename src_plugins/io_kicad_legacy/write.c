/*
 *														COPYRIGHT
 *
 *	PCB, interactive printed circuit board design
 *	Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
#include "plug_io.h"
#include "error.h"
#include "uniq_name.h"
#include "data.h"

#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)

/* layer "0" is first copper layer = "0. Back - Solder"
 * and layer "15" is "15. Front - Component"
 * and layer "20" SilkScreen Back
 * and layer "21" SilkScreen Front
 */

/* generates a line by line listing of the elements being saved */
static int io_kicad_legacy_write_element_index(FILE * FP, DataTypePtr Data);

/* generates a default via drill size for the layout */
static int write_kicad_legacy_layout_via_drill_size(FILE * FP);

/* writes the buffer to file */
int io_kicad_legacy_write_buffer(plug_io_t *ctx, FILE * FP, BufferType *buff)
{
	/*fputs("io_kicad_legacy_write_buffer()", FP); */

	fputs("PCBNEW-LibModule-V1	jan 01 jan 2016 00:00:01 CET\n",FP);
	fputs("Units mm\n",FP);
	fputs("$INDEX\n",FP);
	io_kicad_legacy_write_element_index(FP, buff->Data);
	fputs("$EndINDEX\n",FP);

	/* WriteViaData(FP, buff->Data); */

	WriteElementData(FP, buff->Data, "kicadl");

	/*
		for (i = 0; i < max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(buff->Data->Layer[i]));
	*/
	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int io_kicad_legacy_write_pcb(plug_io_t *ctx, FILE * FP)
{
	/* this is the first step in exporting a layout;
	 * creating a kicd module containing the elements used in the layout
	 */

	/*fputs("io_kicad_legacy_write_pcb()", FP);*/

	Cardinal i;
	Cardinal j;

	Coord LayoutXOffset;
	Coord LayoutYOffset;

	fputs("PCBNEW-BOARD Version 1 jan 01 jan 2016 00:00:01 CET\n",FP);

	fputs("$GENERAL\n",FP);
	fputs("Ly 1FFF8001\n",FP); /* obsolete, needed for old pcbnew */
	/*puts("Units mm\n",FP);*/ /*decimils most universal legacy format */
	fputs("$EndGENERAL\n",FP);

	fputs("$SHEETDESCR\n",FP);

	/* Kicad expects a layout "sheet" size to be specified in mils, and A4, A3 etc... */
	int A4HeightMil = 8267;
	int A4WidthMil = 11700;
	int sheetHeight = A4HeightMil;
	int sheetWidth = A4WidthMil;
	int paperSize = 4; /* default paper size is A4 */

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
	fputs("Layers 2\n",FP);
	fputs("Layer[0] Cuivre signal\n",FP);
	fputs("Layer[15] Composant signal\n",FP);
	write_kicad_legacy_layout_via_drill_size(FP);
	fputs("$EndSETUP\n",FP);

	/* module description stuff would go here */

	fputs("$TRACK\n",FP);
	write_kicad_legacy_layout_vias(FP, PCB->Data, LayoutXOffset, LayoutYOffset);

	for (i = 0, j = 0; i < max_copper_layer + 2; i++)
		{
			j += write_kicad_legacy_layout_tracks(FP, j, &(PCB->Data->Layer[i]),
																						LayoutXOffset, LayoutYOffset);
		}
	fputs("$EndTRACK\n",FP);
	fputs("$EndBOARD\n",FP);
	/*WriteElementData(FP, PCB->Data, "kicadl");*/	/* this may be needed in a different file */
	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes (eventually) de-duplicated list of element names in kicad legacy format module $INDEX
 */
static int io_kicad_legacy_write_element_index(FILE * FP, DataTypePtr Data)
{
	gdl_iterator_t eit;
	LineType *line;
	ArcType *arc;
	ElementType *element;

	elementlist_dedup_initializer(ededup);

	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	/* Now initialize the group with defaults */
	unm_init(&group1);

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		PinType *pin;
		PadType *pad;

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


int write_kicad_legacy_layout_vias(FILE * FP, DataTypePtr Data, Coord xOffset, Coord yOffset)
{
	gdl_iterator_t it;
	PinType *via;
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

int write_kicad_legacy_layout_tracks(FILE * FP, Cardinal number,
																		 LayerTypePtr layer, Coord xOffset, Coord yOffset)
{
	gdl_iterator_t it;
	LineType *line;
	/*ArcType *arc;
		TextType *text;
		PolygonType *polygon;
	*/
	/* write information about non empty layers */
	if (!LAYER_IS_EMPTY(layer) || (layer->Name && *layer->Name)) {
		/*
			fprintf(FP, "Layer(%i ", (int) Number + 1);
			PrintQuotedString(FP, (char *) EMPTY(layer->Name));
			fputs(")\n(\n", FP);
			WriteAttributeList(FP, &layer->Attributes, "\t");
		*/
		int localFlag = 0;
		linelist_foreach(&layer->Line, &it, line) {
			pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n",
									line->Point1.X + xOffset, line->Point1.Y + yOffset,
									line->Point2.X + xOffset, line->Point2.Y + yOffset,
									line->Thickness);
			pcb_fprintf(FP, "De %d 0 0 0 0\n", number); /* omitting net info */
			localFlag |= 1;
		}
		return localFlag;
	} else {
		return 0;
	}
}


/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format
 */
int io_kicad_legacy_write_element(plug_io_t *ctx, FILE * FP, DataTypePtr Data)
{


	//write_kicad_legacy_module_header(FP);
	//fputs("io_kicad_legacy_write_element()", FP);
	//return 0;


	gdl_iterator_t eit;
	LineType *line;
	ArcType *arc;
	ElementType *element;
	elementlist_dedup_initializer(ededup);

	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	/* Now initialize the group with defaults */
	unm_init(&group1);

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		PinType *pin;
		PadType *pad;

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
			 PrintQuotedString(FP, (char *) EMPTY(DESCRIPTION_NAME(element)));
			 fputc(' ', FP);
			 PrintQuotedString(FP, (char *) EMPTY(NAMEONPCB_NAME(element)));
			 fputc(' ', FP);
			 PrintQuotedString(FP, (char *) EMPTY(VALUE_NAME(element)));
			 pcb_fprintf(FP, " %mm %mm %mm %mm %d %d %s]\n(\n",
			 element->MarkX, element->MarkY,
			 DESCRIPTION_TEXT(element).X - element->MarkX,
			 DESCRIPTION_TEXT(element).Y - element->MarkY,
			 DESCRIPTION_TEXT(element).Direction,
			 DESCRIPTION_TEXT(element).Scale, F2S(&(DESCRIPTION_TEXT(element)), PCB_TYPE_ELEMENT_NAME));

		*/

		/*		//WriteAttributeList(FP, &element->Attributes, "\t");
		 */

		const char * currentElementName = unm_name(&group1, element->Name[0].TextString, element);
		fprintf(FP, "$MODULE %s\n", currentElementName);
		fputs("Po 0 0 0 15 51534DFF 00000000 ~~\n",FP);
		fprintf(FP, "Li %s\n", currentElementName);
		fprintf(FP, "Cd %s\n", currentElementName);
		fputs("Sc 0\n",FP);
		fputs("AR\n",FP);
		fputs("Op 0 0 0\n",FP);
		fputs("T0 0 -4134 600 600 0 120 N V 21 N \"S***\"\n",FP);

		pinlist_foreach(&element->Pin, &it, pin) {
			fputs("$PAD\n",FP);	 // start pad descriptor for a pin

			pcb_fprintf(FP, "Po %.3mm %.3mm\n", // positions of pad
									pin->X - element->MarkX,
									pin->Y - element->MarkY);

			fputs("Sh ",FP); // pin shape descriptor
			PrintQuotedString(FP, (char *) EMPTY(pin->Number));
			fputs(" C ",FP); // circular
			pcb_fprintf(FP, "%.3mm %.3mm ", pin->Thickness, pin->Thickness); // height = width
			fputs("0 0 0\n",FP);// deltaX deltaY Orientation as float in decidegrees

			fputs("Dr ",FP); // drill details; size and x,y pos relative to pad location
			pcb_fprintf(FP, "%mm 0 0\n", pin->DrillingHole);

			fputs("At STD N 00E0FFFF\n", FP); // through hole STD pin, all copper layers

			fputs("Ne 0 \"\"\n",FP); // library parts have empty net descriptors
			/*
				PrintQuotedString(FP, (char *) EMPTY(pin->Name));
				fprintf(FP, " %s\n", F2S(pin, PCB_TYPE_PIN));
			*/
			fputs("$EndPAD\n",FP);
		}
		pinlist_foreach(&element->Pad, &it, pad) {
			fputs("$PAD\n",FP);	 // start pad descriptor for an smd pad

			pcb_fprintf(FP, "Po %.3mm %.3mm\n", // positions of pad
									(pad->Point1.X + pad->Point2.X)/2- element->MarkX,
									(pad->Point1.Y + pad->Point2.Y)/2- element->MarkY);

			fputs("Sh ",FP); // pin shape descriptor
			PrintQuotedString(FP, (char *) EMPTY(pad->Number));
			fputs(" R ",FP); // rectangular, not a pin

			if ((pad->Point1.X-pad->Point2.X) <= 0
					&& (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point2.X-pad->Point1.X + pad->Thickness,	 // width
										pad->Point2.Y-pad->Point1.Y + pad->Thickness); // height
			} else if ((pad->Point1.X-pad->Point2.X) <= 0
								 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point2.X-pad->Point1.X + pad->Thickness,	 // width
										pad->Point1.Y-pad->Point2.Y + pad->Thickness); // height
			} else if ((pad->Point1.X-pad->Point2.X) > 0
								 && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point1.X-pad->Point2.X + pad->Thickness,	 // width
										pad->Point1.Y-pad->Point2.Y + pad->Thickness); // height
			} else if ((pad->Point1.X-pad->Point2.X) > 0
								 && (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
				pcb_fprintf(FP, "%.3mm %.3mm ",
										pad->Point1.X-pad->Point2.X + pad->Thickness,	 // width
										pad->Point2.Y-pad->Point1.Y + pad->Thickness); // height
			}

			fputs("0 0 0\n",FP);// deltaX deltaY Orientation as float in decidegrees

			fputs("Dr 0 0 0\n",FP); // drill details; zero size; x,y pos vs pad location

			fputs("At SMD N 00888000\n", FP); // SMD pin, need to use right layer mask

			fputs("Ne 0 \"\"\n",FP); // library parts have empty net descriptors
			fputs("$EndPAD\n",FP);

		}
		linelist_foreach(&element->Line, &it, line) {
			pcb_fprintf(FP, "DS %.3mm %.3mm %.3mm %.3mm %.3mm ",
									line->Point1.X - element->MarkX,
									line->Point1.Y - element->MarkY,
									line->Point2.X - element->MarkX,
									line->Point2.Y - element->MarkY,
									line->Thickness);
			fputs("21\n",FP); // an arbitrary Kicad layer, front silk, need to refine this
		}

		linelist_foreach(&element->Arc, &it, arc) {
			if ((arc->Delta == 360) || (arc->Delta == -360)) { // it's a circle
				pcb_fprintf(FP, "DC %.3mm %.3mm %.3mm %.3mm %.3mm ",
										arc->X - element->MarkX, // x_1 centre
										arc->Y - element->MarkY, // y_2 centre
										(arc->X - element->MarkX + arc->Thickness/2), // x_2 on circle
										arc->Y - element->MarkY,									// y_2 on circle
										arc->Thickness); // stroke thickness
			} else {
				// as far as can be determined from the Kicad documentation,
				// http://en.wikibooks.org/wiki/Kicad/file_formats#Drawings
				//
				// the origin for rotation is the positive x direction, and going CW
				//
				// whereas in gEDA, the gEDA origin for rotation is the negative x axis,
				// with rotation CCW, so we need to reverse delta angle
				//
				// deltaAngle is CW in Kicad in deci-degrees, and CCW in degrees in gEDA
				// NB it is in degrees in the newer s-file kicad module/footprint format
				pcb_fprintf(FP, "DA %.3mm %.3mm %.3mm %.3mm %.3ma %.3mm ",
										arc->X - element->MarkX, // x_1 centre
										arc->Y - element->MarkY, // y_2 centre
										arc->X - element->MarkX + (arc->Thickness/2)*cos(M_PI*(arc->StartAngle+180)/360), // x_2 on circle
										arc->Y - element->MarkY + (arc->Thickness/2)*sin(M_PI*(arc->StartAngle+180)/360), // y_2 on circle
										-arc->Delta*10,		// CW delta angle in decidegrees
										arc->Thickness); // stroke thickness
			}
			fputs("21\n",FP); // and now append a suitable Kicad layer, front silk = 21
		}

		fprintf(FP, "$EndMODULE %s\n", currentElementName);
	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);

	return 0;
}
