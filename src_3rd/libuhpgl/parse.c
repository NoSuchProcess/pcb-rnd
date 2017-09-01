/*
    libuhpgl - the micro HP-GL library
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


    This library is part of pcb-rnd: http://repo.hu/projects/pcb-rnd
*/

#include <stdlib.h>
#include <ctype.h>
#include "libuhpgl.h"
#include "parse.h"

typedef enum state_e {
	ST_IDLE,
	ST_INST,
	ST_INST_END,
	ST_COORDS
} state_t;

typedef struct {
	size_t token_offs;
	size_t token_line, token_col;
	char token[32];
	int len; /* token length */
	state_t state;
	unsigned error:1;
	unsigned eof:1;
} parse_t;

static int error(uhpgl_ctx_t *ctx, const char *msg)
{
	parse_t *p = ctx->parser;

	ctx->error.offs = p->token_offs;
	ctx->error.line = p->token_line;
	ctx->error.col = p->token_col;
	ctx->error.msg = msg;

	p->error = 1;
	return -1;
}

static const char *err_beyond_end = "Character after EOF";
static const char *err_not_open   = "Parser is not open";

#define decl_parser_ctx \
	parse_t *p = ctx->parser; \
	if (p == NULL) {\
		ctx->error.msg = err_not_open; \
		return -1; \
	} \
	if (p->error) \
		return -1; \
	if (p->eof) \
		return error(ctx, err_beyond_end);

int uhpgl_parse_str(uhpgl_ctx_t *ctx, const char *str)
{
	int ret;
	decl_parser_ctx;

	for(;*str != '\0'; str++) {
		ret = uhpgl_parse_char(ctx, *str);
		if (ret != 0)
			return ret;
	}
	return 0;
}

int uhpgl_parse_file(uhpgl_ctx_t *ctx, FILE *f)
{
	int ret, c;
	decl_parser_ctx;

	while((c = fgetc(f)) != EOF) {
		ret = uhpgl_parse_char(ctx, c);
		if (ret != 0)
			return ret;
	}
	return 0;
}

int uhpgl_parse_open(uhpgl_ctx_t *ctx)
{
	if (ctx->parser != NULL) {
		ctx->error.msg = "Parser already open";
		return -1;
	}
	ctx->parser = calloc(sizeof(parse_t), 1);
	ctx->state.offs = 0;
	ctx->state.line = 1;
	ctx->state.col = 1;
	return 0;
}

int uhpgl_parse_close(uhpgl_ctx_t *ctx)
{
	decl_parser_ctx;
	if (p->state != ST_IDLE) {
		error(ctx, "premature end of stream");
		free(p);
		ctx->parser = NULL;
		return -1;
	}

	free(p);
	ctx->parser = NULL;
	return 0;
}

/*** the actual parser: high level (grammar) ***/
static int parse_inst(uhpgl_ctx_t *ctx)
{
	parse_t *p = ctx->parser;
	return error(ctx, "unimplemented instruction");
}

static int parse_coord(uhpgl_ctx_t *ctx, long int coord, int is_last)
{
	parse_t *p = ctx->parser;
	return error(ctx, "unimplemented coord instruction");
}

/*** the actual parser: low level (tokens) ***/
#define token_start() \
do { \
	p->len = 0; \
	p->token_offs = ctx->state.offs; \
	p->token_line = ctx->state.line; \
	p->token_col = ctx->state.col-1; \
} while(0)

int uhpgl_parse_char(uhpgl_ctx_t *ctx, int c)
{
	int res;
	decl_parser_ctx;

	ctx->state.col++;
	switch(c) {
		case EOF:
			p->eof = 1;
			return 0;
		case '\n': 
			ctx->state.line++;
			ctx->state.col = 1;
			return 0;
		case '\r':
		case '\t':
		case ' ':
			return 0;
	}

	switch(p->state) {
		case ST_IDLE:
			if (c == ';') /* be liberal: accept multiple terminators or empty instructions */
				return 0;
			p->state = ST_INST;
			token_start();
			/* fall through to read the first char */
		case ST_INST:
			if (!isalpha(c))
				return error(ctx, "Invalid character in instruction (must be alpha)");
			p->token[p->len] = toupper(c);
			p->len++;
			if (p->len == 2) {
				p->len = 0;
				return parse_inst(ctx);
			}
			return 0;
		case ST_INST_END:
			if (c != ';')
				return error(ctx, "Expected semicolon to terminate instruction");
			p->state = ST_IDLE;
			return 0;
		case ST_COORDS:
			if ((c == ',') || (c == ';')) {
				int last = (c == ';');
				p->token[p->len] = '\0';
				res = parse_coord(ctx, strtol(p->token, NULL, 10), last);
				token_start();
				if (last)
					p->state = ST_IDLE;
				return res;
			}
			if (!isdigit(c))
				return error(ctx, "Expected digit or separator in coordinate");
			p->token[p->len] = c;
			p->len++;
			return 0;
	}
	return error(ctx, "Internal error: broken state machine");
}
