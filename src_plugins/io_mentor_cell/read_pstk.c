/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
 *
 *  This module, io_mentor_cell, was written and is Copyright (C) 2016 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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

static int parse_dia(hkp_ctx_t *ctx, node_t *roundn, rnd_coord_t *dia)
{
	node_t *hr = roundn->first_child;
	if ((hr == NULL) || (strcmp(hr->argv[0], "DIAMETER") != 0))
		return hkp_error(hr, "Expected DIAMETER as first child of ROUND\n");
	if (parse_coord(ctx, hr->argv[1], dia) != 0)
		return hkp_error(hr, "Invalid ROUND DIAMETER value '%s'\n", hr->argv[1]);
	return 0;
}


static hkp_hole_t *parse_hole(hkp_ctx_t *ctx, const char *name)
{
	const rnd_unit_t *old_unit;
	node_t *hr, *ho;
	hkp_hole_t *h = htsp_get(&ctx->holes, name);

	if (h == NULL)
		return NULL;
	if (h->valid)
		return h;

	old_unit = ctx->unit;
	ctx->unit = h->unit;

	ho = find_nth(h->subtree->first_child, "HOLE_OPTIONS", 0);
	if (ho != NULL) {
		int n;
		for(n = 1; n < ho->argc; n++)
			if (strcmp(ho->argv[n], "PLATED") == 0)
				h->plated = 1;
	}

	hr = find_nth(h->subtree->first_child, "ROUND", 0);
	if (hr != NULL) {
		if (parse_dia(ctx, hr, &h->dia) != 0)
			goto error;
	}
	else {
		hr = find_nth(h->subtree->first_child, "SLOT", 0);
		if (hr != NULL) {
			node_t *tmp;
			tmp = find_nth(hr->first_child, "WIDTH", 0);
			if (parse_coord(ctx, tmp->argv[1], &h->w) != 0) {
				hkp_error(tmp, "Invalid SLOT WIDTH value '%s'\n", tmp->argv[1]);
				return NULL;
			}
			tmp = find_nth(hr->first_child, "HEIGHT", 0);
			if (parse_coord(ctx, tmp->argv[1], &h->h) != 0) {
				hkp_error(tmp, "Invalid SLOT HEIGHT value '%s'\n", tmp->argv[1]);
				return NULL;
			}
		}
	}


	h->valid = 1;
	ctx->unit = old_unit;
	return h;

	error:;
	ctx->unit = old_unit;
	return NULL;
}

#define SHAPE_CHECK_DUP \
do { \
	if (has_shape) { \
		hkp_error(n, "PAD with multiple shapes\n"); \
		goto error; \
	} \
	has_shape = 1; \
} while(0)
static hkp_shape_t *parse_shape(hkp_ctx_t *ctx, const char *name)
{
	const rnd_unit_t *old_unit;
	node_t *n, *on, *tmp;
	rnd_coord_t ox = 0, oy = 0;
	hkp_shape_t *s = htsp_get(&ctx->shapes, name);
	int has_shape = 0;

	if (s == NULL)
		return NULL;
	if (s->valid)
		return s;

	old_unit = ctx->unit;
	ctx->unit = s->unit;

	on = find_nth(s->subtree->first_child, "OFFSET", 0);
	if (on != NULL)
		parse_xy(ctx, on->argv[1], &ox, &oy, 0);
	ox = -ox;

	memset(&s->shp, 0, sizeof(pcb_pstk_shape_t));

	for(n = s->subtree->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "ROUND") == 0) {
			rnd_coord_t dia;
			SHAPE_CHECK_DUP;
			if (parse_dia(ctx, n, &dia) != 0)
				goto error;
			s->shp.shape = PCB_PSSH_CIRC;
			s->shp.data.circ.dia = dia;
			s->shp.data.circ.x = ox;
			s->shp.data.circ.y = oy;
		}
		else if ((strcmp(n->argv[0], "RECTANGLE") == 0) || (strcmp(n->argv[0], "SQUARE") == 0)) {
			rnd_coord_t w, h;
			SHAPE_CHECK_DUP;
			tmp = find_nth(n->first_child, "WIDTH", 0);
			if (parse_coord(ctx, tmp->argv[1], &w) != 0) {
				hkp_error(tmp, "Invalid RECTANGLE WIDTH value '%s'\n", tmp->argv[1]);
				return NULL;
			}
			if (*n->argv[0] == 'R') {
				tmp = find_nth(n->first_child, "HEIGHT", 0);
				if (parse_coord(ctx, tmp->argv[1], &h) != 0) {
					hkp_error(tmp, "Invalid RECTANGLE WIDTH value '%s'\n", tmp->argv[1]);
					return NULL;
				}
			}
			else
				h = w; /* square */

			s->shp.shape = PCB_PSSH_POLY;
			pcb_pstk_shape_alloc_poly(&s->shp.data.poly, 4);
			s->shp.data.poly.x[0] = ox - w/2; s->shp.data.poly.y[0] = oy - h/2;
			s->shp.data.poly.x[1] = ox - w/2; s->shp.data.poly.y[1] = oy + h/2;
			s->shp.data.poly.x[2] = ox + w/2; s->shp.data.poly.y[2] = oy + h/2;
			s->shp.data.poly.x[3] = ox + w/2; s->shp.data.poly.y[3] = oy - h/2;
		}
		else if (strcmp(n->argv[0], "OBLONG") == 0) {
			rnd_coord_t w, h;
			SHAPE_CHECK_DUP;
			tmp = find_nth(n->first_child, "WIDTH", 0);
			if (parse_coord(ctx, tmp->argv[1], &w) != 0) {
				hkp_error(tmp, "Invalid OBLONG WIDTH value '%s'\n", tmp->argv[1]);
				return NULL;
			}
			tmp = find_nth(n->first_child, "HEIGHT", 0);
			if (parse_coord(ctx, tmp->argv[1], &h) != 0) {
				hkp_error(tmp, "Invalid OBLONG WIDTH value '%s'\n", tmp->argv[1]);
				return NULL;
			}
			pcb_shape_oval(&(s->shp), w, h);
		}
	}

	if (!has_shape) {
		hkp_error(s->subtree, "PAD without a shape\n");
		return NULL;
	}

	s->valid = 1;
	ctx->unit = old_unit;
	return s;

	error:;
	ctx->unit = old_unit;
	return NULL;
}
#undef SHAPE_CHECK_DUP

typedef struct {
	const char *name;
	pcb_layer_type_t lyt;
	pcb_layer_combining_t lyc;
} lyt_name_t;

lyt_name_t lyt_names[] = {
	{"TOP_PAD",                PCB_LYT_TOP | PCB_LYT_COPPER,    0 },
	{"BOTTOM_PAD",             PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0 },
	{"INTERNAL_PAD",           PCB_LYT_INTERN | PCB_LYT_COPPER, 0 },
	{"TOP_SOLDERMASK_PAD",     PCB_LYT_TOP | PCB_LYT_MASK,      PCB_LYC_AUTO | PCB_LYC_SUB },
	{"BOTTOM_SOLDERMASK_PAD",  PCB_LYT_BOTTOM | PCB_LYT_MASK,   PCB_LYC_AUTO | PCB_LYC_SUB },
	{"TOP_SOLDERPASTE_PAD",    PCB_LYT_TOP | PCB_LYT_PASTE,     PCB_LYC_AUTO },
	{"BOTTOM_SOLDERPASTE_PAD", PCB_LYT_BOTTOM | PCB_LYT_PASTE,  PCB_LYC_AUTO },
	{NULL, 0}
};

static void slot_shape(pcb_pstk_shape_t *shape, rnd_coord_t sx, rnd_coord_t sy)
{
	shape->shape = PCB_PSSH_LINE;
	if (sx > sy) { /* horizontal */
		shape->data.line.thickness = sy;
		shape->data.line.x1 = (-sx + sy)/2;
		shape->data.line.x2 = (+sx - sy)/2;
		shape->data.line.y1 = shape->data.line.y2 = 0;
	}
	else { /* vertical */
		shape->data.line.thickness = sx;
		shape->data.line.y1 = (-sy + sx)/2;
		shape->data.line.y2 = (+sy - sx)/2;
		shape->data.line.x1 = shape->data.line.x2 = 0;
	}
	shape->layer_mask = PCB_LYT_MECH;
	shape->comb = PCB_LYC_AUTO;
}

static hkp_pstk_t *parse_pstk(hkp_ctx_t *ctx, const char *ps)
{
	const rnd_unit_t *old_unit;
	rnd_coord_t ox = 0, oy = 0;
	node_t *n, *hn, *on, *tn;
	hkp_pstk_t *p = htsp_get(&ctx->pstks, ps);
	int top_only = 0;
	pcb_pstk_tshape_t *ts;

	if (p == NULL)
		return NULL;
	if (p->valid)
		return p;

	n = find_nth(p->subtree->first_child, "TECHNOLOGY", 0);
	if (n == NULL) {
		hkp_error(p->subtree, "Padstack without technology\n");
		return NULL;
	}

	old_unit = ctx->unit;
	ctx->unit = p->unit;

	tn = find_nth(p->subtree->first_child, "PADSTACK_TYPE", 0);
	if (tn == NULL) {
		hkp_error(p->subtree, "Padstack without PADSTACK_TYPE\n");
		return NULL;
	}
	if (strcmp(tn->argv[1], "PIN_SMD") == 0) top_only = 1;
	else if (strcmp(tn->argv[1], "PIN_THROUGH") == 0) top_only = 0;
	else if (strcmp(tn->argv[1], "VIA") == 0) top_only = 0;
	else {
		hkp_error(tn, "Unknown PADSTACK_TYPE %s\n", tn->argv[1]);
		return NULL;
	}




	ts = pcb_vtpadstack_tshape_alloc_insert(&p->proto.tr, 0, 1);

	memset(ts, 0, sizeof(pcb_vtpadstack_tshape_t));
	ts->shape = calloc(sizeof(pcb_pstk_shape_t), 8); /* large enough to host all possible shapes; ->len will be smaller; when copied to the final prorotype e.g. in subcircuits, only entries used will be allocated */

	/* parse the shapes */
	for(n = n->first_child; n != NULL; n = n->next) {
		lyt_name_t *ln;
		for(ln = lyt_names; ln->name != NULL; ln++) {
			if (top_only && ((ln->lyt & PCB_LYT_TOP) == 0)) /* ignore anything but top shapes for smd pads */
				continue;
			if (strcmp(ln->name, n->argv[0]) == 0) {
				hkp_shape_t *shp;

				shp = parse_shape(ctx, n->argv[1]);
				if (shp == NULL) {
					hkp_error(n, "Undefined shape '%s'\n", n->argv[1]);
					goto error;
				}
				ts->shape[ts->len] = shp->shp;
				ts->shape[ts->len].layer_mask = ln->lyt;
				ts->shape[ts->len].comb = ln->lyc;
				ts->len++;
			}
		}
	}

	/* hole needs special threatment for two reasons:
	    - it's normally not a shape (except when it is a slot)
	    - it may have an offset on input, which will be an offset of all shapes
	      so we can keep the hole at 0;0 */
	n = find_nth(p->subtree->first_child, "TECHNOLOGY", 0);
	hn = find_nth(n->first_child, "HOLE_NAME", 0);
	if (hn != NULL) {
		hkp_hole_t *hole;

		on = find_nth(hn->first_child, "OFFSET", 0);
		if (on != NULL) {
			parse_xy(ctx, on->argv[1], &ox, &oy, 1);
			if (ox != 0) ox = -ox;
			if (oy != 0) oy = -oy;
			TODO("[easy] test this when ox;oy != 0");
		}

		hole = parse_hole(ctx, hn->argv[1]);
		if (hole == NULL) {
			hkp_error(hn, "Undefined hole '%s'\n", hn->argv[1]);
			goto error;
		}
		if (hole->dia > 0) {
			p->proto.hdia = hole->dia;
			p->proto.hplated = hole->plated;
		}
		else {
			slot_shape(&(ts->shape[ts->len]), hole->w, hole->h);
			ts->len++;
		}
		TODO("htop/hbottom: do we get bbvia span from the hole or from the padstack?");
	}

	p->proto.in_use = 1;
	p->valid = 1;
	ctx->unit = old_unit;
	return p;

	error:;
	ctx->unit = old_unit;
	return NULL;
}

static void set_pstk_clearance(hkp_ctx_t *ctx, const hkp_netclass_t *nc, pcb_pstk_t *ps, node_t *errnd)
{
	pcb_layergrp_t *clgrp = pcb_get_layergrp(ctx->pcb, pcb_layergrp_get_top_copper());
	if ((clgrp != NULL) && (clgrp->len > 0))
		ps->Clearance = net_get_clearance_(ctx, clgrp->lid[0], nc, HKP_CLR_POLY2TERM, errnd);
}

static void parse_pin(hkp_ctx_t *ctx, pcb_subc_t *subc, const hkp_netclass_t *nc, node_t *nd, int on_bottom)
{
	node_t *tmp;
	rnd_coord_t px, py;
	hkp_pstk_t *hpstk;
	rnd_cardinal_t pid;
	pcb_pstk_t *ps;
	double rot = 0;

	tmp = find_nth(nd->first_child, "XY", 0);
	if (tmp == NULL) {
		hkp_error(nd, "Ignoring pin with no coords\n");
		return;
	}

	parse_x(ctx, tmp->argv[1], &px);
	parse_y(ctx, tmp->argv[2], &py);

	tmp = find_nth(nd->first_child, "ROTATION", 0);
	if (tmp != NULL)
		parse_rot(ctx, tmp, &rot, on_bottom);

	tmp = find_nth(nd->first_child, "PADSTACK", 0);
	if (tmp == NULL) {
		hkp_error(nd, "Ignoring pin with no padstack\n");
		return;
	}
	hpstk = parse_pstk(ctx, tmp->argv[1]);
	if (hpstk == NULL) {
		hkp_error(tmp, "Ignoring pin with undefined padstack '%s'\n", tmp->argv[1]);
		return;
	}

	pid = pcb_pstk_proto_insert_dup(subc->data, &hpstk->proto, 1, 0);
	ps = pcb_pstk_alloc(subc->data);
	ps->Flags = DEFAULT_OBJ_FLAG;
	ps->x = px;
	ps->y = py;
	ps->rot = rot;
	ps->proto = pid;
	ps->xmirror = ps->smirror = on_bottom;
	pcb_pstk_add(subc->data, ps);
	pcb_attribute_put(&ps->Attributes, "term", nd->argv[1]);

	set_pstk_clearance(ctx, nc, ps, nd);
}

static int io_mentor_cell_pstks(hkp_ctx_t *ctx, const char *fn)
{
	FILE *fpstk;
	node_t *n;

	fpstk = rnd_fopen(&ctx->pcb->hidlib, fn, "r");
	if (fpstk == NULL) {
		rnd_message(RND_MSG_ERROR, "can't open padstack hkp '%s' for read\n", fn);
		return -1;
	}

	load_hkp(&ctx->padstacks, fpstk, fn);

	/* build the indices */
	htsp_init(&ctx->shapes, strhash, strkeyeq);
	htsp_init(&ctx->holes, strhash, strkeyeq);
	htsp_init(&ctx->pstks, strhash, strkeyeq);

	for(n = ctx->padstacks.root->first_child; n != NULL; n = n->next) {
		if (strcmp(n->argv[0], "UNITS") == 0) {
			ctx->pstk_unit = parse_units(n->argv[1]);
			if (ctx->pstk_unit == NULL)
				return hkp_error(n, "Unknown unit '%s'\n", n->argv[1]);
		}
		else if (strcmp(n->argv[0], "PAD") == 0) {
			if (!htsp_has(&ctx->shapes, n->argv[1])) {
				hkp_shape_t *shp = calloc(sizeof(hkp_shape_t), 1);
				shp->subtree = n;
				shp->unit = ctx->pstk_unit;
				htsp_insert(&ctx->shapes, n->argv[1], shp);
			}
			else
				return hkp_error(n, "Duplicate PAD '%s'\n", n->argv[1]);
		}
		else if (strcmp(n->argv[0], "HOLE") == 0) {
			if (!htsp_has(&ctx->holes, n->argv[1])) {
				hkp_shape_t *hole = calloc(sizeof(hkp_hole_t), 1);
				hole->subtree = n;
				hole->unit = ctx->pstk_unit;
				htsp_insert(&ctx->holes, n->argv[1], hole);
			}
			else
				return hkp_error(n, "Duplicate HOLE '%s'\n", n->argv[1]);
		}
		else if (strcmp(n->argv[0], "PADSTACK") == 0) {
			if (!htsp_has(&ctx->pstks, n->argv[1])) {
				hkp_shape_t *pstk = calloc(sizeof(hkp_pstk_t), 1);
				pstk->subtree = n;
				pstk->unit = ctx->pstk_unit;
				htsp_insert(&ctx->pstks, n->argv[1], pstk);
			}
			else
				return hkp_error(n, "Duplicate PADSTACK '%s'\n", n->argv[1]);
		}
	}

	fclose(fpstk);
	return 0;
}

static void free_pstks(hkp_ctx_t *ctx)
{
	htsp_entry_t *e;

	for(e = htsp_first(&ctx->shapes); e != NULL; e = htsp_next(&ctx->shapes, e)) {
		hkp_shape_t *shp = e->value;
		free(shp);
	}
	htsp_uninit(&ctx->shapes);

	for(e = htsp_first(&ctx->holes); e != NULL; e = htsp_next(&ctx->holes, e)) {
		hkp_hole_t *hole = e->value;
		free(hole);
	}
	htsp_uninit(&ctx->holes);

	for(e = htsp_first(&ctx->pstks); e != NULL; e = htsp_next(&ctx->pstks, e)) {
		hkp_pstk_t *pstk = e->value;
		free(pstk);
	}
	htsp_uninit(&ctx->pstks);
}
