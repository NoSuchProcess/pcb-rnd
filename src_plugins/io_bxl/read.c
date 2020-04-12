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
#include "read.h"
#include "bxl_decode.h"
#include "bxl_lex.h"
#include "bxl_gram.h"

#define SKIP if (!ctx->in_target_fp) return

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
	ctx->state.layer = -1;
}

void pcb_bxl_set_layer(pcb_bxl_ctx_t *ctx, const char *layer_name)
{
	SKIP;
	ctx->state.layer = 0;
}

void pcb_bxl_set_coord(pcb_bxl_ctx_t *ctx, int idx, pcb_coord_t val)
{
	SKIP;
	ctx->state.coord[idx] = val;
}

void pcb_bxl_add_line(pcb_bxl_ctx_t *ctx)
{
	SKIP;
	printf("Line!\n");
}


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

	memset(&bctx, 0, sizeof(bctx));
	bctx.subc = pcb_subc_new();
TODO("This reads the first footprint only:");
	bctx.in_target_fp = 1;

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
				break;
			}
			pcb_bxl_lex_reset(&lctx); /* prepare for the next token */
		}
	}

	pcb_subc_reg(data, bctx.subc);
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
