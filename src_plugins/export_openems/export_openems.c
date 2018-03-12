/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "board.h"
#include "data.h"
#include "data_it.h"
#include "plugins.h"
#include "safe_fs.h"
#include "obj_subc_parent.h"
#include "obj_pstk_inlines.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_helper.h"
#include "hid_flags.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "hid_draw_helpers.h"
#include "../src_plugins/lib_polyhelp/topoly.h"
#include "mesh.h"

static pcb_hid_t openems_hid;

const char *openems_cookie = "openems HID";

#define MESH_NAME "openems"

typedef struct hid_gc_s {
	pcb_hid_t *me_pointer;
	pcb_cap_style_t cap;
	int width;
} hid_gc_s;

typedef struct {
	/* input/output */
	FILE *f;
	pcb_board_t *pcb;
	pcb_hid_attr_val_t *options;

	/* local cache */
	int lg_pcb2ems[PCB_MAX_LAYERGRP]; /* indexed by gid, gives 0 or the ems-side layer ID */
	int lg_ems2pcb[PCB_MAX_LAYERGRP]; /* indexed by the ems-side layer ID, gives -1 or a gid */
	int lg_next;
	int clayer; /* current layer (lg index really) */
	long oid; /* unique object ID - we need some unique variable names, keep on counting them */
	long pad_id; /* unique pad ID for the same reason (a single padstack object may make multiple pads) */
	pcb_coord_t ox, oy;
	unsigned warn_subc_term:1;
} wctx_t;

static FILE *f = NULL;
static wctx_t *ems_ctx;

#define THMAX PCB_MM_TO_COORD(100)

pcb_hid_attribute_t openems_attribute_list[] = {
	{"outfile", "Graphics output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_openemsfile 0

	{"def-copper-thick", "Default copper thickness",
	 PCB_HATT_COORD, 0, THMAX, {0, 0, 0, PCB_MM_TO_COORD(0.035)}, 0, 0},
#define HA_def_copper_thick 1

	{"def-substrate-thick", "Default substrate thickness",
	 PCB_HATT_COORD, 0, THMAX, {0, 0, 0, PCB_MM_TO_COORD(0.8)}, 0, 0},
#define HA_def_substrate_thick 2

	{"def-copper-cond", "Default copper conductivity",
	 PCB_HATT_STRING, 0, 0, {0, "56*10^6", 0}, 0, 0},
#define HA_def_copper_cond 3

	{"def-subst-epsilon", "Default substrate epsilon",
	 PCB_HATT_STRING, 0, 0, {0, "3.66", 0}, 0, 0},
#define HA_def_subst_epsilon 4

	{"def-subst-mue", "Default substrate mue",
	 PCB_HATT_STRING, 0, 0, {0, "0", 0}, 0, 0},
#define HA_def_subst_mue 5

	{"def-subst-kappa", "Default substrate kappa",
	 PCB_HATT_STRING, 0, 0, {0, "0", 0}, 0, 0},
#define HA_def_subst_kappa 6

	{"def-subst-sigma", "Default substrate sigma",
	 PCB_HATT_STRING, 0, 0, {0, "0", 0}, 0, 0},
#define HA_def_subst_sigma 7

	{"void-name", "Name of the void (sorrunding material)",
	 PCB_HATT_STRING, 0, 0, {0, "AIR", 0}, 0, 0},
#define HA_void_name 8

	{"void-epsilon", "epsilon value for the void (sorrunding material)",
	 PCB_HATT_REAL, 0, 1000, {0, 0, 1}, 0, 0},
#define HA_void_epsilon 9

	{"void-mue", "mue value for the void (sorrunding material)",
	 PCB_HATT_REAL, 0, 1000, {0, 0, 1}, 0, 0},
#define HA_void_mue 10

	{"segments", "kludge: number of segments used to approximate round cap trace ends",
	 PCB_HATT_INTEGER, 0, 100, {10, 0, 0}, 0, 0},
#define HA_segments 11

	{"base-prio", "base priority: if the board displaces the chassis",
	 PCB_HATT_INTEGER, 0, 10, {0, 0, 0}, 0, 0},
#define HA_base_prio 12

	{"f_max", "maximum frequency",
	 PCB_HATT_STRING, 0, 0, {0, "7e9", 0}, 0, 0},
#define HA_f_max 13

	{"excite", "Excite directive",
	 PCB_HATT_STRING, 0, 0, {0, "SetGaussExcite(FDTD, f_max/2, f_max/2)", 0}, 0, 0},
#define HA_excite 14

};

#define NUM_OPTIONS (sizeof(openems_attribute_list)/sizeof(openems_attribute_list[0]))

PCB_REGISTER_ATTRIBUTES(openems_attribute_list, openems_cookie)

static pcb_hid_attr_val_t openems_values[NUM_OPTIONS];

static pcb_hid_attribute_t *openems_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".m";
	pcb_mesh_t *mesh = pcb_mesg_get(MESH_NAME);

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &openems_attribute_list[HA_openemsfile], suffix, &last_made_filename);

	if (mesh != NULL) {
		openems_attribute_list[HA_def_substrate_thick].default_val.coord_value = mesh->def_subs_thick;
		openems_attribute_list[HA_def_copper_thick].default_val.coord_value = mesh->def_copper_thick;
	}

	if (n)
		*n = NUM_OPTIONS;
	return openems_attribute_list;
}

/* Find the openems 0;0 mark, if there is any */
static void find_origin_bump(void *ctx_, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	wctx_t *ctx = ctx_;
	if (pcb_attribute_get(&line->Attributes, "openems-origin") != NULL) {
		ctx->ox = (line->BoundingBox.X1 + line->BoundingBox.X2) / 2;
		ctx->oy = (line->BoundingBox.Y1 + line->BoundingBox.Y2) / 2;
	}
}

static void find_origin(wctx_t *ctx)
{
	pcb_loop_layers(ctx->pcb, ctx, NULL, find_origin_bump, NULL, NULL, NULL);
}

static void openems_write_tunables(wctx_t *ctx)
{
	fprintf(ctx->f, "%%%%%% User tunables\n");
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "%%%% base_priority and offset: chassis for the board to sit in.\n");
	fprintf(ctx->f, "%% base priority: if the board displaces the model of the chassis or the other way around.\n");
	fprintf(ctx->f, "base_priority=%d;\n", ctx->options[HA_base_prio].int_value);
	fprintf(ctx->f, "\n");
	fprintf(ctx->f, "%% offset on the whole layout to locate it relative to the simulation origin\n");
	pcb_fprintf(ctx->f, "offset.x = %mm;\n", -ctx->ox);
	pcb_fprintf(ctx->f, "offset.y = %mm;\n", ctx->oy);
	fprintf(ctx->f, "offset.z = 0;\n");
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "%% void is the material used for: fill holes, cutouts in substrate, etc\n");
	fprintf(ctx->f, "void.name = '%s';\n", ctx->options[HA_void_name].str_value);
	fprintf(ctx->f, "void.epsilon = %f;\n", ctx->options[HA_void_epsilon].real_value);
	fprintf(ctx->f, "void.mue = %f;\n", ctx->options[HA_void_mue].real_value);
	fprintf(ctx->f, "%% void.kappa = kappa;\n");
	fprintf(ctx->f, "%% void.sigma = sigma;\n");
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "%% how many points should be used to describe the round end of traces.\n");
	fprintf(ctx->f, "kludge.segments = %d;\n", ctx->options[HA_segments].int_value);
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "\n");
}

static void print_lparm(wctx_t *ctx, pcb_layergrp_t *grp, const char *attr, int cop_opt, int subs_opt, int is_str)
{
	int opt;

#warning TODO: this needs layer group attributes in core (planned for lihata v5)
#if 0
#warning TODO: try openems::attr first - make a new core call for prefixed get, this will be a real common pattern
	const char *val = pcb_attribute_get(&grp->Attributes, attr);

	if (val != NULL) {
		/* specified by a layer group attribute: overrides anything else */
		if (is_str)
			fprintf(ctx->f, "%s", val);
		else
			TODO: getvalue, print as coord
		return;
	}
#endif

	opt = (grp->type & PCB_LYT_COPPER) ? cop_opt : subs_opt;
	assert(opt >= 0);
	if (is_str)
		pcb_fprintf(ctx->f, "%s", ctx->options[opt].str_value);
	else
		pcb_fprintf(ctx->f, "%mm", ctx->options[opt].coord_value);
}

static void openems_write_layers(wctx_t *ctx)
{
	pcb_layergrp_id_t gid;
	int next = 1;

	for(gid = 0; gid < PCB_MAX_LAYERGRP; gid++)
		ctx->lg_ems2pcb[gid] = -1;

	fprintf(ctx->f, "%%%%%% Layer mapping\n");

	/* linear map of copper and substrate layers */
	for(gid = 0; gid < ctx->pcb->LayerGroups.len; gid++) {
		pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[gid];
		int iscop = (grp->type & PCB_LYT_COPPER);

		if (!(iscop) && !(grp->type & PCB_LYT_SUBSTRATE))
			continue;
		ctx->lg_ems2pcb[next] = gid;
		ctx->lg_pcb2ems[gid] = next;

		fprintf(ctx->f, "layers(%d).number = %d;\n", next, next); /* type index really */
		fprintf(ctx->f, "layers(%d).name = '%s';\n", next, (grp->name == NULL ? "anon" : grp->name));
		fprintf(ctx->f, "layers(%d).clearn = 0;\n", next);
		fprintf(ctx->f, "layer_types(%d).name = '%s_%d';\n", next, iscop ? "COPPER" : "SUBSTRATE", next);
		fprintf(ctx->f, "layer_types(%d).subtype = %d;\n", next, iscop ? 2 : 3);

		fprintf(ctx->f, "layer_types(%d).thickness = ", next);
		print_lparm(ctx, grp, "thickness", HA_def_copper_thick, HA_def_substrate_thick, 0);
		fprintf(ctx->f, ";\n");

		if (iscop) {
			fprintf(ctx->f, "layer_types(%d).conductivity = ", next);
			print_lparm(ctx, grp, "conductivity", HA_def_copper_cond, -1, 1);
			fprintf(ctx->f, ";\n");
		}
		else { /* substrate */
			fprintf(ctx->f, "layer_types(%d).epsilon = ", next);
			print_lparm(ctx, grp, "epsilon", -1, HA_def_subst_epsilon, 1);
			fprintf(ctx->f, ";\n");

			fprintf(ctx->f, "layer_types(%d).mue = ", next);
			print_lparm(ctx, grp, "mue", -1, HA_def_subst_mue, 1);
			fprintf(ctx->f, ";\n");

			fprintf(ctx->f, "layer_types(%d).kappa = ", next);
			print_lparm(ctx, grp, "kappa", -1, HA_def_subst_kappa, 1);
			fprintf(ctx->f, ";\n");

			fprintf(ctx->f, "layer_types(%d).sigma = ", next);
			print_lparm(ctx, grp, "sigma", -1, HA_def_subst_sigma, 1);
			fprintf(ctx->f, ";\n");

		}

		next++;
		fprintf(ctx->f, "\n");
	}
	fprintf(ctx->f, "\n");

	ctx->lg_next = next;
}

static void openems_write_init(wctx_t *ctx)
{
	fprintf(ctx->f, "%%%%%% Initialize pcb2csx\n");
	fprintf(ctx->f, "PCBRND = InitPCBRND(layers, layer_types, void, base_priority, offset, kludge);\n");
	fprintf(ctx->f, "CSX = InitPcbrndLayers(CSX, PCBRND);\n");
	fprintf(ctx->f, "\n");
}

static void openems_write_outline(wctx_t *ctx)
{
	int n;
	pcb_any_obj_t *out1;

	fprintf(ctx->f, "%%%%%% Board outline\n");

	out1 = pcb_topoly_find_1st_outline(ctx->pcb);
	if (out1 != NULL) {
		long n;
		pcb_poly_t *p = pcb_topoly_conn(ctx->pcb, out1, PCB_TOPOLY_KEEP_ORIG | PCB_TOPOLY_FLOATING);
		for(n = 0; n < p->PointN; n++)
			pcb_fprintf(ctx->f, "outline_xy(1, %ld) = %mm; outline_xy(2, %ld) = %mm;\n", n+1, p->Points[n].X, n+1, -p->Points[n].Y);
		pcb_poly_free(p);
	}
	else {
		/* rectangular board size */
		pcb_fprintf(ctx->f, "outline_xy(1, 1) = 0; outline_xy(2, 1) = 0;\n");
		pcb_fprintf(ctx->f, "outline_xy(1, 2) = %mm; outline_xy(2, 2) = 0;\n", ctx->pcb->MaxWidth);
		pcb_fprintf(ctx->f, "outline_xy(1, 3) = %mm; outline_xy(2, 3) = %mm;\n", ctx->pcb->MaxWidth, -ctx->pcb->MaxHeight);
		pcb_fprintf(ctx->f, "outline_xy(1, 4) = 0; outline_xy(2, 4) = %mm;\n", -ctx->pcb->MaxHeight);
	}

	/* create all substrate layers using this polygon*/
	for(n = 1; n < ctx->lg_next; n++) {
		pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[ctx->lg_ems2pcb[n]];
		if (grp->type & PCB_LYT_SUBSTRATE)
			fprintf(ctx->f, "CSX = AddPcbrndPoly(CSX, PCBRND, %d, outline_xy, 1);\n", n);
	}

	fprintf(ctx->f, "\n");
}

static void openems_write_testpoint_(wctx_t *ctx, pcb_coord_t x, pcb_coord_t y, int layer, const char *refdes, const char *term)
{
	long oid = ctx->oid++;
	pcb_coord_t sx = PCB_MM_TO_COORD(0.1), sy = PCB_MM_TO_COORD(0.1);

	pcb_fprintf(ctx->f, "points%ld(1, 1) = %mm; points%ld(2, 1) = %mm;\n", oid, x-sx, oid, -(y-sy));
	pcb_fprintf(ctx->f, "points%ld(1, 2) = %mm; points%ld(2, 2) = %mm;\n", oid, x+sx, oid, -(y-sy));
	pcb_fprintf(ctx->f, "points%ld(1, 3) = %mm; points%ld(2, 3) = %mm;\n", oid, x+sx, oid, -(y+sy));
	pcb_fprintf(ctx->f, "points%ld(1, 4) = %mm; points%ld(2, 4) = %mm;\n", oid, x-sx, oid, -(y+sy));
	fprintf(ctx->f, "refdes = '%s';\n", refdes);
	fprintf(ctx->f, "pad.number = '%s';\n", term);
	fprintf(ctx->f, "pad.id = '%ld';\n", ++ctx->pad_id);
	fprintf(ctx->f, "PCBRND = RegPcbrndPad(PCBRND, %d, points%ld, refdes, pad);\n", layer, oid);
	fprintf(ctx->f, "[pad_points layer_number] = LookupPcbrndPort(PCBRND, refdes, pad);\n");
	fprintf(ctx->f, "[ start stop] = CalcPcbrndPoly2Port(PCBRND, points%ld, layer_number);\n", oid);
}


static void openems_write_testpoint_on(wctx_t *ctx, const char *refdes, const char *termid, pcb_layergrp_id_t gid, pcb_coord_t x, pcb_coord_t y)
{
	int layer;

	layer = ctx->lg_pcb2ems[gid];
	if (layer <= 0) {
		pcb_message(PCB_MSG_ERROR, "Can't determine EMS layer for pad (%$mm;%$mm)\n", x, y);
		return;
	}

	openems_write_testpoint_(ctx, x, y, layer, refdes, termid);
}

static void openems_write_testpoint(wctx_t *ctx, pcb_any_obj_t *o, pcb_coord_t x, pcb_coord_t y)
{
	pcb_layergrp_id_t gid;
	const char *refdes = NULL;
	pcb_subc_t *sc;

	sc = pcb_obj_parent_subc(o);
	if (sc != NULL)
		refdes = sc->refdes;

	if (refdes == NULL)
		refdes = "none";

	if (o->type == PCB_OBJ_PSTK) { /* light terminal: padstack */
		for(gid = 0; gid < ctx->pcb->LayerGroups.len; gid++) { /* put testpoint on all interesting layers */
			pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[gid];
			pcb_layer_id_t lid;
			pcb_pstk_shape_t *sh;

			if (grp->len <= 0) /* group has no layers -> probably substrate */
				continue;

			if (!(grp->type & PCB_LYT_COPPER)) /* testpoint goes on copper only */
				continue;

			if (!(grp->type & PCB_LYT_TOP) && !(grp->type & PCB_LYT_BOTTOM)) /* do not put testpoints on inner layers */
				continue;

			lid = grp->lid[0];
			if (lid < 0) /* invalid layer in group, what?! */
				continue;

			sh = pcb_pstk_shape_at(ctx->pcb, (pcb_pstk_t *)o, &ctx->pcb->Data->Layer[lid]);
			if (sh == NULL) /* padstack has no shape on layer */
				continue;

			openems_write_testpoint_on(ctx, refdes, o->term, gid, x, y);
		}
	}
	else { /* heavy terminal: plain layer object */
		assert(o->parent_type == PCB_PARENT_LAYER);

		gid = pcb_layer_get_group_(o->parent.layer);
		if (gid < 0) {
			pcb_message(PCB_MSG_ERROR, "Can't determine pad layer (%$mm;%$mm)\n", x, y);
			return;
		}

		openems_write_testpoint_on(ctx, refdes, o->term, gid, x, y);
	}
}


#define TPMASK (PCB_OBJ_LINE | PCB_OBJ_PSTK | PCB_OBJ_SUBC)
static void openems_write_testpoints(wctx_t *ctx, pcb_data_t *data)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	
	for(o = pcb_data_first(&it, data, TPMASK); o != NULL; o = pcb_data_next(&it)) {
		if (o->type == PCB_OBJ_SUBC)
			openems_write_testpoints(ctx, ((pcb_subc_t *)o)->data);
		if (o->term == NULL)
			continue;
		if (o->type == PCB_OBJ_SUBC) {
			if (!ctx->warn_subc_term)
				pcb_message(PCB_MSG_ERROR, "Subcircuit being a terminal is not supported.\n");
			ctx->warn_subc_term = 1;
			continue;
		}

		/* place the test pad */
		switch(o->type) {
			case PCB_OBJ_PSTK: openems_write_testpoint(ctx, o, ((pcb_pstk_t *)o)->x, ((pcb_pstk_t *)o)->y); break;
			default: openems_write_testpoint(ctx, o, (o->BoundingBox.X1+o->BoundingBox.X2)/2, (o->BoundingBox.Y1+o->BoundingBox.Y2)/2);
		}
	}
}

static void openems_write_mesh_lines(wctx_t *ctx, pcb_mesh_lines_t *l)
{
	pcb_cardinal_t n;
	for(n = 0; n < vtc0_len(&l->result); n++)
		pcb_fprintf(ctx->f, "%s%mm", (n == 0 ? "" : " "), l->result.array[n]);
}

static void openems_write_mesh(wctx_t *ctx)
{
	pcb_mesh_t *mesh = pcb_mesg_get(MESH_NAME);
	int n;

	if (mesh == NULL) {
		fprintf(ctx->f, "%%%%%% Board mesh (NOT defined in pcb-rnd)\n");
		return;
	}

	fprintf(ctx->f, "%%%%%% Board mesh (defined in pcb-rnd)\n");
	fprintf(ctx->f, "unit = 1.0e-3;\n");
	fprintf(ctx->f, "f_max = %s;\n", ctx->options[HA_f_max].str_value);
	fprintf(ctx->f, "FDTD = InitFDTD();\n");
	fprintf(ctx->f, "FDTD = %s;\n", ctx->options[HA_excite].str_value);
	
	fprintf(ctx->f, "BC = {");

	for(n = 0; n < 6; n++)
		fprintf(ctx->f, "%s'%s'", (n == 0 ? "" : " "), mesh->bnd[n]);
	fprintf(ctx->f, "};\n");

	fprintf(ctx->f, "FDTD = SetBoundaryCond(FDTD, BC);\n");
	fprintf(ctx->f, "physical_constants;\n");
	fprintf(ctx->f, "CSX = InitCSX();\n");
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "mesh.x=[");
	openems_write_mesh_lines(ctx, &mesh->line[PCB_MESH_HORIZONTAL]);
	fprintf(ctx->f, "];\n");

	fprintf(ctx->f, "mesh.y=[");
	openems_write_mesh_lines(ctx, &mesh->line[PCB_MESH_VERTICAL]);
	fprintf(ctx->f, "];\n");

	fprintf(ctx->f, "mesh.z=[");
	openems_write_mesh_lines(ctx, &mesh->line[PCB_MESH_Z]);
	fprintf(ctx->f, "];\n");

	fprintf(ctx->f, "mesh.x = mesh.x .+ offset.x;\n");
	fprintf(ctx->f, "mesh.y = mesh.y .+ offset.y;\n");
	fprintf(ctx->f, "mesh.z = mesh.z .+ offset.z;\n");
	fprintf(ctx->f, "CSX = DefineRectGrid(CSX, unit, mesh);\n");
	fprintf(ctx->f, "\n");
}


void openems_hid_export_to_file(FILE *the_file, pcb_hid_attr_val_t *options)
{
	pcb_hid_expose_ctx_t ctx;
	wctx_t wctx;

	memset(&wctx, 0, sizeof(wctx));
	wctx.f = the_file;
	wctx.pcb = PCB;
	wctx.options = options;
	ems_ctx = &wctx;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->MaxWidth;
	ctx.view.Y2 = PCB->MaxHeight;

	f = the_file;

	conf_force_set_bool(conf_core.editor.thin_draw, 0);
	conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
/*		conf_force_set_bool(conf_core.editor.check_planes, 0);*/
	conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	find_origin(&wctx);
	openems_write_tunables(&wctx);
	openems_write_layers(&wctx);
	openems_write_init(&wctx);
	openems_write_mesh(&wctx);
	openems_write_outline(&wctx);

	fprintf(wctx.f, "%%%%%% Copper objects\n");
	pcb_hid_expose_all(&openems_hid, &ctx);

	fprintf(wctx.f, "%%%%%% Testpoints on terminals\n");
	openems_write_testpoints(&wctx, wctx.pcb->Data);

	conf_update(NULL, -1); /* restore forced sets */
}

static void openems_do_export(pcb_hid_attr_val_t * options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER + 2];
	int i;

	if (!options) {
		openems_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			openems_values[i] = openems_attribute_list[i].default_val;
		options = openems_values;
	}

	filename = options[HA_openemsfile].str_value;
	if (!filename)
		filename = "pcb.m";

	f = pcb_fopen(filename, "wb");
	if (!f) {
		perror(filename);
		return;
	}

	pcb_hid_save_and_show_layer_ons(save_ons);

	openems_hid_export_to_file(f, options);

	pcb_hid_restore_layer_ons(save_ons);

	fclose(f);
	f = NULL;
}

static void openems_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(openems_attribute_list, sizeof(openems_attribute_list) / sizeof(openems_attribute_list[0]), openems_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

static int openems_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	if (flags & PCB_LYT_COPPER) { /* export copper layers only */
		ems_ctx->clayer = ems_ctx->lg_pcb2ems[group];
		return 1;
	}
	return 0;
}

static pcb_hid_gc_t openems_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) calloc(sizeof(hid_gc_s), 1);
	rv->me_pointer = &openems_hid;
	return rv;
}

static void openems_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static void openems_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	switch(op) {
		case PCB_HID_COMP_RESET:
			break;
		case PCB_HID_COMP_POSITIVE:
			break;
		case PCB_HID_COMP_NEGATIVE:
			pcb_message(PCB_MSG_ERROR, "Can't draw composite layer, especially not on copper\n");
			break;
		case PCB_HID_COMP_FLUSH:
			break;
	}
}

static void openems_set_color(pcb_hid_gc_t gc, const char *name)
{
}

static void openems_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void openems_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}


static void openems_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

static void openems_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void openems_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void openems_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
}

static void openems_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	wctx_t *ctx = ems_ctx;
	long oid = ctx->oid++;

	pcb_fprintf(ctx->f, "points%ld(1, 1) = %mm; points%ld(2, 1) = %mm;\n", oid, cx, oid, -cy);
	pcb_fprintf(ctx->f, "points%ld(1, 2) = %mm; points%ld(2, 2) = %mm;\n", oid, cx, oid, -cy);
	pcb_fprintf(ctx->f, "CSX = AddPcbrndTrace(CSX, PCBRND, %d, points%ld, %mm, 0);\n", ctx->clayer, oid, radius*2);
}

static void openems_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	wctx_t *ctx = ems_ctx;
	int n;
	long oid = ctx->oid++;

	for(n = 0; n < n_coords; n++)
		pcb_fprintf(ctx->f, "poly%ld_xy(1, %ld) = %mm; poly%ld_xy(2, %ld) = %mm;\n", oid, n+1, x[n]+dx, oid, n+1, -(y[n]+dy));

	fprintf(ctx->f, "CSX = AddPcbrndPoly(CSX, PCBRND, %d, poly%ld_xy, 1);\n", ctx->clayer, oid);
}

static void openems_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	openems_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void openems_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	wctx_t *ctx = ems_ctx;

	if (gc->cap == Square_Cap) {
		pcb_coord_t x[4], y[4];
		pcb_line_t tmp;
		tmp.Point1.X = x1;
		tmp.Point1.Y = y1;
		tmp.Point2.X = x2;
		tmp.Point2.Y = y2;
		pcb_sqline_to_rect(&tmp, x, y);
		openems_fill_polygon_offs(gc, 4, x, y, 0, 0);
	}
	else {
		long oid = ctx->oid++;
		pcb_fprintf(ctx->f, "points%ld(1, 1) = %mm; points%ld(2, 1) = %mm;\n", oid, x1, oid, -y1);
		pcb_fprintf(ctx->f, "points%ld(1, 2) = %mm; points%ld(2, 2) = %mm;\n", oid, x2, oid, -y2);
		pcb_fprintf(ctx->f, "CSX = AddPcbrndTrace(CSX, PCBRND, %d, points%ld, %mm, 0);\n", ctx->clayer, oid, gc->width);
	}
}


static void openems_calibrate(double xval, double yval)
{
	pcb_message(PCB_MSG_ERROR, "openems_calibrate() not implemented");
	return;
}

static void openems_set_crosshair(int x, int y, int a)
{
}

static int openems_usage(const char *topic)
{
	fprintf(stderr, "\nopenems exporter command line arguments:\n\n");
	pcb_hid_usage(openems_attribute_list, sizeof(openems_attribute_list) / sizeof(openems_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x openems [openems options] foo.pcb\n\n");
	return 0;
}

static pcb_hid_action_t openems_action_list[] = {
	{"mesh", NULL, pcb_act_mesh, pcb_acth_mesh, pcb_acts_mesh}
};

PCB_REGISTER_ACTIONS(openems_action_list, openems_cookie)

#include "dolists.h"

int pplg_check_ver_export_openems(int ver_needed) { return 0; }

void pplg_uninit_export_openems(void)
{
	pcb_hid_remove_actions_by_cookie(openems_cookie);
	pcb_hid_remove_attributes_by_cookie(openems_cookie);
}

int pplg_init_export_openems(void)
{
	memset(&openems_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&openems_hid);
	pcb_dhlp_draw_helpers_init(&openems_hid);

	openems_hid.struct_size = sizeof(pcb_hid_t);
	openems_hid.name = "openems";
	openems_hid.description = "OpenEMS exporter";
	openems_hid.exporter = 1;
	openems_hid.holes_after = 1;

	openems_hid.get_export_options = openems_get_export_options;
	openems_hid.do_export = openems_do_export;
	openems_hid.parse_arguments = openems_parse_arguments;
	openems_hid.set_layer_group = openems_set_layer_group;
	openems_hid.make_gc = openems_make_gc;
	openems_hid.destroy_gc = openems_destroy_gc;
	openems_hid.set_drawing_mode = openems_set_drawing_mode;
	openems_hid.set_color = openems_set_color;
	openems_hid.set_line_cap = openems_set_line_cap;
	openems_hid.set_line_width = openems_set_line_width;
	openems_hid.set_draw_xor = openems_set_draw_xor;
	openems_hid.draw_line = openems_draw_line;
	openems_hid.draw_arc = openems_draw_arc;
	openems_hid.draw_rect = openems_draw_rect;
	openems_hid.fill_circle = openems_fill_circle;
	openems_hid.fill_polygon = openems_fill_polygon;
	openems_hid.fill_polygon_offs = openems_fill_polygon_offs;
	openems_hid.fill_rect = openems_fill_rect;
	openems_hid.calibrate = openems_calibrate;
	openems_hid.set_crosshair = openems_set_crosshair;

	openems_hid.usage = openems_usage;

	pcb_hid_register_hid(&openems_hid);

	PCB_REGISTER_ACTIONS(openems_action_list, openems_cookie);

	return 0;
}
