 /*
  *                            COPYRIGHT
  *
  *  PCB, interactive printed circuit board design
  *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
  */

#define TRX(x)   (x)
#define TRY(y)   (PCB->MaxHeight - (y))

static void dxf_draw_handle(dxf_ctx_t *ctx)
{
	ctx->handle++;
	fprintf(ctx->f, "5\n%lu\n", ctx->handle);
}

static void dxf_draw_line_props(dxf_ctx_t *ctx, pcb_hid_gc_t gc)
{
	fprintf(ctx->f, "100\nAcDbEntity\n");
	fprintf(ctx->f, "8\n%s\n", ctx->layer_name); /* layer name */
	fprintf(ctx->f, "6\nByLayer\n"); /* linetype name */
	fprintf(ctx->f, "62\n256\n"); /* color; 256=ByLayer */

	/* lineweight enum (width in 0.01mm) */
	if (ctx->enable_force_thin && ctx->force_thin)
		fprintf(ctx->f, "370\n1\n");
	else
		fprintf(ctx->f, "370\n%d\n", (int)pcb_round(PCB_COORD_TO_MM(gc->width)*100.0));
}

static void dxf_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	dxf_ctx_t *ctx = &dxf_ctx;
	fprintf(ctx->f, "0\nLINE\n");
	dxf_draw_handle(ctx);
	dxf_draw_line_props(ctx, gc);
	fprintf(ctx->f, "100\nAcDbLine\n");
	pcb_fprintf(ctx->f, "10\n%mm\n20\n%mm\n", TRX(x1), TRY(y1));
	pcb_fprintf(ctx->f, "11\n%mm\n21\n%mm\n", TRX(x2), TRY(y2));
}

static void dxf_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r)
{
	dxf_ctx_t *ctx = &dxf_ctx;
	fprintf(ctx->f, "0\nCIRCLE\n");
	dxf_draw_handle(ctx);
	dxf_draw_line_props(ctx, gc);
	fprintf(ctx->f, "100\nAcDbCircle\n");
	pcb_fprintf(ctx->f, "10\n%mm\n20\n%mm\n", TRX(cx), TRY(cy));
	pcb_fprintf(ctx->f, "40\n%mm\n", r);
}

static void dxf_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	pcb_angle_t end_angle, tmp;
	dxf_ctx_t *ctx = &dxf_ctx;

	if (delta_angle > 0) {
		end_angle = start_angle-delta_angle;
		start_angle -= 90;
		end_angle -= 90;

		if (end_angle >= 360)
			end_angle -= 360;
		if (end_angle < 0)
			end_angle += 360;

		tmp = start_angle;
		start_angle = end_angle;
		end_angle = tmp;
	}
	else {
		end_angle = start_angle-delta_angle;
		start_angle += 90;
		end_angle += 90;

		if (end_angle >= 360)
			end_angle -= 360;
		if (end_angle < 0)
			end_angle += 360;
	}

	fprintf(ctx->f, "0\nARC\n");
	dxf_draw_handle(ctx);
	dxf_draw_line_props(ctx, gc);
	fprintf(ctx->f, "100\nAcDbCircle\n");
	pcb_fprintf(ctx->f, "10\n%mm\n20\n%mm\n", TRX(cx), TRY(cy));
	pcb_fprintf(ctx->f, "40\n%mm\n", (width + height)/2);
	fprintf(ctx->f, "100\nAcDbArc\n");
	fprintf(ctx->f, "50\n%f\n", start_angle);
	fprintf(ctx->f, "51\n%f\n", end_angle);
}

static void dxf_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	dxf_ctx_t *ctx = &dxf_ctx;
	int n, to;

#if HATCH_NEEDS_BBOX
	pcb_coord_t x_min, x_max, y_min, y_max;
	x_max = x_min = *x;
	y_max = y_min = *y;
	for(n = 1; n < n_coords; n++) {
		if (x[n] < x_min) x_min = x[n];
		if (x[n] > x_max) x_max = x[n];
		if (y[n] < y_min) y_min = y[n];
		if (y[n] > y_max) y_max = y[n];
	}
#endif

	fprintf(ctx->f, "0\nHATCH\n");
	dxf_draw_handle(ctx);
	dxf_draw_line_props(ctx, gc);
	fprintf(ctx->f, "100\nAcDbHatch\n");
	fprintf(ctx->f, "10\n0\n20\n0\n30\n0\n"); /* elevation */
	fprintf(ctx->f, "210\n0\n220\n0\n230\n1\n"); /* extrusion */
	fprintf(ctx->f, "2\nSOLID\n");
	fprintf(ctx->f, "70\n1\n"); /* solid fill */
	fprintf(ctx->f, "71\n0\n"); /* associativity: non */
	fprintf(ctx->f, "91\n1\n"); /* number of loops (contours) */
	fprintf(ctx->f, "92\n0\n"); /* boundary path type: default */
	fprintf(ctx->f, "93\n%d\n", n_coords);

	for(n = 0; n < n_coords; n++) {
		to = n+1;
		if (to == n_coords)
			to = 0;
		fprintf(ctx->f, "72\n1\n"); /* edge=line */
		pcb_fprintf(ctx->f, "10\n%mm\n20\n%mm\n", TRX(x[n]), TRY(y[n]));
		pcb_fprintf(ctx->f, "11\n%mm\n21\n%mm\n", TRX(x[to]), TRY(y[to]));
	}

	fprintf(ctx->f, "97\n0\n"); /* number of source boundaries */
	fprintf(ctx->f, "75\n0\n"); /* hatch style: normal, odd parity */
	fprintf(ctx->f, "76\n1\n"); /* pattern type: predefined */
	fprintf(ctx->f, "98\n0\n"); /* number of seed points */

}

static void dxf_gen_layer(dxf_ctx_t *ctx, const char *name)
{
	fprintf(ctx->f, "0\nLAYER\n");
	dxf_draw_handle(ctx);
	fprintf(ctx->f, "330\n2\n"); /* BLOCK_RECORD handle */
	fprintf(ctx->f, "100\nAcDbSymbolTableRecord\n");
	fprintf(ctx->f, "100\nAcDbLayerTableRecord\n");
	fprintf(ctx->f, "2\n%s\n", name);
	fprintf(ctx->f, "70\n0\n"); /* flags */
	fprintf(ctx->f, "62\n7\n"); /* color */
	fprintf(ctx->f, "6\nCONTINUOUS\n"); /* default line type */
	fprintf(ctx->f, "370\n15\n"); /* default line width in 0.01mm */
	fprintf(ctx->f, "390\nF\n"); /* plot style */
}
