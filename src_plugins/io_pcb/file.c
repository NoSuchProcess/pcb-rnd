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
#include "flag_str.h"
#include "compat_fs.h"
#include "compat_misc.h"
#include "paths.h"
#include "rats_patch.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "flag_str.h"
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

#define F2S(OBJ, TYPE) pcb_strflg_f2s((OBJ)->Flags, TYPE)

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
	PCB_POLY_ALL_LOOP(PCB->Data);
	{
		if (polygon->HoleIndexN > 0)
			return PCB_FILE_VERSION_HOLES;
	}
	PCB_ENDALL_LOOP;

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

/* Returns pointer to private buffer */
static char *LayerGroupsToString(pcb_layer_stack_t *lg)
{
#if PCB_MAX_LAYER < 9998
	/* Allows for layer numbers 0..9999 */
	static char buf[(PCB_MAX_LAYER + 2) * 5 + 1];
#endif
	char *cp = buf;
	char sep = 0;
	int group, entry;
#warning layer TODO: revise this loop to save only what the original code saved
	for (group = 0; group < pcb_max_group; group++)
		if (PCB->LayerGroups.grp[group].len) {
			unsigned int gflg = pcb_layergrp_flags(group);

			if (gflg & PCB_LYT_SILK) /* silk is hacked in asusming there's a top and bottom copper */
				continue;

			if (sep)
				*cp++ = ':';
			sep = 1;
			for (entry = 0; entry < PCB->LayerGroups.grp[group].len; entry++) {
				pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[entry];
				unsigned int lflg = pcb_layer_flags(layer);


				sprintf(cp, "%ld", layer + 1);
				while (*++cp);

				if (entry != PCB->LayerGroups.grp[group].len - 1)
					*cp++ = ',';
			}

			if ((gflg & PCB_LYT_COPPER) && (gflg & PCB_LYT_TOP)) {
				*cp++ = ',';
				*cp++ = 'c';
			}
			else if ((gflg & PCB_LYT_COPPER) && (gflg & PCB_LYT_BOTTOM)) {
				*cp++ = ',';
				*cp++ = 's';
			}
		}
	*cp++ = 0;
	return buf;
}


/* ---------------------------------------------------------------------------
 * writes out an attribute list
 */
static void WriteAttributeList(FILE * FP, pcb_attribute_list_t *list, const char *prefix)
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

static void conf_update_pcb_flag(pcb_flag_t *dest, const char *hash_path, int binflag)
{
	conf_native_t *n = conf_get_field(hash_path);
	struct {
		pcb_flag_t Flags;
	} *tmp = (void *)dest;

	if ((n == NULL) || (n->type != CFN_BOOLEAN) || (n->used < 0) || (!n->val.boolean[0]))
		PCB_FLAG_CLEAR(binflag, tmp);
	else
		PCB_FLAG_SET(binflag, tmp);
}

/* ---------------------------------------------------------------------------
 * writes data header
 * the name of the PCB, cursor location, zoom and grid
 * layergroups and some flags
 */
static void WritePCBDataHeader(FILE * FP)
{
	int group;
	pcb_flag_t pcb_flags;

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
	   with PCB_FLAG_TEST() but got moved to the conf system */
	conf_update_pcb_flag(&pcb_flags, "plugins/mincut/enable", PCB_ENABLEPCB_FLAG_MINCUT);
	conf_update_pcb_flag(&pcb_flags, "editor/show_number", PCB_SHOWNUMBERFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/show_drc", PCB_SHOWPCB_FLAG_DRC);
	conf_update_pcb_flag(&pcb_flags, "editor/rubber_band_mode", PCB_RUBBERBANDFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/auto_drc", PCB_AUTOPCB_FLAG_DRC);
	conf_update_pcb_flag(&pcb_flags, "editor/all_direction_lines", PCB_ALLDIRECTIONFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/swap_start_direction", PCB_SWAPSTARTDIRFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/unique_names", PCB_UNIQUENAMEFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/clear_line", PCB_CLEARNEWFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/full_poly", PCB_NEWPCB_FLAG_FULLPOLY);
	conf_update_pcb_flag(&pcb_flags, "editor/snap_pin", PCB_SNAPPCB_FLAG_PIN);
	conf_update_pcb_flag(&pcb_flags, "editor/orthogonal_moves", PCB_ORTHOMOVEFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/live_routing", PCB_LIVEROUTEFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/lock_names", PCB_LOCKNAMESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/only_names", PCB_ONLYNAMESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/hide_names", PCB_HIDENAMESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/thin_draw", PCB_THINDRAWFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/thin_draw_poly", PCB_THINDRAWPOLYFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/local_ref", PCB_LOCALREFFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/check_planes",PCB_CHECKPLANESFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/description", PCB_DESCRIPTIONFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/name_on_pcb", PCB_NAMEONPCBFLAG);
	conf_update_pcb_flag(&pcb_flags, "editor/show_mask", PCB_SHOWMASKFLAG);

	fprintf(FP, "\n# To read pcb files, the pcb version (or the git source date) must be >= the file version\n");
	fprintf(FP, "FileVersion[%i]\n", PCBFileVersionNeeded());

	fputs("\nPCB[", FP);
	pcb_print_quoted_string(FP, (char *) PCB_EMPTY(PCB->Name));
	pcb_fprintf(FP, " %[0] %[0]]\n\n", PCB->MaxWidth, PCB->MaxHeight);
	pcb_fprintf(FP, "Grid[%[0] %[0] %[0] %d]\n", PCB->Grid, PCB->GridOffsetX, PCB->GridOffsetY, conf_core.editor.draw_grid);
	pcb_fprintf(FP, "Cursor[%[0] %[0] %s]\n", pcb_crosshair.X, pcb_crosshair.Y, c_dtostr(PCB->Zoom));
	/* PolyArea should be output in square cmils, no suffix */
	fprintf(FP, "PolyArea[%s]\n", c_dtostr(PCB_COORD_TO_MIL(PCB_COORD_TO_MIL(PCB->IsleArea) * 100) * 100));
	pcb_fprintf(FP, "Thermal[%s]\n", c_dtostr(PCB->ThermScale));
	pcb_fprintf(FP, "DRC[%[0] %[0] %[0] %[0] %[0] %[0]]\n", PCB->Bloat, PCB->Shrink,
							PCB->minWid, PCB->minSlk, PCB->minDrill, PCB->minRing);
	fprintf(FP, "Flags(%s)\n", pcb_strflg_board_f2s(pcb_flags));
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

	for (font = pcb_font(PCB, 0, 1), i = 0; i <= PCB_MAX_FONTPOSITION; i++) {
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
	pcb_pin_t *via;

	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "Via[%[0] %[0] %[0] %[0] %[0] %[0] ", via->X, via->Y,
								via->Thickness, via->Clearance, via->Mask, via->DrillingHole);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(via->Name));
		fprintf(FP, " %s]\n", F2S(via, PCB_TYPE_VIA));
	}
}

/* ---------------------------------------------------------------------------
 * writes rat-line data
 */
static void WritePCBRatData(FILE * FP)
{
	gdl_iterator_t it;
	pcb_rat_t *line;

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
	if (PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN) {
		int n, p;
		fprintf(FP, "NetList()\n(\n");

		for (n = 0; n < PCB->NetlistLib[PCB_NETLIST_INPUT].MenuN; n++) {
			pcb_lib_menu_t *menu = &PCB->NetlistLib[PCB_NETLIST_INPUT].Menu[n];
			fprintf(FP, "\tNet(");
			pcb_print_quoted_string(FP, &menu->Name[2]);
			fprintf(FP, " ");
			pcb_print_quoted_string(FP, (char *) PCB_UNKNOWN(menu->Style));
			fprintf(FP, ")\n\t(\n");
			for (p = 0; p < menu->EntryN; p++) {
				pcb_lib_entry_t *entry = &menu->Entry[p];
				fprintf(FP, "\t\tConnect(");
				pcb_print_quoted_string(FP, entry->ListEntry);
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
		pcb_ratspatch_fexport(PCB, FP, 1);
		fprintf(FP, ")\n");
	}
}

/* ---------------------------------------------------------------------------
 * writes element data
 */
int io_pcb_WriteElementData(pcb_plug_io_t *ctx, FILE * FP, pcb_data_t *Data)
{
	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_element_t *element;

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;
	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;
		/* the coordinates and text-flags are the same for
		 * both names of an element
		 */
		fprintf(FP, "\nElement[%s ", F2S(element, PCB_TYPE_ELEMENT));
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(PCB_ELEM_NAME_DESCRIPTION(element)));
		fputc(' ', FP);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(PCB_ELEM_NAME_REFDES(element)));
		fputc(' ', FP);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(PCB_ELEM_NAME_VALUE(element)));
		pcb_fprintf(FP, " %[0] %[0] %[0] %[0] %d %d %s]\n(\n",
								element->MarkX, element->MarkY,
								PCB_ELEM_TEXT_DESCRIPTION(element).X - element->MarkX,
								PCB_ELEM_TEXT_DESCRIPTION(element).Y - element->MarkY,
								PCB_ELEM_TEXT_DESCRIPTION(element).Direction,
								PCB_ELEM_TEXT_DESCRIPTION(element).Scale, F2S(&(PCB_ELEM_TEXT_DESCRIPTION(element)), PCB_TYPE_ELEMENT_NAME));
		WriteAttributeList(FP, &element->Attributes, "\t");
		pinlist_foreach(&element->Pin, &it, pin) {
			pcb_fprintf(FP, "\tPin[%[0] %[0] %[0] %[0] %[0] %[0] ",
									pin->X - element->MarkX,
									pin->Y - element->MarkY, pin->Thickness, pin->Clearance, pin->Mask, pin->DrillingHole);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
			fprintf(FP, " ");
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number));
			fprintf(FP, " %s]\n", F2S(pin, PCB_TYPE_PIN));
		}
		pinlist_foreach(&element->Pad, &it, pad) {
			pcb_fprintf(FP, "\tPad[%[0] %[0] %[0] %[0] %[0] %[0] %[0] ",
									pad->Point1.X - element->MarkX,
									pad->Point1.Y - element->MarkY,
									pad->Point2.X - element->MarkX, pad->Point2.Y - element->MarkY, pad->Thickness, pad->Clearance, pad->Mask);
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pad->Name));
			fprintf(FP, " ");
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pad->Number));
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
	pcb_text_t *text;
	pcb_polygon_t *polygon;

	/* write information about non empty layers */
	if (!PCB_LAYER_IS_EMPTY(layer) || (layer->Name && *layer->Name)) {
		fprintf(FP, "Layer(%i ", (int) Number + 1);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->Name));
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
			pcb_print_quoted_string(FP, (char *) PCB_EMPTY(text->TextString));
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
int io_pcb_WriteBuffer(pcb_plug_io_t *ctx, FILE * FP, pcb_buffer_t *buff)
{
	pcb_cardinal_t i;

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;
	WriteViaData(FP, buff->Data);
	io_pcb_WriteElementData(ctx, FP, buff->Data);
	for (i = 0; i < pcb_max_layer; i++)
		WriteLayerData(FP, i, &(buff->Data->Layer[i]));
	return (0);
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int io_pcb_WritePCB(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	pcb_cardinal_t i;

	pcb_attribute_put(&PCB->Attributes, "PCB::loader", ctx->description, 1);

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;
	WritePCBInfoHeader(FP);
	WritePCBDataHeader(FP);
	WritePCBFontData(FP);
	WriteAttributeList(FP, &PCB->Attributes, "");
	WriteViaData(FP, PCB->Data);
	io_pcb_WriteElementData(ctx, FP, PCB->Data);
	WritePCBRatData(FP);
	for (i = 0; i < pcb_max_layer; i++)
		WriteLayerData(FP, i, &(PCB->Data->Layer[i]));
	WritePCBNetlistData(FP);
	WritePCBNetlistPatchData(FP);

	return (0);
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

	yyFont = &yyPCB->fontkit.dflt;
	yyData = yyPCB->Data;
	yyData->pcb = yyPCB;
	yyData->LayerN = 0;
}

void PostLoadElementPCB()
{
	pcb_board_t *pcb_save = PCB;
	pcb_element_t *e;

	if (!yyPCB)
		return;

	pcb_board_new_postproc(yyPCB, 0);
	pcb_layer_parse_group_string("1,c:2,s", &yyPCB->LayerGroups, yyData->LayerN, 0);
	e = elementlist_first(&yyPCB->Data->Element);	/* we know there's only one */
	PCB = yyPCB;
	pcb_element_move(yyPCB->Data, e, -e->BoundingBox.X1, -e->BoundingBox.Y1);
	PCB = pcb_save;
	yyPCB->MaxWidth = e->BoundingBox.X2;
	yyPCB->MaxHeight = e->BoundingBox.Y2;
	yyPCB->is_footprint = 1;
}


int io_pcb_test_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, FILE *f)
{
	char line[1024];
	int bad = 0;

/*
	Look for any of these in the top few lines of the file:
	# release: pcb-bin 20050609
	PCB["name" 600000 500000]

	or
	PCB("name" 600000 500000]
*/

	while(!(feof(f))) {
		if (fgets(line, sizeof(line), f) != NULL) {
			char *s = line;
			while(isspace(*s)) s++;
			if ((strncmp(s, "# release: pcb", 14) == 0) )
				return 1;
			if ((strncmp(s, "PCB", 3) == 0) && ((s[3] == '(') || (s[3] == '[')))
				return 1;
			if ((strncmp(s, "Element", 7) == 0) && ((s[7] == '(') || (s[7] == '[')))
				return 1;
			if ((*s == '\r') || (*s == '\n') || (*s == '#') || (*s == '\0')) /* ignore empty lines and comments */
				continue;
			/* non-comment, non-empty line: tolerate at most 16 of these before giving up */
			bad++;
			if (bad > 16)
				return 0;
		}
	}

	/* hit eof before finding anything familiar or too many bad lines: the file
	is surely not a .pcb */
	return 0;
}


pcb_layer_id_t static new_ly_end(pcb_board_t *pcb, const char *name)
{
	if (pcb->Data->LayerN >= PCB_MAX_LAYER)
		return -1;
	pcb->Data->Layer[pcb->Data->LayerN].Name = pcb_strdup(name);
	return pcb->Data->LayerN++;
}

pcb_layer_id_t static new_ly_old(pcb_board_t *pcb, const char *name)
{
	pcb_layer_id_t lid;
	for(lid = 0; lid < PCB_MAX_LAYER; lid++) {
		if (pcb->Data->Layer[lid].grp == 0) {
			free(pcb->Data->Layer[lid].Name);
			pcb->Data->Layer[lid].Name = pcb_strdup(name);
			return lid;
		}
	}
	return -1;
}

#warning TODO: make pcb_layer_add_in_group_ accept pcb* and use that instead
static int add_in_group(pcb_board_t *pcb, pcb_layer_group_t *grp, pcb_layergrp_id_t group_id, pcb_layer_id_t layer_id)
{
	if ((layer_id < 0) || (layer_id >= pcb_max_layer))
		return -1;

	grp->lid[grp->len] = layer_id;
	grp->len++;
	pcb->Data->Layer[layer_id].grp = group_id;

	return 0;
}

int pcb_layer_improvise(pcb_board_t *pcb)
{
	pcb_layergrp_id_t gid;
	pcb_layer_id_t lid, silk = -1;

	pcb_layer_group_setup_default(&pcb->LayerGroups);

	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		if (strcmp(pcb->Data->Layer[lid].Name, "silk") == 0) {
			if (silk < 0)
				pcb_layer_group_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, &gid, 1);
			else
				pcb_layer_group_list(PCB_LYT_TOP | PCB_LYT_SILK, &gid, 1);
			add_in_group(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
			silk = lid;
		}
		else {
			pcb_layer_group_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, &gid, 1);
			add_in_group(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
		}
	}

	pcb_layer_group_list(PCB_LYT_BOTTOM | PCB_LYT_SILK, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = new_ly_end(pcb, "silk");
		if (lid < 0)
			return -1;
		add_in_group(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

	pcb_layer_group_list(PCB_LYT_TOP | PCB_LYT_SILK, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = new_ly_end(pcb, "silk");
		if (lid < 0)
			return -1;
		add_in_group(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

	pcb_layer_group_list(PCB_LYT_TOP | PCB_LYT_COPPER, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = new_ly_old(pcb, "top_copper");
		if (lid < 0)
			return -1;
		add_in_group(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

	pcb_layer_group_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = new_ly_old(pcb, "bottom_copper");
		if (lid < 0)
			return -1;
		add_in_group(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

	pcb_hid_action("dumplayers");

	return 0;
}
