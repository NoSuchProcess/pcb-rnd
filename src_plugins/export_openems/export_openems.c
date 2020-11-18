/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
#include "draw.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>
#include "layer_vis.h"
#include "obj_subc_parent.h"
#include "obj_pstk_inlines.h"

#include <librnd/core/hid.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_nogui.h>
#include "hid_cam.h"
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_init.h>
#include "../src_plugins/lib_polyhelp/topoly.h"
#include "mesh.h"

static rnd_hid_t openems_hid;

const char *openems_cookie = "openems HID";

#include "excitation.c"

#define MESH_NAME "openems"

#define PRIO_SUBSTRATE 1
#define PRIO_COPPER 2
#define PRIO_PORT 999

typedef struct rnd_hid_gc_s {
	rnd_core_gc_t core_gc;
	rnd_hid_t *me_pointer;
	rnd_cap_style_t cap;
	int width;
} rnd_hid_gc_s;

typedef struct {
	/* input/output */
	FILE *f, *fsim;
	pcb_board_t *pcb;
	rnd_hid_attr_val_t *options;

	/* local cache */
	const char *filename;
	int lg_pcb2ems[PCB_MAX_LAYERGRP]; /* indexed by gid, gives 0 or the ems-side layer ID */
	int lg_ems2pcb[PCB_MAX_LAYERGRP]; /* indexed by the ems-side layer ID, gives -1 or a gid */
	int lg_next;
	int clayer; /* current layer (lg index really) */
	long oid; /* unique object ID - we need some unique variable names, keep on counting them */
	long port_id; /* unique port ID for similar reasons */
	rnd_coord_t ox, oy;
	unsigned warn_subc_term:1;
	unsigned warn_port_pstk:1;
	unsigned fmt_matlab:1; /* when 1, use matlab syntax; 0 means xml syntax */

	/* xml */
	unsigned cond_sheet_open:1;
	double elevation; /* in mm */
} wctx_t;

static FILE *f = NULL;
static wctx_t *ems_ctx;
static int openems_ovr;


#define THMAX RND_MM_TO_COORD(100)

rnd_export_opt_t openems_attribute_list[] = {
	{"outfile", "Graphics output file",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_openemsfile 0

	{"def-copper-thick", "Default copper thickness",
	 RND_HATT_COORD, 0, THMAX, {0, 0, 0, RND_MM_TO_COORD(0.035)}, 0, 0},
#define HA_def_copper_thick 1

	{"def-substrate-thick", "Default substrate thickness",
	 RND_HATT_COORD, 0, THMAX, {0, 0, 0, RND_MM_TO_COORD(0.8)}, 0, 0},
#define HA_def_substrate_thick 2

	{"def-copper-cond", "Default copper conductivity",
	 RND_HATT_STRING, 0, 0, {0, "56e6", 0}, 0, 0},
#define HA_def_copper_cond 3

	{"def-subst-epsilon", "Default substrate epsilon",
	 RND_HATT_STRING, 0, 0, {0, "3.66", 0}, 0, 0},
#define HA_def_subst_epsilon 4

	{"def-subst-mue", "Default substrate mue",
	 RND_HATT_STRING, 0, 0, {0, "0", 0}, 0, 0},
#define HA_def_subst_mue 5

	{"def-subst-kappa", "Default substrate kappa",
	 RND_HATT_STRING, 0, 0, {0, "0", 0}, 0, 0},
#define HA_def_subst_kappa 6

	{"def-subst-sigma", "Default substrate sigma",
	 RND_HATT_STRING, 0, 0, {0, "0", 0}, 0, 0},
#define HA_def_subst_sigma 7

	{"void-name", "Name of the void (sorrunding material)",
	 RND_HATT_STRING, 0, 0, {0, "AIR", 0}, 0, 0},
#define HA_void_name 8

	{"void-epsilon", "epsilon value for the void (sorrunding material)",
	 RND_HATT_REAL, 0, 1000, {0, 0, 1}, 0, 0},
#define HA_void_epsilon 9

	{"void-mue", "mue value for the void (sorrunding material)",
	 RND_HATT_REAL, 0, 1000, {0, 0, 1}, 0, 0},
#define HA_void_mue 10

	{"segments", "kludge: number of segments used to approximate round cap trace ends",
	 RND_HATT_INTEGER, 0, 100, {10, 0, 0}, 0, 0},
#define HA_segments 11

	{"base-prio", "base priority: if the board displaces the chassis",
	 RND_HATT_INTEGER, 0, 10, {0, 0, 0}, 0, 0},
#define HA_base_prio 12

	{"port-resistance", "default port resistance",
	 RND_HATT_REAL, 0, 1000, {0, 0, 50}, 0, 0}
#define HA_def_port_res 13

};

#define NUM_OPTIONS (sizeof(openems_attribute_list)/sizeof(openems_attribute_list[0]))

static rnd_hid_attr_val_t openems_values[NUM_OPTIONS];

static rnd_export_opt_t *openems_get_export_options(rnd_hid_t *hid, int *n)
{
	const char *suffix = ".m";
	pcb_mesh_t *mesh = pcb_mesh_get(MESH_NAME);
	char **val = openems_attribute_list[HA_openemsfile].value;

	if ((PCB != NULL) && ((val == NULL) || (*val == NULL) || (**val == '\0')))
		pcb_derive_default_filename(PCB->hidlib.filename, &openems_attribute_list[HA_openemsfile], suffix);

	if (mesh != NULL) {
		openems_attribute_list[HA_def_substrate_thick].default_val.crd = mesh->def_subs_thick;
		openems_attribute_list[HA_def_copper_thick].default_val.crd = mesh->def_copper_thick;
	}

TODO(": when export dialogs change into DAD, this hack to convert the strings to allocated ones will not be needed anymore")
	openems_attribute_list[HA_def_copper_cond].default_val.str = rnd_strdup(openems_attribute_list[HA_def_copper_cond].default_val.str);
	openems_attribute_list[HA_def_subst_epsilon].default_val.str = rnd_strdup(openems_attribute_list[HA_def_subst_epsilon].default_val.str);
	openems_attribute_list[HA_def_subst_mue].default_val.str = rnd_strdup(openems_attribute_list[HA_def_subst_mue].default_val.str);
	openems_attribute_list[HA_def_subst_kappa].default_val.str = rnd_strdup(openems_attribute_list[HA_def_subst_kappa].default_val.str);
	openems_attribute_list[HA_def_subst_sigma].default_val.str = rnd_strdup(openems_attribute_list[HA_def_subst_sigma].default_val.str);
	openems_attribute_list[HA_void_name].default_val.str = rnd_strdup(openems_attribute_list[HA_void_name].default_val.str);

	if (n)
		*n = NUM_OPTIONS;
	return openems_attribute_list;
}

rnd_coord_t ems_layergrp_thickness(pcb_layergrp_t *grp)
{
	return mesh_layergrp_thickness(grp, (grp->ltype & PCB_LYT_COPPER) ? openems_attribute_list[HA_def_copper_thick].default_val.crd : openems_attribute_list[HA_def_substrate_thick].default_val.crd);
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
	pcb_loop_layers(ctx->pcb, ctx, NULL, find_origin_bump, NULL, NULL, NULL, NULL);
}

static void openems_wr_m_tunables(wctx_t *ctx)
{
	fprintf(ctx->f, "%%%%%% User tunables\n");
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "%%%% base_priority and offset: chassis for the board to sit in.\n");
	fprintf(ctx->f, "%% base priority: if the board displaces the model of the chassis or the other way around.\n");
	fprintf(ctx->f, "base_priority=%ld;\n", ctx->options[HA_base_prio].lng);
	fprintf(ctx->f, "\n");
	fprintf(ctx->f, "%% offset on the whole layout to locate it relative to the simulation origin\n");
	rnd_fprintf(ctx->f, "offset.x = %mm;\n", -ctx->ox);
	rnd_fprintf(ctx->f, "offset.y = %mm;\n", ctx->oy);
	fprintf(ctx->f, "offset.z = 0;\n");
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "%% void is the material used for: fill holes, cutouts in substrate, etc\n");
	fprintf(ctx->f, "void.name = '%s';\n", ctx->options[HA_void_name].str);
	fprintf(ctx->f, "void.epsilon = %f;\n", ctx->options[HA_void_epsilon].dbl);
	fprintf(ctx->f, "void.mue = %f;\n", ctx->options[HA_void_mue].dbl);
	fprintf(ctx->f, "%% void.kappa = kappa;\n");
	fprintf(ctx->f, "%% void.sigma = sigma;\n");
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "%% how many points should be used to describe the round end of traces.\n");
	fprintf(ctx->f, "kludge.segments = %ld;\n", ctx->options[HA_segments].lng);
	fprintf(ctx->f, "\n");

	fprintf(ctx->f, "\n");
}

static void print_lparm(wctx_t *ctx, pcb_layergrp_t *grp, const char *attr, int cop_opt, int subs_opt, const char *cop_fmt, const char *subs_fmt)
{
	const char *fmt;
	int opt;

TODO(": this needs layer group attributes in core (planned for lihata v5)")
#if 0
TODO(": try openems::attr first - make a new core call for prefixed get, this will be a real common pattern")
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

	opt = (grp->ltype & PCB_LYT_COPPER) ? cop_opt : subs_opt;
	fmt = (grp->ltype & PCB_LYT_COPPER) ? cop_fmt : subs_fmt;
	assert(opt >= 0);
	if (fmt == NULL)
		rnd_fprintf(ctx->f, "%s", ctx->options[opt].str);
	else
		rnd_fprintf(ctx->f, fmt, ctx->options[opt].crd);
}

static void openems_wr_m_layers(wctx_t *ctx)
{
	rnd_layergrp_id_t gid;
	int next = 1;

	for(gid = 0; gid < PCB_MAX_LAYERGRP; gid++)
		ctx->lg_ems2pcb[gid] = -1;

	fprintf(ctx->f, "%%%%%% Layer mapping\n");

	/* linear map of copper and substrate layers */
	for(gid = 0; gid < ctx->pcb->LayerGroups.len; gid++) {
		pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[gid];
		int iscop = (grp->ltype & PCB_LYT_COPPER);

		if (!(iscop) && !(grp->ltype & PCB_LYT_SUBSTRATE))
			continue;
		ctx->lg_ems2pcb[next] = gid;
		ctx->lg_pcb2ems[gid] = next;

		fprintf(ctx->f, "layers(%d).number = %d;\n", next, next); /* type index really */
		fprintf(ctx->f, "layers(%d).name = '%s';\n", next, (grp->name == NULL ? "anon" : grp->name));
		fprintf(ctx->f, "layers(%d).clearn = 0;\n", next);
		fprintf(ctx->f, "layer_types(%d).name = '%s_%d';\n", next, iscop ? "COPPER" : "SUBSTRATE", next);
		fprintf(ctx->f, "layer_types(%d).subtype = %d;\n", next, iscop ? 2 : 3);

		fprintf(ctx->f, "layer_types(%d).thickness = ", next);
		print_lparm(ctx, grp, "thickness", HA_def_copper_thick, HA_def_substrate_thick, "%.09mm/1000", "%.09mm"); /* the /1000 works around that openems ignores the default mm unit and reads this one value in meter */
		fprintf(ctx->f, ";\n");

		if (iscop) {
			fprintf(ctx->f, "layer_types(%d).conductivity = ", next);
			print_lparm(ctx, grp, "conductivity", HA_def_copper_cond, -1, NULL, NULL);
			fprintf(ctx->f, ";\n");
		}
		else { /* substrate */
			fprintf(ctx->f, "layer_types(%d).epsilon = ", next);
			print_lparm(ctx, grp, "epsilon", -1, HA_def_subst_epsilon, NULL, NULL);
			fprintf(ctx->f, ";\n");

			fprintf(ctx->f, "layer_types(%d).mue = ", next);
			print_lparm(ctx, grp, "mue", -1, HA_def_subst_mue, NULL, NULL);
			fprintf(ctx->f, ";\n");

			fprintf(ctx->f, "layer_types(%d).kappa = ", next);
			print_lparm(ctx, grp, "kappa", -1, HA_def_subst_kappa, NULL, NULL);
			fprintf(ctx->f, ";\n");

			fprintf(ctx->f, "layer_types(%d).sigma = ", next);
			print_lparm(ctx, grp, "sigma", -1, HA_def_subst_sigma, NULL, NULL);
			fprintf(ctx->f, ";\n");

		}

		next++;
		fprintf(ctx->f, "\n");
	}
	fprintf(ctx->f, "\n");

	ctx->lg_next = next;
}

static void openems_wr_m_init(wctx_t *ctx)
{
	fprintf(ctx->f, "%%%%%% Initialize pcb2csx\n");
	fprintf(ctx->f, "PCBRND = InitPCBRND(layers, layer_types, void, base_priority, offset, kludge);\n");
	fprintf(ctx->f, "CSX = InitPcbrndLayers(CSX, PCBRND);\n");
	fprintf(ctx->f, "\n");
}

static void openems_wr_m_outline(wctx_t *ctx)
{
	int n;
	pcb_any_obj_t *out1;

	fprintf(ctx->f, "%%%%%% Board outline\n");

TODO("layer: consider multiple outline layers instead")
	out1 = pcb_topoly_find_1st_outline(ctx->pcb);
	if (out1 != NULL) {
		long n;
		pcb_poly_t *p = pcb_topoly_conn(ctx->pcb, out1, PCB_TOPOLY_KEEP_ORIG | PCB_TOPOLY_FLOATING);
		for(n = 0; n < p->PointN; n++)
			rnd_fprintf(ctx->f, "outline_xy(1, %ld) = %mm; outline_xy(2, %ld) = %mm;\n", n+1, p->Points[n].X, n+1, -p->Points[n].Y);
		pcb_poly_free(p);
	}
	else {
		/* rectangular board size */
		rnd_fprintf(ctx->f, "outline_xy(1, 1) = 0; outline_xy(2, 1) = 0;\n");
		rnd_fprintf(ctx->f, "outline_xy(1, 2) = %mm; outline_xy(2, 2) = 0;\n", ctx->pcb->hidlib.size_x);
		rnd_fprintf(ctx->f, "outline_xy(1, 3) = %mm; outline_xy(2, 3) = %mm;\n", ctx->pcb->hidlib.size_x, -ctx->pcb->hidlib.size_y);
		rnd_fprintf(ctx->f, "outline_xy(1, 4) = 0; outline_xy(2, 4) = %mm;\n", -ctx->pcb->hidlib.size_y);
	}

	/* create all substrate layers using this polygon*/
	for(n = 1; n < ctx->lg_next; n++) {
		pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[ctx->lg_ems2pcb[n]];
		if (grp->ltype & PCB_LYT_SUBSTRATE)
			fprintf(ctx->f, "CSX = AddPcbrndPoly(CSX, PCBRND, %d, outline_xy, 1);\n", n);
	}

	fprintf(ctx->f, "\n");
}

static void openems_wr_m_vport(wctx_t *ctx, pcb_any_obj_t *o, rnd_coord_t x, rnd_coord_t y, rnd_layergrp_id_t gid1, rnd_layergrp_id_t gid2, const char *safe_name, double resistance, int act)
{
	ctx->port_id++;

	rnd_fprintf(ctx->f, "\npoint%s(1, 1) = %mm; point%s(2, 1) = %mm;\n", safe_name, x, safe_name, -y);
	fprintf(ctx->f, "[start%s, stop%s] = CalcPcbrnd2PortV(PCBRND, point%s, %d, %d);\n", safe_name, safe_name, safe_name, ctx->lg_pcb2ems[gid1], ctx->lg_pcb2ems[gid2]);
	fprintf(ctx->f, "[CSX, port{%ld}] = AddLumpedPort(CSX, 999, %ld, %f, start%s, stop%s, [0 0 -1]%s);\n", ctx->port_id, ctx->port_id, resistance, safe_name, safe_name, act ? ", true" : "");
}

static void openems_wr_xml_vport(wctx_t *ctx, pcb_any_obj_t *o, rnd_coord_t x, rnd_coord_t y, rnd_layergrp_id_t gid1, rnd_layergrp_id_t gid2, const char *safe_name, double resistance, int act);

static void openems_vport_write(wctx_t *ctx, pcb_any_obj_t *o, rnd_coord_t x, rnd_coord_t y, rnd_layergrp_id_t gid1, rnd_layergrp_id_t gid2, const char *port_name)
{
	char *end, *s, *safe_name = rnd_strdup(port_name);
	const char *att;
	double resistance = ctx->options[HA_def_port_res].dbl;
	int act = 1;

	att = pcb_attribute_get(&o->Attributes, "openems::resistance");
	if (att != NULL) {
		double tmp = strtod(att, &end);
		if (*end == '\0')
			resistance = tmp;
		else
			rnd_message(RND_MSG_WARNING, "Ignoring invalid openems::resistance value for port %s: '%s' (must be a number without suffix)\n", port_name, att);
	}

	att = pcb_attribute_get(&o->Attributes, "openems::active");
	if (att != NULL) {
		if (rnd_strcasecmp(att, "true") == 0)
			act = 1;
		else if (rnd_strcasecmp(att, "false") == 0)
			act = 0;
		else
			rnd_message(RND_MSG_WARNING, "Ignoring invalid openems::active value for port %s: '%s' (must be true or false)\n", port_name, att);
	}

	for(s = safe_name; *s != '\0'; s++)
		if (!isalnum(*s))
			*s = '_';

	if (ctx->fmt_matlab)
		openems_wr_m_vport(ctx, o, x, y, gid1, gid2, safe_name, resistance, act);
	else
		openems_wr_xml_vport(ctx, o, x, y, gid1, gid2, safe_name, resistance, act);

	free(safe_name);
}

rnd_layergrp_id_t openems_vport_main_group_pstk(pcb_board_t *pcb, pcb_pstk_t *ps, int *gstep, const char *port_name)
{
	int top, bot, intern;
	rnd_layergrp_id_t gid1;

	top = (pcb_pstk_shape(ps, PCB_LYT_COPPER | PCB_LYT_TOP, 0) != NULL);
	bot = (pcb_pstk_shape(ps, PCB_LYT_COPPER | PCB_LYT_BOTTOM, 0) != NULL);
	intern = (pcb_pstk_shape(ps, PCB_LYT_INTERN | PCB_LYT_BOTTOM, 0) != NULL);
	if (intern) {
		rnd_message(RND_MSG_ERROR, "Can not export openems vport %s: it has internal copper\n(must be either top or bottom copper)\n", port_name);
		return -1;
	}
	if (top && bot) {
		rnd_message(RND_MSG_ERROR, "Can not export openems vport %s: it has both top and bottom copper\n", port_name);
		return -1;
	}
	if (!top && !bot) {
		rnd_message(RND_MSG_ERROR, "Can not export openems vport %s: it does not have copper either on top or bottom\n", port_name);
		return -1;
	}

	/* pick main group */
	if (top) {
		gid1 = pcb_layergrp_get_top_copper();
		*gstep = +1;
	}
	else {
		gid1 = pcb_layergrp_get_bottom_copper();
		*gstep = -1;
	}
	if (gid1 < 0) {
		rnd_message(RND_MSG_ERROR, "Can not export openems vport %s: can not find top or bottom layer group ID\n", port_name);
		return -1;
	}

	return gid1;
}

rnd_layergrp_id_t openems_vport_aux_group(pcb_board_t *pcb, rnd_layergrp_id_t gid1, int gstep, const char *port_name)
{
	rnd_layergrp_id_t gid2;

	for(gid2 = gid1 + gstep; (gid2 >= 0) && (gid2 <= pcb->LayerGroups.len); gid2 += gstep)
		if (pcb->LayerGroups.grp[gid2].ltype & PCB_LYT_COPPER)
			return gid2;

	rnd_message(RND_MSG_ERROR, "Can not export openems vport %s: can not find pair layer\n", port_name);
	return -1;
}


#define TPMASK (PCB_OBJ_LINE | PCB_OBJ_PSTK | PCB_OBJ_SUBC)
static void openems_wr_testpoints(wctx_t *ctx, pcb_data_t *data)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;

	for(o = pcb_data_first(&it, data, TPMASK); o != NULL; o = pcb_data_next(&it)) {
		const char *port_name;
		if (o->type == PCB_OBJ_SUBC)
			openems_wr_testpoints(ctx, ((pcb_subc_t *)o)->data);

		port_name = pcb_attribute_get(&o->Attributes, "openems::vport");
		if (port_name == NULL)
			continue;

		if (o->type == PCB_OBJ_SUBC) {
			if (!ctx->warn_subc_term)
				rnd_message(RND_MSG_ERROR, "Subcircuit being a terminal is not supported.\n");
			ctx->warn_subc_term = 1;
			continue;
		}

		/* place the vertical port */
		switch(o->type) {
			case PCB_OBJ_PSTK:
				{
					int gstep;
					rnd_layergrp_id_t gid1, gid2;
					pcb_pstk_t *ps = (pcb_pstk_t *)o;

					if (port_name == NULL)
						break;

					gid1 = openems_vport_main_group_pstk(ctx->pcb, ps, &gstep, port_name);
					if (gid1 < 0)
						break;

					gid2 = openems_vport_aux_group(ctx->pcb, gid1, gstep, port_name);
					if (gid2 < 0)
						break;

TODO(": check if there is copper object on hid2 at x;y")

					if (pcb_attribute_get(&o->Attributes, "openems::vport-reverse") == NULL)
						openems_vport_write(ctx, (pcb_any_obj_t *)ps, ps->x, ps->y, gid1, gid2, port_name);
					else
						openems_vport_write(ctx, (pcb_any_obj_t *)ps, ps->x, ps->y, gid2, gid1, port_name);
				}
				break;
				default:
					if (!ctx->warn_port_pstk)
						rnd_message(RND_MSG_ERROR, "Only padstacks can be openems ports at the moment\n");
					ctx->warn_port_pstk = 1;
					break;
		}
	}
}

static void openems_wr_m_mesh_lines(wctx_t *ctx, pcb_mesh_lines_t *l)
{
	rnd_cardinal_t n;
	for(n = 0; n < vtc0_len(&l->result); n++)
		rnd_fprintf(ctx->f, "%s%mm", (n == 0 ? "" : " "), l->result.array[n]);
}

static void openems_wr_m_mesh1(wctx_t *ctx)
{
	pcb_mesh_t *mesh = pcb_mesh_get(MESH_NAME);
	char *exc = pcb_openems_excitation_get(ctx->pcb, 1);
	int n;

	fprintf(ctx->fsim, "%%%%%% Board mesh, part 1\n");
	fprintf(ctx->fsim, "unit = 1.0e-3;\n");
	fprintf(ctx->fsim, "FDTD = InitFDTD();\n");
	fprintf(ctx->fsim, "%% Excitation begin\n");
	fprintf(ctx->fsim, "%s\n", exc);
	fprintf(ctx->fsim, "%% Excitation end\n");
	free(exc);

	if (mesh != NULL) {

		fprintf(ctx->fsim, "BC = {");

		for(n = 0; n < 6; n++)
			fprintf(ctx->fsim, "%s'%s'", (n == 0 ? "" : " "), mesh->bnd[n]);
		fprintf(ctx->fsim, "};\n");

		fprintf(ctx->fsim, "FDTD = SetBoundaryCond(FDTD, BC);\n");
	}
	fprintf(ctx->fsim, "physical_constants;\n");
	fprintf(ctx->fsim, "CSX = InitCSX();\n");
	fprintf(ctx->fsim, "\n");
}

static void openems_wr_m_mesh2(wctx_t *ctx)
{
	pcb_mesh_t *mesh = pcb_mesh_get(MESH_NAME);

	if (mesh == NULL) {
		fprintf(ctx->f, "%%%%%% Board mesh (NOT defined in pcb-rnd)\n");
		return;
	}
	fprintf(ctx->f, "%%%%%% Board mesh, part 2\n");

	rnd_fprintf(ctx->f, "z_bottom_copper=%mm\n", mesh->z_bottom_copper);

	fprintf(ctx->f, "mesh.y=[");
	openems_wr_m_mesh_lines(ctx, &mesh->line[PCB_MESH_HORIZONTAL]);
	fprintf(ctx->f, "];\n");

	fprintf(ctx->f, "mesh.x=[");
	openems_wr_m_mesh_lines(ctx, &mesh->line[PCB_MESH_VERTICAL]);
	fprintf(ctx->f, "];\n");

	fprintf(ctx->f, "mesh.z=[");
	openems_wr_m_mesh_lines(ctx, &mesh->line[PCB_MESH_Z]);
	fprintf(ctx->f, "];\n");

	fprintf(ctx->f, "mesh.x = mesh.x .+ offset.x;\n");
	fprintf(ctx->f, "mesh.y = offset.y .- mesh.y;\n");
	fprintf(ctx->f, "mesh.z = z_bottom_copper .- mesh.z .+ offset.z;\n");
	if (mesh->pml > 0)
		fprintf(ctx->f, "mesh = AddPML(mesh, %d);\n", mesh->pml);
	fprintf(ctx->f, "CSX = DefineRectGrid(CSX, unit, mesh);\n");
	fprintf(ctx->f, "\n");
}

static void openems_wr_m_sim(wctx_t *wctx)
{
	openems_wr_m_mesh1(wctx);

	fprintf(wctx->fsim, "run %s\n\n", wctx->filename);

	fprintf(wctx->fsim, "Sim_Path = '.';\n");
	fprintf(wctx->fsim, "Sim_CSX = 'csxcad.xml';\n");
	fprintf(wctx->fsim, "WriteOpenEMS( [Sim_Path '/' Sim_CSX], FDTD, CSX );\n");
}

#include "openems_xml.c"

static void openems_hid_export_to_file(const char *filename, FILE *the_file, FILE *fsim, rnd_hid_attr_val_t *options, int fmt_matlab)
{
	rnd_hid_expose_ctx_t ctx;
	wctx_t wctx;

	memset(&wctx, 0, sizeof(wctx));
	wctx.filename = filename;
	wctx.f = the_file;
	wctx.fsim = fsim;
	wctx.pcb = PCB;
	wctx.options = options;
	wctx.fmt_matlab = fmt_matlab;
	ems_ctx = &wctx;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->hidlib.size_x;
	ctx.view.Y2 = PCB->hidlib.size_y;

	f = the_file;

	rnd_conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	find_origin(&wctx);

	if (fmt_matlab) {
		openems_wr_m_sim(&wctx);
		openems_wr_m_tunables(&wctx);
		openems_wr_m_mesh2(&wctx);
		openems_wr_m_layers(&wctx);
		openems_wr_m_init(&wctx);
		openems_wr_m_outline(&wctx);

		fprintf(wctx.f, "%%%%%% Copper objects\n");
		rnd_expose_main(&openems_hid, &ctx, NULL);

		fprintf(wctx.f, "%%%%%% Port(s) on terminals\n");
		openems_wr_testpoints(&wctx, wctx.pcb->Data);
	}
	else { /* xml */
		if (openems_wr_xml(&wctx) != 0)
			rnd_message(RND_MSG_ERROR, "openEMS: Due to errors, the exported file is invalid.\n");
	}

	rnd_conf_update(NULL, -1); /* restore forced sets */
}

static void openems_do_export(rnd_hid_t *hid, rnd_hid_attr_val_t *options)
{
	const char *filename;
	char *runfn = NULL, *end;
	int save_ons[PCB_MAX_LAYER];
	int len, fmt_matlab;
	FILE *fsim;

	openems_ovr = 0;

	if (!options) {
		openems_get_export_options(hid, 0);
		options = openems_values;
	}

	filename = options[HA_openemsfile].str;
	if (!filename)
		filename = "pcb.m";

	f = rnd_fopen_askovr(&PCB->hidlib, filename, "wb", &openems_ovr);
	if (!f) {
		perror(filename);
		return;
	}

	end = strrchr(filename, '.');
	fmt_matlab = !((end != NULL) && (rnd_strcasecmp(end, ".xml") == 0));

	if (fmt_matlab) {
		/* create the run.m file */
		len = strlen(filename);
		runfn = malloc(len+16);
		memcpy(runfn, filename, len+1);
		end = runfn + len - 2;
		if (strcmp(end, ".m") != 0)
			end = runfn + len;
		strcpy(end, ".sim.m");

		fsim = rnd_fopen_askovr(&PCB->hidlib, runfn, "wb", &openems_ovr);
		if (fsim == NULL) {
			perror(runfn);
			return;
		}
	}
	else { /* xml */
		fsim = NULL;
	}

	pcb_hid_save_and_show_layer_ons(save_ons);

	openems_hid_export_to_file(filename, f, fsim, options, fmt_matlab);

	pcb_hid_restore_layer_ons(save_ons);

	fclose(f);
	if (fsim != NULL)
		fclose(fsim);
	f = NULL;
	free(runfn);
}

static int openems_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, openems_attribute_list, sizeof(openems_attribute_list) / sizeof(openems_attribute_list[0]), openems_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int openems_set_layer_group(rnd_hid_t *hid, rnd_layergrp_id_t group, const char *purpose, int purpi, rnd_layer_id_t layer, unsigned int flags, int is_empty, rnd_xform_t **xform)
{
	if (flags & PCB_LYT_COPPER) { /* export copper layers only */
		ems_ctx->clayer = ems_ctx->lg_pcb2ems[group];
		if ((!ems_ctx->fmt_matlab) && (!is_empty))
			if (openems_wr_xml_layergrp_begin(ems_ctx, &ems_ctx->pcb->LayerGroups.grp[group]) != 0)
				return 0;
		return 1;
	}
	return 0;
}

static rnd_hid_gc_t openems_make_gc(rnd_hid_t *hid)
{
	rnd_hid_gc_t rv = (rnd_hid_gc_t) calloc(sizeof(rnd_hid_gc_s), 1);
	rv->me_pointer = &openems_hid;
	return rv;
}

static void openems_destroy_gc(rnd_hid_gc_t gc)
{
	free(gc);
}

static void openems_set_drawing_mode(rnd_hid_t *hid, rnd_composite_op_t op, rnd_bool direct, const rnd_box_t *screen)
{
	switch(op) {
		case RND_HID_COMP_RESET:
			break;
		case RND_HID_COMP_POSITIVE:
		case RND_HID_COMP_POSITIVE_XOR:
			break;
		case RND_HID_COMP_NEGATIVE:
			rnd_message(RND_MSG_ERROR, "Can't draw composite layer, especially not on copper\n");
			break;
		case RND_HID_COMP_FLUSH:
			break;
	}
}

static void openems_set_color(rnd_hid_gc_t gc, const rnd_color_t *name)
{
}

static void openems_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	gc->cap = style;
}

static void openems_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	gc->width = width;
}


static void openems_set_draw_xor(rnd_hid_gc_t gc, int xor_)
{
	;
}

static void openems_fill_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void openems_draw_rect(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
}

static void openems_draw_arc(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t width, rnd_coord_t height, rnd_angle_t start_angle, rnd_angle_t delta_angle)
{
}

static void openems_fill_circle(rnd_hid_gc_t gc, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t radius)
{
	wctx_t *ctx = ems_ctx;
	long oid = ctx->oid++;

	if (ctx->fmt_matlab) {
		rnd_fprintf(ctx->f, "points%ld(1, 1) = %mm; points%ld(2, 1) = %mm;\n", oid, cx, oid, -cy);
		rnd_fprintf(ctx->f, "points%ld(1, 2) = %mm; points%ld(2, 2) = %mm;\n", oid, cx, oid, -cy);
		rnd_fprintf(ctx->f, "CSX = AddPcbrndTrace(CSX, PCBRND, %d, points%ld, %mm, 0);\n", ctx->clayer, oid, radius*2);
	}
	else {
		double a, step, x = RND_COORD_TO_MM(cx), y = -RND_COORD_TO_MM(cy), r = RND_COORD_TO_MM(radius);
		step = r*10;
		if (step < 8) step = 8;
		step = 2*M_PI/step;

		rnd_fprintf(ctx->f, "          <Polygon Priority='%d' CoordSystem='0' Elevation='%f' NormDir='2' QtyVertices='%.0f'>\n", PRIO_COPPER, ctx->elevation, floor(2*M_PI/step));
		for(a = 0; a < 2*M_PI; a += step)
			rnd_fprintf(ctx->f, "            <Vertex X1='%f' X2='%f'/>\n", x + cos(a)*r, y + sin(a)*r);
		rnd_fprintf(ctx->f, "          </Polygon>\n");
	}
}

static void openems_fill_polygon_offs(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy)
{
	wctx_t *ctx = ems_ctx;
	int n;

	if (ctx->fmt_matlab) {
		long oid = ctx->oid++;

		for(n = 0; n < n_coords; n++)
			rnd_fprintf(ctx->f, "poly%ld_xy(1, %ld) = %mm; poly%ld_xy(2, %ld) = %mm;\n", oid, n+1, x[n]+dx, oid, n+1, -(y[n]+dy));

		fprintf(ctx->f, "CSX = AddPcbrndPoly(CSX, PCBRND, %d, poly%ld_xy, 1);\n", ctx->clayer, oid);
	}
	else {
		rnd_fprintf(ctx->f, "          <Polygon Priority='%d' CoordSystem='0' Elevation='%f' NormDir='2' QtyVertices='%d'>\n", PRIO_COPPER, ctx->elevation, n_coords);
		for(n = 0; n < n_coords; n++)
			rnd_fprintf(ctx->f, "            <Vertex X1='%f' X2='%f'/>\n", RND_COORD_TO_MM(x[n]+dx), RND_COORD_TO_MM(-(y[n]+dy)));
		rnd_fprintf(ctx->f, "          </Polygon>\n");
	}
}

static void openems_fill_polygon(rnd_hid_gc_t gc, int n_coords, rnd_coord_t *x, rnd_coord_t *y)
{
	openems_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void openems_draw_line_body(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	rnd_coord_t x[4], y[4];
	pcb_line_t tmp = {0};
	tmp.Point1.X = x1;
	tmp.Point1.Y = y1;
	tmp.Point2.X = x2;
	tmp.Point2.Y = y2;
	tmp.Thickness = gc->width;
	pcb_sqline_to_rect(&tmp, x, y);
	openems_fill_polygon_offs(gc, 4, x, y, 0, 0);

}

static void openems_draw_line(rnd_hid_gc_t gc, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	wctx_t *ctx = ems_ctx;

	if (gc->cap == rnd_cap_square) {
		openems_draw_line_body(gc, x1, y1, x2, y2);
		return;
	}

	if (ctx->fmt_matlab) {
		long oid = ctx->oid++;
		rnd_fprintf(ctx->f, "points%ld(1, 1) = %mm; points%ld(2, 1) = %mm;\n", oid, x1, oid, -y1);
		rnd_fprintf(ctx->f, "points%ld(1, 2) = %mm; points%ld(2, 2) = %mm;\n", oid, x2, oid, -y2);
		rnd_fprintf(ctx->f, "CSX = AddPcbrndTrace(CSX, PCBRND, %d, points%ld, %mm, 0);\n", ctx->clayer, oid, gc->width);
	}
	else {
		openems_fill_circle(gc, x1, y1, gc->width/2);
		openems_fill_circle(gc, x2, y2, gc->width/2);
		openems_draw_line_body(gc, x1, y1, x2, y2);
	}
}


static void openems_calibrate(rnd_hid_t *hid, double xval, double yval)
{
	rnd_message(RND_MSG_ERROR, "openems_calibrate() not implemented");
	return;
}

static void openems_set_crosshair(rnd_hid_t *hid, rnd_coord_t x, rnd_coord_t y, int a)
{
}

static int openems_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nopenems exporter command line arguments:\n\n");
	rnd_hid_usage(openems_attribute_list, sizeof(openems_attribute_list) / sizeof(openems_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x openems [openems options] foo.pcb\n\n");
	return 0;
}

static rnd_action_t openems_action_list[] = {
	{"mesh", pcb_act_mesh, pcb_acth_mesh, pcb_acts_mesh},
	{"OpenemsExcitation", pcb_act_OpenemsExcitation, pcb_acth_OpenemsExcitation, pcb_acts_OpenemsExcitation}
};

int pplg_check_ver_export_openems(int ver_needed) { return 0; }

void pplg_uninit_export_openems(void)
{
	pcb_openems_excitation_uninit();
	rnd_remove_actions_by_cookie(openems_cookie);
	rnd_export_remove_opts_by_cookie(openems_cookie);
	rnd_hid_remove_hid(&openems_hid);
}

int pplg_init_export_openems(void)
{
	RND_API_CHK_VER;

	memset(&openems_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&openems_hid);

	openems_hid.struct_size = sizeof(rnd_hid_t);
	openems_hid.name = "openems";
	openems_hid.description = "OpenEMS exporter";
	openems_hid.exporter = 1;

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
	openems_hid.argument_array = openems_values;

	openems_hid.usage = openems_usage;

	rnd_hid_register_hid(&openems_hid);

	RND_REGISTER_ACTIONS(openems_action_list, openems_cookie);

	pcb_openems_excitation_init();

	return 0;
}
