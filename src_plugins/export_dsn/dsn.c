/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *
 *  Specctra .dsn export HID
 *  Copyright (C) 2008, 2011 Josh Jordan, Dan McMahill, and Jared Casper
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/*
This program exports specctra .dsn files.
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

#include "board.h"
#include "data.h"
#include "error.h"
#include "buffer.h"
#include "change.h"
#include "draw.h"
#include "undo.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "compat_misc.h"
#include "layer.h"
#include "safe_fs.h"
#include "netlist2.h"

#include "hid.h"
#include "hid_draw_helpers.h"
#include "hid_nogui.h"
#include "actions.h"
#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_cam.h"
#include "plugins.h"
#include "obj_line.h"
#include "obj_pstk_inlines.h"

static const char *dsn_cookie = "dsn exporter";

#define GRP_NAME(grp_) ((grp_) - PCB->LayerGroups.grp), ((grp_)->name)

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
	if ((PCB != NULL)  && (dsn_options[HA_dsnfile].default_val.str_value == NULL))
		pcb_derive_default_filename(PCB->Filename, &dsn_options[HA_dsnfile], ".dsn");
	if (n)
		*n = NUM_OPTIONS;
	return dsn_options;
}

static void print_structure(FILE * fp)
{
	pcb_layergrp_id_t group, top_group, bot_group;

	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &top_group, 1);
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &bot_group, 1);

	fprintf(fp, "  (structure\n");
	for (group = 0; group < pcb_max_group(PCB); group++) {
		htsp_entry_t *e;
		pcb_layergrp_t *g = &PCB->LayerGroups.grp[group];
		char *layeropts = pcb_strdup("(type signal)");
		
		if (!(g->ltype & PCB_LYT_COPPER))
			continue;

TODO("revise this; use attributes instead")
		/* see if group has same name as a net and make it a power layer */
		/* loop thru all nets */
		for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
			pcb_net_t *net = e->value;
			if (!strcmp(g->name, net->name)) {
				free(layeropts);
				layeropts = pcb_strdup_printf("(type power) (use_net \"%s\")", g->name);
			}
		}
		fprintf(fp, "    (layer \"%ld__%s\"\n", (long)GRP_NAME(g));
		fprintf(fp, "      %s\n", layeropts);
		fprintf(fp, "    )\n");
		free(layeropts);
	}

	/* PCB outline */
	pcb_fprintf(fp, "    (boundary\n");
	pcb_fprintf(fp, "      (rect pcb 0.0 0.0 %.6mm %.6mm)\n", PCB->hidlib.size_x, PCB->hidlib.size_y);
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
		pcb_fprintf(fp, "      (place \"%s\" %.6mm %.6mm %s 0 (PN 0))\n", ename, ox, PCB->hidlib.size_y - oy, side);
		pcb_fprintf(fp, "    )\n");
		free(ename);
	}
	PCB_END_LOOP;

TODO("padstack: check if real shapes are exported")
	PCB_PADSTACK_LOOP(PCB->Data);
	{ /* add mounting holes */
		pcb_fprintf(fp, "    (component %d\n", padstack->ID);
		pcb_fprintf(fp, "      (place %d %.6mm %.6mm %s 0 (PN 0))\n", padstack->ID, padstack->x, (PCB->hidlib.size_y - padstack->y), "front");
		pcb_fprintf(fp, "    )\n");
	}
	PCB_END_LOOP;

	fprintf(fp, "  )\n");
}

static void print_polyshape(gds_t *term_shapes, pcb_pstk_poly_t *ply, pcb_coord_t ox, pcb_coord_t oy, pcb_layergrp_t *grp, int partsidesign)
{
	char tmp[512];
	int fld;
	int n;

	pcb_snprintf(tmp, sizeof(tmp), "        (polygon \"%d__%s\" 0", GRP_NAME(grp));
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

static void print_lineshape(gds_t *term_shapes, pcb_pstk_line_t *lin, pcb_coord_t ox, pcb_coord_t oy, pcb_layergrp_t *grp, int partsidesign)
{
	char tmp[512];
	int fld;
	pcb_coord_t x[4], y[4];
	int n;
	pcb_line_t ltmp;

	pcb_snprintf(tmp, sizeof(tmp), "        (polygon \"%d__%s\" 0", GRP_NAME(grp));
	gds_append_str(term_shapes, tmp);

	memset(&ltmp, 0, sizeof(ltmp));
	ltmp.Point1.X = lin->x1;
	ltmp.Point1.Y = lin->y1;
	ltmp.Point2.X = lin->x2;
	ltmp.Point2.Y = lin->y2;
	ltmp.Thickness = lin->thickness;
	pcb_sqline_to_rect(&ltmp, x, y);

TODO("padstack: this ignores round cap")

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

static void print_circshape(gds_t *term_shapes, pcb_pstk_circ_t *circ, pcb_coord_t ox, pcb_coord_t oy, pcb_layergrp_t *grp, int partsidesign)
{
	char tmp[512];

	pcb_snprintf(tmp, sizeof(tmp), "        (circle \"%d__%s\"", GRP_NAME(grp));
	gds_append_str(term_shapes, tmp);

TODO("padstack: this ignores circle center offset")

	pcb_snprintf(tmp, sizeof(tmp), " %.6mm)\n", circ->dia);
	gds_append_str(term_shapes, tmp);
}

static void print_polyline(gds_t *term_shapes, pcb_poly_it_t *it, pcb_pline_t *pl, pcb_coord_t ox, pcb_coord_t oy, pcb_layergrp_t *grp, int partsidesign)
{
	char tmp[512];
	int fld;
	pcb_coord_t x, y;
	int go;

	if (pl != NULL) {
		pcb_snprintf(tmp, sizeof(tmp), "(polygon \"%d__%s\" 0", GRP_NAME(grp));
		gds_append_str(term_shapes, tmp);

		fld = 0;
		for(go = pcb_poly_vect_first(it, &x, &y); go; go = pcb_poly_vect_next(it, &x, &y)) {
			if ((fld % 3) == 0)
				gds_append_str(term_shapes, "\n       ");
			pcb_snprintf(tmp, sizeof(tmp), " %.6mm %.6mm", (x - ox) * partsidesign, -(y - oy));
			gds_append_str(term_shapes, tmp);
			fld++;
		}

		gds_append_str(term_shapes, "\n        )\n");
	}
}

static void print_term_poly(FILE *fp, gds_t *term_shapes, pcb_poly_t *poly, pcb_coord_t ox, pcb_coord_t oy, int term_on_bottom, int partsidesign)
{
	if (poly->term != NULL) {
		pcb_layergrp_id_t gid = term_on_bottom ? pcb_layergrp_get_bottom_copper() : pcb_layergrp_get_top_copper();
		pcb_layergrp_t *grp = pcb_get_layergrp(PCB, gid);
		char *padstack = pcb_strdup_printf("Term_poly_%ld", poly->ID);
		pcb_poly_it_t it;
		pcb_polyarea_t *pa;


		pcb_fprintf(fp, "      (pin %s \"%s\" %.6mm %.6mm)\n", padstack, poly->term, 0, 0);

		gds_append_str(term_shapes, "    (padstack ");
		gds_append_str(term_shapes, padstack);
		gds_append_str(term_shapes, "\n");

		gds_append_str(term_shapes, "      (shape ");

		for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
			pcb_pline_t *pl;

			pl = pcb_poly_contour(&it);

			print_polyline(term_shapes, &it, pl, ox, oy, grp, partsidesign);
		}

		gds_append_str(term_shapes, "      )\n");
		gds_append_str(term_shapes, "      (attach off)\n");
		gds_append_str(term_shapes, "    )\n");
	}
}

void print_pstk_shape(gds_t *term_shapes, pcb_pstk_t *padstack, pcb_layergrp_id_t gid, pcb_coord_t ox, pcb_coord_t oy, int partsidesign)
{
	pcb_pstk_shape_t *shp;
	pcb_layergrp_t *grp = pcb_get_layergrp(PCB, gid);
	pcb_layer_type_t lyt = grp->ltype;

	shp = pcb_pstk_shape(padstack, lyt, 0);
	if (shp == NULL)
		return;

	/* if the subc is placed on the other side, need to invert the output layerstack as well */
	if (partsidesign < 0) {
		pcb_layergrp_id_t n, offs = 0;

		/* determine copper offset from the top */
		for(n = 0; (n < PCB->LayerGroups.len) && (n != gid); n++)
			if (PCB->LayerGroups.grp[n].type & PCB_LYT_COPPER)
				offs++;

		/* count it back from the bottom and set grp */
		for(n = PCB->LayerGroups.len-1; (n > 0) && (n != gid); n--) {
			if (PCB->LayerGroups.grp[n].type & PCB_LYT_COPPER) {
				if (offs == 0) {
					grp = &PCB->LayerGroups.grp[n];
					break;
				}
				offs--;
			}
		}
	}

	switch(shp->shape) {
		case PCB_PSSH_POLY:
			print_polyshape(term_shapes, &shp->data.poly, ox, oy, grp, partsidesign);
			break;
		case PCB_PSSH_LINE:
			print_lineshape(term_shapes, &shp->data.line, ox, oy, grp, partsidesign);
			break;
		case PCB_PSSH_CIRC:
			print_circshape(term_shapes, &shp->data.circ, ox, oy, grp, partsidesign);
			break;
		case PCB_PSSH_HSHADOW:
			break;
	}
}

static void print_library(FILE * fp)
{
	gds_t term_shapes;

	gds_init(&term_shapes);

	fprintf(fp, "  (library\n");

	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_coord_t ox, oy;
		int partsidesign, subc_on_solder = 0;
		pcb_layer_type_t lyt_side;

		pcb_subc_get_side(subc, &subc_on_solder);
		partsidesign = subc_on_solder ? -1 : 1;
		lyt_side = subc_on_solder ? PCB_LYT_BOTTOM : PCB_LYT_TOP;

		pcb_subc_get_origin(subc, &ox, &oy);

		fprintf(fp, "    (image %ld\n", subc->ID); /* map every subc by ID */
		PCB_POLY_COPPER_LOOP(subc->data);
		{
			pcb_layer_type_t lyt = pcb_layer_flags_(layer);
			if ((lyt & PCB_LYT_COPPER) && ((lyt & PCB_LYT_TOP) || (lyt & PCB_LYT_BOTTOM)))
				print_term_poly(fp, &term_shapes, polygon, ox, oy, !(lyt & lyt_side), partsidesign);
		}
		PCB_ENDALL_LOOP;

		PCB_PADSTACK_LOOP(subc->data);
		{
			pcb_layergrp_id_t group;
			char *pid = pcb_strdup_printf("Pstk_shape_%ld", padstack->ID);

			pcb_fprintf(fp, "      (pin %s \"%s\" %.6mm %.6mm)\n", pid, padstack->term, (padstack->x-ox)*partsidesign, -(padstack->y-oy));

			gds_append_str(&term_shapes, "    (padstack ");
			gds_append_str(&term_shapes, pid);
			gds_append_str(&term_shapes, "\n");

			gds_append_str(&term_shapes, "      (shape\n");

			for (group = 0; group < pcb_max_group(PCB); group++) {
				pcb_layergrp_t *g = &PCB->LayerGroups.grp[group];
				if (g->ltype & PCB_LYT_COPPER)
					print_pstk_shape(&term_shapes, padstack, group, 0, 0, partsidesign);
			}

			gds_append_str(&term_shapes, "      )\n");
			gds_append_str(&term_shapes, "      (attach off)\n");
			gds_append_str(&term_shapes, "    )\n");
		}
		PCB_END_LOOP;

		fprintf(fp, "    )\n");
	}
	PCB_END_LOOP;

	/* add padstack for terminals */
	pcb_fprintf(fp, "%s", term_shapes.array);
	pcb_fprintf(fp, "  )\n");


	gds_uninit(&term_shapes);
}

static void print_quoted_pin(FILE *fp, const pcb_net_term_t *t)
{
	fprintf(fp, " \"%s\"-\"%s\"", t->refdes, t->term);
}

static void print_network(FILE * fp)
{
	htsp_entry_t *e;

	fprintf(fp, "  (network\n");

	for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
		pcb_net_term_t *t;
		pcb_net_t *net = e->value;

		fprintf(fp, "    (net \"%s\"\n", net->name);
		fprintf(fp, "      (pins");

		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t))
			print_quoted_pin(fp, t);
		fprintf(fp, ")\n");
		fprintf(fp, "    )\n");
	}

	fprintf(fp, "    (class pcb_rnd_default");
	for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
		pcb_net_t *net = e->value;
		fprintf(fp, " \"%s\"", net->name);
	}
	pcb_fprintf(fp, "\n");
	pcb_fprintf(fp, "      (circuit\n");
	pcb_fprintf(fp, "        (use_via via_%ld_%ld)\n", viawidth, viadrill);
	pcb_fprintf(fp, "      )\n");
	pcb_fprintf(fp, "      (rule (width %.6mm))\n    )\n  )\n", trackwidth);
}

static void print_wires(FILE * fp)
{
	int group;
	fprintf(fp, "    (wiring\n");

	for (group = 0; group < pcb_max_group(PCB); group++) {
		pcb_layergrp_t *g = &PCB->LayerGroups.grp[group];
		pcb_cardinal_t n;
		if (!(g->ltype & PCB_LYT_COPPER))
			continue;
		for(n = 0; n < g->len; n++) {
			pcb_layer_t *lay = pcb_get_layer(PCB->Data, g->lid[n]);

			PCB_LINE_LOOP(lay);
			{
				pcb_fprintf(fp,
					"        (wire (path %d__%s %.6mm %.6mm %.6mm %.6mm %.6mm)\n", GRP_NAME(g), line->Thickness,
					line->Point1.X, (PCB->hidlib.size_y - line->Point1.Y),
					line->Point2.X, (PCB->hidlib.size_y - line->Point2.Y));
				fprintf(fp, "            (type protect))\n");
			}
			PCB_END_LOOP;
		}
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

