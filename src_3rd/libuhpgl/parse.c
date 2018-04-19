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
#include "uhpgl_math.h"
#include "libuhpgl.h"
#include "parse.h"
#include "arc_iterate.h"

#define inst2num(s1, s2) ((((int)s1) << 8) | (int)s2)

typedef enum state_e {
	ST_IDLE,
	ST_INST,
	ST_INST_END,
	ST_NUMBERS_OR_END,
	ST_SPC_NUMBERS_OR_END,
	ST_NUMBERS,
	ST_ESCAPED
} state_t;

typedef struct {
	size_t token_offs;
	size_t token_line, token_col;
	char inst[2];
	char token[32];
	int len; /* token length */

	double argv[32];
	int argc;

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

/*** execute ***/
static void move_to(uhpgl_ctx_t *ctx, uhpgl_coord_t x, uhpgl_coord_t y)
{
	ctx->state.at.x = x;
	ctx->state.at.y = y;
}

static int draw_line(uhpgl_ctx_t *ctx, uhpgl_coord_t x1, uhpgl_coord_t y1, uhpgl_coord_t x2, uhpgl_coord_t y2)
{
	uhpgl_line_t line;
	line.pen = ctx->state.pen;
	line.p1.x = x1;
	line.p1.y = y1;
	line.p2.x = x2;
	line.p2.y = y2;
	move_to(ctx, x2, y2);
	return ctx->conf.line(ctx, &line);
}

static int draw_arc_(uhpgl_ctx_t *ctx, uhpgl_arc_t *arc, double resolution)
{
	uhpgl_arc_it_t it;
	int cnt;
	uhpgl_point_t *p, prev;

	move_to(ctx, arc->endp.x, arc->endp.y);
	if (ctx->conf.arc != NULL)
		return ctx->conf.arc(ctx, arc);

	for(cnt = 0, p = uhpgl_arc_it_first(ctx, &it, arc, resolution); p != NULL; cnt++, p = uhpgl_arc_it_next(&it)) {
		if (cnt > 0) {
			if (draw_line(ctx, prev.x, prev.y, p->x, p->y) != 0)
				return -1;
		}
		prev = *p;
	}

	return 0;
}

static int draw_arc(uhpgl_ctx_t *ctx, uhpgl_coord_t cx, uhpgl_coord_t cy, double da, double res)
{
	uhpgl_arc_t arc;
	arc.pen = ctx->state.pen;
	arc.center.x = cx;
	arc.center.y = cy;
	arc.startp.x = ctx->state.at.x;
	arc.startp.y = ctx->state.at.y;
	arc.r = ROUND(DDIST(arc.startp.y - arc.center.y, arc.startp.x - arc.center.x));
	arc.deltaa = da;

	arc.starta = RAD2DEG(atan2(arc.startp.y - arc.center.y, arc.startp.x - arc.center.x));
	arc.enda = arc.starta + da;

	arc.endp.x = ROUND((double)cx + arc.r * cos(DEG2RAD(arc.enda)));
	arc.endp.y = ROUND((double)cy + arc.r * sin(DEG2RAD(arc.enda)));

	return draw_arc_(ctx, &arc, res);
}


static int draw_circ(uhpgl_ctx_t *ctx, uhpgl_coord_t cx, uhpgl_coord_t cy, uhpgl_coord_t r, uhpgl_coord_t res)
{
	uhpgl_arc_t arc;
	arc.pen = ctx->state.pen;
	arc.center.x = cx;
	arc.center.y = cy;
	arc.r = r;
	arc.startp.x = cx + r;
	arc.startp.y = cy;
	arc.endp.x = cx + r;
	arc.endp.y = cy;
	arc.starta = 0;
	arc.enda = 360;
	arc.deltaa = 360;
	if (ctx->conf.circ != NULL) {
		move_to(ctx, arc.endp.x, arc.endp.y);
		return ctx->conf.circ(ctx, &arc);
	}
	return draw_arc_(ctx, &arc, res);
}


/*** the actual parser: high level (grammar) ***/
static int parse_inst(uhpgl_ctx_t *ctx)
{
	parse_t *p = ctx->parser;
	p->inst[0] = p->token[0];
	p->inst[1] = p->token[1];
	switch(inst2num(p->inst[0], p->inst[1])) {
		case inst2num('I','N'):
			/* ignore */
			p->state = ST_INST_END;
			return 0;
		case inst2num('P','U'):
			ctx->state.pen_down = 0;
			p->state = ST_NUMBERS_OR_END;
			return 0;
		case inst2num('P','D'):
			ctx->state.pen_down = 1;
			p->state = ST_NUMBERS_OR_END;
			return 0;
		case inst2num('W','U'):
		case inst2num('P','W'):
			p->state = ST_NUMBERS_OR_END;
			return 0;

		case inst2num('P','A'):
		case inst2num('P','R'):
		case inst2num('S','P'):
		case inst2num('C','T'):
		case inst2num('C','I'):
		case inst2num('A','A'):
		case inst2num('A','R'):
		case inst2num('F','T'):
		case inst2num('P','T'):
/*
		case inst2num('L','T'):
		case inst2num('W','G'):
		case inst2num('E','W'):
		case inst2num('R','A'):
		case inst2num('E','A'):
		case inst2num('R','R'):
		case inst2num('E','R'):
		case inst2num('P','M'):
		case inst2num('E','P'):
		case inst2num('F','P'):
*/
			/* prepare to read coords */
			p->state = ST_NUMBERS;
			return 0;
		case inst2num('V','S'):
			p->state = ST_SPC_NUMBERS_OR_END;
			return 0;
	}
	return error(ctx, "unimplemented instruction");
}

/* Consume all current arguments and report end or go on for the next set
   of arguments; useful for vararg instructions like PA */
#define shift_all() \
do { \
	if (!is_last) {\
		p->argc = 0; \
		p->state = ST_NUMBERS; \
	} \
	else \
		p->state = ST_INST_END; \
} while(0)

static int parse_coord(uhpgl_ctx_t *ctx, double coord, int is_last)
{
	parse_t *p = ctx->parser;
	p->argv[p->argc] = coord;
	p->argc++;
	switch(inst2num(p->inst[0], p->inst[1])) {
		case inst2num('S','P'):
			if ((coord < 0) || (coord > 255))
				return error(ctx, "invalid pen index");
			ctx->state.pen = coord;
			p->state = ST_INST_END;
			return 0;
		case inst2num('C','T'):
			ctx->state.ct = coord;
			p->state = ST_INST_END;
			return 0;
		case inst2num('P','U'):
		case inst2num('P','A'):
		case inst2num('P','D'):
			p->state = ST_NUMBERS; /* make sure to load even a single pair */
			if (p->argc == 2) {
				if (ctx->state.pen_down) {
					if (draw_line(ctx, ctx->state.at.x, ctx->state.at.y, p->argv[0], p->argv[1]) < 0)
						return -1;
				}
				else
					move_to(ctx, p->argv[0], p->argv[1]);
				shift_all();
			}
			return 0;
		case inst2num('P','R'):
			if (p->argc == 2) {
				if (ctx->state.pen_down) {
					if (draw_line(ctx, ctx->state.at.x, ctx->state.at.y, ctx->state.at.x + p->argv[0], ctx->state.at.y + p->argv[1]) < 0)
						return -1;
				}
				else
					move_to(ctx, ctx->state.at.x + p->argv[0], ctx->state.at.y + p->argv[1]);
				shift_all();
			}
			return 0;
		case inst2num('C','I'):
			if ((p->argc == 2) || (is_last)) {
				p->state = ST_INST_END;
				if (draw_circ(ctx, ctx->state.at.x, ctx->state.at.y, p->argv[0], (p->argc == 2 ? p->argv[1] : -1)) < 0)
					return -1;
			}
			return 0;
		case inst2num('A','A'):
			if ((p->argc == 4) || (is_last)) {
				p->state = ST_INST_END;
				if (draw_arc(ctx, p->argv[0], p->argv[1], p->argv[2], (p->argc == 4 ? p->argv[3] : 0)) < 0)
					return -1;
			}
			return 0;
		case inst2num('A','R'):
			if ((p->argc == 4) || (is_last)) {
				p->state = ST_INST_END;
				if (draw_arc(ctx, ctx->state.at.x + p->argv[0], ctx->state.at.y + p->argv[1], p->argv[2], (p->argc == 4 ? p->argv[3] : 0)) < 0)
					return -1;
			}
			return 0;
		case inst2num('F','T'):
			if (is_last) {
				switch(p->argc) {
					case 3: ctx->state.fill.angle = p->argv[2];
					case 2: ctx->state.fill.spacing = p->argv[1];
					case 1: ctx->state.fill.type = p->argv[0]; break;
					case 0: ctx->state.fill.type = 1; break;
					default:
						return error(ctx, "Wrong number of arguments (expected 0, 1, 2 or 3)");
				}
			}
			return 0;
		case inst2num('P','T'):
			if ((p->argc == 1) && (is_last)) {
				ctx->state.fill.pen_thick = p->argv[0];
				return 0;
			}
			return error(ctx, "PT needs 1 argument");
		case inst2num('V','S'):
			if (is_last) {
				if ((p->argc == 1) || (p->argc == 2)) {
					ctx->state.pen_speed = p->argv[0];
					p->state = ST_INST_END;
					return 0;
				}
				printf("argc=%d\n", p->argc);
				return error(ctx, "VS needs 1 or 2 arguments");
			}
			return 0;
		case inst2num('W','U'): /* unknown */
			if (is_last) {
				if (p->argc == 1)
					return 0;
				return error(ctx, "WU needs 1 argument");
			}
			return 0;
		case inst2num('P','W'): /* unknown; maybe pen width? */
			if (is_last) {
				if (p->argc == 2)
					return 0;
				return error(ctx, "PW needs 2 arguments");
			}
			return 0;
	}
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
			if (c == 0x1B) { /* starting at ESC, spans till the first ':' - ignore it */
				p->state = ST_ESCAPED;
				return 0;
			}
			if (c == ';') /* be liberal: accept multiple terminators or empty instructions */
				return 0;
			p->state = ST_INST;
			p->argc = 0;
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
			got_end:;
			if (c != ';')
				return error(ctx, "Expected semicolon to terminate instruction");
			p->state = ST_IDLE;
			return 0;
		case ST_SPC_NUMBERS_OR_END:
			if (c == ' ')
				return 0;
			p->state = ST_NUMBERS;
			/* fall thru: number */
		case ST_NUMBERS_OR_END:
			if (c == ';')
				goto got_end;
			/* fall thru: number */
		case ST_NUMBERS:
			if ((c == ',') || (c == ';')) {
				char *end;
				int last = (c == ';');
				p->token[p->len] = '\0';
				res = parse_coord(ctx, strtod(p->token, &end), last);
				if (*end != '\0')
					return error(ctx, "Invalid numeric format");
				token_start();
				if ((p->state == ST_INST_END) && (!last))
					return error(ctx, "Expected semicolon");
				else if ((p->state != ST_INST_END) && (last))
					return error(ctx, "Premature semicolon");
				else if ((p->state == ST_INST_END) && (last))
					p->state = ST_IDLE; /* wanted to finish and finished */
				return res;
			}
			if (isdigit(c) || (c == '.') || ((c == '-') && (p->len == 0))) {
				p->token[p->len] = c;
				p->len++;
				return 0;
			}
			return error(ctx, "Expected digit or separator in number");
		case ST_ESCAPED:
			if (c == ':')
				p->state = ST_IDLE;
			return 0;
	}
	return error(ctx, "Internal error: broken state machine");
}
