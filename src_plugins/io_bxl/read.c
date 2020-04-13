/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - read: decode, parse, interpret
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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

#include <stdio.h>
#include <assert.h>

#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include "read.h"
#include "bxl_decode.h"
#include "bxl_lex.h"
#include "bxl_gram.h"

#define SKIP if (!ctx->in_target_fp) return

static const pcb_dflgmap_t bxl_layer_names[] = {
	/* name                 layer type                        purpose     comb         flags */
	{"TOP",                 PCB_LYT_TOP | PCB_LYT_COPPER,     NULL,       0,           0},
	{"BOTTOM",              PCB_LYT_BOTTOM | PCB_LYT_COPPER,  NULL,       0,           0},
	{"TOP_SILKSCREEN",      PCB_LYT_TOP | PCB_LYT_SILK,       NULL,       0,           0},
	{"BOTTOM_SILKSCREEN",   PCB_LYT_BOTTOM | PCB_LYT_SILK,    NULL,       0,           0},
	{"TOP_ASSEMBLY",        PCB_LYT_TOP | PCB_LYT_DOC,        "assy",     0,           0},
	{"BOTTOM_ASSEMBLY",     PCB_LYT_BOTTOM | PCB_LYT_DOC,     "assy",     0,           0},
	{"TOP_SOLDER_MASK",     PCB_LYT_TOP | PCB_LYT_MASK,       NULL,       PCB_LYC_SUB, 0},
	{"BOTTOM_SOLDER_MASK",  PCB_LYT_BOTTOM | PCB_LYT_MASK,    NULL,       PCB_LYC_SUB, 0},
	{"TOP_SOLDER_PASTE",    PCB_LYT_TOP | PCB_LYT_PASTE,      NULL,       0,           0},
	{"BOTTOM_SOLDER_PASTE", PCB_LYT_BOTTOM | PCB_LYT_PASTE,   NULL,       0,           0},

	/* these are better not drawn by default */
	{"3D_DXF",              PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"PIN_DETAIL",          PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"TOP_NO-PROBE",        PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"BOTTOM_NO-PROBE",     PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"TOP_VALOR",           PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"BOTTOM_VALOR",        PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"TOP_PLACE_BOUND",     PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"BOTTOM_PLACE_BOUND",  PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"TOP_PLACEBOUND_3",    PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"BOTTOM_PLACEBOUND_3", PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{"PLACEMENT_BODY",      PCB_LYT_VIRTUAL,                  NULL,       0,           0},
	{NULL, 0, 0, 0, 0}
};

static const pcb_dflgmap_t bxl_layer_fragments[] = {
	{"TOP_",                PCB_LYT_TOP,                      NULL,       0,           0},
	{"BOTTOM_",             PCB_LYT_BOTTOM,                   NULL,       0,           0},
	{NULL, 0, 0, 0, 0}
};


static const pcb_dflgmap_t *bcl_layer_resolve_name(const char *name)
{
	static pcb_dflgmap_t tmp;
	const pcb_dflgmap_t *n;

	for(n = bxl_layer_names; n->name != NULL; n++)
		if (strcmp(name, n->name) == 0)
			return n;

	memset(&tmp, 0, sizeof(tmp));
	for(n = bxl_layer_fragments; n->name != NULL; n++) {
		if (strstr(name, n->name) != NULL) {
			tmp.lyt |= n->lyt;
			tmp.comb |= n->comb;
			if (n->purpose != NULL)
				tmp.purpose = n->purpose;
		}
	}
	tmp.name = name;

	if ((tmp.lyt & PCB_LYT_ANYTHING) == 0)
		tmp.lyt |= PCB_LYT_DOC;

	return &tmp;
}

void pcb_bxl_pattern_begin(pcb_bxl_ctx_t *ctx, const char *name)
{
	TODO("implement");
}

void pcb_bxl_pattern_end(pcb_bxl_ctx_t *ctx)
{
	ctx->in_target_fp = 0;
}

void pcb_bxl_reset(pcb_bxl_ctx_t *ctx)
{
	SKIP;
	memset(&ctx->state, 0, sizeof(ctx->state));
}

void pcb_bxl_set_layer(pcb_bxl_ctx_t *ctx, const char *layer_name)
{
	htsp_entry_t *e;
	SKIP;
	e = htsp_getentry(&ctx->layer_name2ly, layer_name);
	if (e == NULL) {
		const pcb_dflgmap_t *lm = bcl_layer_resolve_name(layer_name);
		pcb_layer_t *ly;

		ly = pcb_subc_get_layer(ctx->subc, lm->lyt, lm->comb, 1, layer_name, pcb_true);
		htsp_set(&ctx->layer_name2ly, pcb_strdup(layer_name), ly);
		ctx->state.layer = ly;
	}
	else
		ctx->state.layer = e->value;

	if (ctx->state.delayed_poly) {
		ctx->state.poly = pcb_poly_new(ctx->state.layer, 0, pcb_flag_make(PCB_FLAG_CLEARPOLY));
		ctx->state.delayed_poly = 0;
	}
}

void pcb_bxl_set_justify(pcb_bxl_ctx_t *ctx, const char *str)
{
	/* special case for single center */
	if (pcb_strcasecmp(str, "center") == 0) { ctx->state.hjust = ctx->state.vjust = PCB_BXL_JUST_CENTER; return; }

	if (pcb_strncasecmp(str, "lower", 5) == 0)       { ctx->state.vjust = PCB_BXL_JUST_BOTTOM; str+=5; }
	else if (pcb_strncasecmp(str, "upper", 5) == 0)  { ctx->state.vjust = PCB_BXL_JUST_TOP; str+=5; }
	else if (pcb_strncasecmp(str, "center", 6) == 0) { ctx->state.vjust = PCB_BXL_JUST_CENTER; str+=6; }

	if (pcb_strncasecmp(str, "left", 4) == 0)        ctx->state.hjust = PCB_BXL_JUST_LEFT;
	else if (pcb_strncasecmp(str, "right", 5) == 0)  ctx->state.hjust = PCB_BXL_JUST_RIGHT;
	else if (pcb_strncasecmp(str, "center", 6) == 0) ctx->state.hjust = PCB_BXL_JUST_CENTER;
}

void pcb_bxl_add_property(pcb_bxl_ctx_t *ctx, pcb_any_obj_t *obj, const char *keyval)
{
	char *tmp, *val;
	const char *sep;

	SKIP;

	if (obj == NULL) {
		ctx->warn.property_null_obj++;
		return;
	}

	sep = strchr(keyval, '=');
	if (sep == NULL) {
		ctx->warn.property_nosep++;
		return;
	}

	tmp = pcb_strdup(keyval);
	tmp[sep-keyval] = '\0';
	val = tmp+(sep-keyval)+1;
	pcb_attribute_put(&obj->Attributes, tmp, val);
	free(tmp);
}


void pcb_bxl_add_line(pcb_bxl_ctx_t *ctx)
{
	pcb_coord_t width;
	SKIP;
	width = ctx->state.width;
	if (width == 0)
		width = 1;
	pcb_line_new(ctx->state.layer, 
		ctx->state.origin_x, ctx->state.origin_y,
		ctx->state.endp_x, ctx->state.endp_y,
		width, 0, pcb_flag_make(PCB_FLAG_CLEARLINE));
}

void pcb_bxl_add_arc(pcb_bxl_ctx_t *ctx)
{
	pcb_coord_t width;
	SKIP;
	width = ctx->state.width;
	if (width == 0)
		width = 1;
	pcb_arc_new(ctx->state.layer, 
		ctx->state.origin_x, ctx->state.origin_y,
		ctx->state.radius, ctx->state.radius,
		ctx->state.arc_start, ctx->state.arc_delta,
		width, 0, pcb_flag_make(PCB_FLAG_CLEARLINE), 0);
}

void pcb_bxl_poly_begin(pcb_bxl_ctx_t *ctx)
{
	SKIP;
	ctx->state.delayed_poly = 1;
}

void pcb_bxl_poly_add_vertex(pcb_bxl_ctx_t *ctx, pcb_coord_t x, pcb_coord_t y)
{
	SKIP;
	assert(ctx->state.poly != NULL);
	pcb_poly_point_new(ctx->state.poly, x + ctx->state.origin_x, y + ctx->state.origin_y);
}

static int poly_is_valid(pcb_poly_t *p)
{
	pcb_pline_t *contour = NULL;
	pcb_polyarea_t *np1 = NULL, *np = NULL;
	pcb_cardinal_t n;
	pcb_vector_t v;
	int res = 1;

	np1 = np = pcb_polyarea_create();
	if (np == NULL)
		return 0;

	/* first make initial polygon contour */
	for (n = 0; n < p->PointN; n++) {
		/* No current contour? Make a new one starting at point */
		/*   (or) Add point to existing contour */

		v[0] = p->Points[n].X;
		v[1] = p->Points[n].Y;
		if (contour == NULL) {
			if ((contour = pcb_poly_contour_new(v)) == NULL)
				goto err;
		}
		else {
			pcb_poly_vertex_include(contour->head->prev, pcb_poly_node_create(v));
		}

		/* Is current point last in contour? If so process it. */
		if (n == p->PointN - 1) {
			pcb_poly_contour_pre(contour, pcb_true);

			if (contour->Count == 0)
				return 0;

			{ /* count number of not-on-the-same-line vertices to make sure there's more than 2*/
				pcb_vnode_t *cur;
				int r;

				cur = contour->head;
				do {
					r++;
				} while ((cur = cur->next) != contour->head);
				if (r < 3)
					goto err;
			}

			/* make sure it is a positive contour (outer) or negative (hole) */
			if (contour->Flags.orient != PCB_PLF_DIR) {
				pcb_poly_contour_inv(contour);
			}

			pcb_polyarea_contour_include(np, contour);
			if (contour->Count == 0)
				return 0;
			contour = NULL;

			if (!pcb_poly_valid(np))
				res = 0;
		}
	}
	pcb_polyarea_free(&np1);
	return res;

	err:;
	pcb_polyarea_free(&np1);
	return 0;
}


void pcb_bxl_poly_end(pcb_bxl_ctx_t *ctx)
{
	SKIP;
	assert(ctx->state.poly != NULL);

	printf("poly end\n");
	if (poly_is_valid(ctx->state.poly)) {
		pcb_add_poly_on_layer(ctx->state.layer, ctx->state.poly);
/*		pcb_poly_init_clip(ctx->subc->data, ctx->state.layer, ctx->state.poly);*/
	}
	else {
		ctx->warn.poly_broken++;
		pcb_poly_free(ctx->state.poly);
	}
	ctx->state.poly = NULL;
	ctx->state.delayed_poly = 0;
}


#define WARN_CNT(_count_, args) \
do { \
	long cnt = (bctx->warn._count_); \
	if (cnt > 0) pcb_message args; \
} while(0)


static void pcb_bxl_init(pcb_bxl_ctx_t *bctx, const char *fpname)
{
	memset(bctx, 0, sizeof(pcb_bxl_ctx_t));
	bctx->subc = pcb_subc_new();
TODO("This reads the first footprint only:");
	bctx->in_target_fp = 1;
	htsp_init(&bctx->layer_name2ly, strhash, strkeyeq);
}

static void pcb_bxl_uninit(pcb_bxl_ctx_t *bctx)
{
	htsp_entry_t *e;

	/* emit all accumulated warnings */
	WARN_CNT(poly_broken,        (PCB_MSG_WARNING, "footprint contains %ld invalid polygons (polygons ignored)\n", cnt));
	WARN_CNT(property_null_obj,  (PCB_MSG_WARNING, "footprint contains %ld properties that could not be attached to any object\n", cnt));
	WARN_CNT(property_nosep,     (PCB_MSG_WARNING, "footprint contains %ld properties without separator between key and value\n", cnt));


	for(e = htsp_first(&bctx->layer_name2ly); e != NULL; e = htsp_next(&bctx->layer_name2ly, e))
		free(e->key);
	htsp_uninit(&bctx->layer_name2ly);
}

#undef WARN_CNT

/* Error is handled on the push side */
void pcb_bxl_error(pcb_bxl_ctx_t *ctx, pcb_bxl_STYPE tok, const char *s) { }

int io_bxl_parse_footprint(pcb_plug_io_t *ctx, pcb_data_t *data, const char *filename)
{
	pcb_hidlib_t *hl = NULL;
	FILE *f;
	int chr, tok, yres, ret = 0;
	hdecode_t hctx;
	pcb_bxl_ureglex_t lctx;
	pcb_bxl_yyctx_t yyctx;
	pcb_bxl_ctx_t bctx;

	f = pcb_fopen(hl, filename, "rb");
	if (f == NULL)
		return -1;

	pcb_bxl_init(&bctx, NULL);

	pcb_bxl_decode_init(&hctx);
	pcb_bxl_lex_init(&lctx, pcb_bxl_rules);
	pcb_bxl_parse_init(&yyctx);

	/* read all bytes of the binary file */
	while((chr = fgetc(f)) != EOF) {
		int n, ilen;

		/* feed the binary decoding */
		ilen = pcb_bxl_decode_char(&hctx, chr);
		assert(ilen >= 0);
		if (ilen == 0)
			continue;

		/* feed the lexer */
		for(n = 0; n < ilen; n++) {
			pcb_bxl_STYPE lval;
			tok = pcb_bxl_lex_char(&lctx, &lval, hctx.out[n]);
			if (tok == UREGLEX_MORE)
				continue;

			/* feed the grammar */
			lval.line = lctx.loc_line[0];
			lval.first_col = lctx.loc_col[0];
			yres = pcb_bxl_parse(&yyctx, &bctx, tok, &lval);
			if (yres != 0) {
				printf("Syntax error at %ld:%ld\n", lval.line, lval.first_col);
				ret = -1;
				if (bctx.subc != NULL)
					pcb_subc_free(bctx.subc);
				goto error;
			}
			pcb_bxl_lex_reset(&lctx); /* prepare for the next token */
		}
	}

	pcb_subc_reg(data, bctx.subc);

	error:;
	pcb_bxl_uninit(&bctx);
	fclose(f);
	return ret;
}

int io_bxl_test_parse2(pcb_hidlib_t *hl, pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *filename, FILE *f_ignore)
{
	FILE *f;
	int chr, tok, found_tok = 0, ret = 0;
	hdecode_t hctx;
	pcb_bxl_ureglex_t lctx;
	
	f = pcb_fopen(hl, filename, "rb"); /* need to open it again, for binary access */
	if (f == NULL)
		return 0;

	pcb_bxl_decode_init(&hctx);
	pcb_bxl_lex_init(&lctx, pcb_bxl_rules);

	/* read all bytes of the binary file */
	while((chr = fgetc(f)) != EOF) {
		int n, ilen;

		/* feed the binary decoding */
		ilen = pcb_bxl_decode_char(&hctx, chr);
		assert(ilen >= 0);
		if (ilen == 0)
			continue;

		/* feed the lexer */
		for(n = 0; n < ilen; n++) {
			pcb_bxl_STYPE lval;
			tok = pcb_bxl_lex_char(&lctx, &lval, hctx.out[n]);
			if (tok == UREGLEX_MORE)
				continue;

			/* simplified "grammar": find opening tokens, save the next token
			   as ID and don't do anything until finding the closing token */

			switch(found_tok) {
				/* found an opening token, tok is the ID */
				case T_PADSTACK:
					pcb_trace("BXL testparse; padstack '%s'\n", lval.un.s);
					found_tok = T_ENDPADSTACK;
					break;
				case T_PATTERN:
					pcb_trace("BXL testparse; footprint '%s'\n", lval.un.s);
					if (typ & PCB_IOT_FOOTPRINT)
						ret++;
					found_tok = T_ENDPATTERN;
					break;
				case T_SYMBOL:    found_tok = T_ENDSYMBOL; break;
				case T_COMPONENT: found_tok = T_ENDCOMPONENT; break;

				default:;
					switch(tok) {
						/* closing token: watch for an open again */
						case T_ENDPADSTACK:
						case T_ENDPATTERN:
						case T_ENDSYMBOL:
						case T_ENDCOMPONENT:
							found_tok = 0;
							break;

						/* outside of known context */
							case T_PADSTACK:
							case T_PATTERN:
							case T_COMPONENT:
							case T_SYMBOL:
								if (found_tok == 0) /* do not allow nested opens */
									found_tok = tok;
								break;
							default:;
								/* ignore anything else */
								break;
					}
					break;
			}
			pcb_bxl_lex_reset(&lctx); /* prepare for the next token */
		}
	}

	fclose(f);
	return ret;
}

int io_bxl_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *filename, FILE *f)
{
	return io_bxl_test_parse2(NULL, ctx, typ, filename, f);
}
