/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - std format read: high level tree parsing
 *  pcb-rnd Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 Entrust Fund in 2024)
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
#include <librnd/core/compat_misc.h>
#include <librnd/core/error.h>
#include "board.h"
#include "layer.h"
#include "layer_grp.h"

#include "rnd_inclib/lib_svgpath/svgpath.h"
#include <libnanojson/nanojson.c>
#include <libnanojson/semantic.c>

#include <rnd_inclib/lib_easyeda/gendom.c>
#include <rnd_inclib/lib_easyeda/gendom_json.c>

#include "io_easyeda_conf.h"

#include "read_common.h"

/* EasyEDA std has a static layer assignment, layers identified by their
   integer ID, not by their name and there's no layer type saved. */
pcb_layer_type_t easyeda_layer_id2type[200] = {
/*1~TopLayer*/               PCB_LYT_TOP | PCB_LYT_COPPER,
/*2~BottomLayer*/            PCB_LYT_BOTTOM | PCB_LYT_COPPER,
/*3~TopSilkLayer*/           PCB_LYT_TOP | PCB_LYT_SILK,
/*4~BottomSilkLayer*/        PCB_LYT_BOTTOM | PCB_LYT_SILK,
/*5~TopPasteMaskLayer*/      PCB_LYT_TOP | PCB_LYT_PASTE,
/*6~BottomPasteMaskLayer*/   PCB_LYT_BOTTOM | PCB_LYT_PASTE,
/*7~TopSolderMaskLayer*/     PCB_LYT_TOP | PCB_LYT_MASK,
/*8~BottomSolderMaskLayer*/  PCB_LYT_BOTTOM | PCB_LYT_MASK,
/*9~Ratlines*/               0,
/*10~BoardOutLine*/          PCB_LYT_BOUNDARY,
/*11~Multi-Layer*/           0,
/*12~Document*/              PCB_LYT_DOC,
/*13~TopAssembly*/           PCB_LYT_TOP | PCB_LYT_DOC,
/*14~BottomAssembly*/        PCB_LYT_BOTTOM | PCB_LYT_DOC,
/*15~Mechanical*/            PCB_LYT_MECH,
/*19~3DModel*/               0,
/*21~Inner1*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*22~Inner2*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*23~Inner3*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*24~Inner4*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*25~Inner5*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*26~Inner6*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*27~Inner7*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*28~Inner8*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*29~Inner9*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*30~Inner10*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*31~Inner11*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*32~Inner12*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*33~Inner13*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*34~Inner14*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*35~Inner15*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*36~Inner16*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*37~Inner17*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*38~Inner18*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*39~Inner19*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*40~Inner20*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*41~Inner21*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*42~Inner22*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*43~Inner23*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*44~Inner24*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*45~Inner25*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*46~Inner26*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*47~Inner27*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*48~Inner28*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*49~Inner29*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*50~Inner30*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*51~Inner31*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*52~Inner32*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
0
};

int easyeda_layer_id2type_size = sizeof(easyeda_layer_id2type) / sizeof(easyeda_layer_id2type[0]);

/* load layers in a specific order so the pcb-rnd layer stack looks normal;
   these numbers are base-1 to match the layer ID in comments above */
const int easyeda_layertab[] = {5, 3, 7, 1, LAYERTAB_INNER, 2, 8, 4, 6, 10, 12, 13, 14, 15, 99, 100, 101   , 0};
const int easyeda_layertab_in_first = 21;
const int easyeda_layertab_in_last = 52;

int easyeda_create_misc_layers(std_read_ctx_t *ctx)
{
	pcb_layer_t *ly[8];
	pcb_layergrp_t *grp[8];
	rnd_layer_id_t lid;
	const char **name, *names[] = {"slot-plated", "slot-unplated", NULL};
	int n;

	if (ctx->pcb == NULL)
		return 0;

	for(n = 0, name = names; *name != NULL; n++, name++) {
		grp[n] = pcb_get_grp_new_raw(ctx->pcb, 0);
		grp[n]->name = rnd_strdup(*name);

		lid = pcb_layer_create(ctx->pcb, grp[n] - ctx->pcb->LayerGroups.grp, rnd_strdup(*name), 0);
		ly[n] = pcb_get_layer(ctx->pcb->Data, lid);
	}

	grp[0]->ltype = PCB_LYT_MECH;
	ly[0]->comb |= PCB_LYC_AUTO;
	pcb_layergrp_set_purpose__(grp[0], rnd_strdup("proute"), 0);
	grp[1]->ltype = PCB_LYT_MECH;
	ly[1]->comb |= PCB_LYC_AUTO;
	pcb_layergrp_set_purpose__(grp[1], rnd_strdup("uroute"), 0);

	return 0;
}

/*** drawing object helper: svgpath ***/

static svgpath_cfg_t pathcfg;
typedef struct {
	std_read_ctx_t *ctx;
	pcb_layer_t *layer;
	rnd_coord_t thickness;
	pcb_poly_t *in_poly;
	gdom_node_t *nd;

} path_ctx_t;

static void easyeda_mkpath_line(void *uctx, double x1, double y1, double x2, double y2)
{
	path_ctx_t *pctx = uctx;
	std_read_ctx_t *ctx = pctx->ctx;

	if (pctx->in_poly != NULL) {
		rnd_point_t *pt = pcb_poly_point_alloc(pctx->in_poly);
		pt->X = TRX(x2);
		pt->Y = TRY(y2);
	}
	else {
		pcb_line_t *line = pcb_line_alloc(pctx->layer);

		line->Point1.X = TRX(x1);
		line->Point1.Y = TRY(y1);
		line->Point2.X = TRX(x2);
		line->Point2.Y = TRY(y2);
		line->Thickness = pctx->thickness;
		line->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
		line->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

		pcb_add_line_on_layer(pctx->layer, line);
	}
}

static void easyeda_mkpath_carc(void *uctx, double cx, double cy, double r, double sa, double da)
{
	path_ctx_t *pctx = uctx;
	std_read_ctx_t *ctx = pctx->ctx;
	pcb_arc_t *arc;

	/* this is not called for polygons, we have line approximation there */

	arc = pcb_arc_alloc(pctx->layer);

	arc->X = TRX(cx);
	arc->Y = TRY(cy);
	arc->Width = arc->Height = TRR(r);
	arc->Thickness = pctx->thickness;
	arc->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
	arc->StartAngle = -sa * RND_RAD_TO_DEG + 180.0;
	arc->Delta = -da * RND_RAD_TO_DEG;
	arc->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

	pcb_add_arc_on_layer(pctx->layer, arc);
}

static void easyeda_mkpath_error(void *uctx, const char *errmsg, long offs)
{
/*	path_ctx_t *pctx = uctx;*/
	rnd_message(RND_MSG_ERROR, "easyeda svg-path: '%s' at offset %ld\n", errmsg, offs);
}

static void easyeda_svgpath_setup(void)
{
	if (pathcfg.line == NULL) {
		pathcfg.line = easyeda_mkpath_line;
		pathcfg.error = easyeda_mkpath_error;
		pathcfg.curve_approx_seglen = conf_io_easyeda.plugins.io_easyeda.line_approx_seg_len;
	}
}

/* Create an (svg)path as a line approximation within parent */
int easyeda_parse_path(std_read_ctx_t *ctx, const char *pathstr, gdom_node_t *nd, pcb_layer_t *layer, rnd_coord_t thickness, pcb_poly_t *in_poly)
{
	path_ctx_t pctx;

	easyeda_svgpath_setup();

	pctx.ctx = ctx;
	pctx.layer = layer;
	pctx.thickness = thickness;
	pctx.nd = nd;
	pctx.in_poly = in_poly;

	/* turn off arcs in polygon, polygons have lines only for now */
	if (in_poly != NULL)
		pathcfg.carc = NULL;
	else
		pathcfg.carc = easyeda_mkpath_carc;

	return svgpath_render(&pathcfg, &pctx, pathstr);
}

double easyeda_get_double(std_read_ctx_t *ctx, gdom_node_t *nd)
{
	if (nd == NULL) {
		rnd_message(RND_MSG_ERROR, "Missing data (double)\n");
		return 0;
	}
	if (nd->type != GDOM_DOUBLE) {
		error_at(ctx, nd, ("Expected data type: double\n"));
		return 0;
	}
	return nd->value.dbl;
}

