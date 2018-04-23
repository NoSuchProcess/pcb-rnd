/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *
 *  Specctra .dsn export HID
 *  Copyright (C) 2008, 2011 Josh Jordan, Dan McMahill, and Jared Casper
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 */

/*
This program exports specctra .dsn files from geda .pcb files.
By Josh Jordan and Dan McMahill, modified from bom.c
  -- Updated to use Coord and other fixes by Jared Casper 16 Sep 2011
*/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <genvector/gds_char.h>

#include <glib.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "rats.h"
#include "buffer.h"
#include "change.h"
#include "draw.h"
#include "undo.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "compat_misc.h"
#include "layer.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "hid_actions.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_helper.h"
#include "plugins.h"
#include "obj_line.h"
#include "obj_pstk_inlines.h"

static const char *dsn_cookie = "dsn exporter";

static pcb_coord_t trackwidth = 8;  /* user options defined in export dialog */
static pcb_coord_t clearance = 8;
static pcb_coord_t viawidth = 45;
static pcb_coord_t viadrill = 25;

static pcb_hid_t dsn_hid;

static pcb_hid_attribute_t dsn_options[] = {
	{"dsnfile", "SPECCTRA output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_dsnfile 0
	{"trackwidth", "track width in mils",
	 PCB_HATT_COORD, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(8)}, 0, 0},
#define HA_trackwidth 1
	{"clearance", "clearance in mils",
	 PCB_HATT_COORD, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(8)}, 0, 0},
#define HA_clearance 2
	{"viawidth", "via width in mils",
	 PCB_HATT_COORD, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(27)}, 0,
	 0},
#define HA_viawidth 3
	{"viadrill", "via drill diameter in mils",
	 PCB_HATT_COORD, PCB_MIL_TO_COORD(0), PCB_MIL_TO_COORD(100), {0, 0, 0, PCB_MIL_TO_COORD(15)}, 0,
	 0},
#define HA_viadrill 4
};

#define NUM_OPTIONS (sizeof(dsn_options)/sizeof(dsn_options[0]))
PCB_REGISTER_ATTRIBUTES(dsn_options, dsn_cookie)

static pcb_hid_attr_val_t dsn_values[NUM_OPTIONS];

static const char *dsn_filename;

static pcb_hid_attribute_t *dsn_get_export_options(int *n)
{
	static char *last_dsn_filename = 0;
	if (PCB) {
		pcb_derive_default_filename(PCB->Filename, &dsn_options[HA_dsnfile], ".dsn", &last_dsn_filename);
	}
	if (n)
		*n = NUM_OPTIONS;
	return dsn_options;
}

static GList *layerlist = NULL;  /* contain routing layers */

static void print_structure(FILE * fp)
{
	pcb_layergrp_id_t group, top_group, bot_group;
	pcb_layer_id_t top_layer, bot_layer;

	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &top_group, 1);
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &bot_group, 1);

	top_layer = PCB->LayerGroups.grp[top_group].lid[0];
	bot_layer = PCB->LayerGroups.grp[bot_group].lid[0];


	g_list_free(layerlist);				/* might be around from the last export */

	if (PCB->Data->Layer[top_layer].meta.real.vis) {
		layerlist = g_list_append(layerlist, &PCB->Data->Layer[top_layer]);
	}
	else {
		pcb_message(PCB_MSG_WARNING, "WARNING! DSN export does not include the top layer. "
						 "Router will consider an inner layer to be the \"top\" layer.\n");
	}

	for (group = 0; group < pcb_max_group(PCB); group++) {
		pcb_layer_t *first_layer;
		unsigned int gflg = pcb_layergrp_flags(PCB, group);

		if (gflg & PCB_LYT_SILK)
			continue;

		if (group == top_group || group == bot_group)
			continue;

		if (PCB->LayerGroups.grp[group].len < 1)
			continue;

		first_layer = &PCB->Data->Layer[PCB->LayerGroups.grp[group].lid[0]];
		if (!first_layer->meta.real.vis)
			continue;

		layerlist = g_list_append(layerlist, first_layer);

		if (group < top_group) {
			pcb_message(PCB_MSG_WARNING, "WARNING! DSN export moved layer group with the \"%s\" layer "
							 "after the top layer group.  DSN files must have the top " "layer first.\n", first_layer->name);
		}

		if (group > bot_group) {
			pcb_message(PCB_MSG_WARNING, "WARNING! DSN export moved layer group with the \"%s\" layer "
							 "before the bottom layer group.  DSN files must have the " "bottom layer last.\n", first_layer->name);
		}

		PCB_COPPER_GROUP_LOOP(PCB->Data, group);
		{
			if (entry > 0) {
				pcb_message(PCB_MSG_WARNING, "WARNING! DSN export squashed layer \"%s\" into layer "
								 "\"%s\", DSN files do not have layer groups.", layer->name, first_layer->name);
			}
		}
		PCB_END_LOOP;
	}

	if (PCB->Data->Layer[bot_layer].meta.real.vis) {
		layerlist = g_list_append(layerlist, &PCB->Data->Layer[bot_layer]);
	}
	else {
		pcb_message(PCB_MSG_WARNING, "WARNING! DSN export does not include the bottom layer. "
						 "Router will consider an inner layer to be the \"bottom\" layer.\n");
	}

	fprintf(fp, "  (structure\n");

	for (GList * iter = layerlist; iter; iter = g_list_next(iter)) {
		pcb_layer_t *layer = iter->data;
		char *layeropts = pcb_strdup("(type signal)");
		
		if (!(pcb_layer_flags_(layer) & PCB_LYT_COPPER))
			continue;
		
		/* see if layer has same name as a net and make it a power layer */
		/* loop thru all nets */
		for (int ni = 0; ni < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; ni++) {
			char *nname;
			nname = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Name + 2;
			if (!strcmp(layer->name, nname)) {
				g_free(layeropts);
				layeropts = pcb_strdup_printf("(type power) (use_net \"%s\")", layer->name);
			}
		}
		fprintf(fp, "    (layer \"%s\"\n", layer->name);
		fprintf(fp, "      %s\n", layeropts);
		fprintf(fp, "    )\n");
		g_free(layeropts);
	}

	/* PCB outline */
	pcb_fprintf(fp, "    (boundary\n");
	pcb_fprintf(fp, "      (rect pcb 0.0 0.0 %.6mm %.6mm)\n", PCB->MaxWidth, PCB->MaxHeight);
	pcb_fprintf(fp, "    )\n");
	pcb_fprintf(fp, "    (via via_%ld_%ld)\n", viawidth, viadrill);

	/* DRC rules */
	pcb_fprintf(fp, "    (rule\n");
	pcb_fprintf(fp, "      (width %mm)\n", trackwidth);
	pcb_fprintf(fp, "      (clear %mm)\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type wire_area))\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type via_smd via_pin))\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type smd_smd))\n", clearance);
	pcb_fprintf(fp, "      (clear %mm (type default_smd))\n", clearance);
	pcb_fprintf(fp, "    )\n  )\n");
}

static void print_placement(FILE * fp)
{
	fprintf(fp, "  (placement\n");

	PCB_SUBC_LOOP(PCB->Data);
	{
		char *ename;
		pcb_coord_t ox, oy;
		char *side;
		int res;
		int subc_on_solder = 0;

		pcb_subc_get_side(subc, &subc_on_solder);
		side = subc_on_solder ? "back" : "front";

		res = pcb_subc_get_origin(subc, &ox, &oy);
		assert(res == 0);
		if (subc->refdes != NULL)
			ename = pcb_strdup(subc->refdes);
		else
			ename = pcb_strdup("null");
		pcb_fprintf(fp, "    (component %d\n", subc->ID);
		pcb_fprintf(fp, "      (place \"%s\" %.6mm %.6mm %s 0 (PN 0))\n", ename, ox, PCB->MaxHeight - oy, side);
		pcb_fprintf(fp, "    )\n");
		g_free(ename);
	}
	PCB_END_LOOP;

#warning padstack TODO: check if real shapes are exported
	PCB_PADSTACK_LOOP(PCB->Data);
	{ /* add mounting holes */
		pcb_fprintf(fp, "    (component %d\n", padstack->ID);
		pcb_fprintf(fp, "      (place %d %.6mm %.6mm %s 0 (PN 0))\n", padstack->ID, padstack->x, (PCB->MaxHeight - padstack->y), "front");
		pcb_fprintf(fp, "    )\n");
	}
	PCB_END_LOOP;

	fprintf(fp, "  )\n");
}

static void add_padstack(GList **pads, char *padstack)
{
	if (!g_list_find_custom(*pads, padstack, (GCompareFunc) strcmp))
		*pads = g_list_append(*pads, padstack);
	else
		free(padstack);
}

static void print_polyshape(gds_t *term_shapes, pcb_pstk_poly_t *ply, pcb_coord_t ox, pcb_coord_t oy, const char *layer_name, int partsidesign)
{
	char tmp[512];
	int fld;
	pcb_coord_t x, y;
	int n;

	pcb_snprintf(tmp, sizeof(tmp), "        (polygon \"%s\" 0", layer_name);
	gds_append_str(term_shapes, tmp);

	fld = 0;
	for(n = 0; n < ply->len; n++) {
		if ((fld % 3) == 0)
			gds_append_str(term_shapes, "\n       ");
		pcb_snprintf(tmp, sizeof(tmp), " %.6mm %.6mm", (ply->x[n] - ox) * partsidesign, -(ply->y[n] - oy));
		gds_append_str(term_shapes, tmp);
		fld++;
	}

	gds_append_str(term_shapes, "\n        )\n");
}

static void print_lineshape(gds_t *term_shapes, pcb_pstk_line_t *lin, pcb_coord_t ox, pcb_coord_t oy, const char *layer_name, int partsidesign)
{
	char tmp[512];
	int fld;
	pcb_coord_t x[4], y[4];
	int n;
	pcb_line_t ltmp;

	pcb_snprintf(tmp, sizeof(tmp), "        (polygon \"%s\" 0", layer_name);
	gds_append_str(term_shapes, tmp);

	memset(&ltmp, 0, sizeof(ltmp));
	ltmp.Point1.X = lin->x1;
	ltmp.Point1.Y = lin->y1;
	ltmp.Point2.X = lin->x2;
	ltmp.Point2.Y = lin->y2;
	ltmp.Thickness = lin->thickness;
	pcb_sqline_to_rect(&ltmp, x, y);

#warning padstack TODO: this ignores round cap

	fld = 0;
	for(n = 0; n < 4; n++) {
		if ((fld % 3) == 0)
			gds_append_str(term_shapes, "\n       ");
		pcb_snprintf(tmp, sizeof(tmp), " %.6mm %.6mm", (x[n] - ox) * partsidesign, -(y[n] - oy));
		gds_append_str(term_shapes, tmp);
		fld++;
	}

	gds_append_str(term_shapes, "\n        )\n");
}

static void print_circshape(gds_t *term_shapes, pcb_pstk_circ_t *circ, pcb_coord_t ox, pcb_coord_t oy, const char *layer_name, int partsidesign)
{
	char tmp[512];

	pcb_snprintf(tmp, sizeof(tmp), "        (circle \"%s\"", layer_name);
	gds_append_str(term_shapes, tmp);

#warning padstack TODO: this ignores circle center offset

	pcb_snprintf(tmp, sizeof(tmp), " %.6mm)\n", circ->dia);
	gds_append_str(term_shapes, tmp);
}

static void print_polyline(gds_t *term_shapes, pcb_poly_it_t *it, pcb_pline_t *pl, pcb_coord_t ox, pcb_coord_t oy, const char *layer_name, int partsidesign)
{
	char tmp[512];
	int fld;
	pcb_coord_t x, y;
	int go;

	if (pl != NULL) {
		pcb_snprintf(tmp, sizeof(tmp), "(polygon \"%s\" 0", layer_name);
		gds_append_str(term_shapes, tmp);

		fld = 0;
		for(go = pcb_poly_vect_first(it, &x, &y); go; go = pcb_poly_vect_next(it, &x, &y)) {
			if ((fld % 3) == 0)
				gds_append_str(term_shapes, "\n       ");
			pcb_snprintf(tmp, sizeof(tmp), " %.6mm %.6mm", (x - ox) * partsidesign, y - oy);
			gds_append_str(term_shapes, tmp);
			fld++;
		}

		gds_append_str(term_shapes, "\n        )\n");
	}
}

static void print_term_poly(FILE *fp, gds_t *term_shapes, pcb_poly_t *poly, pcb_coord_t ox, pcb_coord_t oy, int term_side, int partsidesign)
{
	if (poly->term != NULL) {
		pcb_layer_t *lay = g_list_nth_data(layerlist, term_side);
		char *padstack = pcb_strdup_printf("Term_poly_%ld", poly->ID);
		pcb_poly_it_t it;
		pcb_polyarea_t *pa;

		pcb_fprintf(fp, "      (pin %s \"%s\" %.6mm %.6mm)\n", padstack, poly->term, ox, oy);

		gds_append_str(term_shapes, "    (padstack ");
		gds_append_str(term_shapes, padstack);
		gds_append_str(term_shapes, "\n");

		gds_append_str(term_shapes, "      (shape ");

		for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
			pcb_pline_t *pl;

			pl = pcb_poly_contour(&it);

			print_polyline(term_shapes, &it, pl, ox, oy, lay->name, partsidesign);
		}

		gds_append_str(term_shapes, "      )\n");
		gds_append_str(term_shapes, "      (attach off)\n");
		gds_append_str(term_shapes, "    )\n");
	}
}

void print_pstk_shape(gds_t *term_shapes, pcb_pstk_t *padstack, int lid, pcb_coord_t ox, pcb_coord_t oy, int partsidesign)
{
	pcb_pstk_shape_t *shp;
	pcb_layer_t *lay = g_list_nth_data(layerlist, lid);
	pcb_layer_type_t lyt = pcb_layer_flags_(lay);
	pcb_poly_it_t it;

	if (!(lyt & PCB_LYT_COPPER))
		return;

	shp = pcb_pstk_shape(padstack, lyt, 0);
	if (shp == NULL)
		return;

	/* if the subc is placed on the other side, need to invert the output layerstack as well */
	if (partsidesign < 0)
		lay = g_list_nth_data(layerlist, g_list_length(layerlist) - 1 - lid);
	else
		lay = g_list_nth_data(layerlist, lid);

	switch(shp->shape) {
		case PCB_PSSH_POLY:
			print_polyshape(term_shapes, &shp->data.poly, ox, oy, lay->name, partsidesign);
			break;
		case PCB_PSSH_LINE:
			print_lineshape(term_shapes, &shp->data.line, ox, oy, lay->name, partsidesign);
			break;
		case PCB_PSSH_CIRC:
			print_circshape(term_shapes, &shp->data.circ, ox, oy, lay->name, partsidesign);
			break;
	}
}

static void print_library(FILE * fp)
{
	GList *pads = NULL, *iter; /* contain unique pad names */
	gchar *padstack;
	gds_t term_shapes;

	gds_init(&term_shapes);

	fprintf(fp, "  (library\n");

	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_coord_t ox, oy;
		pcb_point_t centroid;
		int partside, partsidesign, subc_on_solder = 0;
		pcb_layer_type_t lyt_side;

		pcb_subc_get_side(subc, &subc_on_solder);
		partside = subc_on_solder ? g_list_length(layerlist) - 1 : 0;
		partsidesign = subc_on_solder ? -1 : 1;
		lyt_side = subc_on_solder ? PCB_LYT_BOTTOM : PCB_LYT_TOP;

		pcb_subc_get_origin(subc, &ox, &oy);
		centroid.X = ox;
		centroid.Y = oy;

		fprintf(fp, "    (image %ld\n", subc->ID); /* map every subc by ID */
		PCB_POLY_COPPER_LOOP(subc->data);
		{
			pcb_layer_type_t lyt = pcb_layer_flags_(layer);
			if ((lyt & PCB_LYT_COPPER) && ((lyt & PCB_LYT_TOP) || (lyt & PCB_LYT_BOTTOM))) {
				int termside = (lyt & lyt_side) ? 0 : g_list_length(layerlist) - 1;
				print_term_poly(fp, &term_shapes, polygon, ox, oy, termside, partsidesign);
			}
		}
		PCB_ENDALL_LOOP;

		PCB_PADSTACK_LOOP(subc->data);
		{
			int n;
			char *pid = pcb_strdup_printf("Pstk_shape_%ld", padstack->ID);
			pcb_fprintf(fp, "      (pin %s \"%s\" %.6mm %.6mm)\n", pid, padstack->term, padstack->x-ox, -(padstack->y-oy));

			gds_append_str(&term_shapes, "    (padstack ");
			gds_append_str(&term_shapes, pid);
			gds_append_str(&term_shapes, "\n");

			gds_append_str(&term_shapes, "      (shape\n");
			for(n = 0; n < g_list_length(layerlist); n++)
				print_pstk_shape(&term_shapes, padstack, n, 0, 0, partsidesign);
			gds_append_str(&term_shapes, "      )\n");
			gds_append_str(&term_shapes, "      (attach off)\n");
			gds_append_str(&term_shapes, "    )\n");
		}
		PCB_END_LOOP;

		fprintf(fp, "    )\n");
	}
	PCB_END_LOOP;

	/* loop thru padstacks and define them all */
	for (iter = pads; iter; iter = g_list_next(iter)) {
		pcb_coord_t dim1, dim2;
		long int dim1l, dim2l;
		padstack = iter->data;
		fprintf(fp, "    (padstack %s\n", padstack);

		/* print info about pad here */
		if (sscanf(padstack, "Smd_rect_%ldx%ld", &dim1l, &dim2l) == 2) { /* then pad is smd */
			dim1 = dim1l;
			dim2 = dim2l;
			pcb_fprintf(fp,
									"      (shape (rect \"%s\" %.6mm %.6mm %.6mm %.6mm))\n",
									((pcb_layer_t *) (g_list_first(layerlist)->data))->name, dim1 / -2, dim2 / -2, dim1 / 2, dim2 / 2);
		}
		else if (sscanf(padstack, "Th_square_%ld", &dim1l) == 1) {
			dim1 = dim1l;
			pcb_fprintf(fp, "      (shape (rect signal %.6mm %.6mm %.6mm %.6mm))\n", dim1 / -2, dim1 / -2, dim1 / 2, dim1 / 2);
		}
		else {
			sscanf(padstack, "Th_round_%ld", &dim1l);
			dim1 = dim1l;
			pcb_fprintf(fp, "      (shape (circle signal %.6mm))\n", dim1);
		}
		fprintf(fp, "      (attach off)\n");
		fprintf(fp, "    )\n");
	}

	/* add padstack for via */
	pcb_fprintf(fp, "    (padstack via_%ld_%ld\n", viawidth, viadrill);
	pcb_fprintf(fp, "      (shape (circle signal %.6mm))\n", viawidth);
	pcb_fprintf(fp, "      (attach off)\n    )\n");

	/* add padstack for terminals */
	pcb_fprintf(fp, "%s", term_shapes.array);

	pcb_fprintf(fp, "  )\n");
	g_list_foreach(pads, (GFunc)free, NULL);
	g_list_free(pads);
	gds_uninit(&term_shapes);
}

static void print_quoted_pin(FILE * fp, const char *s)
{
	char *hyphen_pos = strchr(s, '-');
	if (!hyphen_pos) {
		fprintf(fp, " %s", s);
	}
	else {
		char refdes_name[1024];
		int copy_len = hyphen_pos - s;
		if (copy_len >= sizeof(refdes_name))
			copy_len = sizeof(refdes_name) - 1;
		strncpy(refdes_name, s, copy_len);
		refdes_name[copy_len] = 0;
		fprintf(fp, " \"%s\"-\"%s\"", refdes_name, hyphen_pos + 1);
	}
}

static void print_network(FILE * fp)
{
	int ni, nei;
	fprintf(fp, "  (network\n");
	for (ni = 0; ni < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; ni++) {
		fprintf(fp, "    (net \"%s\"\n", PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Name + 2);
		fprintf(fp, "      (pins");
		for (nei = 0; nei < PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].EntryN; nei++)
			print_quoted_pin(fp, PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Entry[nei].ListEntry);
		fprintf(fp, ")\n");
		fprintf(fp, "    )\n");
	}

	fprintf(fp, "    (class geda_default");
	for (ni = 0; ni < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; ni++) {
		fprintf(fp, " \"%s\"", PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[ni].Name + 2);
	}
	pcb_fprintf(fp, "\n");
	pcb_fprintf(fp, "      (circuit\n");
	pcb_fprintf(fp, "        (use_via via_%ld_%ld)\n", viawidth, viadrill);
	pcb_fprintf(fp, "      )\n");
	pcb_fprintf(fp, "      (rule (width %.6mm))\n    )\n  )\n", trackwidth);
}

static void print_wires(FILE * fp)
{
	GList *iter;
	pcb_layer_t *lay;
	fprintf(fp, "    (wiring\n");

	for (iter = layerlist; iter; iter = g_list_next(iter)) {
		lay = iter->data;
		PCB_LINE_LOOP(lay);
		{
			pcb_fprintf(fp,
									"        (wire (path %s %.6mm %.6mm %.6mm %.6mm %.6mm)\n",
									lay->name, line->Thickness, line->Point1.X,
									(PCB->MaxHeight - line->Point1.Y), line->Point2.X, (PCB->MaxHeight - line->Point2.Y));
			fprintf(fp, "            (type protect))\n");
		}
		PCB_END_LOOP;
	}
	fprintf(fp, "\n    )\n)\n"); /* close all braces */
}

static int PrintSPECCTRA(void)
{
	FILE *fp;
	/* Print out the dsn .dsn file. */
	fp = pcb_fopen(dsn_filename, "w");
	if (!fp) {
		pcb_message(PCB_MSG_WARNING, "Cannot open file %s for writing\n", dsn_filename);
		return 1;
	}

	/* pcb [required] */
	fprintf(fp, "(pcb %s\n", ((PCB->Name) && *(PCB->Name) ? (PCB->Name) : "notnamed"));

	/* parser descriptor [optional] */
	fprintf(fp, "  (parser\n");
	fprintf(fp, "    (string_quote \")\n");
	fprintf(fp, "    (space_in_quoted_tokens on)\n");
	fprintf(fp, "    (host_cad \"pcb-rnd\")\n");
	fprintf(fp, "    (host_version \"%s\")\n", PCB_VERSION);
	fprintf(fp, "  )\n");

	/* capacitance resolution descriptor [optional] */

	/* conductance resolution descriptor [optional] */

	/* current resolution descriptor [optional] */

	/* inductance resolution descriptor [optional] */

	/* resistance resolution descriptor [optional] */

	/* resolution descriptor [optional] */
	fprintf(fp, "  (resolution mm 1000000)\n");

	/* time resolution descriptor [optional] */

	/* voltage resolution descriptor [optional] */

	/* unit descriptor [optional] */

	/* structure descriptor [required] */
	print_structure(fp);

	/* placement descriptor [optional] */
	print_placement(fp);

	/* library descriptor [required] */
	print_library(fp);

	/* floor plan descriptor [optional] */

	/* part library descriptor [optional] */

	/* network descriptor [required] */
	print_network(fp);

	/* wiring descriptor [optional] */
	print_wires(fp);

	/* color descriptor [optional] */

	fclose(fp);

	return 0;
}


static void dsn_do_export(pcb_hid_attr_val_t * options)
{
	int i;
	if (!options) {
		dsn_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			dsn_values[i] = dsn_options[i].default_val;
		options = dsn_values;
	}
	dsn_filename = options[HA_dsnfile].str_value;
	if (!dsn_filename)
		dsn_filename = "pcb-out.dsn";

	trackwidth = options[HA_trackwidth].coord_value;
	clearance = options[HA_clearance].coord_value;
	viawidth = options[HA_viawidth].coord_value;
	viadrill = options[HA_viadrill].coord_value;
	PrintSPECCTRA();
}

static int dsn_parse_arguments(int *argc, char ***argv)
{
	return pcb_hid_parse_command_line(argc, argv);
}

int pplg_check_ver_export_dsn(int ver_needed) { return 0; }

void pplg_uninit_export_dsn(void)
{
	pcb_hid_remove_attributes_by_cookie(dsn_cookie);
}

#include "dolists.h"
int pplg_init_export_dsn(void)
{
	PCB_API_CHK_VER;
	memset(&dsn_hid, 0, sizeof(pcb_hid_t));
	pcb_hid_nogui_init(&dsn_hid);

	dsn_hid.struct_size = sizeof(pcb_hid_t);
	dsn_hid.name = "dsn";
	dsn_hid.description = "Exports DSN format";
	dsn_hid.exporter = 1;
	dsn_hid.get_export_options = dsn_get_export_options;
	dsn_hid.do_export = dsn_do_export;
	dsn_hid.parse_arguments = dsn_parse_arguments;
	pcb_hid_register_hid(&dsn_hid);

	pcb_hid_register_attributes(dsn_options, sizeof(dsn_options) / sizeof(dsn_options[0]), dsn_cookie, 0);
	return 0;
}

