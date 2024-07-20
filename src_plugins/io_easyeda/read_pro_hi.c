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

/* EasyEDA std has a static layer assignment, layers identified by their
   integer ID, not by their name and there's no layer type saved. */
pcb_layer_type_t easypro_layer_id2type[200] = {
/*1~TopLayer*/               PCB_LYT_TOP | PCB_LYT_COPPER,
/*2~BottomLayer*/            PCB_LYT_BOTTOM | PCB_LYT_COPPER,
/*3~TopSilkLayer*/           PCB_LYT_TOP | PCB_LYT_SILK,
/*4~BottomSilkLayer*/        PCB_LYT_BOTTOM | PCB_LYT_SILK,
/*5~TopPasteMaskLayer*/      PCB_LYT_TOP | PCB_LYT_PASTE,
/*6~BottomPasteMaskLayer*/   PCB_LYT_BOTTOM | PCB_LYT_PASTE,
/*7~TopSolderMaskLayer*/     PCB_LYT_TOP | PCB_LYT_MASK,
/*8~BottomSolderMaskLayer*/  PCB_LYT_BOTTOM | PCB_LYT_MASK,
/*9~TopAssembly*/            PCB_LYT_TOP | PCB_LYT_DOC,
/*10~BottomAssembly*/        PCB_LYT_BOTTOM | PCB_LYT_DOC,
/*11~BoardOutLine*/          PCB_LYT_BOUNDARY,
/*12~Multi-Layer*/           0,
/*13~Document*/              PCB_LYT_DOC,
/*14~Mechanical*/            PCB_LYT_MECH,
/*15~Inner1*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*16~Inner2*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*17~Inner3*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*18~Inner4*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*19~Inner5*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*20~Inner6*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*21~Inner7*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*22~Inner8*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*23~Inner9*/                PCB_LYT_INTERN | PCB_LYT_COPPER,
/*24~Inner10*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*25~Inner11*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*26~Inner12*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*27~Inner13*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*28~Inner14*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*29~Inner15*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*30~Inner16*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*31~Inner17*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*32~Inner18*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*33~Inner19*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*34~Inner20*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*35~Inner21*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*36~Inner22*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*37~Inner23*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*38~Inner24*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*39~Inner25*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*40~Inner26*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*41~Inner27*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*42~Inner28*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*43~Inner29*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*44~Inner30*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*45~Inner31*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*46~Inner32*/               PCB_LYT_INTERN | PCB_LYT_COPPER,
/*47~Hole*/                  PCB_LYT_MECH,
/*48~COMPONENT_SHAPE*/       PCB_LYT_DOC,
/*49~COMPONENT_MARKING*/     PCB_LYT_DOC,
/*50~PIN_SOLDERING*/         PCB_LYT_DOC,
/*51~PIN_FLOATING*/          PCB_LYT_DOC,
/*52~COMPONENT_MODEL*/       PCB_LYT_DOC,
/*53~3DShell outline*/       0,
/*54~3DShell top*/           0,
/*55~3DShell bottom*/        0,
/*56~drill drawing*/         0,
/*57~Ratlines*/              0,
/*58~Top stiffener*/         PCB_LYT_DOC,
/*59~Bottom stiffener*/      PCB_LYT_DOC,
0
};

int easypro_layer_id2type_size = sizeof(easypro_layer_id2type) / sizeof(easypro_layer_id2type[0]);

/* load layers in a specific order so the pcb-rnd layer stack looks normal;
   these numbers are base-1 to match the layer ID in comments above */
const int easypro_layertab[] = {5, 6, 3, 1, LAYERTAB_INNER, 2, 8, 6, 4,    9, 10, 11, 12, 13, 14, 48, 49, 50,   0};
const int easypro_layertab_in_first = 15;
const int easypro_layertab_in_last = 46;



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
	gdom_node_t *__tmp__ = (nd)->value.array.child[num]; \
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
	gdom_node_t *__tmp__ = (nd)->value.array.child[num]; \
	if (__tmp__->type != GDOM_DOUBLE) { \
		error_at(ctx, nd, ("%s: wrong argument type for arg #%ld (expected double)\n", errstr, (long)num)); \
		errstmt; \
	} \
	dst = __tmp__->value.dbl; \
} while(0)

#define GET_ARG_HASH(dst, nd, num, errstr, errstmt) \
do { \
	gdom_node_t *__tmp__ = (nd)->value.array.child[num]; \
	if (__tmp__->type != GDOM_HASH) { \
		error_at(ctx, nd, ("%s: wrong argument type for arg #%ld; expected a hash\n", errstr, (long)num)); \
		errstmt; \
	} \
	dst = __tmp__; \
} while(0)

#define GET_ARG_ARRAY(dst, nd, num, errstr, errstmt) \
do { \
	gdom_node_t *__tmp__ = (nd)->value.array.child[num]; \
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

/*** parse objects: global/meta ***/

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



/*** parse objects: PAD ***/

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

	(void)r;TODO("this ignores the rounding radius");

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
		error_at(ctx, nd, ("round hole offset not yet supported;\nplease report this bug to pcb-rnd sending the file!\nfalling back to centered hole\n")); /* move all existing shapes in the opposite direction */

	*holed = dx;
	return 0;
}

static int pro_parse_slot_shape_slot(easy_read_ctx_t *ctx, pcb_pstk_shape_t *dst, double *holed, gdom_node_t *nd, double offx, double offy, double rot)
{
	rnd_coord_t w, h;

	REQ_ARGC_GTE(nd, 2, "PAD slot shape slot", return -1);
	GET_ARG_DBL(w, nd, 1, "PAD slot shape slot width", return -1);
	GET_ARG_DBL(h, nd, 2, "PAD slot shape slot height", return -1);


	dst->shape = PCB_PSSH_LINE;
	dst->layer_mask = PCB_LYT_MECH;
	dst->comb = PCB_LYC_AUTO;
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

	if (rot != 0) {
		double rad = rot / RND_RAD_TO_DEG;
		double cosa = cos(rad), sina = sin(rad);
		rnd_rotate(&dst->data.line.x1, &dst->data.line.y1, 0, 0, cosa, sina);
		rnd_rotate(&dst->data.line.x2, &dst->data.line.y2, 0, 0, cosa, sina);
	}

	if (offx != 0) {
		dst->data.line.x1 += TRR(offx);
		dst->data.line.x2 += TRR(offx);
	}
	if (offy != 0) {
		dst->data.line.y1 -= TRR(offy);
		dst->data.line.y2 -= TRR(offy);
	}

	return 0;
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
	double x, y, rot, dmask, dpaste, plating, slot_offx, slot_offy, slot_rot, holed = 0;
	gdom_node_t *shape_nd, *slot_nd = NULL;
	int is_plated, is_any, nopaste, sloti;
	pcb_pstk_shape_t shapes[9] = {0};
	pcb_layer_type_t side;
	pcb_pstk_t *pstk;
	rnd_coord_t cmask, cpaste;

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
	GET_ARG_DBL(dmask, nd, 17, "PAD mask", return -1);
	GET_ARG_DBL(dpaste, nd, 19, "PAD paste", return -1);

	if ((lid < 0) || (lid >= EASY_MAX_LAYERS)) {
		error_at(ctx, nd, ("PAD: invalid layer number %ld\n", lid));
		return -1;
	}

	if ((plating < 0) || (plating > 1))
		error_at(ctx, nd, ("PAD: invalid plating value %f\n", plating));

	is_any = (lid == (EASY_MULTI_LAYER+1));
	is_plated = (plating == 1);
	cmask = TRR(dmask);
	cpaste = TRR(dpaste);
	nopaste = is_any;

	/* create the main shape in shape[0] */
	if (pro_parse_pad_shape(ctx, &shapes[0], shape_nd) != 0)
		return -1;

	if (!is_any) {
		pcb_layer_t *layer = ctx->layers[lid];

		side = pcb_layer_flags_(layer) & PCB_LYT_ANYWHERE;
		shapes[0].layer_mask = side | PCB_LYT_COPPER;

		pcb_pstk_shape_copy(&shapes[1], &shapes[0]);
		shapes[1].layer_mask = side | PCB_LYT_MASK;
		shapes[1].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
		pcb_pstk_shape_grow_(&shapes[1], 0, cmask);

		pcb_pstk_shape_copy(&shapes[2], &shapes[0]);
		shapes[2].layer_mask = side | PCB_LYT_PASTE;
		shapes[2].comb = PCB_LYC_AUTO;
		pcb_pstk_shape_grow_(&shapes[2], 0, -cpaste);

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
		pcb_pstk_shape_grow_(&shapes[3], 0, cmask);


		pcb_pstk_shape_copy(&shapes[4], &shapes[0]);
		shapes[4].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_MASK;
		shapes[4].comb = PCB_LYC_AUTO | PCB_LYC_SUB;
		pcb_pstk_shape_grow_(&shapes[4], 0, cmask);

		if (!nopaste) {
			pcb_pstk_shape_copy(&shapes[5], &shapes[0]);
			shapes[5].layer_mask = PCB_LYT_TOP | PCB_LYT_PASTE;
			shapes[5].comb = PCB_LYC_AUTO;
			pcb_pstk_shape_grow_(&shapes[5], 0, -cpaste);

			pcb_pstk_shape_copy(&shapes[6], &shapes[0]);
			shapes[6].layer_mask = PCB_LYT_BOTTOM | PCB_LYT_PASTE;
			shapes[6].comb = PCB_LYC_AUTO;
			pcb_pstk_shape_grow_(&shapes[6], 0, -cpaste);

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

	return 0;
}


/*** parse objects: poly-objects ***/

static void pro_draw_polyarc(easy_read_ctx_t *ctx, pcb_poly_t *in_poly, double c_x, double c_y, double r, double start_rad, double delta_rad)
{
	static const double side_len_mil = 10;
	long n, steps = (r*delta_rad/side_len_mil)+1;
	double astep, cosa, sina, cr;
	rnd_coord_t ax, ay, cx, cy;
	rnd_point_t *pt;

	if (steps < 8)
		steps = 8;
	
	astep = delta_rad/(double)steps;
	cosa = cos(astep);
	sina = sin(astep);
	cx = TRX(c_x); cy = TRY(c_y);
	cr = TRR(r);
	ax = cx + cos(start_rad) * cr;
	ay = cy - sin(start_rad) * cr;

/*	rnd_trace("arc str: %ml %ml start=%f\n", ax, -ay, start_rad);*/


	for(n = 0; n < steps; n++) {
		pt = pcb_poly_point_alloc(in_poly);
		pt->X = ax;
		pt->Y = ay;
		if (n != steps-1)
			rnd_rotate(&ax, &ay, cx, cy, cosa, sina);
	}

	/* place last point accurately */
	pt = pcb_poly_point_alloc(in_poly);
	pt->X = cx + cos(start_rad+delta_rad) * cr;
	pt->Y = cy - sin(start_rad+delta_rad) * cr;

/*	rnd_trace("arc end: %ml %ml\n", pt->X, -pt->Y);*/

}

static int pro_draw_polyobj(easy_read_ctx_t *ctx, gdom_node_t *path, pcb_layer_t *layer, pcb_poly_t *in_poly, rnd_coord_t thick)
{
	int res;
	double lx, ly, x, y, r, cx, cy, ex, ey, delta, srad, erad;
	const char *cmd;
	gdom_node_t p;

#define ASHIFT(n) p.value.array.child+=n,p.value.array.used-=n

	/* cursor version of path where ->child and ->used can be modified */
	memcpy(&p, path, sizeof(p));

	if (p.value.array.child[0]->type == GDOM_DOUBLE) {
		/* parse path startpoint; L, ARC */
		GET_ARG_DBL(lx, path, 0, "POLY path lx", return -1);
		GET_ARG_DBL(ly, path, 1, "POLY path ly", return -1);
		ASHIFT(2);
	}

	/* parse path commands */
	while(p.value.array.used > 0) {
		GET_ARG_STR(cmd, &p, 0, "POLY path cmd", return -1);
		ASHIFT(1);
		if ((cmd == NULL) || (*cmd == '\0')) {
			error_at(ctx, path, ("Missing or empty path command\n"));
			return -1;
		}

		switch(*cmd) {
			case 'L':
				if (cmd[1] != '\0') goto unknown;
				while(p.value.array.used > 0) {
					REQ_ARGC_GTE(&p, 2, "POLY path L coords", return -1);
					GET_ARG_DBL(x, &p, 0, "POLY path L x", return -1);
					GET_ARG_DBL(y, &p, 1, "POLY path  :y", return -1);
					ASHIFT(2);

					if (in_poly == NULL) {
						pcb_line_t *line = pcb_line_alloc(layer);

						line->Point1.X = TRX(lx);
						line->Point1.Y = TRY(ly);
						line->Point2.X = TRX(x);
						line->Point2.Y = TRY(y);
						line->Thickness = thick;
						line->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
						line->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

						pcb_add_line_on_layer(layer, line);
					}
					else {
						rnd_point_t *pt = pcb_poly_point_alloc(in_poly);
						pt->X = TRX(x);
						pt->Y = TRY(y);
/*				rnd_trace("line: %f %f\n", x, y);*/
					}

					lx = x;
					ly = y;
					if ((p.value.array.used > 0) && (p.value.array.child[0]->type != GDOM_DOUBLE))
						break; /* proceed to read next command */
				}
				break;
			case 'C':
				if (strcmp(cmd, "CIRCLE") != 0) goto unknown;
				REQ_ARGC_GTE(&p, 3, "POLY path CIRCLE coords", return -1);
				GET_ARG_DBL(x, &p, 0, "POLY path CIRCLE x", return -1);
				GET_ARG_DBL(y, &p, 1, "POLY path CIRCLE y", return -1);
				GET_ARG_DBL(r, &p, 2, "POLY path CIRCLE r", return -1);
				ASHIFT(3);

				if (in_poly == NULL) {
					pcb_arc_t *arc = pcb_arc_alloc(layer);

					arc->X = TRX(x);
					arc->Y = TRY(y);
					arc->Width = arc->Height = TRR(r);
					arc->Thickness = thick;
					arc->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
					arc->StartAngle = 0;
					arc->Delta = 360;
					arc->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

					pcb_add_arc_on_layer(layer, arc);
				}
				else
					pro_draw_polyarc(ctx, in_poly, x, y, r, 0, 2*3.141592654);
				break;

			case 'A':
				if (strcmp(cmd, "ARC") != 0) goto unknown;

				REQ_ARGC_GTE(&p, 3, "POLY path ARC coords", return -1);
				GET_ARG_DBL(delta, &p, 0, "POLY path ARC delta angle", return -1);
				GET_ARG_DBL(ex, &p, 1, "POLY path ARC ex", return -1);
				GET_ARG_DBL(ey, &p, 2, "POLY path ARC ey", return -1);
				ASHIFT(3);

				res = arc_start_end_delta(lx, ly, ex, ey, delta, &cx, &cy, &r, &srad, &erad);
				if (res != 0) {
					error_at(ctx, path, ("Failed to work out arc parameters\nplease report this bug to pcb-rnd, with the board file included\n"));
					return -1;
				}

/*				rnd_trace("arc STR: %f %f srad: %f erad: %f\n", lx, ly, srad, erad);*/
				if (in_poly == NULL) {
					pcb_arc_t *arc = pcb_arc_alloc(layer);

					arc->X = TRX(cx);
					arc->Y = TRY(cy);
					arc->Width = arc->Height = TRR(r);
					arc->Thickness = thick;
					arc->Clearance = RND_MIL_TO_COORD(0.1); /* need to have a valid clearance so that the polygon can override it */
					arc->StartAngle = srad * RND_RAD_TO_DEG + 180;
					arc->Delta = delta;
					arc->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE);

					pcb_add_arc_on_layer(layer, arc);
				}
				else
					pro_draw_polyarc(ctx, in_poly, cx, cy, r, srad, delta / RND_RAD_TO_DEG);

				lx = ex;
				ly = ey;
/*				rnd_trace("arc END: %f %f\n", lx, ly);*/
				break;

			default:
				unknown:;
				error_at(ctx, path, ("Unknown path command '%s'\n", cmd));
				return -1;
			}
	}

#undef ASHIFT
	return 0;
}


/* polyline and other drawing objects
   "POLY","e34",  0,  "",  11,     10,   [0,0,"L",0,3937,3937,3937,3937,0,0,0] ,0
            id        net  layer  thick  path                                   locked */
static int easyeda_pro_parse_poly(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	double lid, thick, locked;
	gdom_node_t *path;
	pcb_layer_t *layer;


	REQ_ARGC_GTE(nd, 8, "POLY", return -1);
	GET_ARG_DBL(lid, nd, 4, "POLY layer", return -1);
	GET_ARG_DBL(thick, nd, 5, "POLY thickness", return -1);
	GET_ARG_ARRAY(path, nd, 6, "POLY path", return -1);
	GET_ARG_DBL(locked, nd, 7, "POLY locked", return -1);

	GET_LAYER(layer, (int)lid, nd, return -1);

	(void)locked; /* ignored for now */

	return pro_draw_polyobj(ctx, path, layer, NULL, TRR(thick));
}


/* polygon
   "FILL","e46",0, "", 50,   0.2,   0,[[-165.354,135.827,"L",-149.606,135.827,-149.606,127.953...]],0]
            id    net layer  thick     path                                                         locked */
static int easyeda_pro_parse_fill(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	double lid, dthick, locked;
	rnd_coord_t cthick;
	int n, res = 0;
	gdom_node_t *paths;
	pcb_layer_t *layer = NULL;
	pcb_poly_t *poly;


	REQ_ARGC_GTE(nd, 8, "FILL", return -1);
	GET_ARG_DBL(lid, nd, 4, "FILL layer", return -1);
	GET_ARG_DBL(dthick, nd, 5, "FILL thickness", return -1);
	GET_ARG_ARRAY(paths, nd, 7, "FILL path", return -1);
	GET_ARG_DBL(locked, nd, 8, "FILL locked", return -1);

	GET_LAYER(layer, (int)lid, nd, return -1);
	cthick = TRR(dthick);

	(void)locked; /* ignored for now */

	for(n = 0; (n < paths->value.array.used) && (res == 0); n++) {
		poly = pcb_poly_alloc(layer);

		res |= pro_draw_polyobj(ctx, paths->value.array.child[n], layer, poly, cthick);

		pcb_add_poly_on_layer(layer, poly);
		if (ctx->pcb != NULL)
			pcb_poly_init_clip(layer->parent.data, layer, poly);
	}

	return res;
}

static int pro_create_text(easy_read_ctx_t *ctx, gdom_node_t *nd, double lid, double x, double y, double anchor, double rot, double xmir, double height, double thick, int keyvis, int valvis, const char *key, const char *val)
{
	pcb_text_t *t;
	pcb_layer_t *layer;
	int acx, acy; /* -1 is left/top, 0 is center, +1 is bottom/right */
	rnd_coord_t offx, offy, cx, cy;
	int dyn = 0;

	GET_LAYER(layer, (int)lid, nd, return -1);

	switch((int)anchor) {
		case 1: acx = -1; acy = -1; break; /* left-top */
		case 2: acx = -1; acy =  0; break; /* left-middle */
		case 3: acx = -1; acy = +1; break; /* left-bottom */
		case 4: acx =  0; acy = -1; break; /* cent-top */
		case 5: acx =  0; acy =  0; break; /* cent-middle */
		case 6: acx =  0; acy = +1; break; /* cent-bottom */
		case 7: acx = +1; acy = -1; break; /* right-top */
		case 8: acx = +1; acy =  0; break; /* right-middle */
		case 9: acx = +1; acy = +1; break; /* right-bottom */
		default:
			error_at(ctx, nd, ("ATTR with invalid anchor (text alignment)\n"));
			return -1;
	}
	

	t = pcb_text_alloc(layer);
	if (t == NULL) {
		error_at(ctx, nd, ("Failed to allocate text object\n"));
		return -1;
	}

	t->X = 0;
	t->Y = 0;
	t->rot = 0;
	t->mirror_x = 0;

	if (keyvis && valvis) {
		t->TextString = rnd_strdup_printf("%s: %%a.parent.%s%%", key, key);
		dyn = 1;
	}
	else if (!keyvis && valvis) {
		t->TextString = rnd_strdup_printf("%%a.parent.%s%%", key);
		dyn = 1;
	}
	else if (keyvis && !valvis)
		t->TextString = rnd_strdup(key);

	t->Scale = height/8.0 * 15.0;
	t->thickness = TRR(thick);
	t->Flags = pcb_flag_make(PCB_FLAG_CLEARLINE | (dyn ? PCB_FLAG_DYNTEXT : 0) | PCB_FLAG_FLOATER);

	pcb_text_bbox(pcb_font(ctx->pcb, 0, 1), t);
	switch(acx) {
		case -1: offx = 0; break;
		case  0: offx = -t->BoundingBox.X2 / 2;
		case +1: offx = -t->BoundingBox.X2;
	}
	switch(acy) {
		case -1: offy = 0; break;
		case  0: offy = -t->BoundingBox.Y2 / 2;
		case +1: offy = -t->BoundingBox.Y2;
	}

	cx = TRX(x);
	cy = TRY(y);
	t->X = cx + offx;
	t->Y = cy + offy;

	if (rot != 0) {
		double rad = (xmir ? -rot : rot) / RND_RAD_TO_DEG;
		double cosa = cos(rad), sina = sin(rad);
		rnd_rotate(&t->X, &t->Y, cx, cy, cosa, sina);
	}

	t->rot = rot;
	t->mirror_x = xmir;
	pcb_text_bbox(pcb_font(ctx->pcb, 0, 1), t);

	pcb_add_text_on_layer(layer, t, pcb_font(ctx->pcb, 0, 1));

	return 0;
}

/* attribute with or without floater dyntext 
   "ATTR",  "e0",  0,  "",  3,   -321.652,-590.45,  "Price",  "",   1,       1,   "default",  45,     6,  0,  0,  3,     0,    0,       0,           0,  0
                          layer     x        y        key     val  keyvis  valvis   font     height thick ?   ?  anchor rot  invert invert-expand   xmir locked
   anchors: 1=left-top 2=left-middle 3=left-bottom 4=cent-top 5=cent-middle 6=cent-bottom 7=right-top 8=right-middle 9=right-bottom */
static int easyeda_pro_parse_attr(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	double lid, x, y, keyvis, valvis, height, thick, anchor, rot, xmir, locked;
	int mktext;
	const char *key, *val;

	REQ_ARGC_GTE(nd, 22, "ATTR", return -1);

	GET_ARG_DBL(lid, nd, 4, "ATTR layer", return -1);
	GET_ARG_DBL(x, nd, 5, "ATTR x", return -1);
	GET_ARG_DBL(y, nd, 6, "ATTR y", return -1);
	GET_ARG_STR(key, nd, 7, "ATTR key", return -1);
	GET_ARG_STR(val, nd, 8, "ATTR val", return -1);
	GET_ARG_DBL(keyvis, nd, 9, "ATTR keyvis", return -1);
	GET_ARG_DBL(valvis, nd, 10, "ATTR valvis", return -1);
	GET_ARG_DBL(height, nd, 12, "ATTR height", return -1);
	GET_ARG_DBL(thick, nd, 13, "ATTR height", return -1);
	GET_ARG_DBL(anchor, nd, 16, "ATTR anchor", return -1);
	GET_ARG_DBL(rot, nd, 17, "ATTR anchor", return -1);
	GET_ARG_DBL(xmir, nd, 20, "ATTR xmir", return -1);
	GET_ARG_DBL(locked, nd, 21, "ATTR locked", return -1);

	mktext = (x != -1) && (y != -1) && (keyvis || valvis);

	if (ctx->in_subc == NULL) {
		error_at(ctx, nd, ("ATTR without subcircuit\n"));
		return -1;
	}

	pcb_attribute_put(&ctx->in_subc->Attributes, key, val);

	if (!mktext)
		return 0;

	return pro_create_text(ctx, nd, lid, x, y, anchor, rot, xmir, height, thick, (int)keyvis, (int)valvis, key, val);
	(void)locked; /* ignored for now */
}

/* static text object
   "STRING","e73",0,  3,  -305, -510,  "foshenger",  "default",  120,     8,    0,  0,  3,  44,    0,      0,      0,    0]
                     layer  x     y     textstr        font     height  thick   ?   ?  anch rot  invert inv-exten xmir  locked  */
static int easyeda_pro_parse_string(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	double lid, x, y, height, thick, anchor, rot, xmir, locked;
	const char *textstr;

	REQ_ARGC_GTE(nd, 18, "STRING", return -1);

	GET_ARG_DBL(lid, nd, 3, "STRING layer", return -1);
	GET_ARG_DBL(x, nd, 4, "STRING x", return -1);
	GET_ARG_DBL(y, nd, 5, "STRING y", return -1);
	GET_ARG_STR(textstr, nd, 6, "STRING textstr", return -1);
	GET_ARG_DBL(height, nd, 8, "STRING height", return -1);
	GET_ARG_DBL(thick, nd, 9, "STRING height", return -1);
	GET_ARG_DBL(anchor, nd, 12, "STRING anchor", return -1);
	GET_ARG_DBL(rot, nd, 13, "STRING anchor", return -1);
	GET_ARG_DBL(xmir, nd, 16, "STRING xmir", return -1);
	GET_ARG_DBL(locked, nd, 17, "STRING locked", return -1);

	return pro_create_text(ctx, nd, lid, x, y, anchor, rot, xmir, height, thick, 1, 0, textstr, NULL);
	(void)locked; /* ignored for now */
}


/*** parse objects: dispatcher ***/

static int easyeda_pro_parse_drawing_obj(easy_read_ctx_t *ctx, gdom_node_t *nd)
{
	switch(nd->name) {
		case easy_PAD: return easyeda_pro_parse_pad(ctx, nd);
		case easy_POLY: return easyeda_pro_parse_poly(ctx, nd);
		case easy_FILL: return easyeda_pro_parse_fill(ctx, nd);
		case easy_ATTR: return easyeda_pro_parse_attr(ctx, nd);
		case easy_STRING: return easyeda_pro_parse_string(ctx, nd);

		TODO("handle these");
		case easy_LAYER_PHYS: /* physical stackup with extra info on substrate */
		case easy_NET:
		case easy_CONNECT:

		/* ignored (no support) */
		case easy_ACTIVE_LAYER:
		case easy_RULE_TEMPLATE:
		case easy_RULE:
		case easy_RULE_SELECTOR:
		case easy_PANELIZE:
		case easy_PANELIZE_STAMP:
		case easy_PANELIZE_SIDE:
		case easy_REGION: /* could draw objects on a keepout layer */
		case easy_PRIMITIVE: /* visibility per object type */
		case easy_PREFERENCE: /* tool states; L45 is refraction setting */
		case easy_SILK_OPTS: /* color-only */
		case easy_PROP: /* per object (silk) color */

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

		if (line->type != GDOM_ARRAY) continue;
		if (easyeda_pro_parse_drawing_obj(ctx, line) != 0)
			return -1;
	}

	return 0;
}


/*** glue ***/

static int easyeda_pro_parse_fp_as_board(pcb_board_t *pcb, const char *fn, FILE *f, rnd_conf_role_t settings_dest)
{
	easy_read_ctx_t ctx = {0};
	int res = 0;
	pcb_subc_t *subc_as_board;

	ctx.is_pro = 1;
	ctx.is_footprint = 1;
	ctx.pcb = pcb;
	ctx.fn = fn;
	ctx.data = pcb->Data;

	ctx.settings_dest = settings_dest;


	/* eat up the bom */
	if (easyeda_eat_bom(f, fn) != 0)
		return -1;

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
		ctx.in_subc = subc_as_board;

		res = easyeda_pro_parse_drawing_objs(&ctx, ctx.root);

		ctx.data = pcb->Data;
		easyeda_subc_finalize(&ctx, subc_as_board, 0, 0, 0);
	}

	return res;
}

/* is_footprint should be set to 0 when loading a footprint as part of a board */
static int easyeda_pro_parse_fp(pcb_data_t *data, const char *fn, int is_footprint)
{
	pcb_board_t *pcb = NULL;
	easy_read_ctx_t ctx = {0};
	int res = 0;
	pcb_subc_t *subc;
	pcb_data_t *save;

	easyeda_data_layer_reset(&pcb, data);

	ctx.is_pro = 1;
	ctx.is_footprint = is_footprint;
	ctx.pcb = NULL; /* indicate we are not loading a board but a footprint to buffer */
	ctx.data = data;
	ctx.fn = fn;
	ctx.f = rnd_fopen(&pcb->hidlib, fn, "r");
	ctx.settings_dest = -1;
	if (ctx.f == NULL) {
		rnd_message(RND_MSG_ERROR, "filed to open %s for read\n", fn);
		return -1;
	}

	/* eat up the bom */
	if (easyeda_eat_bom(ctx.f, fn) != 0)
		return -1;

	/* run low level */
	ctx.root = easypro_low_parse(ctx.f);
	fclose(ctx.f);
	if (ctx.root == NULL) {
		rnd_message(RND_MSG_ERROR, "easyeda pro: failed to run the low level parser on %s\n", fn);
		return -1;
	}

	rnd_trace("load efoo as board\n");

	assert(ctx.root->type == GDOM_ARRAY);
	if (res == 0) res = easyeda_pro_parse_canvas(&ctx);
	if (res == 0) res = easyeda_pro_parse_layers(&ctx);

	pcb_data_binding_update(pcb, data);

	save = ctx.data;
	subc = easyeda_subc_create(&ctx);
	ctx.data = subc->data;
	ctx.in_subc = subc;

	easyeda_subc_layer_bind(&ctx, subc);

	if (res == 0) res |= easyeda_pro_parse_drawing_objs(&ctx, ctx.root);

	ctx.data = save;
	easyeda_subc_finalize(&ctx, subc, 0, 0, 0);

	return res;
}
