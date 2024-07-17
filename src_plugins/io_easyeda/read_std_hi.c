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

static int std_parse_any_shapes(std_read_ctx_t *ctx, gdom_node_t *shape);
static int std_parse_shapes_array(std_read_ctx_t *ctx, gdom_node_t *shapes);

/*** board meta ***/

static int std_parse_layer_(std_read_ctx_t *ctx, gdom_node_t *src, long idx, int easyeda_id)
{
	pcb_layer_t *dst;
	const char *config, *name, *clr;
	pcb_layergrp_t *grp;
	rnd_layer_id_t lid;
	int load_clr;
	unsigned ltype;

	if (idx > std_layer_id2type_size)
		return 0; /* ignore layers not in the table */

	if (std_layer_id2type[idx] == 0)
		return 0; /* table dictated skip */

	HASH_GET_STRING(config, src, easy_config, return -1);
	if ((easyeda_id < 99) && (*config != 't'))
		return 0; /* not configured -> do not create */

	if (ctx->data->LayerN >= PCB_MAX_LAYER) {
		rnd_message(RND_MSG_ERROR, "Board has more layers than supported by this compilation of pcb-rnd (%d)\nIf this is a valid board, please increase PCB_MAX_LAYER and recompile.\n", PCB_MAX_LAYER);
		return -1;
	}

	HASH_GET_STRING(name, src, easy_name, return -1);

	ltype = std_layer_id2type[idx];

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
			dst->meta.bound.stack_offs = easyeda_id - layertab_in_first + 1;
	}

	if (ltype & (PCB_LYT_SILK | PCB_LYT_MASK | PCB_LYT_PASTE))
		dst->comb |= PCB_LYC_AUTO;
	if (ltype & PCB_LYT_MASK)
		dst->comb |= PCB_LYC_SUB;


	if ((easyeda_id >= 0) && (easyeda_id < EASY_MAX_LAYERS))
		ctx->layers[easyeda_id] = dst;

	load_clr = (ltype & PCB_LYT_COPPER) ? conf_io_easyeda.plugins.io_easyeda.load_color_copper : conf_io_easyeda.plugins.io_easyeda.load_color_noncopper;
	if ((ctx->pcb != NULL) && load_clr) {
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

	std_layer_id2type[99-1] = PCB_LYT_DOC;
	std_layer_id2type[100-1] = PCB_LYT_DOC;
	std_layer_id2type[101-1] = PCB_LYT_DOC;

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

	res |= std_create_misc_layers(ctx);

	return res;
}

static int std_parse_head(std_read_ctx_t *ctx)
{
	gdom_node_t *head;
	const char *doctype;

	head = gdom_hash_get(ctx->root, easy_head);
	if ((head == NULL) || (head->type != GDOM_HASH)) {
		rnd_message(RND_MSG_ERROR, "EasyEDA std: missing or wrong typed head tree\n");
		return -1;
	}

	HASH_GET_STRING(doctype, head, easy_docType, return -1);
	if (doctype[1] != '\0')
		goto badtype;

	switch(doctype[0]) {
		case '3': break;
		case '4': ctx->is_footprint = 1; break;
		default: badtype:;
			rnd_message(RND_MSG_ERROR, "EasyEDA std: wrong doc type '%s'\n", doctype);
			return -1;
	}

	return 0;
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

	if (ctx->pcb != NULL) {
		ctx->oy = oy - h;
		ctx->pcb->hidlib.dwg.X1 = TRR(0);
		ctx->pcb->hidlib.dwg.Y1 = TRR(0);
		ctx->pcb->hidlib.dwg.X2 = TRR(w);
		ctx->pcb->hidlib.dwg.Y2 = TRR(h);
	}
	else
		ctx->oy = oy;

#if 0
	HASH_GET_DOUBLE(w, canvas, easy_routing_width, return -1);
	rnd_snprintf(tmp, sizeof(tmp), "%$mS", TRR(w));
	rnd_conf_set(ctx->settings_dest, "design/line_thickness", -1, tmp, RND_POL_OVERWRITE);
#endif

	return 0;
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
			line->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
			line->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

			pcb_add_line_on_layer(layer, line);
		}

		lx = x;
		ly = y;
	}

	return 0;
}


static int std_parse_arc(std_read_ctx_t *ctx, gdom_node_t *arc)
{
	const char *path;
	double swd;
	pcb_layer_t *layer;

	HASH_GET_STRING(path, arc, easy_path, return -1);
	HASH_GET_LAYER(layer, arc, easy_layer, return -1);
	HASH_GET_DOUBLE(swd, arc, easy_stroke_width, return -1);

	return std_parse_path(ctx, path, arc, layer, TRR(swd), 0);
}

static int std_parse_circle(std_read_ctx_t *ctx, gdom_node_t *circ)
{
	double cx, cy, r, swd;
	pcb_layer_t *layer;
	pcb_arc_t *arc;

	HASH_GET_LAYER(layer, circ, easy_layer, return -1);
	HASH_GET_DOUBLE(cx, circ, easy_x, return -1);
	HASH_GET_DOUBLE(cy, circ, easy_y, return -1);
	HASH_GET_DOUBLE(r, circ, easy_r, return -1);
	HASH_GET_DOUBLE(swd, circ, easy_stroke_width, return -1);

	arc = pcb_arc_alloc(layer);

	arc->X = TRX(cx);
	arc->Y = TRY(cy);
	arc->Width = arc->Height = TRR(r);
	arc->Thickness = TRR(swd);
	arc->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
	arc->StartAngle = 0;
	arc->Delta = 360;
	arc->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

	pcb_add_arc_on_layer(layer, arc);

	return 0;
}

static int std_parse_via(std_read_ctx_t *ctx, gdom_node_t *via)
{
	double cx, cy, dia, holer;
	pcb_pstk_t *pstk;

	HASH_GET_DOUBLE(cx, via, easy_x, return -1);
	HASH_GET_DOUBLE(cy, via, easy_y, return -1);
	HASH_GET_DOUBLE(dia, via, easy_dia, return -1);
	HASH_GET_DOUBLE(holer, via, easy_hole_radius, return -1);

	pstk = pcb_pstk_new_compat_via(ctx->data, -1, TRX(cx), TRY(cy), TRR(holer)*2, TRR(dia), 0, 0, PCB_PSTK_COMPAT_ROUND, 1);
	if (pstk == NULL) {
		error_at(ctx, via, ("Failed to create padstack for via\n"));
		return -1;
	}
	pstk->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
	pstk->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

	return 0;
}

static int std_parse_hole(std_read_ctx_t *ctx, gdom_node_t *hole)
{
	double cx, cy, dia;
	pcb_pstk_t *pstk;

	HASH_GET_DOUBLE(cx, hole, easy_x, return -1);
	HASH_GET_DOUBLE(cy, hole, easy_y, return -1);
	HASH_GET_DOUBLE(dia, hole, easy_dia, return -1);

	pstk = pcb_pstk_new_hole(ctx->data, TRX(cx), TRY(cy), TRR(dia), 0);
	if (pstk == NULL) {
		error_at(ctx, hole, ("Failed to create padstack for hole\n"));
		return -1;
	}
	pstk->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
	pstk->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

	return 0;
}

static int std_parse_pad(std_read_ctx_t *ctx, gdom_node_t *pad)
{
	gdom_node_t *slot_points, *points;
	double cx, cy, holer, w, h, rot;
	long number;
	int is_any, is_plated, nopaste = 0, n, sloti;
	pcb_layer_t *layer;
	pcb_pstk_t *pstk;
	const char *sshape, *splated, *netname;
	pcb_pstk_shape_t shapes[8] = {0};
	pcb_layer_type_t side;
	char termid[64];

	HASH_GET_LAYER_GLOBAL(layer, is_any, pad, easy_layer, return -1);
	HASH_GET_DOUBLE(cx, pad, easy_x, return -1);
	HASH_GET_DOUBLE(cy, pad, easy_y, return -1);
	HASH_GET_DOUBLE(w, pad, easy_width, return -1);
	HASH_GET_DOUBLE(h, pad, easy_height, return -1);
	HASH_GET_STRING(sshape, pad, easy_shape, return -1);
	HASH_GET_STRING(splated, pad, easy_plated, return -1);
	HASH_GET_LONG(number, pad, easy_number, return -1);
	HASH_GET_DOUBLE(holer, pad, easy_hole_radius, return -1);
	HASH_GET_DOUBLE(rot, pad, easy_rot, return -1);
	HASH_GET_SUBTREE(slot_points, pad, easy_slot_points, GDOM_ARRAY, return -1);
	HASH_GET_STRING(netname, pad, easy_net, return -1);

	is_plated = (*splated == 'Y');
	if (slot_points->value.array.used <= 1) {
		slot_points = NULL;
	}
	else if (slot_points->value.array.used != 4) {
		error_at(ctx, points, ("Invalid number of pad slot_points (must be 4)\n"));
		return -1;
	}

	if ((slot_points != NULL) || (holer > 0))
		nopaste = 1;

	/* create the main shape in shape[0] */
	if (strcmp(sshape, "ELLIPSE") == 0) {
		shapes[0].shape = PCB_PSSH_CIRC;
		shapes[0].data.circ.dia = TRR(w);
	}
	else if (strcmp(sshape, "RECT") == 0) {
		rnd_coord_t w2 = TRR(w/2.0), h2 = TRR(h/2.0);
		shapes[0].shape = PCB_PSSH_POLY;
		pcb_pstk_shape_alloc_poly(&shapes[0].data.poly, 4);
		shapes[0].data.poly.x[0] = -w2; shapes[0].data.poly.y[0] = -h2;
		shapes[0].data.poly.x[1] = +w2; shapes[0].data.poly.y[1] = -h2;
		shapes[0].data.poly.x[2] = +w2; shapes[0].data.poly.y[2] = +h2;
		shapes[0].data.poly.x[3] = -w2; shapes[0].data.poly.y[3] = +h2;
		shapes[0].data.poly.len = 4;
	}
	else if (strcmp(sshape, "OVAL") == 0) {
		shapes[0].shape = PCB_PSSH_LINE;
		shapes[0].data.line.x1 = 0;
		shapes[0].data.line.y1 = -TRR(h/2.0)+TRR(w/2.0);
		shapes[0].data.line.x2 = 0;
		shapes[0].data.line.y2 = +TRR(h/2.0)-TRR(w/2.0);
		shapes[0].data.line.thickness = TRR(w);
	}
	else if (strcmp(sshape, "POLYGON") == 0) {
		long n, i;

		HASH_GET_SUBTREE(points, pad, easy_points, GDOM_ARRAY, return -1);

		if ((points->value.array.used < 6) || ((points->value.array.used % 2) != 0)) {
			error_at(ctx, points, ("Invalid number of pad polygon points (must be even and at least 6)\n"));
			return -1;
		}

		shapes[0].shape = PCB_PSSH_POLY;
		pcb_pstk_shape_alloc_poly(&shapes[0].data.poly, points->value.array.used/2);

		for(n = i = 0; n < points->value.array.used; n += 2, i++) {
			double dx = easyeda_get_double(ctx, points->value.array.child[n]);
			double dy = easyeda_get_double(ctx, points->value.array.child[n+1]);

			shapes[0].data.poly.x[i] = TRR(dx - cx);
			shapes[0].data.poly.y[i] = TRR(dy - cy);
		}
		shapes[0].data.poly.len = points->value.array.used/2;
	}
	if (!is_any) {
		side = pcb_layer_flags_(layer) & PCB_LYT_ANYWHERE;
		shapes[0].layer_mask = side | PCB_LYT_COPPER;

		pcb_pstk_shape_copy(&shapes[1], &shapes[0]);
		shapes[1].layer_mask = side | PCB_LYT_MASK;
		shapes[1].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
		pcb_pstk_shape_grow_(&shapes[1], 0, RND_MIL_TO_COORD(4));

		pcb_pstk_shape_copy(&shapes[2], &shapes[0]);
		shapes[2].layer_mask = side | PCB_LYT_PASTE;
		shapes[2].comb = PCB_LYC_AUTO;
		pcb_pstk_shape_grow_(&shapes[2], 0, -RND_MIL_TO_COORD(4));

		sloti = 3;
	}
	else {
		shapes[0].layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER;
		pcb_pstk_shape_copy(&shapes[1], &shapes[0]);
		shapes[1].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER;
		pcb_pstk_shape_copy(&shapes[2], &shapes[0]);
		shapes[2].layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER;

		pcb_pstk_shape_copy(&shapes[3], &shapes[0]);
		shapes[3].layer_mask = PCB_LYT_TOP | PCB_LYT_MASK;
		shapes[3].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
		pcb_pstk_shape_grow_(&shapes[3], 0, RND_MIL_TO_COORD(4));


		pcb_pstk_shape_copy(&shapes[4], &shapes[0]);
		shapes[4].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK;
		shapes[4].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
		pcb_pstk_shape_grow_(&shapes[4], 0, RND_MIL_TO_COORD(4));

		if (!nopaste) {
			pcb_pstk_shape_copy(&shapes[5], &shapes[0]);
			shapes[5].layer_mask = PCB_LYT_TOP | PCB_LYT_PASTE;
			shapes[5].comb = PCB_LYC_AUTO;
			pcb_pstk_shape_grow_(&shapes[5], 0, -RND_MIL_TO_COORD(4));

			pcb_pstk_shape_copy(&shapes[6], &shapes[0]);
			shapes[6].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_PASTE;
			shapes[6].comb = PCB_LYC_AUTO;
			pcb_pstk_shape_grow_(&shapes[6], 0, -RND_MIL_TO_COORD(4));

			sloti = 7;
		}
		else
			sloti = 5;
	}

	if (slot_points != NULL) {
		double dx1 = easyeda_get_double(ctx, slot_points->value.array.child[0]) - cx;
		double dy1 = easyeda_get_double(ctx, slot_points->value.array.child[1]) - cy;
		double dx2 = easyeda_get_double(ctx, slot_points->value.array.child[2]) - cx;
		double dy2 = easyeda_get_double(ctx, slot_points->value.array.child[3]) - cy;
		rnd_coord_t x1 = TRR(dx1), y1 = TRR(dy1);
		rnd_coord_t x2 = TRR(dx2), y2 = TRR(dy2);

		if (rot != 0) {
			double rad = -rot / RND_RAD_TO_DEG;
			double cosa = cos(rad), sina = sin(rad);
			/* EasyEDA stores slot in rotated form while shapes are stored unrotated;
			   the whole padstakc will be rotated so rotate the slot back to
			   compensate */
			rnd_rotate(&x1, &y1, 0, 0, cosa, sina);
			rnd_rotate(&x2, &y2, 0, 0, cosa, sina);
		}

		shapes[sloti].shape = PCB_PSSH_LINE;
		shapes[sloti].data.line.x1 = x1;
		shapes[sloti].data.line.y1 = y1;
		shapes[sloti].data.line.x2 = x2;
		shapes[sloti].data.line.y2 = y2;
		shapes[sloti].data.line.thickness = TRR(holer);
		shapes[sloti].layer_mask = PCB_LYT_MECH;
		shapes[sloti].comb = PCB_LYC_AUTO;

		holer = 0;
	}

	pstk = pcb_pstk_new_from_shape(ctx->data, TRX(cx), TRY(cy), TRR(holer)*2, is_plated, 0, shapes);
	if (pstk == NULL) {
		error_at(ctx, pad, ("Failed to create padstack for pad\n"));
		return -1;
	}
	pstk->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
	pstk->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

	if (rot != 0) {
		double rad = rot / RND_RAD_TO_DEG;
		pcb_pstk_rotate(pstk, TRX(cx), TRY(cy), cos(rad), sin(rad), rot);
	}

	rnd_snprintf(termid, sizeof(termid), "%ld", number);
	pcb_attribute_put(&pstk->Attributes, "term", termid);

	/* add term conn to the netlist if therminal has a net field */
	if ((netname != NULL) && (*netname != '\0')) {
		pcb_subc_t *subc = pcb_gobj_parent_subc(pstk->parent_type, &pstk->parent);
		if (subc != NULL) {
			const char *refdes = pcb_attribute_get(&subc->Attributes, "refdes");
			if (refdes != NULL) {
				pcb_net_t *net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_INPUT], netname, 1);
				pcb_net_term_get(net, refdes, termid, 1);
/*				rnd_trace("NETLIST TERM: %s-%s to %s\n", refdes, termid, netname);*/
			}
			else
				rnd_message(RND_MSG_ERROR, "EasyEDA pad before refdes text - please report this but to pcb-rnd developers\n");
		}
	}

	return 0;
}


static int std_parse_dimension(std_read_ctx_t *ctx, gdom_node_t *dimension)
{
	const char *path;
	double swd;
	pcb_layer_t *layer;

	HASH_GET_STRING(path, dimension, easy_path, return -1);
	HASH_GET_LAYER(layer, dimension, easy_layer, return -1);

	return std_parse_path(ctx, path, dimension, layer, RND_MIL_TO_COORD(5), 0);
}

static int std_parse_text(std_read_ctx_t *ctx, gdom_node_t *text)
{
	double x, y, rot, height, strokew;
	rnd_coord_t tx, ty;
	const char *str, *mirr, *type;
	int xmir, is_refdes;
	pcb_layer_t *layer;
	pcb_text_t *t;

	HASH_GET_LAYER(layer, text, easy_layer, return -1);
	HASH_GET_DOUBLE(x, text, easy_x, return -1);
	HASH_GET_DOUBLE(y, text, easy_y, return -1);
	HASH_GET_DOUBLE(rot, text, easy_rot, return -1);
	HASH_GET_DOUBLE(height, text, easy_height, return -1);
	HASH_GET_DOUBLE(strokew, text, easy_stroke_width, return -1);
	HASH_GET_STRING(str, text, easy_string, return -1);
	HASH_GET_STRING(mirr, text, easy_mirror, return -1);
	HASH_GET_STRING(type, text, easy_type, return -1);

	is_refdes = (*type == 'P');
	tx = TRX(x);
	ty = TRY(y - height);
	xmir = (*mirr == '1');
	if (xmir) {
		double rad = rot / RND_RAD_TO_DEG;
		rnd_rotate(&tx, &ty, TRX(x), TRY(y), cos(rad), sin(rad));
		rot = -rot;
	}

	t = pcb_text_alloc(layer);
	if (t == NULL) {
		error_at(ctx, text, ("Failed to allocate text object\n"));
		return -1;
	}

	t->X = tx;
	t->Y = ty;
	t->rot = rot;
	t->mirror_x = xmir;
	t->TextString = rnd_strdup(is_refdes ? "%a.parent.refdes%" : str);
	t->Scale = height/8.0 * 150.0;
	t->thickness = TRR(strokew);
	t->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

	if (is_refdes) {
		pcb_subc_t *subc = pcb_lobj_parent_subc(t->parent_type, &t->parent);

		t->Flags = pcb_flag_add(t->Flags, PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER);
		ctx->last_refdes = t;

		if (subc != NULL)
			pcb_attribute_put(&subc->Attributes, "refdes", str);
	}

	pcb_add_text_on_layer(layer, t, pcb_font(PCB, 0, 1));

	return 0;
}

static int std_parse_solidregion(std_read_ctx_t *ctx, gdom_node_t *nd)
{
	pcb_layer_t *layer;
	const char *path;
	pcb_poly_t *poly;

	HASH_GET_LAYER(layer, nd, easy_layer, return -1);
	HASH_GET_STRING(path, nd, easy_path, return -1);

	poly = pcb_poly_alloc(layer);

	std_parse_path(ctx, path, nd, layer, 0, poly);

	pcb_add_poly_on_layer(layer, poly);
	if (ctx->pcb != NULL)
		pcb_poly_init_clip(layer->parent.data, layer, poly);

	return 0;
}

static int std_parse_copperarea(std_read_ctx_t *ctx, gdom_node_t *nd)
{
	pcb_layer_t *layer;
	const char *path;
	pcb_poly_t *poly;
	double sw, clearance;

	HASH_GET_LAYER(layer, nd, easy_layer, return -1);
	HASH_GET_STRING(path, nd, easy_path, return -1);
	HASH_GET_DOUBLE(sw, nd, easy_stroke_width, return -1);
	HASH_GET_DOUBLE(clearance, nd, easy_clearance, return -1);

	poly = pcb_poly_alloc(layer);

	std_parse_path(ctx, path, nd, layer, 0, poly);
	poly->enforce_clearance = TRR(clearance);
	poly->Flags = pcb_flag_add(poly->Flags, PCB_FLAG_CLEARPOLY);

	pcb_add_poly_on_layer(layer, poly);
	pcb_poly_init_clip(layer->parent.data, layer, poly);

	return 0;
}

static int std_parse_rect(std_read_ctx_t *ctx, gdom_node_t *nd)
{
	double x, y, w, h;
	pcb_poly_t *poly;
	pcb_layer_t *layer;

	HASH_GET_LAYER(layer, nd, easy_layer, return -1);
	HASH_GET_DOUBLE(x, nd, easy_x, return -1);
	HASH_GET_DOUBLE(y, nd, easy_y, return -1);
	HASH_GET_DOUBLE(w, nd, easy_width, return -1);
	HASH_GET_DOUBLE(h, nd, easy_height, return -1);

	poly = pcb_poly_alloc(layer);

	pcb_poly_point_prealloc(poly, 4);
	poly->PointN = 4;
	poly->Points[0].X = TRX(x);   poly->Points[0].Y = TRY(y);
	poly->Points[1].X = TRX(x+w); poly->Points[1].Y = TRY(y);
	poly->Points[2].X = TRX(x+w); poly->Points[2].Y = TRY(y+h);
	poly->Points[3].X = TRX(x);   poly->Points[3].Y = TRY(y+h);

	pcb_add_poly_on_layer(layer, poly);
	pcb_poly_init_clip(layer->parent.data, layer, poly);

	return 0;
}

static pcb_subc_t *std_subc_create(std_read_ctx_t *ctx)
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

static void std_subc_finalize(std_read_ctx_t *ctx, pcb_subc_t *subc, rnd_coord_t x, rnd_coord_t y, double rot)
{
	int on_bottom = 0;

	if (ctx->last_refdes != NULL) {
		int side = pcb_layer_flags_(ctx->last_refdes->parent.layer) & PCB_LYT_ANYWHERE;
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

static int std_parse_subc(std_read_ctx_t *ctx, gdom_node_t *nd)
{
	double x, y, rot;
	pcb_poly_t *poly;
	pcb_layer_t *layer;
	gdom_node_t *shapes;
	pcb_subc_t *subc;
	pcb_data_t *save;
	int res, n, on_bottom = 0;

	HASH_GET_DOUBLE(x, nd, easy_x, return -1);
	HASH_GET_DOUBLE(y, nd, easy_y, return -1);
	HASH_GET_DOUBLE(rot, nd, easy_rot, return -1);
	HASH_GET_SUBTREE(shapes, nd, easy_shape, GDOM_ARRAY, return -1);


	subc = std_subc_create(ctx);

	save = ctx->data;
	ctx->data = subc->data;
	res = std_parse_shapes_array(ctx, shapes);
	ctx->data = save;

	std_subc_finalize(ctx, subc, TRX(x), TRY(y), rot);

	return res;
}

static int std_parse_any_shapes(std_read_ctx_t *ctx, gdom_node_t *shape)
{
	switch(shape->name) {
		case easy_track: return std_parse_track(ctx, shape);
		case easy_arc: return std_parse_arc(ctx, shape);
		case easy_circle: return std_parse_circle(ctx, shape);
		case easy_via: return std_parse_via(ctx, shape);
		case easy_hole: return std_parse_hole(ctx, shape);
		case easy_pad: return std_parse_pad(ctx, shape);
		case easy_dimension: return std_parse_dimension(ctx, shape);
		case easy_text: return std_parse_text(ctx, shape);
		case easy_solidregion: return std_parse_solidregion(ctx, shape);
		case easy_copperarea: return std_parse_copperarea(ctx, shape);
		case easy_rect: return std_parse_rect(ctx, shape);
		case easy_subc: return std_parse_subc(ctx, shape);
		case easy_svgnode: return 0; /* ignore this for now */
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
	pcb_subc_t *subc_as_board;
	pcb_data_t *save;

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

	if (ctx.root == NULL) res = -1;
	if (res == 0) res |= std_parse_head(&ctx);
	if (res == 0) res |= std_parse_layers(&ctx);
	if (res == 0) res |= std_parse_canvas(&ctx);

	if (ctx.is_footprint) {
		dst->is_footprint = 1;
		save = ctx.data;
		subc_as_board = std_subc_create(&ctx);
		ctx.data = subc_as_board->data;
	}

	if (res == 0) res |= std_parse_shapes_array(&ctx, gdom_hash_get(ctx.root, easy_shape));

	if (ctx.is_footprint) {
		ctx.data = save;
		std_subc_finalize(&ctx, subc_as_board, 0, 0, 0);
	}


	return res;
}


static int easyeda_std_parse_fp(pcb_data_t *data, const char *fn)
{
	pcb_board_t *pcb = NULL;
	std_read_ctx_t ctx = {0};
	long n;
	int res = 0;
	pcb_subc_t *subc;
	pcb_data_t *save;

	/* reset buffer layers that are set up to board layers by default;
	   new layers are loaded from the file */
	for(n = 0; n < data->LayerN; n++) {
		pcb_layer_t *rl = data->Layer[n].meta.bound.real;

		if ((pcb == NULL) && (rl != NULL)) { /* resolve target pcb from a real layer */
			pcb_data_t *rd = rl->parent.data;
			pcb = rd->parent.board;
		}
		pcb_layer_free_fields(&data->Layer[n], 0);
	}
	data->LayerN = 0;

	ctx.pcb = NULL; /* indicate we are not loading a board but a footprint to buffer */
	ctx.data = data;
	ctx.fn = fn;
	ctx.f = rnd_fopen(&pcb->hidlib, fn, "r");
	ctx.settings_dest = -1;
	if (ctx.f == NULL) {
		rnd_message(RND_MSG_ERROR, "filed to open %s for read\n", fn);
		return -1;
	}

	ctx.root = easystd_low_parse(ctx.f, 0);
	fclose(ctx.f);

	if (ctx.root == NULL) res = -1;
	if (res == 0) res |= std_parse_head(&ctx);
	if (!ctx.is_footprint) {
		rnd_message(RND_MSG_ERROR, "EasyEDA internal error: trying to load %s as footprint while it is not a footprint\n", fn);
		return -1;
	}

	if (res == 0) res |= std_parse_layers(&ctx);
	if (res == 0) res |= std_parse_canvas(&ctx);

	pcb_data_binding_update(pcb, data);

	save = ctx.data;
	subc = std_subc_create(&ctx);
	ctx.data = subc->data;

	/* rewire ctx.layers so they point to the corresponding subc layer so that
	   objects are created within the subc, not in parent data */
	for(n = 0; n < subc->data->LayerN; n++) {
		int i, idx = 0;
		for(i = 0; i < subc->data->LayerN; i++) {
			pcb_layer_t *cl =ctx.layers[n];
			if ((cl != NULL) && (cl->meta.bound.type == subc->data->Layer[i].meta.bound.type)) {
				idx = i;
				break;
			}
		}
		ctx.layers[n] = &subc->data->Layer[idx];
	}

	if (res == 0) res |= std_parse_shapes_array(&ctx, gdom_hash_get(ctx.root, easy_shape));

	ctx.data = save;
	std_subc_finalize(&ctx, subc, 0, 0, 0);

	return res;
}
