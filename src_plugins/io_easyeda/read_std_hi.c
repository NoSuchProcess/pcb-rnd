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

/* Return the first node that has valid location info (or fall back to root),
   starting from node, traversing up in the tree */
static gdom_node_t *node_parent_with_loc(gdom_node_t *node)
{
	for(;node->parent != NULL; node = node->parent)
		if (node->lineno > 0)
			return node;

	return node;
}

#define EASY_MAX_LAYERS 64

/* raw coord transform (e.g. for radius, diameter, width) */
#define TRR(c)   RND_MIL_TO_COORD((c) * 10.0)
#define TRX(c)   TRR((c) - ctx->ox)
#define TRY(c)   TRR((c) - ctx->oy)

typedef struct std_read_ctx_s {
	FILE *f;
	gdom_node_t *root;
	pcb_board_t *pcb;
	pcb_data_t *data;
	const char *fn;
	rnd_conf_role_t settings_dest;
	pcb_layer_t *layers[EASY_MAX_LAYERS];
	double ox, oy;
} std_read_ctx_t;

#define error_at(ctx, node, args) \
	do { \
		gdom_node_t *__loc__ = node_parent_with_loc(node); \
		rnd_message(RND_MSG_ERROR, "easyeda parse error at %s:%ld.%ld\n", ctx->fn, __loc__->lineno, __loc__->col); \
		rnd_msg_error args; \
	} while(0)

#define warn_at(ctx, node, args) \
	do { \
		gdom_node_t *__loc__ = node_parent_with_loc(node); \
		rnd_message(RND_MSG_WARNING, "easyeda parse warning at %s:%ld.%ld\n", ctx->fn, __loc__->lineno, __loc__->col); \
		rnd_msg_error args; \
	} while(0)

static double easyeda_get_double(std_read_ctx_t *ctx, gdom_node_t *nd)
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

/* Look up (long) lname within (gdom_nod_t *)nd and load the result in dst.
   Require dst->type to be typ. Invoke err_stmt when anything fails and
   print an error message */
#define HASH_GET_SUBTREE(dst, nd, lname, typ, err_stmt) \
do { \
	dst = gdom_hash_get(nd, lname); \
	if (dst == NULL) { \
		error_at(ctx, nd, ("internal: fieled to find " #lname " within %s\n", easy_keyname(nd->name))); \
		err_stmt; \
	} \
	if (dst->type != typ) { \
		error_at(ctx, dst, ("internal: " #lname " in %s must be of type " #typ "\n", easy_keyname(nd->name))); \
		err_stmt; \
	} \
} while(0)

/* Look up (long) lname within (gdom_nod_t *)nd, expect a double or long there
   and load its value into dst.
   Invoke err_stmt when anything fails and print an error message */
#define HASH_GET_DOUBLE(dst, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_DOUBLE, err_stmt); \
	dst = tmp->value.dbl; \
} while(0)

#define HASH_GET_LONG(dst, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_LONG, err_stmt); \
	dst = tmp->value.lng; \
} while(0)

#define HASH_GET_STRING(dst, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_STRING, err_stmt); \
	dst = tmp->value.str; \
} while(0)

#define HASH_GET_LAYER(dst, nd, lname, err_stmt) \
do { \
	gdom_node_t *tmp; \
	HASH_GET_SUBTREE(tmp, nd, lname, GDOM_LONG, err_stmt); \
	if ((tmp->value.lng < 0) || (tmp->value.lng >= EASY_MAX_LAYERS)) { \
		error_at(ctx, nd, ("layer ID %ld is out of range [0..%d]\n", tmp->value.lng, EASY_MAX_LAYERS-1)); \
		err_stmt; \
	} \
	dst = ctx->layers[tmp->value.lng]; \
	if (dst == NULL) { \
		error_at(ctx, nd, ("layer ID %ld does not exist\n", tmp->value.lng)); \
		err_stmt; \
	} \
} while(0)



/* EasyEDA std has a static layer assignment, layers identified by their
   integer ID, not by their name and there's no layer type saved. */
static const pcb_layer_type_t std_layer_id2type[] = {
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
/*52~Inner32*/               PCB_LYT_INTERN | PCB_LYT_COPPER
};

/* load layers in a specific order so the pcb-rnd layer stack looks normal;
   these numbers are base-1 to match the layer ID in comments above */
#define LAYERTAB_INNER -1
static const int layertab[] = {5, 3, 7, 1, LAYERTAB_INNER, 2, 8, 4, 6, 10, 12, 13, 14, 15, 0};
static const int layertab_in_first = 21;
static const int layertab_in_last = 52;

/*** board meta ***/

static int std_parse_layer_(std_read_ctx_t *ctx, gdom_node_t *src, long idx, int easyeda_id)
{
	pcb_layer_t *dst;
	const char *config, *name, *clr;
	pcb_layergrp_t *grp;
	rnd_layer_id_t lid;
	int load_clr;

	if (idx > sizeof(std_layer_id2type) / sizeof(std_layer_id2type[0]))
		return 0; /* ignore layers not in the table */

	if (std_layer_id2type[idx] == 0)
		return 0; /* table dictated skip */

	HASH_GET_STRING(config, src, easy_config, return -1);
	if (*config != 't')
		return 0; /* not configured -> do not create */

	if (ctx->data->LayerN >= PCB_MAX_LAYER) {
		rnd_message(RND_MSG_ERROR, "Board has more layers than supported by this compilation of pcb-rnd (%d)\nIf this is a valid board, please increase PCB_MAX_LAYER and recompile.\n", PCB_MAX_LAYER);
		return -1;
	}

	HASH_GET_STRING(name, src, easy_name, return -1);

	grp = pcb_get_grp_new_raw(ctx->pcb, 0);
	grp->name = rnd_strdup(name);
	grp->ltype = std_layer_id2type[idx];

	lid = pcb_layer_create(ctx->pcb, grp - ctx->pcb->LayerGroups.grp, name, 0);
	dst = pcb_get_layer(ctx->pcb->Data, lid);
	dst->name = rnd_strdup(name);

	if ((easyeda_id >= 0) && (easyeda_id < EASY_MAX_LAYERS))
		ctx->layers[easyeda_id] = dst;

	load_clr = (grp->ltype & PCB_LYT_COPPER) ? conf_io_easyeda.plugins.io_easyeda.load_color_copper : conf_io_easyeda.plugins.io_easyeda.load_color_noncopper;
	if (load_clr) {
		HASH_GET_STRING(clr, src, easy_color, goto skip_clr);
		rnd_color_load_str(&dst->meta.real.color, clr);
		skip_clr:;
	}
	return 0;
}


static int std_parse_layer(std_read_ctx_t *ctx, gdom_node_t *layers, long idx, long want_layer_id)
{
	int srci;

	for(srci = 0; srci < layers->value.array.used; srci++) {
		gdom_node_t *src = layers->value.array.child[srci];
		long id;
		HASH_GET_LONG(id, src, easy_id, return -1);
		if (id == want_layer_id)
			return std_parse_layer_(ctx, src, idx, id);
	}

	return 0; /* ignore layers not specified in input */
}



static int std_parse_layers(std_read_ctx_t *ctx)
{
	gdom_node_t *layers;
	int res = 0;
	const int *lt;

	layers = gdom_hash_get(ctx->root, easy_layers);
	if ((layers == NULL) || (layers->type != GDOM_ARRAY)) {
		rnd_message(RND_MSG_ERROR, "EasyEDA std: missing or wrong typed layers tree\n");
		return -1;
	}

	for(lt = layertab; *lt != 0; lt++) {
		if (*lt == LAYERTAB_INNER) {
			long n;
			for(n = layertab_in_first; n <= layertab_in_last; n++)
				res |= std_parse_layer(ctx, layers, n - 1, n);
		}
		else
			res |= std_parse_layer(ctx, layers, *lt - 1, *lt);
	}

	return res;
}

static int std_parse_canvas(std_read_ctx_t *ctx)
{
	gdom_node_t *canvas;
	double ox, oy, w, h;

	canvas = gdom_hash_get(ctx->root, easy_canvas);
	if ((canvas == NULL) || (canvas->type != GDOM_HASH)) {
		rnd_message(RND_MSG_ERROR, "EasyEDA std: missing or wrong typed canvas tree\n");
		return -1;
	}

	HASH_GET_DOUBLE(ox, canvas, easy_origin_x, return -1);
	HASH_GET_DOUBLE(oy, canvas, easy_origin_y, return -1);
	HASH_GET_DOUBLE(w, canvas, easy_canvas_width, return -1);
	HASH_GET_DOUBLE(h, canvas, easy_canvas_height, return -1);

	ctx->ox = ox;
	ctx->oy = oy - h;
	ctx->pcb->hidlib.dwg.X1 = TRR(0);
	ctx->pcb->hidlib.dwg.Y1 = TRR(0);
	ctx->pcb->hidlib.dwg.X2 = TRR(w);
	ctx->pcb->hidlib.dwg.Y2 = TRR(h);

#if 0
	HASH_GET_DOUBLE(w, canvas, easy_routing_width, return -1);
	rnd_snprintf(tmp, sizeof(tmp), "%$mS", TRR(w));
	rnd_conf_set(ctx->settings_dest, "design/line_thickness", -1, tmp, RND_POL_OVERWRITE);
#endif

	return 0;
}

/*** drawing object helper: svgpath ***/
TODO("pro: move this to somewhere more central")

static svgpath_cfg_t pathcfg;
typedef struct {
	std_read_ctx_t *ctx;
	pcb_layer_t *layer;
	rnd_coord_t thickness;
	void *in_poly;
	gdom_node_t *nd;

} path_ctx_t;

static void easyeda_mkpath_line(void *uctx, double x1, double y1, double x2, double y2)
{
	path_ctx_t *pctx = uctx;
	std_read_ctx_t *ctx = pctx->ctx;

TODO("this will be needed in poly");
#if 0
	if (pctx->in_poly)
		csch_alien_append_poly_line(&pctx->ctx->alien, pctx->in_poly, x1, y1, x2, y2);
	else
#endif
	{
		pcb_line_t *line = pcb_line_alloc(pctx->layer);

		line->Point1.X = TRX(x1);
		line->Point1.Y = TRY(y1);
		line->Point2.X = TRX(x2);
		line->Point2.Y = TRY(y2);
		line->Thickness = pctx->thickness;
		line->Clearance = 0;
	
		pcb_add_line_on_layer(pctx->layer, line);
	}
}

static void easyeda_mkpath_error(void *uctx, const char *errmsg, long offs)
{
	path_ctx_t *pctx = uctx;
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
static int std_parse_path(std_read_ctx_t *ctx, const char *pathstr, gdom_node_t *nd, pcb_layer_t *layer, rnd_coord_t thickness, int filled)
{
	path_ctx_t pctx;

	easyeda_svgpath_setup();

	pctx.ctx = ctx;
	pctx.layer = layer;
	pctx.thickness = thickness;
	pctx.nd = nd;

TODO("poly");
#if 0
	if (filled)
		pctx.in_poly = csch_alien_mkpoly(&ctx->alien, parent, penname, penname);
	else
#endif
		pctx.in_poly = NULL;

	return svgpath_render(&pathcfg, &pctx, pathstr);
}


/*** drawing object parsers ***/
static int std_parse_track(std_read_ctx_t *ctx, gdom_node_t *track)
{
	gdom_node_t *points;
	long n;
	double swd;
	rnd_coord_t x, y, lx, ly, th;
	pcb_layer_t *layer;

	HASH_GET_SUBTREE(points, track, easy_points, GDOM_ARRAY, return -1);
	HASH_GET_LAYER(layer, track, easy_layer, return -1);
	HASH_GET_DOUBLE(swd, track, easy_stroke_width, return -1);

	th = TRR(swd);

	if ((points->value.array.used % 2) != 0) {
		error_at(ctx, points, (" odd number of points (should be coord pairs)\n"));
		return -1;
	}

	for(n = 0; n < points->value.array.used; n += 2) {
		double dx = easyeda_get_double(ctx, points->value.array.child[n]);
		double dy = easyeda_get_double(ctx, points->value.array.child[n+1]);
		x = TRX(dx);
		y = TRY(dy);

		if (n > 0) {
			pcb_line_t *line = pcb_line_alloc(layer);

			line->Point1.X = lx;
			line->Point1.Y = ly;
			line->Point2.X = x;
			line->Point2.Y = y;
			line->Thickness = th;
			line->Clearance = 0;
	
			pcb_add_line_on_layer(layer, line);
		}

		lx = x;
		ly = y;
	}

	return 0;
}


static int std_parse_arc(std_read_ctx_t *ctx, gdom_node_t *track)
{
	const char *path;
	long n;
	double swd;
	pcb_layer_t *layer;

	HASH_GET_STRING(path, track, easy_path, return -1);
	HASH_GET_LAYER(layer, track, easy_layer, return -1);
	HASH_GET_DOUBLE(swd, track, easy_stroke_width, return -1);

	return std_parse_path(ctx, path, track, layer, TRR(swd), 0);
}

static int std_parse_circle(std_read_ctx_t *ctx, gdom_node_t *track)
{
	return 0;
}

static int std_parse_any_shapes(std_read_ctx_t *ctx, gdom_node_t *shape)
{
	switch(shape->name) {
		case easy_track: return std_parse_track(ctx, shape);
		case easy_arc: return std_parse_arc(ctx, shape);
		case easy_circle: return std_parse_circle(ctx, shape);
	}

	error_at(ctx, shape, ("Unknown shape '%s'\n", easy_keyname(shape->name)));
	if (shape->type == GDOM_STRING)
		error_at(ctx, shape, (" shape string is '%s'\n", shape->value.str));
	return -1;
}

static int std_parse_shapes_array(std_read_ctx_t *ctx, gdom_node_t *shapes)
{
	long n;

	if ((shapes == NULL) || (shapes->type != GDOM_ARRAY)) {
		rnd_message(RND_MSG_ERROR, "No or invalid shape array\n");
		return -1;
	}

	for(n = 0; n < shapes->value.array.used; n++)
		if (std_parse_any_shapes(ctx, shapes->value.array.child[n]) != 0)
			return -1;

	return 0;
}

/*** main tree parser entries ***/

static int easyeda_std_parse_board(pcb_board_t *dst, const char *fn, rnd_conf_role_t settings_dest)
{
	std_read_ctx_t ctx;
	int res = 0;

	ctx.pcb = dst;
	ctx.data = dst->Data;
	ctx.fn = fn;
	ctx.f = rnd_fopen(&dst->hidlib, fn, "r");
	ctx.settings_dest = settings_dest;
	if (ctx.f == NULL) {
		rnd_message(RND_MSG_ERROR, "filed to open %s for read\n", fn);
		return -1;
	}

	ctx.root = easystd_low_parse(ctx.f, 0);
	fclose(ctx.f);

	if (res == 0) res |= std_parse_layers(&ctx);
	if (res == 0) res |= std_parse_canvas(&ctx);
	if (res == 0) res |= std_parse_shapes_array(&ctx, gdom_hash_get(ctx.root, easy_shape));

	return res;
}


