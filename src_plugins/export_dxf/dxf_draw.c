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

static void dxf_draw_line_props(dxf_ctx_t *ctx)
{
	fprintf(ctx->f, "100\nAcDbEntity\n");
	fprintf(ctx->f, "8\n0\n"); /* layer name */
	fprintf(ctx->f, "6\nByLayer\n"); /* linetype name */
	fprintf(ctx->f, "62\n256\n"); /* color; 256=ByLayer */
	fprintf(ctx->f, "370\n-1\n"); /* lineweight enum (width in mm*100?) */
}

static void dxf_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	dxf_ctx_t *ctx = &dxf_ctx;
	fprintf(ctx->f, "0\nLINE\n");
	dxf_draw_handle(ctx);
	dxf_draw_line_props(ctx);
	fprintf(ctx->f, "100\nAcDbLine\n");
	pcb_fprintf(ctx->f, "10\n%mm\n20\n%mm\n", TRX(x1), TRY(y1));
	pcb_fprintf(ctx->f, "11\n%mm\n21\n%mm\n", TRX(x2), TRY(y2));
}

static void dxf_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t r)
{
	dxf_ctx_t *ctx = &dxf_ctx;
	fprintf(ctx->f, "0\nCIRCLE\n");
	dxf_draw_handle(ctx);
	dxf_draw_line_props(ctx);
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
	dxf_draw_line_props(ctx);
	fprintf(ctx->f, "100\nAcDbCircle\n");
	pcb_fprintf(ctx->f, "10\n%mm\n20\n%mm\n", TRX(cx), TRY(cy));
	pcb_fprintf(ctx->f, "40\n%mm\n", (width + height)/2);
	fprintf(ctx->f, "100\nAcDbArc\n");
	fprintf(ctx->f, "50\n%f\n", start_angle);
	fprintf(ctx->f, "51\n%f\n", end_angle);
}

