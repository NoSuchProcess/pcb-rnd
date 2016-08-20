/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include "plug_io.h"
#include "error.h"
#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)

// layer "0" is first copper layer = "0. Back - Solder"
// and layer "15" is "15. Front - Component"
// and layer "20" SilkScreen Back
// and layer "21" SilkScreen Front


/* writes the buffer to file */
int io_kicad_legacy_write_buffer(plug_io_t *ctx, FILE * FP, BufferType *buff)
{
        write_kicad_legacy_module_header(FP);
	//fputs("io_kicad_legacy_write_buffer()", FP);

        Cardinal i;

        // WriteViaData(FP, buff->Data);

        WriteElementData(FP, buff->Data, "kicadl");

	fprintf(FP, "$EndMODULE pcb-rnd-exported-module\n");

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
	fputs("io_kicad_legacy_write_pcb()", FP);
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes kicad element = module = footprint header information
 */
int write_kicad_legacy_module_header(FILE * FP)
{
	/* write some useful comments */

	fputs("PCBNEW-LibModule-V1  jan 01 jan 2016 00:00:01 CET\n",FP);
        fputs("Units mm\n",FP);
        fputs("$INDEX\n",FP);
        fputs("pcb-rnd-exported-module\n",FP);
        fputs("$EndINDEX\n",FP);
        fputs("$MODULE pcb-rnd-exported-module\n",FP);
        fputs("Po 0 0 0 15 51534DFF 00000000 ~~\n",FP);
        fputs("Li pcb-rnd-exported-module\n",FP);
        fputs("Cd pcb-rnd-exported-module\n",FP);
        fputs("Sc 0\n",FP);
        fputs("AR\n",FP);
        fputs("Op 0 0 0\n",FP);
        fputs("T0 0 -4134 600 600 0 120 N V 21 N \"S***\"\n",FP);

	//fprintf(FP, "# release: pcb-rnd " VERSION "\n");

	//fprintf(FP, "$EndMODULE pcb-rnd-exported-module\n");

	/* avoid writing things like user name or date, as these cause merge
	 * conflicts in collaborative environments using version control systems
	 */
	return 0;
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

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		PinType *pin;
		PadType *pad;

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
		//WriteAttributeList(FP, &element->Attributes, "\t");
		pinlist_foreach(&element->Pin, &it, pin) {
			fputs("$PAD\n",FP);  // start pad descriptor for a pin

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
                        fputs("$PAD\n",FP);  // start pad descriptor for an smd pad

                        pcb_fprintf(FP, "Po %.3mm %.3mm\n", // positions of pad
                                        (pad->Point1.X + pad->Point2.X)/2- element->MarkX,
                                        (pad->Point1.Y + pad->Point2.Y)/2- element->MarkY);

                        fputs("Sh ",FP); // pin shape descriptor
                        PrintQuotedString(FP, (char *) EMPTY(pad->Number));
                        fputs(" R ",FP); // rectangular, not a pin

                        if ((pad->Point1.X-pad->Point2.X) <= 0 
				&& (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
	                        pcb_fprintf(FP, "%.3mm %.3mm ",
                                        pad->Point2.X-pad->Point1.X + pad->Thickness,  // width
                                        pad->Point2.Y-pad->Point1.Y + pad->Thickness); // height
                        } else if ((pad->Point1.X-pad->Point2.X) <= 0 
                                && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
                                pcb_fprintf(FP, "%.3mm %.3mm ",
                                        pad->Point2.X-pad->Point1.X + pad->Thickness,  // width
                                        pad->Point1.Y-pad->Point2.Y + pad->Thickness); // height
                        } else if ((pad->Point1.X-pad->Point2.X) > 0        
                                && (pad->Point1.Y-pad->Point2.Y) > 0 ) {
                                pcb_fprintf(FP, "%.3mm %.3mm ",
                                        pad->Point1.X-pad->Point2.X + pad->Thickness,  // width
                                        pad->Point1.Y-pad->Point2.Y + pad->Thickness); // height
                        } else if ((pad->Point1.X-pad->Point2.X) > 0        
                                  && (pad->Point1.Y-pad->Point2.Y) <= 0 ) {
                                pcb_fprintf(FP, "%.3mm %.3mm ",
                                        pad->Point1.X-pad->Point2.X + pad->Thickness,  // width
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
					arc->Y - element->MarkY,                  // y_2 on circle
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
                                        arc->X - element->MarkX + (arc->Thickness/2)*cos(3.141592*(arc->StartAngle+180)/360), // x_2 on circle
					arc->Y - element->MarkY + (arc->Thickness/2)*sin(3.141592*(arc->StartAngle+180)/360), // y_2 on circle
                                        -arc->Delta*10,   // CW delta angle in decidegrees
                                        arc->Thickness); // stroke thickness
                        }
                        fputs("21\n",FP); // and now append a suitable Kicad layer, front silk = 21
		}
	}
	return 0;
}


