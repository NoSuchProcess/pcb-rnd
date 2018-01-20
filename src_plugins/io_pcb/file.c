/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015, 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
#include "thermal.h"
#include "event.h"

#include "src_plugins/lib_compat_help/layer_compat.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"
#include "src_plugins/lib_compat_help/subc_help.h"

#define VIA_COMPAT_FLAGS (PCB_FLAG_CLEARLINE | PCB_FLAG_SELECTED | PCB_FLAG_WARN | PCB_FLAG_USETHERMAL | PCB_FLAG_LOCK)

pcb_unit_style_t pcb_io_pcb_usty_seen;

static void WritePCBInfoHeader(FILE *);
static void WritePCBDataHeader(FILE *);
static void WritePCBFontData(FILE *);
static void WriteViaData(FILE *, pcb_data_t *);
static void WritePCBRatData(FILE *);
static void WriteLayerData(FILE *, pcb_cardinal_t, pcb_layer_t *);

#define F2S(OBJ, TYPE) (pcb_strflg_f2s((OBJ)->Flags, TYPE, &((OBJ)->intconn)))

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
	for (group = 0; group < pcb_max_group(PCB); group++)
		if (PCB->LayerGroups.grp[group].len) {
			unsigned int gflg = pcb_layergrp_flags(PCB, group);

			if (gflg & PCB_LYT_SILK) /* silk is hacked in asusming there's a top and bottom copper */
				continue;

			if (sep)
				*cp++ = ':';
			sep = 1;
			for (entry = 0; entry < PCB->LayerGroups.grp[group].len; entry++) {
				pcb_layer_id_t layer = PCB->LayerGroups.grp[group].lid[entry];

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


static void WriteAttributeList(FILE * FP, pcb_attribute_list_t *list, const char *prefix)
{
	int i;

	for (i = 0; i < list->Number; i++)
		fprintf(FP, "%sAttribute(\"%s\" \"%s\")\n", prefix, list->List[i].name, list->List[i].value);
}

static void WritePCBInfoHeader(FILE * FP)
{
	/* write some useful comments */
	fprintf(FP, "# release: pcb-rnd " PCB_VERSION "\n");

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

/* data header: the name of the PCB, cursor location, zoom and grid
 * layergroups and some flags */
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

/* writes font data of non empty symbols */
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

static pcb_flag_t pinvia_flag(pcb_pstk_t *ps, pcb_pstk_compshape_t cshape)
{
	pcb_flag_t flg;
	int n;

	memset(&flg, 0, sizeof(flg));
	flg.f = ps->Flags.f & VIA_COMPAT_FLAGS;
	switch(cshape) {
		case PCB_PSTK_COMPAT_ROUND:
			break;
		case PCB_PSTK_COMPAT_OCTAGON:
			flg.f |= PCB_FLAG_OCTAGON;
			break;
		case PCB_PSTK_COMPAT_SQUARE:
			flg.f |= PCB_FLAG_SQUARE;
			flg.q = 1;
			break;
		default:
			if ((cshape >= PCB_PSTK_COMPAT_SHAPED) && (cshape <= PCB_PSTK_COMPAT_SHAPED_END)) {
				flg.f |= PCB_FLAG_SQUARE;
				cshape -= PCB_PSTK_COMPAT_SHAPED;
				if (cshape == 1)
					cshape = 17;
				flg.q = cshape;
			}
			else
				pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "Failed to convert shape to old-style pin/via", "Old pin/via format is very much restricted; try to use a simpler shape (e.g. circle)");
	}

	for(n = 0; n < sizeof(flg.t) / sizeof(flg.t[0]); n++) {
		unsigned char *ot = pcb_pstk_get_thermal(ps, n, 0);
		int nt;
		if ((ot == NULL) || (*ot == 0) || !((*ot) & PCB_THERMAL_ON))
			continue;
		switch(((*ot) & ~PCB_THERMAL_ON)) {
			case PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL: nt = 1; break;
			case PCB_THERMAL_SHARP: nt = 2; break;
			case PCB_THERMAL_SOLID: nt = 3; break;
			case PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL: nt = 4; break;
			case PCB_THERMAL_ROUND: nt = 5; break;
			default: nt = 0; pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "Failed to convert thermal to old-style via", "Old via format is very much restricted; try to use a simpler thermal shape");
		}
		PCB_FLAG_THERM_ASSIGN_(n, nt, flg);
	}
	return flg;
}

static void WriteViaData(FILE * FP, pcb_data_t *Data)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	pcb_pstk_t *ps;

	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		pcb_fprintf(FP, "Via[%[0] %[0] %[0] %[0] %[0] %[0] ", via->X, via->Y,
								via->Thickness, via->Clearance, via->Mask, via->DrillingHole);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(via->Name));
		fprintf(FP, " %s]\n", F2S(via, PCB_TYPE_VIA));
	}
	padstacklist_foreach(&Data->padstack, &it, ps) {
		pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask;
		pcb_pstk_compshape_t cshape;
		pcb_bool plated;
		char *name = pcb_attribute_get(&ps->Attributes, "name");

		if (!pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "Failed to convert to old-style via", "Old via format is very much restricted; try to use a simpler, unform shape padstack");
			continue;
		}

		pcb_fprintf(FP, "Via[%[0] %[0] %[0] %[0] %[0] %[0] ", x, y,
			pad_dia, clearance*2, mask, drill_dia);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(name));
		fprintf(FP, " %s]\n", pcb_strflg_f2s(pinvia_flag(ps, cshape), PCB_TYPE_VIA, NULL));
	}
}

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

static void WritePCBNetlistPatchData(FILE * FP)
{
	if (PCB->NetlistPatches != NULL) {
		fprintf(FP, "NetListPatch()\n(\n");
		pcb_ratspatch_fexport(PCB, FP, 1);
		fprintf(FP, ")\n");
	}
}

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
		arclist_foreach(&element->Arc, &it, arc) {
			pcb_fprintf(FP, "\tElementArc [%[0] %[0] %[0] %[0] %ma %ma %[0]]\n",
									arc->X - element->MarkX,
									arc->Y - element->MarkY, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness);
		}
		fputs("\n\t)\n", FP);
	}
	return 0;
}


int io_pcb_WriteSubcData(pcb_plug_io_t *ctx, FILE *FP, pcb_data_t *Data)
{
	gdl_iterator_t sit, it;
	pcb_subc_t *sc;
	int l;

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;

	subclist_foreach(&Data->subc, &sit, sc) {
		pcb_coord_t ox, oy, rx, ry;
		int rdir = 0, rscale = 100;
		pcb_text_t *trefdes;
		pcb_pstk_t *ps;

		pcb_subc_get_origin(sc, &ox, &oy);
		trefdes = pcb_subc_get_refdes_text(sc);

		if (trefdes != NULL) {
			rx = trefdes->X - ox;
			ry = trefdes->Y - oy;
			rdir = trefdes->Direction;
			rscale = trefdes->Scale;
		}
		else {
			rx = ox;
			ry = oy;
		}

		fprintf(FP, "\nElement[%s ", F2S(sc, PCB_TYPE_ELEMENT));
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pcb_attribute_get(&sc->Attributes, "footprint")));
		fputc(' ', FP);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pcb_attribute_get(&sc->Attributes, "refdes")));
		fputc(' ', FP);
		pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pcb_attribute_get(&sc->Attributes, "value")));
		pcb_fprintf(FP, " %[0] %[0] %[0] %[0] %d %d %s]\n(\n", ox, oy, rx, ry, rdir, rscale, F2S(trefdes, PCB_TYPE_ELEMENT_NAME));
		WriteAttributeList(FP, &sc->Attributes, "\t");

		padstacklist_foreach(&sc->data->padstack, &it, ps) {
			pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask, x1, y1, x2, y2, thickness;
			pcb_pstk_compshape_t cshape;
			pcb_bool plated, square, nopaste;
			unsigned char ic = ps->intconn;
			if (pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
				pcb_fprintf(FP, "\tPin[%[0] %[0] %[0] %[0] %[0] %[0] ", x - ox, y - oy, pad_dia, clearance*2, mask, drill_dia);
				pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pcb_attribute_get(&ps->Attributes, "name")));
				fprintf(FP, " ");
				pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pcb_attribute_get(&ps->Attributes, "term")));
				fprintf(FP, " %s]\n", pcb_strflg_f2s(pinvia_flag(ps, cshape), PCB_TYPE_PIN, &ic));
			}
			else if (pcb_pstk_export_compat_pad(ps, &x1, &y1, &x2, &y2, &thickness, &clearance, &mask, &square, &nopaste)) {
				unsigned long fl = (square ? PCB_FLAG_SQUARE : 0) | (nopaste ? PCB_FLAG_NOPASTE : 0);
				pcb_fprintf(FP, "\tPad[%[0] %[0] %[0] %[0] %[0] %[0] %[0] ",
					x1 - ox, y1 - oy, x2 - ox, y2 - oy, thickness, clearance, mask);
					pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pcb_attribute_get(&ps->Attributes, "name")));
					fprintf(FP, " ");
					pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pcb_attribute_get(&ps->Attributes, "term")));
					fprintf(FP, " %s]\n", pcb_strflg_f2s(pcb_flag_make(fl), PCB_TYPE_PAD, &ic));
			}
			else
				pcb_io_incompat_save(sc->data, (pcb_any_obj_t *)ps, "Padstack can not be exported az pin or pad", "use simpler padstack; for pins, all copper layers must have the same shape and there must be no paste; for pads, use a line or a rectangle; paste and mask must match the copper shape");
		}

		for(l = 0; l < sc->data->LayerN; l++) {
			pcb_layer_t *ly = &sc->data->Layer[l];
			pcb_line_t *line;
			pcb_arc_t *arc;

			if ((ly->meta.bound.type & PCB_LYT_SILK) && (ly->meta.bound.type & PCB_LYT_TOP)) {
				linelist_foreach(&ly->Line, &it, line) {
					pcb_fprintf(FP, "\tElementLine [%[0] %[0] %[0] %[0] %[0]]\n",
						line->Point1.X - ox, line->Point1.Y - oy,
						line->Point2.X - ox, line->Point2.Y - oy,
						line->Thickness);
				}
				arclist_foreach(&ly->Arc, &it, arc) {
					pcb_fprintf(FP, "\tElementArc [%[0] %[0] %[0] %[0] %ma %ma %[0]]\n",
						arc->X - ox, arc->Y - oy, arc->Width, arc->Height,
						arc->StartAngle, arc->Delta, arc->Thickness);
				}
				if (polylist_length(&ly->Polygon) > 0) {
					char *desc = pcb_strdup_printf("Polygons on layer %s can not be exported in an element\n", ly->name);
					pcb_io_incompat_save(sc->data, NULL, desc, "only lines and arcs are exported");
					free(desc);
				}
				if (textlist_length(&ly->Text) > 1) {
					char *desc = pcb_strdup_printf("Text on layer %s can not be exported in an element\n", ly->name);
					pcb_io_incompat_save(sc->data, NULL, desc, "only lines and arcs are exported");
					free(desc);
				}
				continue;
			}

			if (!(ly->meta.bound.type & PCB_LYT_VIRTUAL) && (!pcb_layer_is_pure_empty(ly))) {
				char *desc = pcb_strdup_printf("Objects on layer %s can not be exported in an element\n", ly->name);
				pcb_io_incompat_save(sc->data, NULL, desc, "only top silk lines and arcs are exported; heavy terminals are not supported, use padstacks only");
				free(desc);
			}
		}
		fputs("\n)\n", FP);
	}
	return 0;
}

static const char *layer_name_hack(pcb_layer_t *layer, const char *name)
{
	unsigned long lflg = pcb_layer_flags_(layer);
	/* The old PCB format encodes some properties in layer names - have to
	   alter the real layer name before save to get the same effect */
	if (lflg & PCB_LYT_OUTLINE) {
		if (pcb_strcasecmp(name, "outline") == 0)
			return name;
		return "Outline";
	}
	if (lflg & PCB_LYT_SILK) {
		if (pcb_strcasecmp(name, "silk") == 0)
			return name;
		return "silk";
	}

	/* plain layer */
	return name;
}

static void WriteLayerData(FILE * FP, pcb_cardinal_t Number, pcb_layer_t *layer)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_text_t *text;
	pcb_poly_t *polygon;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		fprintf(FP, "Layer(%i ", (int) Number + 1);
		pcb_print_quoted_string(FP, layer_name_hack(layer, PCB_EMPTY(layer->name)));
		fputs(")\n(\n", FP);
		WriteAttributeList(FP, &layer->meta.real.Attributes, "\t");

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
			fprintf(FP, "\tPolygon(%s)\n\t(", F2S(polygon, PCB_TYPE_POLY));
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

static void LayersFixup(void)
{
	pcb_layer_id_t bs, ts;
	int chg = 0;

	/* the PCB format requires 2 silk layers to be the last; swap layers to make that so */
	bs = pcb_layer_get_bottom_silk();
	ts = pcb_layer_get_top_silk();
	if (bs != pcb_max_layer - 2) {
		pcb_layer_swap(PCB, bs, pcb_max_layer - 2);
		chg = 1;
	}

	bs = pcb_layer_get_bottom_silk();
	ts = pcb_layer_get_top_silk();

	if (ts != pcb_max_layer - 1) {
		pcb_layer_swap(PCB, ts, pcb_max_layer - 1);
		chg = 1;
	}

	if (chg)
		pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);
}

static void WriteLayers(FILE *FP, pcb_data_t *data)
{
	int i;
	for (i = 0; i < data->LayerN; i++) {
		pcb_layer_t *ly = &(data->Layer[i]);
		pcb_layer_type_t lyt = pcb_layer_flags_(ly);
		if (!(lyt & (PCB_LYT_COPPER | PCB_LYT_SILK | PCB_LYT_OUTLINE))) {
			if (!pcb_layer_is_pure_empty(ly)) {
				char *desc = pcb_strdup_printf("Layer %s can be exported only as a copper layer\n", ly->name);
				pcb_io_incompat_save(data, NULL, desc, NULL);
				free(desc);
			}
		}
		WriteLayerData(FP, i, ly);
	}
}

int io_pcb_WriteBuffer(pcb_plug_io_t *ctx, FILE * FP, pcb_buffer_t *buff, pcb_bool elem_only)
{
	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;

	if (elem_only) {
		if (elementlist_length(&buff->Data->Element) == 0) {
			pcb_message(PCB_MSG_ERROR, "Buffer has no elements!\n");
			return -1;
		}
	}
	else {
		LayersFixup();
		WriteViaData(FP, buff->Data);
	}

	io_pcb_WriteElementData(ctx, FP, buff->Data);
	io_pcb_WriteSubcData(ctx, FP, buff->Data);

	if (!elem_only)
		WriteLayers(FP, buff->Data);

	return 0;
}

int io_pcb_WritePCB(pcb_plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	pcb_attribute_put(&PCB->Attributes, "PCB::loader", ctx->description);

	LayersFixup();

	pcb_printf_slot[0] = ((io_pcb_ctx_t *)(ctx->plugin_data))->write_coord_fmt;
	WritePCBInfoHeader(FP);
	WritePCBDataHeader(FP);
	WritePCBFontData(FP);
	WriteAttributeList(FP, &PCB->Attributes, "");
	WriteViaData(FP, PCB->Data);
	io_pcb_WriteElementData(ctx, FP, PCB->Data);
	io_pcb_WriteSubcData(ctx, FP, PCB->Data);
	WritePCBRatData(FP);
	WriteLayers(FP, PCB->Data);
	WritePCBNetlistData(FP);
	WritePCBNetlistPatchData(FP);

	return 0;
}

/*** functions for loading elements-as-pcb ***/
extern pcb_board_t *yyPCB;
extern pcb_data_t *yyData;
extern pcb_font_t *yyFont;

void PreLoadElementPCB()
{

	if (!yyPCB)
		return;

	yyFont = &yyPCB->fontkit.dflt;
	yyData = yyPCB->Data;
	PCB_SET_PARENT(yyData, board, yyPCB);
	yyData->LayerN = 0;
}

void PostLoadElementPCB()
{
	pcb_board_t *pcb_save = PCB;
	pcb_element_t *e;

	if (!yyPCB)
		return;

	pcb_board_new_postproc(yyPCB, 0);
	pcb_layer_group_setup_default(&yyPCB->LayerGroups);
	PCB = yyPCB;
	pcb_layer_group_setup_silks(yyPCB);
	e = elementlist_first(&yyPCB->Data->Element);	/* we know there's only one */
	pcb_element_move(yyPCB->Data, e, -e->BoundingBox.X1, -e->BoundingBox.Y1);
	PCB = pcb_save;
	yyPCB->MaxWidth = e->BoundingBox.X2;
	yyPCB->MaxHeight = e->BoundingBox.Y2;
	yyPCB->is_footprint = 1;

	/* opening a footprint: we don't have a layer stack; make sure top and bottom copper exist */
	{
		pcb_layergrp_id_t gid;
		int res;
		
		res = pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &gid, 1);
		assert(res == 1);
		pcb_layer_create(PCB, gid, "top copper");
		res = pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &gid, 1);
		assert(res == 1);
		pcb_layer_create(PCB, gid, "bottom copper");
	}

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
			if (strncmp(s, "PCB", 3) == 0) {
				char *b = s+3;
				while(isspace(*b)) b++;
				if ((*b == '(') || (*b == '['))
					return 1;
			}
			if (strncmp(s, "Element", 7) == 0) {
				char *b = s+7;
				while(isspace(*b)) b++;
				if ((*b == '(') || (*b == '['))
					return 1;
			}
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
	pcb_layer_id_t lid;
	if (pcb->Data->LayerN >= PCB_MAX_LAYER)
		return -1;
	lid = pcb->Data->LayerN;
	pcb->Data->Layer[lid].name = pcb_strdup(name);
	pcb->Data->Layer[lid].parent = pcb->Data;
	pcb->Data->LayerN++;
	return lid;
}

pcb_layer_id_t static existing_or_new_ly_end(pcb_board_t *pcb, const char *name)
{
	pcb_layer_id_t lid = pcb_layer_by_name(pcb->Data, name);
	if (lid >= 0) {
		if (pcb->Data->Layer[lid].meta.real.grp >= 0) {
			pcb_layergrp_id_t gid = pcb->Data->Layer[lid].meta.real.grp;
			pcb_layergrp_t *grp = &pcb->LayerGroups.grp[gid];
			grp->len = 0;

			pcb->Data->Layer[lid].meta.real.grp = -1;
		}
		return lid;
	}
	return new_ly_end(pcb, name);
}

pcb_layer_id_t static new_ly_old(pcb_board_t *pcb, const char *name)
{
	pcb_layer_id_t lid;
	for(lid = 0; lid < PCB_MAX_LAYER; lid++) {
		if (pcb->Data->Layer[lid].meta.real.grp == 0) {
			free((char *)pcb->Data->Layer[lid].name);
			pcb->Data->Layer[lid].name = pcb_strdup(name);
			pcb->Data->Layer[lid].parent = pcb->Data;
			return lid;
		}
	}
	return -1;
}

int pcb_layer_improvise(pcb_board_t *pcb, pcb_bool setup)
{
	pcb_layergrp_id_t gid;
	pcb_layer_id_t lid, silk = -1;

	if (setup) {
		pcb_layer_group_setup_default(&pcb->LayerGroups);

		for(lid = 0; lid < pcb->Data->LayerN; lid++) {
			if (strcmp(pcb->Data->Layer[lid].name, "silk") == 0) {
				if (silk < 0)
					pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_SILK, &gid, 1);
				else
					pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_SILK, &gid, 1);
				pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
				silk = lid;
			}
			else {
				if (*pcb->Data->Layer[lid].name == '\0') {
					free((char *)pcb->Data->Layer[lid].name);
					pcb->Data->Layer[lid].name = pcb_strdup("anonymous");
				}
				if (lid == 0)
					pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &gid, 1);
				else
					pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &gid, 1);
				pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
			}
		}

		pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_SILK, &gid, 1);
		if (pcb->LayerGroups.grp[gid].len < 1) {
			lid = new_ly_end(pcb, "silk");
			if (lid < 0)
				return -1;
			pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
		}

		pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_SILK, &gid, 1);
		if (pcb->LayerGroups.grp[gid].len < 1) {
			lid = new_ly_end(pcb, "silk");
			if (lid < 0)
				return -1;
			pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
		}

		pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &gid, 1);
		if (pcb->LayerGroups.grp[gid].len < 1) {
			lid = new_ly_old(pcb, "top_copper");
			if (lid < 0)
				return -1;
			pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
		}

		pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &gid, 1);
		if (pcb->LayerGroups.grp[gid].len < 1) {
			lid = new_ly_old(pcb, "bottom_copper");
			if (lid < 0)
				return -1;
			pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
		}
	}

	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_MASK, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = existing_or_new_ly_end(pcb, "top-mask");
		if (lid < 0)
			return -1;
		pcb->Data->Layer[lid].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
		pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_MASK, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = existing_or_new_ly_end(pcb, "bottom-mask");
		if (lid < 0)
			return -1;
		pcb->Data->Layer[lid].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
		pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_PASTE, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = existing_or_new_ly_end(pcb, "top-paste");
		if (lid < 0)
			return -1;
		pcb->Data->Layer[lid].comb = PCB_LYC_AUTO;
		pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_PASTE, &gid, 1);
	if (pcb->LayerGroups.grp[gid].len < 1) {
		lid = existing_or_new_ly_end(pcb, "bottom-paste");
		if (lid < 0)
			return -1;
		pcb->Data->Layer[lid].comb = PCB_LYC_AUTO;
		pcb_layer_add_in_group_(pcb, &pcb->LayerGroups.grp[gid], gid, lid);
	}

/*	pcb_hid_action("dumplayers");*/

	return 0;
}

/*** Compatibility wrappers: create padstack and subc as if they were vias and elements ***/

pcb_pstk_t *io_pcb_via_new(pcb_data_t *data, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, pcb_flag_t Flags)
{
	pcb_pstk_t *p;
	pcb_pstk_compshape_t shp;
	int n;

	if (Flags.f & PCB_FLAG_SQUARE) {
		shp = Flags.q /*+ PCB_PSTK_COMPAT_SHAPED*/;
		if (shp == 0)
			shp = PCB_PSTK_COMPAT_SQUARE;
	}
	else if (Flags.f & PCB_FLAG_OCTAGON)
		shp = PCB_PSTK_COMPAT_OCTAGON;
	else
		shp = PCB_PSTK_COMPAT_ROUND;

	p = pcb_pstk_new_compat_via(data, X, Y, DrillingHole, Thickness, Clearance/2, Mask, shp, !(Flags.f & PCB_FLAG_HOLE));
	p->Flags.f |= Flags.f & VIA_COMPAT_FLAGS;
	for(n = 0; n < sizeof(Flags.t) / sizeof(Flags.t[0]); n++) {
		int nt = PCB_THERMAL_ON, t = ((Flags.t[n/2] >> (4 * (n % 2))) & 0xf);
		if (t != 0) {
			switch(t) {
				case 1: nt |= PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL; break;
				case 2: nt |= PCB_THERMAL_SHARP; break;
				case 3: nt |= PCB_THERMAL_SOLID; break;
				case 4: nt |= PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL; break;
				case 5: nt |= PCB_THERMAL_ROUND; break;
			}
			pcb_pstk_set_thermal(p, n, nt);
		}
	}

	if (Name != NULL)
		pcb_attribute_put(&p->Attributes, "name", Name);

	return p;
}

static int yysubc_bottom;
extern	pcb_subc_t *yysubc;
extern	pcb_coord_t yysubc_ox, yysubc_oy;

pcb_subc_t *io_pcb_element_new(pcb_data_t *Data, pcb_subc_t *subc,
	pcb_font_t *PCBFont, pcb_flag_t Flags, char *Description, char *NameOnPCB,
	char *Value, pcb_coord_t TextX, pcb_coord_t TextY, pcb_uint8_t Direction,
	int TextScale, pcb_flag_t TextFlags, pcb_bool uniqueName)
{
	pcb_subc_t *sc = pcb_subc_alloc();
	pcb_text_t *txt;
	pcb_add_subc_to_data(Data, sc);
	if (Data->padstack_tree == NULL)
		Data->padstack_tree = pcb_r_create_tree();
	sc->data->padstack_tree = Data->padstack_tree;

	yysubc_ox = 0;
	yysubc_oy = 0;
	yysubc_bottom = !!(Flags.f & PCB_FLAG_ONSOLDER);
	Flags.f &= ~PCB_FLAG_ONSOLDER;

	if (Description != NULL)
		pcb_attribute_put(&sc->Attributes, "footprint", Description);
	if (NameOnPCB != NULL)
		pcb_attribute_put(&sc->Attributes, "refdes", NameOnPCB);
	if (Value != NULL)
		pcb_attribute_put(&sc->Attributes, "value", Value);

#warning TODO: TextFlags
	txt = pcb_subc_add_refdes_text(sc, TextX, TextY, Direction, TextScale, yysubc_bottom);

	return sc;
}

void io_pcb_element_fin(pcb_data_t *Data)
{
	double rot = 0;
	pcb_subc_bbox(yysubc);

#warning subc TODO: rotation
	pcb_subc_create_aux(yysubc, yysubc_ox, yysubc_oy, rot);
	pcb_add_subc_to_data(Data, yysubc);

	if (Data->subc_tree == NULL)
		Data->subc_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Data->subc_tree, (pcb_box_t *)yysubc);
}

static pcb_layer_t *subc_silk_layer(pcb_subc_t *subc)
{
	pcb_layer_type_t side = yysubc_bottom ? PCB_LYT_BOTTOM : PCB_LYT_TOP;
	const char *name = yysubc_bottom ? "bottom-silk" : "top-silk";
	return pcb_subc_get_layer(subc, PCB_LYT_SILK | side, /*PCB_LYC_AUTO*/0, pcb_true, name);
}

pcb_line_t *io_pcb_element_line_new(pcb_subc_t *subc, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness)
{
	pcb_layer_t *ly = subc_silk_layer(subc);
	return pcb_line_new_merge(ly, X1, Y1, X2, Y2, Thickness, 0, pcb_no_flags());
}

pcb_arc_t *io_pcb_element_arc_new(pcb_subc_t *subc, pcb_coord_t X, pcb_coord_t Y,
	pcb_coord_t Width, pcb_coord_t Height, pcb_angle_t angle, pcb_angle_t delta, pcb_coord_t Thickness)
{
	pcb_layer_t *ly = subc_silk_layer(subc);
	return pcb_arc_new(ly, X, Y, Width, Height, angle, delta, Thickness, 0, pcb_no_flags());
}

pcb_pstk_t *io_pcb_element_pin_new(pcb_subc_t *subc, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, const char *Number, pcb_flag_t Flags)
{
	pcb_pstk_t *p;
	p = io_pcb_via_new(subc->data, X, Y, Thickness, Clearance, Mask, DrillingHole, Name, Flags);
	if (Number != NULL)
		pcb_attribute_put(&p->Attributes, "term", Number);
	if (Name != NULL)
		pcb_attribute_put(&p->Attributes, "name", Name);

	if (yysubc_bottom)
		pcb_pstk_mirror(p, 0, 1);
	return p;
}

pcb_pstk_t *io_pcb_element_pad_new(pcb_subc_t *subc, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, const char *Name, const char *Number, pcb_flag_t Flags)
{
	pcb_pstk_t *p;

	p = pcb_pstk_new_compat_pad(subc->data, X1, Y1, X2, Y2, Thickness, Clearance, Mask, Flags.f & PCB_FLAG_SQUARE, Flags.f & PCB_FLAG_NOPASTE, (!!(Flags.f & PCB_FLAG_ONSOLDER)) != yysubc_bottom);
	if (Number != NULL)
		pcb_attribute_put(&p->Attributes, "term", Number);
	if (Name != NULL)
		pcb_attribute_put(&p->Attributes, "name", Name);

	if (yysubc_bottom)
		pcb_pstk_mirror(p, 0, 1);

	return p;
}

void io_pcb_postproc_board(pcb_board_t *pcb)
{
	gdl_iterator_t it;
	pcb_subc_t *sc;
	int n;

	/* remove empty layer groups */
	for(n = 0; n < pcb->LayerGroups.len; n++) {
		if (pcb->LayerGroups.grp[n].len == 0) {
			pcb_layergrp_del(pcb, n, 0);
			n--;
		}
	}

	/* have to revind all subcircuits because the layer stack was not ready
	   when they got loaded */
	subclist_foreach(&pcb->Data->subc, &it, sc)
		pcb_subc_rebind(pcb, sc);
}
