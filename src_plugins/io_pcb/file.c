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

/* file save, load, merge ... routines */

#include "config.h"
#include "conf_core.h"

#include <locale.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "change.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "plug_io.h"
#include "hid.h"
#include "layer.h"
#include "move.h"
#include "parse_common.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "set.h"
#include "flag_str.h"
#include "compat_fs.h"
#include "paths.h"
#include "rats_patch.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "flags.h"
#include "attribs.h"
#include "route_style.h"
#include "obj_poly.h"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void WritePCBInfoHeader(FILE *);
static void WritePCBDataHeader(FILE *);
static void WritePCBFontData(FILE *);
static void WriteViaData(FILE *, pcb_data_t *);
static void WritePCBRatData(FILE *);
static void WriteLayerData(FILE *, pcb_cardinal_t, pcb_layer_t *);

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

/* In future all use of this should be supplanted by
 * pcb-printf and %mr/%m# spec */
static const char *c_dtostr(double d)
{
	static char buf[100];
	int i, f;
	char *bufp = buf;

	if (d < 0) {
		*bufp++ = '-';
		d = -d;
	}
	d += 0.0000005;								/* rounding */
	i = floor(d);
	d -= i;
	sprintf(bufp, "%d", i);
	bufp += strlen(bufp);
	*bufp++ = '.';

	f = floor(d * 1000000.0);
	sprintf(bufp, "%06d", f);
	return buf;
}


/* ---------------------------------------------------------------------------
 * writes out an attribute list
 */
static void WriteAttributeList(FILE * FP, AttributeListTypePtr list, const char *prefix)
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
	fprintf(FP, "# release: pcb-rnd " VERSION "\n");

	/* avoid writing things like user name or date, as these cause merge
	 * conflicts in collaborative environments using version control systems
	 */
}

static void conf_update_pcb_flag(FlagType *dest, const char *hash_path, int binflag)
{
	conf_native_t *n = conf_get_field(hash_path);
	struct {
		FlagType Flags;
	} *tmp = (void *)dest;

	if ((n == NULL) || (n->type != CFN_BOOLEAN) || (n->used < 0) || (!n->val.boolean[0]))
		CLEAR_FLAG(binflag, tmp);
	else
		SET_FLAG(binflag, tmp);
}

/* ---------------------------------------------------------------------------
 * writes data header
 * the name of the PCB, cursor location, zoom and grid
 * layergroups and some flags
 */
static void WritePCBDataHeader(FILE * FP)
{
	int group;
	FlagType pcb_flags;

	memset(&pcb_flags, 0, sizeof(pcb_flags));

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

	io_pcb_attrib_c2a(PCB);

	/* set binary flags from conf hash; these flags used to be checked
	   with TEST_FLAG() but got moved to the conf system */
	conf_update_pcb_flag(&pcb_flags, "plugins/mincut/enable", ENABLEPCB_FLAG_MINCUT);
	conf_update_pcb_flag(&pcb_flags, "editor/show_number", SHOWNUMBERFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/show_drc", SHOWPCB_FLAG_DRC);
	conf_update_pcb_flag(&pcb_flags, "editor/rubber_band_mode", RUBBERBANDFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/auto_drc", AUTOPCB_FLAG_DRC);
	conf_update_pcb_flag(&pcb_flags, "editor/all_direction_lines", ALLDIRECTIONFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/swap_start_direction", SWAPSTARTDIRFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/unique_names", UNIQUENAMEFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/clear_line", CLEARNEWFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/full_poly", NEWPCB_FLAG_FULLPOLY);
	conf_update_pcb_flag(&pcb_flags, "editor/snap_pin", SNAPPCB_FLAG_PIN);
	conf_update_pcb_flag(&pcb_flags, "editor/orthogonal_moves", ORTHOMOVEFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/live_routing", LIVEROUTEFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/lock_names", LOCKNAMESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/only_names", ONLYNAMESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/hide_names", HIDENAMESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/thin_draw", THINDRAWFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/thin_draw_poly", THINDRAWPOLYFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/local_ref", LOCALREFFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/check_planes",CHECKPLANESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/description", DESCRIPTIONFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/name_on_pcb", NAMEONPCBFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/show_mask", SHOWMASKFLAG);

	fprintf(FP, "\n# To read pcb files, the pcb version (or the git source date) must be >= the file version\n");
	fprintf(FP, "FileVersion[%i]\n", PCBFileVersionNeeded());

	fputs("\nPCB[", FP);
	PrintQuotedString(FP, (char *) EMPTY(PCB->Name));
	pcb_fprintf(FP, " %[0] %[0]]\n\n", PCB->MaxWidth, PCB->MaxHeight);
	pcb_fprintf(FP, "Grid[%[0] %[0] %[0] %d]\n", PCB->Grid, PCB->GridOffsetX, PCB->GridOffsetY, conf_core.editor.draw_grid);
	pcb_fprintf(FP, "Cursor[%[0] %[0] %s]\n", Crosshair.X, Crosshair.Y, c_dtostr(PCB->Zoom));
	/* PolyArea should be output in square cmils, no suffix */
	fprintf(FP, "PolyArea[%s]\n", c_dtostr(PCB_COORD_TO_MIL(PCB_COORD_TO_MIL(PCB->IsleArea) * 100) * 100));
	pcb_fprintf(FP, "Thermal[%s]\n", c_dtostr(PCB->ThermScale));
	pcb_fprintf(FP, "DRC[%[0] %[0] %[0] %[0] %[0] %[0]]\n", PCB->Bloat, PCB->Shrink,
							PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
	fprintf(FP, "Flags(%s)\n", pcbflags_to_string(pcb_flags));
	fprintf(FP, "Groups(\"%s\")\n", LayerGroupsToString(&PCB->LayerGroups));
	fputs("Styles[\"", FP);

	if (vtroutestyle_len(&PCB->RouteStyle) > 0) {
		for (group = 0; group < vtroutestyle_len(&PCB->RouteStyle) - 1; group++)
			pcb_fprintf(FP, "%s,%[0],%[0],%[0],%[0]:", PCB->RouteStyle.array[group].name,
									PCB->RouteStyle.array[group].Thick,
									PCB->RouteStyle.array[group].Diameter, PCB->RouteStyle.array[group].Hole, PCB->RouteStyle.array[group].Clearance);
		pcb_fprintf(FP, "%s,%[0],%[0],%[0],%[0]\"]\n\n", PCB->RouteStyle.array[group].name,
								PCB->RouteStyle.array[group].Thick,
								PCB->RouteStyle.array[group].Diameter, PCB->RouteStyle.array[group].Hole, PCB->RouteStyle.array[group].Clearance);
	}
	else
		fprintf(FP, "\"]\n\n");
}

/* ---------------------------------------------------------------------------
 * writes font data of non empty symbols
 */
static void WritePCBFontData(FILE * FP)
{
	pcb_cardinal_t i, j;
	pcb_line_t *line;
	pcb_font_t *font;

	for (font = &PCB->Font, i = 0; i <= MAX_FONTPOSITION; i++) {
		if (!font->Symbol[i].Valid)
			continue;

		if (isprint(i))
			pcb_fprintf(FP, "Symbol['%c' %[0]]\n(\n", i, font->Symbol[i].Delta);
		else
			pcb_fprintf(FP, "Symbol[%i %[0]]\n(\n", i, font->Symbol[i].Delta);

		line = font->Symbol[i].Line;
		for (j = font->Symbol[i].LineN; j; j--, line++)
			pcb_fprintf(FP, "\tSymbolLine[%[0] %[0] %[0] %[0] %[0]]\n",
									line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y, line->Thickness);
		fputs(")\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes via data
 */
static void WriteViaData(FILE * FP, pcb_data_t *Data)
{
	gdl_iterator_t it;
	PinType *via;

	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "Via[%[0] %[0] %[0] %[0] %[0] %[0] ", via->X, via->Y,
								via->Thickness, via->Clearance, via->Mask, via->DrillingHole);
		PrintQuotedString(FP, (char *) EMPTY(via->Name));
		fprintf(FP, " %s]\n", F2S(via, PCB_TYPE_VIA));
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
		pcb_fprintf(FP, "Rat[%[0] %[0] %d %[0] %[0] %d ",
								line->Point1.X, line->Point1.Y, line->group1, line->Point2.X, line->Point2.Y, line->group2);
		fprintf(FP, " %s]\n", F2S(line, PCB_TYPE_RATLINE));
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
int io_pcb_WriteElementData(plug_io_t *ctx, FILE * FP, pcb_data_t *Data)
{
	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	ElementType *element;

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;
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
		fprintf(FP, "\nElement[%s ", F2S(element, PCB_TYPE_ELEMENT));
		PrintQuotedString(FP, (char *) EMPTY(DESCRIPTION_NAME(element)));
		fputc(' ', FP);
		PrintQuotedString(FP, (char *) EMPTY(NAMEONPCB_NAME(element)));
		fputc(' ', FP);
		PrintQuotedString(FP, (char *) EMPTY(VALUE_NAME(element)));
		pcb_fprintf(FP, " %[0] %[0] %[0] %[0] %d %d %s]\n(\n",
								element->MarkX, element->MarkY,
								DESCRIPTION_TEXT(element).X - element->MarkX,
								DESCRIPTION_TEXT(element).Y - element->MarkY,
								DESCRIPTION_TEXT(element).Direction,
								DESCRIPTION_TEXT(element).Scale, F2S(&(DESCRIPTION_TEXT(element)), PCB_TYPE_ELEMENT_NAME));
		WriteAttributeList(FP, &element->Attributes, "\t");
		pinlist_foreach(&element->Pin, &it, pin) {
			pcb_fprintf(FP, "\tPin[%[0] %[0] %[0] %[0] %[0] %[0] ",
									pin->X - element->MarkX,
									pin->Y - element->MarkY, pin->Thickness, pin->Clearance, pin->Mask, pin->DrillingHole);
			PrintQuotedString(FP, (char *) EMPTY(pin->Name));
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) EMPTY(pin->Number));
			fprintf(FP, " %s]\n", F2S(pin, PCB_TYPE_PIN));
		}
		pinlist_foreach(&element->Pad, &it, pad) {
			pcb_fprintf(FP, "\tPad[%[0] %[0] %[0] %[0] %[0] %[0] %[0] ",
									pad->Point1.X - element->MarkX,
									pad->Point1.Y - element->MarkY,
									pad->Point2.X - element->MarkX, pad->Point2.Y - element->MarkY, pad->Thickness, pad->Clearance, pad->Mask);
			PrintQuotedString(FP, (char *) EMPTY(pad->Name));
			fprintf(FP, " ");
			PrintQuotedString(FP, (char *) EMPTY(pad->Number));
			fprintf(FP, " %s]\n", F2S(pad, PCB_TYPE_PAD));
		}
		linelist_foreach(&element->Line, &it, line) {
			pcb_fprintf(FP, "\tElementLine [%[0] %[0] %[0] %[0] %[0]]\n",
									line->Point1.X - element->MarkX,
									line->Point1.Y - element->MarkY,
									line->Point2.X - element->MarkX, line->Point2.Y - element->MarkY, line->Thickness);
		}
		linelist_foreach(&element->Arc, &it, arc) {
			pcb_fprintf(FP, "\tElementArc [%[0] %[0] %[0] %[0] %ma %ma %[0]]\n",
									arc->X - element->MarkX,
									arc->Y - element->MarkY, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
		}
		fputs("\n\t)\n", FP);
	}
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes layer data
 */
static void WriteLayerData(FILE * FP, pcb_cardinal_t Number, pcb_layer_t *layer)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_arc_t *arc;
	TextType *text;
	PolygonType *polygon;

	/* write information about non empty layers */
	if (!LAYER_IS_EMPTY(layer) || (layer->Name && *layer->Name)) {
		fprintf(FP, "Layer(%i ", (int) Number + 1);
		PrintQuotedString(FP, (char *) EMPTY(layer->Name));
		fputs(")\n(\n", FP);
		WriteAttributeList(FP, &layer->Attributes, "\t");

		linelist_foreach(&layer->Line, &it, line) {
			pcb_fprintf(FP, "\tLine[%[0] %[0] %[0] %[0] %[0] %[0] %s]\n",
									line->Point1.X, line->Point1.Y,
									line->Point2.X, line->Point2.Y, line->Thickness, line->Clearance, F2S(line, PCB_TYPE_LINE));
		}
		arclist_foreach(&layer->Arc, &it, arc) {
			pcb_fprintf(FP, "\tArc[%[0] %[0] %[0] %[0] %[0] %[0] %ma %ma %s]\n",
									arc->X, arc->Y, arc->Width,
									arc->Height, arc->Thickness, arc->Clearance, arc->StartAngle, arc->Delta, F2S(arc, PCB_TYPE_ARC));
		}
		textlist_foreach(&layer->Text, &it, text) {
			pcb_fprintf(FP, "\tText[%[0] %[0] %d %d ", text->X, text->Y, text->Direction, text->Scale);
			PrintQuotedString(FP, (char *) EMPTY(text->TextString));
			fprintf(FP, " %s]\n", F2S(text, PCB_TYPE_TEXT));
		}
		textlist_foreach(&layer->Polygon, &it, polygon) {
			int p, i = 0;
			pcb_cardinal_t hole = 0;
			fprintf(FP, "\tPolygon(%s)\n\t(", F2S(polygon, PCB_TYPE_POLYGON));
			for (p = 0; p < polygon->PointN; p++) {
				pcb_point_t *point = &polygon->Points[p];

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
				pcb_fprintf(FP, "[%[0] %[0]] ", point->X, point->Y);
			}
			if (hole > 0)
				fputs("\n\t\t)", FP);
			fputs("\n\t)\n", FP);
		}
		fputs(")\n", FP);
	}
}

/* ---------------------------------------------------------------------------
 * writes the buffer to file
 */
int io_pcb_WriteBuffer(plug_io_t *ctx, FILE * FP, pcb_buffer_t *buff)
{
	pcb_cardinal_t i;

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;
	WriteViaData(FP, buff->Data);
	io_pcb_WriteElementData(ctx, FP, buff->Data);
	for (i = 0; i < max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(buff->Data->Layer[i]));
	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int io_pcb_WritePCB(plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	pcb_cardinal_t i;

	AttributePutToList(&PCB->Attributes, "PCB::loader", ctx->description, 1);

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;
	WritePCBInfoHeader(FP);
	WritePCBDataHeader(FP);
	WritePCBFontData(FP);
	WriteAttributeList(FP, &PCB->Attributes, "");
	WriteViaData(FP, PCB->Data);
	io_pcb_WriteElementData(ctx, FP, PCB->Data);
	WritePCBRatData(FP);
	for (i = 0; i < max_copper_layer + 2; i++)
		WriteLayerData(FP, i, &(PCB->Data->Layer[i]));
	WritePCBNetlistData(FP);
	WritePCBNetlistPatchData(FP);

	return (STATUS_OK);
}

/* ---------------------------------------------------------------------------
 * functions for loading elements-as-pcb
 */

extern pcb_board_t *yyPCB;
extern pcb_data_t *yyData;
extern pcb_font_t *yyFont;

void PreLoadElementPCB()
{

	if (!yyPCB)
		return;

	yyFont = &yyPCB->Font;
	yyData = yyPCB->Data;
	yyData->pcb = yyPCB;
	yyData->LayerN = 0;
}

void PostLoadElementPCB()
{
	pcb_board_t *pcb_save = PCB;
	ElementTypePtr e;

	if (!yyPCB)
		return;

	CreateNewPCBPost(yyPCB, 0);
	ParseGroupString("1,c:2,s", &yyPCB->LayerGroups, yyData->LayerN);
	e = elementlist_first(&yyPCB->Data->Element);	/* we know there's only one */
	PCB = yyPCB;
	MoveElementLowLevel(yyPCB->Data, e, -e->BoundingBox.X1, -e->BoundingBox.Y1);
	PCB = pcb_save;
	yyPCB->MaxWidth = e->BoundingBox.X2;
	yyPCB->MaxHeight = e->BoundingBox.Y2;
	yyPCB->is_footprint = 1;
}
