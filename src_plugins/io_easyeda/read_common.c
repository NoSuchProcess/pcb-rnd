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
#include "data.h"
#include "layer.h"
#include "layer_grp.h"

#include "rnd_inclib/lib_svgpath/svgpath.h"
#include <libnanojson/nanojson.c>
#include <libnanojson/semantic.c>

#include <rnd_inclib/lib_easyeda/gendom.c>
#include <rnd_inclib/lib_easyeda/gendom_json.c>

#include "io_easyeda_conf.h"

#include "read_common.h"

int easyeda_create_misc_layers(easy_read_ctx_t *ctx)
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
	easy_read_ctx_t *ctx;
	pcb_layer_t *layer;
	rnd_coord_t thickness;
	pcb_poly_t *in_poly;
	gdom_node_t *nd;

} path_ctx_t;

static void easyeda_mkpath_line(void *uctx, double x1, double y1, double x2, double y2)
{
	path_ctx_t *pctx = uctx;
	easy_read_ctx_t *ctx = pctx->ctx;

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
	easy_read_ctx_t *ctx = pctx->ctx;
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
int easyeda_parse_path(easy_read_ctx_t *ctx, const char *pathstr, gdom_node_t *nd, pcb_layer_t *layer, rnd_coord_t thickness, pcb_poly_t *in_poly)
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

double easyeda_get_double(easy_read_ctx_t *ctx, gdom_node_t *nd)
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

int easyeda_layer_create(easy_read_ctx_t *ctx, unsigned ltype, const char *name, int easyeda_id, const char *clr)
{
	pcb_layer_t *dst;
	pcb_layergrp_t *grp;
	rnd_layer_id_t lid;
	int load_clr;

	if (ctx->pcb != NULL) {
		/* create real board layer */
		grp = pcb_get_grp_new_raw(ctx->pcb, 0);
		grp->name = rnd_strdup(name);
		grp->ltype = ltype;
		lid = pcb_layer_create(ctx->pcb, grp - ctx->pcb->LayerGroups.grp, rnd_strdup(name), 0);

		dst = pcb_get_layer(ctx->pcb->Data, lid);
	}
	else {
		/* create pure bound layer */
		lid = ctx->data->LayerN;
		ctx->data->LayerN++;
		dst = &ctx->data->Layer[lid];
		memset(dst, 0, sizeof(pcb_layer_t));
		dst->name = rnd_strdup(name);
		dst->is_bound = 1;
		dst->meta.bound.type = ltype;
		dst->parent_type = PCB_PARENT_DATA;
		dst->parent.data = ctx->data;
		if (ltype & PCB_LYT_INTERN)
			dst->meta.bound.stack_offs = easyeda_id - easyeda_layertab_in_first + 1;
	}

	if (ltype & (PCB_LYT_SILK | PCB_LYT_MASK | PCB_LYT_PASTE))
		dst->comb |= PCB_LYC_AUTO;
	if (ltype & PCB_LYT_MASK)
		dst->comb |= PCB_LYC_SUB;


	if ((easyeda_id >= 0) && (easyeda_id < EASY_MAX_LAYERS))
		ctx->layers[easyeda_id] = dst;

	load_clr = (ltype & PCB_LYT_COPPER) ? conf_io_easyeda.plugins.io_easyeda.load_color_copper : conf_io_easyeda.plugins.io_easyeda.load_color_noncopper;
	if ((ctx->pcb != NULL) && load_clr && (clr != NULL))
		rnd_color_load_str(&dst->meta.real.color, clr);

	return 0;
}

pcb_subc_t *easyeda_subc_create(easy_read_ctx_t *ctx)
{
	pcb_subc_t *subc = pcb_subc_alloc();
	long n;

	pcb_subc_reg(ctx->data, subc);
	pcb_obj_id_reg(ctx->data, subc);
	for(n = 0; n < ctx->data->LayerN; n++) {
		pcb_layer_t *ly = pcb_subc_alloc_layer_like(subc, &ctx->data->Layer[n]);
		if (ctx->pcb == NULL)
			ly->meta.bound.real = &ctx->data->Layer[n];
	}

	if (ctx->pcb != NULL) {
		pcb_subc_rebind(ctx->pcb, subc);
		pcb_subc_bind_globals(ctx->pcb, subc);
	}

	ctx->last_refdes = NULL;

	return subc;
}

unsigned int easyeda_layer_flags(const pcb_layer_t *layer)
{
	unsigned int res = pcb_layer_flags_(layer);

	if (res != 0)
		return res;

	assert(layer->is_bound);
	return layer->meta.bound.type;
}

void easyeda_subc_finalize(easy_read_ctx_t *ctx, pcb_subc_t *subc, rnd_coord_t x, rnd_coord_t y, double rot)
{
	int on_bottom = 0;

	if (ctx->last_refdes != NULL) {
		int side = easyeda_layer_flags(ctx->last_refdes->parent.layer) & PCB_LYT_ANYWHERE;
		if (side & PCB_LYT_BOTTOM)
			on_bottom = 1;
	}

	pcb_subc_create_aux(subc, x, y, -rot, on_bottom);

	pcb_data_bbox(&subc->BoundingBox, subc->data, rnd_true);
	pcb_data_bbox_naked(&subc->bbox_naked, subc->data, rnd_true);

	if (ctx->data->subc_tree == NULL)
		rnd_rtree_init(ctx->data->subc_tree = malloc(sizeof(rnd_rtree_t)));
	rnd_rtree_insert(ctx->data->subc_tree, subc, (rnd_rtree_box_t *)subc);
}

int easyeda_eat_bom(FILE *f, const char *fn)
{
	unsigned char ul[3];

	if (fread(ul, 1, 3, f) != 3) {
		rnd_message(RND_MSG_ERROR, "easyeda pro: premature EOF on %s (bom)\n", fn);
		return -1;
	}
	if ((ul[0] != 0xef) || (ul[1] != 0xbb) || (ul[2] != 0xbf))
		rewind(f);
	return 0;
}

void easyeda_data_layer_reset(pcb_board_t **pcb, pcb_data_t *data)
{
	long n;

	for(n = 0; n < data->LayerN; n++) {
		pcb_layer_t *rl = data->Layer[n].meta.bound.real;

		if ((*pcb == NULL) && (rl != NULL)) { /* resolve target pcb from a real layer */
			pcb_data_t *rd = rl->parent.data;
			(*pcb) = rd->parent.board;
		}
		pcb_layer_free_fields(&data->Layer[n], 0);
	}
	data->LayerN = 0;
}

void easyeda_subc_layer_bind(easy_read_ctx_t *ctx, pcb_subc_t *subc)
{
	long n;

	for(n = 0; n < subc->data->LayerN; n++) {
		int i, idx = 0;
		for(i = 0; i < subc->data->LayerN; i++) {
			pcb_layer_t *cl = ctx->layers[n];
			if ((cl != NULL) && (cl->meta.bound.type == subc->data->Layer[i].meta.bound.type)) {
				idx = i;
				break;
			}
		}
		ctx->layers[n] = &subc->data->Layer[idx];
	}
}

void easyeda_read_common_init(void)
{
	easystd_layer_id2type[99-1] = PCB_LYT_DOC;
	easystd_layer_id2type[100-1] = PCB_LYT_DOC;
	easystd_layer_id2type[101-1] = PCB_LYT_DOC;
}

#define HT(x) htsc_ ## x
#include <genht/ht.c>
#undef HT
