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

#include <stdio.h>
#include <string.h>
#include <libuhpgl/libuhpgl.h>
#include <libuhpgl/parse.h>


static int dump_line(uhpgl_ctx_t *ctx, uhpgl_line_t *line)
{
	printf("line {%d}: %ld;%ld  %ld;%ld\n", line->pen, line->p1.x, line->p1.y, line->p2.x, line->p2.y);
	return 0;
}

static void dump_arc_(uhpgl_ctx_t *ctx, uhpgl_arc_t *arc, const char *type)
{
	printf("%s {%d}: %ld;%ld;%ld %f->%f (%f) (%ld;%ld->%ld;%ld)\n", type, arc->pen,
		arc->center.x, arc->center.y, arc->r,
		arc->starta, arc->enda, arc->deltaa,
		arc->startp.x, arc->startp.y, arc->endp.x, arc->endp.y);
}

static int dump_arc(uhpgl_ctx_t *ctx, uhpgl_arc_t *arc)
{
	dump_arc_(ctx, arc, "arc");
	return 0;
}


static int dump_circ(uhpgl_ctx_t *ctx, uhpgl_arc_t *circ)
{
	printf("circ {%d}: %ld;%ld;%ld\n", circ->pen,
		circ->center.x, circ->center.y, circ->r);
	return 0;
}

static int dump_poly(uhpgl_ctx_t *ctx, uhpgl_poly_t *poly)
{
/*#warning TODO*/
	return 0;
}

static int dump_wedge(uhpgl_ctx_t *ctx, uhpgl_wedge_t *wedge)
{
	dump_arc_(ctx, &wedge->arc, "wedge");
	return 0;
}

static int dump_rect(uhpgl_ctx_t *ctx, uhpgl_rect_t *rect)
{
	printf("rect {%d}: %ld;%ld  %ld;%ld\n", rect->pen, rect->p1.x, rect->p1.y, rect->p2.x, rect->p2.y);
	return 0;
}

static int print_error(uhpgl_ctx_t *ctx)
{
	fprintf(stderr, "Error on stdin:%d.%d: %s\n", ctx->error.line, ctx->error.col, ctx->error.msg);
	return -1;
}

int main()
{
	uhpgl_ctx_t ctx;

	memset(&ctx, 0, sizeof(ctx));

	ctx.conf.line  = dump_line;
	ctx.conf.arc   = dump_arc;
	ctx.conf.circ  = dump_circ;
	ctx.conf.poly  = dump_poly;
	ctx.conf.wedge = dump_wedge;
	ctx.conf.rect  = dump_rect;

	if (uhpgl_parse_open(&ctx) != 0)
		return print_error(&ctx);
	if (uhpgl_parse_file(&ctx, stdin) != 0)
		return print_error(&ctx);
	if (uhpgl_parse_close(&ctx) != 0)
		return print_error(&ctx);
	return 0;
}

