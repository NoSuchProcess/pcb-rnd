/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  EasyEDA IO plugin - dispatch pro format read
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

#define REQ_ARGC_(nd, op, num, errstr, errstmt) \
do { \
	if (nd->type != GDOM_ARRAY) { \
		error_at(ctx, nd, ("%s: object node is not an array\n", errstr)); \
		errstmt; \
	} \
	if (nd->value.array.used op num) { \
		error_at(ctx, nd, ("%s: not enough fields: need at least %ld, got %ld\n", errstr, (long)num, nd->value.array.used)); \
		errstmt; \
	} \
} while(0)

#define REQ_ARGC_GTE(nd, num, errstr, errstmt) REQ_ARGC_((nd), <, (num), (errstr), errstmt)
#define REQ_ARGC_EQ(nd, num, errstr, errstmt)  REQ_ARGC_((nd), !=, (num), (errstr), errstmt)

/* call these only after a REQ_ARGC_* as it won't do bound check */
#define GET_ARG_STR(dst, nd, num, errstr, errstmt) \
do { \
	gdom_node_t *__tmp__ = nd->value.array.child[num]; \
	if ((__tmp__->type == GDOM_DOUBLE) && (__tmp__->value.dbl == -1)) {\
		dst = NULL; \
	} \
	else { \
		if (__tmp__->type != GDOM_STRING) { \
			error_at(ctx, nd, ("%s: wrong argument type for arg #%ld (expected string)\n", errstr, (long)num)); \
			errstmt; \
		} \
		dst = __tmp__->value.str; \
	} \
} while(0)

#define GET_ARG_DBL(dst, nd, num, errstr, errstmt) \
do { \
	gdom_node_t *__tmp__ = nd->value.array.child[num]; \
	if (__tmp__->type != GDOM_DOUBLE) { \
		error_at(ctx, nd, ("%s: wrong argument type for arg #%ld (expected double)\n", errstr, (long)num)); \
		errstmt; \
	} \
	dst = __tmp__->value.dbl; \
} while(0)

#define GET_ARG_HASH(dst, nd, num, errstr, errstmt) \
do { \
	gdom_node_t *__tmp__ = nd->value.array.child[num]; \
	if (__tmp__->type != GDOM_HASH) { \
		error_at(ctx, nd, ("%s: wrong argument type for arg #%ld; expected a hash\n", errstr, (long)num)); \
		errstmt; \
	} \
	dst = __tmp__; \
} while(0)

#define GET_ARG_ARRAY(dst, nd, num, errstr, errstmt) \
do { \
	gdom_node_t *__tmp__ = nd->value.array.child[num]; \
	if (__tmp__->type != GDOM_ARRAY) { \
		error_at(ctx, nd, ("%s: wrong argument type for arg #%ld; expected an array\n", errstr, (long)num)); \
		errstmt; \
	} \
	dst = __tmp__; \
} while(0)

#define CHK_ARG_KW(nd, num, kwval, errstr, errstmt) \
do { \
	const char *__str__;\
	GET_ARG_STR(__str__, nd, num, errstr, errstmt); \
	if (strcmp(__str__, kwval) != 0) { \
		error_at(ctx, nd, ("%s: arg #%ld must be '%s' but is '%s'\n", errstr, (long)num, kwval, __str__)); \
		errstmt; \
	}\
} while(0)

/*** parse objects ***/

static int easyeda_pro_parse_canvas(easy_read_ctx_t *ctx)
{
	long lineno;
	int found = 0;

	for(lineno = 0; lineno < ctx->root->value.array.used; lineno++) {
		gdom_node_t *line = ctx->root->value.array.child[lineno];

		if ((line->type != GDOM_ARRAY) || (line->name != easy_CANVAS)) continue;

		if (found) {
			error_at(ctx, line, ("multiple CANVAS nodes\n"));
			return -1;
		}
		ctx->ox = easyeda_get_double(ctx, line->value.array.child[1]);
		ctx->oy = easyeda_get_double(ctx, line->value.array.child[2]);
		found = 1;
	}

	if (!found)
		error_at(ctx, ctx->root, ("no CANVAS node; origin unknown (assuming 0;0)\n"));

	return 0;
}

static int pro_parse_layer(easy_read_ctx_t *ctx, gdom_node_t *nd, pcb_layer_type_t lyt, int easyeda_id)
{
	unsigned flags;
	int locked, visible, confd;

	if (nd == NULL)
		return 0;   /* not in the input */
	if (lyt == 0)
		return 0; /* not a valid entry in our layertab */

	flags = easyeda_get_double(ctx, nd->value.array.child[4]);
	confd   = flags & 1;
	locked  = flags & 2;
	visible = flags & 4;

	if (!confd)
		return 0; /* input did not want this layer */

	if (nd->value.array.used < 6) {
		error_at(ctx, nd, ("not enough LAYER arguments\n"));
		return -1;
	}

	if (nd->value.array.child[3]->type != GDOM_STRING) {
		error_at(ctx, nd->value.array.child[3], ("LAYER name must be a string\n"));
		return -1;
	}
	if (nd->value.array.child[5]->type != GDOM_STRING) {
		error_at(ctx, nd->value.array.child[5], ("LAYER color must be a string\n"));
		return -1;
	}

	return easyeda_layer_create(ctx, lyt, nd->value.array.child[3]->value.str, easyeda_id, nd->value.array.child[5]->value.str);

	(void)visible; (void)locked; /* supress compiler warnings on currently unused vars */
}

static int easyeda_pro_parse_layers(easy_read_ctx_t *ctx)
{
	long lineno;
	gdom_node_t *lyline[EASY_MAX_LAYERS] = {0};
	int res = 0;
	const int *lt;

	/* cache each layer line per ID from input */
	for(lineno = 0; lineno < ctx->root->value.array.used; lineno++) {
		gdom_node_t *line = ctx->root->value.array.child[lineno];
		long lid;

		if ((line->type != GDOM_ARRAY) || (line->name != easy_LAYER)) continue;

		lid = easyeda_get_double(ctx, line->value.array.child[1]);
		if ((lid >= 0) && (lid < EASY_MAX_LAYERS))
			lyline[lid] = line;
	}


	/* create layers in ctx->data */
	for(lt = easyeda_layertab; *lt != 0; lt++) {
		int easyeda_id = *lt;
		if (easyeda_id == LAYERTAB_INNER) {
			long n;
			for(n = easyeda_layertab_in_first; n <= easyeda_layertab_in_last; n++) {
				easyeda_id = n;
				res |= pro_parse_layer(ctx, lyline[easyeda_id], easyeda_layer_id2type[easyeda_id-1], easyeda_id);
			}
		}
		else
			res |= pro_parse_layer(ctx, lyline[easyeda_id], easyeda_layer_id2type[easyeda_id-1], easyeda_id);
	}

	res |= easyeda_create_misc_layers(ctx);

	return 0;
}




/* pad shape ELLIPSE:  ELLIPSE, xdia, ydia
   (fill circle, xdia == ydia always) */
static int pro_parse_pad_shape_ellipse(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, gdom_node_t *nd)
{
	double dx, dy;
	REQ_ARGC_GTE(nd, 3, "PAD shape ellipse", return -1);
	GET_ARG_DBL(dx, nd, 1, "PAD shape ellipse x diameter", return -1);
	GET_ARG_DBL(dy, nd, 2, "PAD shape ellipse y diameter", return -1);
	if (dx != dy)
		error_at(ctx, nd, ("real ellipse pad shape not yet supported;\nplease report this bug to pcb-rnd sending the file!\nfalling back to circle\n\n"));

	dst->shape = PCB_PSSH_CIRC;
	dst->data.circ.dia = TRR(dx);
	return 0;
}



/* pad shape OVAL:  OVAL, width, height
   (line in the direction of the bigger size) */
static int pro_parse_pad_shape_oval(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, gdom_node_t *nd)
{
	double w, h;

	REQ_ARGC_GTE(nd, 3, "PAD shape oval", return -1);
	GET_ARG_DBL(w, nd, 1, "PAD shape oval width", return -1);
	GET_ARG_DBL(h, nd, 2, "PAD shape oval height", return -1);

	if (w == h) {
		dst->shape = PCB_PSSH_CIRC;
		dst->data.circ.dia = TRR(w);
		return 0;
	}

	dst->shape = PCB_PSSH_LINE;
	if (h > w) {
		dst->data.line.x1 = 0;
		dst->data.line.y1 = -TRR(h/2.0)+TRR(w/2.0);
		dst->data.line.x2 = 0;
		dst->data.line.y2 = +TRR(h/2.0)-TRR(w/2.0);
		dst->data.line.thickness = TRR(w);
	}
	else {
		dst->data.line.x1 = -TRR(w/2.0)+TRR(h/2.0);
		dst->data.line.y1 = 0;
		dst->data.line.x2 = +TRR(w/2.0)-TRR(h/2.0);
		dst->data.line.y2 = 0;
		dst->data.line.thickness = TRR(h);
	}
	return 0;
}


/* pad shape RECT:  RECT, width, height, r%
   r% is corner radius in percent of the smaller side/2, 100% turning it into a line */
static int pro_parse_pad_shape_rect(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, gdom_node_t *nd)
{
	rnd_coord_t w, w2, h, h2, r;

	REQ_ARGC_GTE(nd, 4, "PAD shape rect", return -1);
	GET_ARG_DBL(w, nd, 1, "PAD shape rect width", return -1);
	GET_ARG_DBL(h, nd, 2, "PAD shape rect height", return -1);
	GET_ARG_DBL(r, nd, 3, "PAD shape rect rounding radius", return -1);

	TODO("this ignores the rounding radius");

	w2 = TRR(w/2.0);
	h2 = TRR(h/2.0);
	dst->shape = PCB_PSSH_POLY;
	pcb_pstk_shape_alloc_poly(&dst->data.poly, 4);
	dst->data.poly.x[0] = -w2; dst->data.poly.y[0] = -h2;
	dst->data.poly.x[1] = +w2; dst->data.poly.y[1] = -h2;
	dst->data.poly.x[2] = +w2; dst->data.poly.y[2] = +h2;
	dst->data.poly.x[3] = -w2; dst->data.poly.y[3] = +h2;
	dst->data.poly.len = 4;

	return 0;
}

static int pro_parse_pad_shape(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, gdom_node_t *nd)
{
	const char *shname;
	REQ_ARGC_GTE(nd, 1, "PAD shape", return -1);
	GET_ARG_STR(shname, nd, 0, "PAD shape name", return -1);

	if (strcmp(shname, "ELLIPSE") == 0) return pro_parse_pad_shape_ellipse(ctx, dst, nd);
	if (strcmp(shname, "RECT") == 0) return pro_parse_pad_shape_rect(ctx, dst, nd);
	if (strcmp(shname, "OVAL") == 0) return pro_parse_pad_shape_oval(ctx, dst, nd);

	error_at(ctx, nd, ("Invalid pad shape name: '%s'\n", shname));
	return -1;
}

static int pro_parse_slot_shape_round(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, double *holed, gdom_node_t *nd, double offx, double offy, double rot)
{
	double dx, dy;
	REQ_ARGC_GTE(nd, 3, "PAD slot shape round", return -1);
	GET_ARG_DBL(dx, nd, 1, "PAD slot shape round x diameter", return -1);
	GET_ARG_DBL(dy, nd, 2, "PAD slot shape round y diameter", return -1);
	if (dx != dy)
		error_at(ctx, nd, ("real ellipse slot shape not yet supported;\nplease report this bug to pcb-rnd sending the file!\nfalling back to circle\n\n"));
	if ((offx != 0) || (offy != 0))
		error_at(ctx, nd, ("round hole offset not yet supported;\nplease report this bug to pcb-rnd sending the file!\nfalling back to centered hole\n"));

	*holed = dx;
	return 0;
}

static int pro_parse_slot_shape_slot(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, double *holed, gdom_node_t *nd, double offx, double offy, double rot)
{
	rnd_coord_t w, w2, h, h2, r;

	REQ_ARGC_GTE(nd, 2, "PAD slot shape slot", return -1);
	GET_ARG_DBL(w, nd, 1, "PAD slot shape slot width", return -1);
	GET_ARG_DBL(h, nd, 2, "PAD slot shape slot height", return -1);

	error_at(ctx, nd, ("TODO: slot\n"));
	return -1;
}

static int pro_parse_slot_shape(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, double *holed, gdom_node_t *nd, double offx, double offy, double rot)
{
	const char *slname;
	REQ_ARGC_GTE(nd, 1, "PAD slot shape", return -1);
	GET_ARG_STR(slname, nd, 0, "PAD slot shape name", return -1);

	if (strcmp(slname, "ROUND") == 0) return pro_parse_slot_shape_round(ctx, dst, holed, nd, offx, offy, rot);
	if (strcmp(slname, "SLOT") == 0) return pro_parse_slot_shape_slot(ctx, dst, holed, nd, offx, offy, rot);

	error_at(ctx, nd, ("Invalid pad slot shape name: '%s'\n", slname));
	return -1;

}


/* "PAD","e6",0,"", 1, "5",75.003,107.867,0,  null,   ["OVAL",23.622,59.843],[],  0,  0,  null,  1,       0,    2,2,  0,0,   0,   null,null,null,null,[]
          id        ly num  x      y      rot [slot]  [ shape ]                  ofx ofy  rot    plate          mask  paste  lock
                                                                                 \---slot---/
   ly==12 means all layers (EASY_MULTI_LAYER+1) */
static int easyeda_pro_parse_pad(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	const char *termid;
	long lid;
	double x, y, rot, mask, paste, plating, slot_offx, slot_offy, slot_rot, holed = 0;
	gdom_node_t *shape_nd, *slot_nd = NULL;
	int is_plated, is_any, nopaste, sloti;
	pcb_pstk_shape_t shapes[9] = {0};
	pcb_layer_type_t side;
	pcb_pstk_t *pstk;
	const char *netname;

	REQ_ARGC_GTE(nd, 23, "PAD", return -1);
	GET_ARG_DBL(lid, nd, 4, "PAD layer", return -1);
	GET_ARG_STR(termid, nd, 5, "PAD number", return -1);
	GET_ARG_DBL(x, nd, 6, "PAD x", return -1);
	GET_ARG_DBL(y, nd, 7, "PAD y", return -1);
	GET_ARG_DBL(rot, nd, 8, "PAD rotation", return -1);
	if (nd->value.array.child[9]->type == GDOM_ARRAY) GET_ARG_ARRAY(slot_nd, nd, 9, "PAD slot shape", return -1);
	GET_ARG_ARRAY(shape_nd, nd, 10, "PAD shape", return -1);
	GET_ARG_DBL(slot_offx, nd, 12, "PAD slot offset x", return -1);
	GET_ARG_DBL(slot_offy, nd, 13, "PAD slot offset y", return -1);
	GET_ARG_DBL(slot_rot, nd, 14, "PAD slot rotation", return -1);
	GET_ARG_DBL(plating, nd, 15, "PAD plating", return -1);
	GET_ARG_DBL(mask, nd, 17, "PAD mask", return -1);
	GET_ARG_DBL(paste, nd, 19, "PAD paste", return -1);

	if ((lid < 0) || (lid >= EASY_MAX_LAYERS)) {
		error_at(ctx, nd, ("PAD: invalid layer number %ld\n", lid));
		return -1;
	}

	if ((plating < 0) || (plating > 1))
		error_at(ctx, nd, ("PAD: invalid plating value %f\n", plating));

	is_any = (lid == (EASY_MULTI_LAYER+1));
	is_plated = (plating == 1);
	TODO("figure nopaste"); nopaste = 0;
	TODO("figure netname"); netname = NULL;

	/* create the main shape in shape[0] */
	if (pro_parse_pad_shape(ctx, &shapes[0], shape_nd) != 0)
		return -1;

	TODO("use local vars mask and shape here");
	if (!is_any) {
		pcb_layer_t *layer = ctx->layers[lid];

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

	if (slot_nd != NULL) {
		/* load slot or hole */
		if (pro_parse_slot_shape(ctx, &shapes[sloti], &holed, slot_nd, slot_offx, slot_offy, slot_rot) != 0)
			return -1;
	}

	pstk = pcb_pstk_new_from_shape(ctx->data, TRX(x), TRY(y), TRR(holed), is_plated, 0, shapes);
	if (pstk == NULL) {
		error_at(ctx, nd, ("Failed to create padstack for pad\n"));
		return -1;
	}
	pstk->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
	pstk->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

	if (rot != 0) {
		double rad = rot / RND_RAD_TO_DEG;
		pcb_pstk_rotate(pstk, TRX(x), TRY(y), cos(rad), sin(rad), rot);
	}

	pcb_attribute_put(&pstk->Attributes, "term", termid);

	TODO("this could be common with std");
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


static int easyeda_pro_parse_drawing_obj(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	switch(nd->name) {
		case easy_PAD: return easyeda_pro_parse_pad(ctx, nd);

		TODO("handle these");
		case easy_ATTR:
		case easy_FILL:
		case easy_POLY:
		case easy_LAYER_PHYS:
		case easy_NET:
		case easy_PRIMITIVE:
		case easy_SILK_OPTS:
		case easy_CONNECT:
		case easy_PREFERENCE:

		/* ignored (no support) */
		case easy_ACTIVE_LAYER:
		case easy_RULE_TEMPLATE:
		case easy_RULE:
		case easy_RULE_SELECTOR:
		case easy_PANELIZE:
		case easy_PANELIZE_STAMP:
		case easy_PANELIZE_SIDE:

		/* already handled elsewhere (not drawing objects) */
		case easy_DOCTYPE:
		case easy_CANVAS:
		case easy_LAYER:
			return 0;
	}
	if ((nd->type == GDOM_ARRAY) && (nd->value.array.used > 0) && (nd->value.array.child[0]->type == GDOM_STRING))
		error_at(ctx, nd, ("unknown object '%s' - ignoring\n", nd->value.array.child[0]->value.str));
	return 0; /* ignore unknowns for now */
}


static int easyeda_pro_parse_drawing_objs(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	long lineno;

	assert(nd->type == GDOM_ARRAY);

	for(lineno = 0; lineno < nd->value.array.used; lineno++) {
		gdom_node_t *line = ctx->root->value.array.child[lineno];
		long lid;

		if (line->type != GDOM_ARRAY) continue;
		if (easyeda_pro_parse_drawing_obj(ctx, line) != 0)
			return -1;
	}

	return 0;
}


/*** glue ***/
static int easyeda_pro_parse_fp_as_board(pcb_board_t *pcb, const char *fn, FILE *f, rnd_conf_role_t settings_dest)
{
	easy_read_ctx_t ctx;
	unsigned char ul[3];
	int res = 0;
	pcb_subc_t *subc_as_board;

	ctx.is_pro = 1;
	ctx.pcb = pcb;
	ctx.fn = fn;
	ctx.data = pcb->Data;

	ctx.settings_dest = settings_dest;


	/* eat up the bom */
	if (fread(ul, 1, 3, f) != 3) {
		rnd_message(RND_MSG_ERROR, "easyeda pro: premature EOF on %s (bom)\n", fn);
		return -1;
	}
	if ((ul[0] != 0xef) || (ul[1] != 0xbb) || (ul[2] != 0xbf))
		rewind(f);

	/* run low level */
	ctx.root = easypro_low_parse(f);
	if (ctx.root == NULL) {
		rnd_message(RND_MSG_ERROR, "easyeda pro: failed to run the low level parser on %s\n", fn);
		return -1;
	}

	rnd_trace("load efoo as board\n");

	assert(ctx.root->type == GDOM_ARRAY);
	if (res == 0) res = easyeda_pro_parse_canvas(&ctx);
	if (res == 0) res = easyeda_pro_parse_layers(&ctx);

	if (res == 0) {
		subc_as_board = easyeda_subc_create(&ctx);
		ctx.data = subc_as_board->data;

		res = easyeda_pro_parse_drawing_objs(&ctx, ctx.root);

		ctx.data = pcb->Data;
		easyeda_subc_finalize(&ctx, subc_as_board, 0, 0, 0);
	}

	return res;
}
