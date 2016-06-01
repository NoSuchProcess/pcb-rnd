/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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

/* file save, load, merge ... routines
 * getpid() needs a cast to (int) to get rid of compiler warnings
 * on several architectures
 */

#include "config.h"
#include "conf_core.h"

#include <locale.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "global.h"

#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <time.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#include "buffer.h"
#include "change.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "plug_io.h"
#include "hid.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "parse_l.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "set.h"
#include "strflags.h"
#include "compat_fs.h"
#include "paths.h"
#include "rats_patch.h"
#include "stub_edif.h"
#include "hid_actions.h"
#include "hid_flags.h"

RCSID("$Id$");

#if !defined(HAS_ATEXIT) && !defined(HAS_ON_EXIT)
/* ---------------------------------------------------------------------------
 * some local identifiers for OS without an atexit() or on_exit()
 * call
 */
static char TMPFilename[80];
#endif

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void WritePCBInfoHeader(FILE *);
static void WritePCBDataHeader(FILE *);
static void WritePCBFontData(FILE *);
static void WriteViaData(FILE *, DataTypePtr);
static void WritePCBRatData(FILE *);
static void WriteElementData(FILE *, DataTypePtr);
static void WriteLayerData(FILE *, Cardinal, LayerTypePtr);
static int WritePCB(FILE *);
int WritePCBFile(char *);
int WritePipe(char *, bool);

/* ---------------------------------------------------------------------------
 * Flag helper functions
 */

#define F2S(OBJ, TYPE) flags_to_string ((OBJ)->Flags, TYPE)

/* --------------------------------------------------------------------------- */

/* The idea here is to avoid gratuitously breaking backwards
   compatibility due to a new but rarely used feature.  The first such
   case, for example, was the polygon Hole - if your design included
   polygon holes, you needed a newer PCB to read it, but if your
   design didn't include holes, PCB would produce a file that older
   PCBs could read, if only it had the correct version number in it.

   If, however, you have to add or change a feature that really does
   require a new PCB version all the time, it's time to remove all the
   tests below and just always output the new version.

   Note: Best practices here is to add support for a feature *first*
   (and bump PCB_FILE_VERSION in file.h), and note the version that
   added that support below, and *later* update the file format to
   need that version (which may then be older than PCB_FILE_VERSION).
   Hopefully, that allows for one release between adding support and
   needing it, which should minimize breakage.  Of course, that's not
   *always* possible, practical, or desirable.

*/

/* Hole[] in Polygon.  */
#define PCB_FILE_VERSION_HOLES 20100606
/* First version ever saved.  */
#define PCB_FILE_VERSION_BASELINE 20070407

int PCBFileVersionNeeded(void)
{
	ALLPOLYGON_LOOP(PCB->Data);
	{
		if (polygon->HoleIndexN > 0)
			return PCB_FILE_VERSION_HOLES;
	}
	ENDALL_LOOP;

	return PCB_FILE_VERSION_BASELINE;
}

/* ---------------------------------------------------------------------------
 * load PCB
 * parse the file with enabled 'PCB mode' (see parser)
 * if successful, update some other stuff
 *
 * If revert is true, we pass "revert" as a parameter
 * to the HID's PCBChanged action.
 */
int real_load_pcb(char *Filename, bool revert, bool require_font, bool is_misc)
{
	const char *unit_suffix;
	char *new_filename;
	PCBTypePtr newPCB = CreateNewPCB_(false);
	PCBTypePtr oldPCB;
#ifdef DEBUG
	double elapsed;
	clock_t start, end;

	start = clock();
#endif

#warning these should be moved to the caller {
	resolve_path(Filename, &new_filename, 0);

	oldPCB = PCB;
	PCB = newPCB;
#warning }

	/* mark the default font invalid to know if the file has one */
	newPCB->Font.Valid = false;

	/* new data isn't added to the undo list */
	if (!ParsePCB(PCB, new_filename)) {
		RemovePCB(oldPCB);

		CreateNewPCBPost(PCB, 0);
		ResetStackAndVisibility();

		if (!is_misc) {
			/* update cursor location */
			Crosshair.X = PCB_CLAMP(PCB->CursorX, 0, PCB->MaxWidth);
			Crosshair.Y = PCB_CLAMP(PCB->CursorY, 0, PCB->MaxHeight);

			/* update cursor confinement and output area (scrollbars) */
			ChangePCBSize(PCB->MaxWidth, PCB->MaxHeight);
		}

		/* enable default font if necessary */
		if (!PCB->Font.Valid) {
			if (require_font)
				Message(_("File '%s' has no font information, using default font\n"), new_filename);
			PCB->Font.Valid = true;
		}

		/* clear 'changed flag' */
		SetChangedFlag(false);
		PCB->Filename = new_filename;
		/* just in case a bad file saved file is loaded */

		/* Use attribute PCB::grid::unit as unit, if we can */
		unit_suffix = AttributeGet(PCB, "PCB::grid::unit");
		if (unit_suffix && *unit_suffix) {
			const Unit *new_unit = get_unit_struct(unit_suffix);
#warning TODO: we MUST NOT overwrite this here; should be handled by pcb-local settings
			if (new_unit)
				conf_core.editor.grid_unit = new_unit;
		}
		AttributePut(PCB, "PCB::grid::unit", conf_core.editor.grid_unit->suffix);

		sort_netlist();
		rats_patch_make_edited(PCB);

		set_some_route_style();

		if (!is_misc) {
			if (revert)
				hid_actionl("PCBChanged", "revert", NULL);
			else
				hid_action("PCBChanged");
		}

#ifdef DEBUG
		end = clock();
		elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
		gui->log("Loading file %s took %f seconds of CPU time\n", new_filename, elapsed);
#endif

		return (0);
	}

#warning these should be moved to the caller {
	PCB = oldPCB;
	hid_action("PCBChanged");

	/* release unused memory */
	RemovePCB(newPCB);
#warning }
	return (1);
}

/* ---------------------------------------------------------------------------
 * writes out an attribute list
 */
static void WriteAttributeList(FILE * FP, AttributeListTypePtr list, char *prefix)
{
	int i;

	for (i = 0; i < list->Number; i++)
		fprintf(FP, "%sAttribute(\"%s\" \"%s\")\n", prefix, list->List[i].name, list->List[i].value);
}

/* ---------------------------------------------------------------------------
 * writes layout header information
 */
static void WritePCBInfoHeader(FILE * FP)
{
	/* write some useful comments */
	fprintf(FP, "# release: %s " VERSION "\n", Progname);

	/* avoid writing things like user name or date, as these cause merge
	 * conflicts in collaborative environments using version control systems
	 */
}

/* ---------------------------------------------------------------------------
 * writes data header
 * the name of the PCB, cursor location, zoom and grid
 * layergroups and some flags
 */
static void WritePCBDataHeader(FILE * FP)
{
	Cardinal group;

	/*
	 * ************************** README *******************
	 * ************************** README *******************
	 *
	 * If the file format is modified in any way, update
	 * PCB_FILE_VERSION in file.h as well as PCBFileVersionNeeded()
	 * at the top of this file.
	 *  
	 * ************************** README *******************
	 * ************************** README *******************
	 */

	fprintf(FP, "\n# To read pcb files, the pcb version (or the git source date) must be >= the file version\n");
	fprintf(FP, "FileVersion[%i]\n", PCBFileVersionNeeded());

	fputs("\nPCB[", FP);
	PrintQuotedString(FP, (char *) EMPTY(PCB->Name));
	pcb_fprintf(FP, " %mr %mr]\n\n", PCB->MaxWidth, PCB->MaxHeight);
	pcb_fprintf(FP, "Grid[%.1mr %mr %mr %d]\n", PCB->Grid, PCB->GridOffsetX, PCB->GridOffsetY, conf_core.editor.draw_grid);
	pcb_fprintf(FP, "Cursor[%mr %mr %s]\n", Crosshair.X, Crosshair.Y, c_dtostr(PCB->Zoom));
	/* PolyArea should be output in square cmils, no suffix */
	fprintf(FP, "PolyArea[%s]\n", c_dtostr(COORD_TO_MIL(COORD_TO_MIL(PCB->IsleArea) * 100) * 100));
	pcb_fprintf(FP, "Thermal[%s]\n", c_dtostr(PCB->ThermScale));
	pcb_fprintf(FP, "DRC[%mr %mr %mr %mr %mr %mr]\n", PCB->Bloat, PCB->Shrink,
							PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
	fprintf(FP, "Flags(%s)\n", pcbflags_to_string(PCB->Flags));
	fprintf(FP, "Groups(\"%s\")\n", LayerGroupsToString(&PCB->LayerGroups));
	fputs("Styles[\"", FP);
	for (group = 0; group < NUM_STYLES - 1; group++)
		pcb_fprintf(FP, "%s,%mr,%mr,%mr,%mr:", PCB->RouteStyle[group].Name,
								PCB->RouteStyle[group].Thick,
								PCB->RouteStyle[group].Diameter, PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
	pcb_fprintf(FP, "%s,%mr,%mr,%mr,%mr\"]\n\n", PCB->RouteStyle[group].Name,
							PCB->RouteStyle[group].Thick,
							PCB->RouteStyle[group].Diameter, PCB->RouteStyle[group].Hole, PCB->RouteStyle[group].Keepaway);
}

/* ---------------------------------------------------------------------------
 * writes font data of non empty symbols
 */
static void WritePCBFontData(FILE * FP)
{
	Cardinal i, j;
	LineTypePtr line;
	FontTypePtr font;

	for (font = &PCB->Font, i = 0; i <= MAX_FONTPOSITION; i++) {
		if (!font->Symbol[i].Valid)
			continue;

		if (isprint(i))
			pcb_fprintf(FP, "Symbol['%c' %mr]\n(\n", i, font->Symbol[i].Delta);
		else
			pcb_fprintf(FP, "Symbol[%i %mr]\n(\n", i, font->Symbol[i].Delta);

		line = font->Symbol[i].Line;
		for (j = font->Symbol[i].LineN; j; j--, line++)
			pcb_fprintf(FP, "\tSymbolLine[%mr %mr %mr %mr %mr]\n",
									line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness);
		fputs(")\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes via data
 */
static void WriteViaData(FILE * FP, DataTypePtr Data)
{
	gdl_iterator_t it;
	PinType *via;

	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "Via[%mr %mr %mr %mr %mr %mr ", via->X, via->Y,
								via->Thickness, via->Clearance, via->Mask, via->DrillingHole);
		PrintQuotedString(FP, (char *) EMPTY(via->Name));
		fprintf(FP, " %s]\n", F2S(via, VIA_TYPE));
	}
}

/* ---------------------------------------------------------------------------
 * writes rat-line data
 */
static void WritePCBRatData(FILE * FP)
{
	gdl_iterator_t it;
	RatType *line;

	/* write information about rats */
	ratlist_foreach(&PCB->Data->Rat, &it, line) {
		pcb_fprintf(FP, "Rat[%mr %mr %d %mr %mr %d ",
								line->Point1.X, line->Point1.Y, line->group1, line->Point2.X, line->Point2.Y, line->group2);
		fprintf(FP, " %s]\n", F2S(line, RATLINE_TYPE));
	}
}

/* ---------------------------------------------------------------------------
 * writes netlist data
 */
static void WritePCBNetlistData(FILE * FP)
{
	/* write out the netlist if it exists */
	if (PCB->NetlistLib[NETLIST_INPUT].MenuN) {
		int n, p;
		fprintf(FP, "NetList()\n(\n");

		for (n = 0; n < PCB->NetlistLib[NETLIST_INPUT].MenuN; n++) {
			LibraryMenuTypePtr menu = &PCB->NetlistLib[NETLIST_INPUT].Menu[n];
			fprintf(FP, "\tNet(");
			PrintQuotedString(FP, &menu->Name[2]);
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) UNKNOWN(menu->Style));
			fprintf(FP, ")\n\t(\n");
			for (p = 0; p < menu->EntryN; p++) {
				LibraryEntryTypePtr entry = &menu->Entry[p];
				fprintf(FP, "\t\tConnect(");
				PrintQuotedString(FP, entry->ListEntry);
				fprintf(FP, ")\n");
			}
			fprintf(FP, "\t)\n");
		}
		fprintf(FP, ")\n");
	}
}

/* ---------------------------------------------------------------------------
 * writes netlist patch data
 */
static void WritePCBNetlistPatchData(FILE * FP)
{
	if (PCB->NetlistPatches != NULL) {
		fprintf(FP, "NetListPatch()\n(\n");
		rats_patch_fexport(PCB, FP, 1);
		fprintf(FP, ")\n");
	}
}

/* ---------------------------------------------------------------------------
 * writes element data
 */
static void WriteElementData(FILE * FP, DataTypePtr Data)
{
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
		fprintf(FP, "\nElement[%s ", F2S(element, ELEMENT_TYPE));
		PrintQuotedString(FP, (char *) EMPTY(DESCRIPTION_NAME(element)));
		fputc(' ', FP);
		PrintQuotedString(FP, (char *) EMPTY(NAMEONPCB_NAME(element)));
		fputc(' ', FP);
		PrintQuotedString(FP, (char *) EMPTY(VALUE_NAME(element)));
		pcb_fprintf(FP, " %mr %mr %mr %mr %d %d %s]\n(\n",
								element->MarkX, element->MarkY,
								DESCRIPTION_TEXT(element).X - element->MarkX,
								DESCRIPTION_TEXT(element).Y - element->MarkY,
								DESCRIPTION_TEXT(element).Direction,
								DESCRIPTION_TEXT(element).Scale, F2S(&(DESCRIPTION_TEXT(element)), ELEMENTNAME_TYPE));
		WriteAttributeList(FP, &element->Attributes, "\t");
		pinlist_foreach(&element->Pin, &it, pin) {
			pcb_fprintf(FP, "\tPin[%mr %mr %mr %mr %mr %mr ",
									pin->X - element->MarkX,
									pin->Y - element->MarkY, pin->Thickness, pin->Clearance, pin->Mask, pin->DrillingHole);
			PrintQuotedString(FP, (char *) EMPTY(pin->Name));
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) EMPTY(pin->Number));
			fprintf(FP, " %s]\n", F2S(pin, PIN_TYPE));
		}
		pinlist_foreach(&element->Pad, &it, pad) {
			pcb_fprintf(FP, "\tPad[%mr %mr %mr %mr %mr %mr %mr ",
									pad->Point1.X - element->MarkX,
									pad->Point1.Y - element->MarkY,
									pad->Point2.X - element->MarkX, pad->Point2.Y - element->MarkY, pad->Thickness, pad->Clearance, pad->Mask);
			PrintQuotedString(FP, (char *) EMPTY(pad->Name));
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) EMPTY(pad->Number));
			fprintf(FP, " %s]\n", F2S(pad, PAD_TYPE));
		}
		linelist_foreach(&element->Line, &it, line) {
			pcb_fprintf(FP, "\tElementLine [%mr %mr %mr %mr %mr]\n",
									line->Point1.X - element->MarkX,
									line->Point1.Y - element->MarkY,
									line->Point2.X - element->MarkX, line->Point2.Y - element->MarkY, line->Thickness);
		}
		linelist_foreach(&element->Arc, &it, arc) {
			pcb_fprintf(FP, "\tElementArc [%mr %mr %mr %mr %ma %ma %mr]\n",
									arc->X - element->MarkX,
									arc->Y - element->MarkY, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
		}
		fputs("\n\t)\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes layer data
 */
static void WriteLayerData(FILE * FP, Cardinal Number, LayerTypePtr layer)
{
	gdl_iterator_t it;
	LineType *line;
	ArcType *arc;
	TextType *text;
	PolygonType *polygon;

	/* write information about non empty layers */
	if (!LAYER_IS_EMPTY(layer) || (layer->Name && *layer->Name)) {
		fprintf(FP, "Layer(%i ", (int) Number + 1);
		PrintQuotedString(FP, (char *) EMPTY(layer->Name));
		fputs(")\n(\n", FP);
		WriteAttributeList(FP, &layer->Attributes, "\t");

		linelist_foreach(&layer->Line, &it, line) {
			pcb_fprintf(FP, "\tLine[%mr %mr %mr %mr %mr %mr %s]\n",
									line->Point1.X, line->Point1.Y,
									line->Point2.X, line->Point2.Y, line->Thickness, line->Clearance, F2S(line, LINE_TYPE));
		}
		arclist_foreach(&layer->Arc, &it, arc) {
			pcb_fprintf(FP, "\tArc[%mr %mr %mr %mr %mr %mr %ma %ma %s]\n",
									arc->X, arc->Y, arc->Width,
									arc->Height, arc->Thickness, arc->Clearance, arc->StartAngle, arc->Delta, F2S(arc, ARC_TYPE));
		}
		textlist_foreach(&layer->Text, &it, text) {
			pcb_fprintf(FP, "\tText[%mr %mr %d %d ", text->X, text->Y, text->Direction, text->Scale);
			PrintQuotedString(FP, (char *) EMPTY(text->TextString));
			fprintf(FP, " %s]\n", F2S(text, TEXT_TYPE));
		}
		textlist_foreach(&layer->Polygon, &it, polygon) {
			int p, i = 0;
			Cardinal hole = 0;
			fprintf(FP, "\tPolygon(%s)\n\t(", F2S(polygon, POLYGON_TYPE));
			for (p = 0; p < polygon->PointN; p++) {
				PointTypePtr point = &polygon->Points[p];

				if (hole < polygon->HoleIndexN && p == polygon->HoleIndex[hole]) {
					if (hole > 0)
						fputs("\n\t\t)", FP);
					fputs("\n\t\tHole (", FP);
					hole++;
					i = 0;
				}

				if (i++ % 5 == 0) {
					fputs("\n\t\t", FP);
					if (hole)
						fputs("\t", FP);
				}
				pcb_fprintf(FP, "[%mr %mr] ", point->X, point->Y);
			}
			if (hole > 0)
				fputs("\n\t\t)", FP);
			fputs("\n\t)\n", FP);
		}
		fputs(")\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes just the elements in the buffer to file
 */
static int WriteBuffer(FILE * FP)
{
	Cardinal i;

	WriteViaData(FP, PASTEBUFFER->Data);
	WriteElementData(FP, PASTEBUFFER->Data);
	for (i = 0; i < max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(PASTEBUFFER->Data->Layer[i]));
	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
static int WritePCB(FILE * FP)
{
	Cardinal i;

	WritePCBInfoHeader(FP);
	WritePCBDataHeader(FP);
	WritePCBFontData(FP);
	WriteAttributeList(FP, &PCB->Attributes, "");
	WriteViaData(FP, PCB->Data);
	WriteElementData(FP, PCB->Data);
	WritePCBRatData(FP);
	for (i = 0; i < max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(PCB->Data->Layer[i]));
	WritePCBNetlistData(FP);
	WritePCBNetlistPatchData(FP);

	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int WritePCBFile(char *Filename)
{
	FILE *fp;
	int result;

	if ((fp = fopen(Filename, "w")) == NULL) {
		OpenErrorMessage(Filename);
		return (STATUS_ERROR);
	}
	result = WritePCB(fp);
	fclose(fp);
	return (result);
}

/* ---------------------------------------------------------------------------
 * writes to pipe using the command defined by conf_core.rc.save_command
 * %f are replaced by the passed filename
 */
int WritePipe(char *Filename, bool thePcb)
{
	FILE *fp;
	int result;
	char *p;
	static gds_t command;
	int used_popen = 0;

	if (EMPTY_STRING_P(conf_core.rc.save_command)) {
		fp = fopen(Filename, "w");
		if (fp == 0) {
			Message("Unable to write to file %s\n", Filename);
			return STATUS_ERROR;
		}
	}
	else {
		used_popen = 1;
		/* setup commandline */
		gds_truncate(&command,0);
		for (p = conf_core.rc.save_command; *p; p++) {
			/* copy character if not special or add string to command */
			if (!(*p == '%' && *(p + 1) == 'f'))
				gds_append(&command, *p);
			else {
				gds_append_str(&command, Filename);

				/* skip the character */
				p++;
			}
		}
		printf("write to pipe \"%s\"\n", command.array);
		if ((fp = popen(command.array, "w")) == NULL) {
			PopenErrorMessage(command.array);
			return (STATUS_ERROR);
		}
	}
	if (thePcb) {
		if (PCB->is_footprint) {
			WriteElementData(fp, PCB->Data);
			result = 0;
		}
		else
			result = WritePCB(fp);
	}
	else
		result = WriteBuffer(fp);

	if (used_popen)
		return (pclose(fp) ? STATUS_ERROR : result);
	return (fclose(fp) ? STATUS_ERROR : result);
}
