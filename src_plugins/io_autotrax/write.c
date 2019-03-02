/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2016, 2017 Erich S. Heinzle
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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
 *
 */
#include <math.h>
#include "config.h"
#include "compat_misc.h"
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "netlist2.h"
#include "data.h"
#include "write.h"
#include "layer.h"
#include "polygon_offs.h"
#include "../lib_polyhelp/polyhelp.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"


typedef struct {
	pcb_layer_type_t lyt;
	pcb_bool plane;
} layer_map_t;

/* The hardwired layer map of autotrax */
#define LAYER_MAP_LEN 14
static layer_map_t layer_map[LAYER_MAP_LEN] = {
	/*  0 */ { 0,                               0}, /* unused */
	/*  1 */ { PCB_LYT_TOP    | PCB_LYT_COPPER, 0}, /* "top" */
	/*  2 */ { PCB_LYT_INTERN | PCB_LYT_COPPER, 0},
	/*  3 */ { PCB_LYT_INTERN | PCB_LYT_COPPER, 0},
	/*  4 */ { PCB_LYT_INTERN | PCB_LYT_COPPER, 0},
	/*  5 */ { PCB_LYT_INTERN | PCB_LYT_COPPER, 0},
	/*  6 */ { PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0}, /* "bottom" */
	/*  7 */ { PCB_LYT_TOP    | PCB_LYT_SILK,   0}, /* "top overlay" */
	/*  8 */ { PCB_LYT_BOTTOM | PCB_LYT_SILK,   0}, /* "bottom overlay" */
	/*  9 */ { PCB_LYT_INTERN | PCB_LYT_COPPER, 1}, /* "ground plane" */
	/* 10 */ { PCB_LYT_INTERN | PCB_LYT_COPPER, 1}, /* "power plane" */
	/* 11 */ { PCB_LYT_BOUNDARY,                0}, /* "board layer" */
	/* 12 */ { 0,                               0}, /* "keepout" */
	/* 13 */ { 0,                               0}, /* "multi layer" - used to indicate padstacks are on all layers */
};

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	pcb_layergrp_t *id2grp[LAYER_MAP_LEN];
	int grp2id[PCB_MAX_LAYERGRP];
} wctx_t;

static int wrax_layer2id(wctx_t *ctx, pcb_layer_t *ly)
{
	pcb_layergrp_id_t gid = pcb_layer_get_group_(ly);
	if ((gid >= 0) && (gid < PCB_MAX_LAYERGRP))
		return ctx->grp2id[gid];
	return 0;
}

static pcb_layergrp_t *wrax_id2grp(wctx_t *ctx, int alayer_id)
{
	if ((alayer_id <= 0) || (alayer_id >= LAYER_MAP_LEN))
		return NULL;
	return ctx->id2grp[alayer_id];
}

static int wrax_lyt2id(wctx_t *ctx, pcb_layer_type_t lyt)
{
	int n;
	for(n = 1; n < LAYER_MAP_LEN; n++)
		if (layer_map[n].lyt == lyt)
			return n;
	return 0;
}

static void wrax_map_layer_error(wctx_t *ctx, pcb_layergrp_t *grp, const char *msg, const char *hint)
{
	char tmp[256];
	pcb_snprintf(tmp, sizeof(tmp), "%s (omitting layer group): %s", msg, grp->name);
	pcb_io_incompat_save(ctx->pcb->Data, NULL, "layer", tmp, hint);
}

static int wrax_map_layers(wctx_t *ctx)
{
	int n, intcnt = 0;

	for(n = 0; n < ctx->pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[n];
		int al;

		if ((grp->ltype & PCB_LYT_SUBSTRATE) || (grp->ltype & PCB_LYT_VIRTUAL) || (grp->ltype & PCB_LYT_PASTE) || (grp->ltype & PCB_LYT_MASK))
			continue;

		al = wrax_lyt2id(ctx, grp->ltype);
		if (al == 0) {
			wrax_map_layer_error(ctx, grp, "Unable to map pcb-rnd layer group to autotrax layer", "change layer type");
			continue;
		}

		if (grp->ltype & PCB_LYT_INTERN) {
			/* intern copper: find the first free slot */
			while((layer_map[al+intcnt].lyt & PCB_LYT_INTERN) && (ctx->id2grp[al+intcnt] != NULL)) intcnt++;
			al += intcnt;
			if (!(layer_map[al+intcnt].lyt & PCB_LYT_INTERN)) {
				wrax_map_layer_error(ctx, grp, "Ran out of internal layer groups while mapping pcb-rnd layer group to autotrax layer", "autotrax supports only 4 internal signal layers - use less internal layers");
				continue;
			}
		}

		if (ctx->id2grp[al] != NULL) {
			wrax_map_layer_error(ctx, grp, "Attempt to map multiple layer groups to the same autotrax layer", "use only one layer group per layer group type");
			continue;
		}

		ctx->id2grp[al] = grp;
		ctx->grp2id[grp - ctx->pcb->LayerGroups.grp] = al;
	}
	return 0;
}


#define PCB_PSTK_COMPAT_RRECT 250
static int wrax_padstack(wctx_t *ctx, pcb_pstk_t *ps, pcb_coord_t dx, pcb_coord_t dy, pcb_bool in_subc)
{
	const char *name;
	pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask, x1, y1, x2, y2, thickness, w, h;
	pcb_pstk_compshape_t cshape;
	pcb_bool plated, square, nopaste;
	int ashape, alayer;

	if (ps->term != NULL) {
		const char *s;
		int len;

		name = ps->term;
		for(s = name, len = 0; *s != '\0'; s++,len++) {
			if (!isalnum(*s)) {
				pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padname", "autotrax pad name (terminal name) must consists of alphanumeric characters only - omitting padstack", "rename the terminal to something simpler");
				return 0;
			}
			if (len >= 4) {
				pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padname", "autotrax pad name (terminal name) must consists of alphanumeric characters only - omitting padstack", "rename the terminal to something simpler");
				return 0;
			}
		}
	}
	else
		name = "none";

	if (pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
		w = h = pad_dia;
		alayer = 13;
	}
	else if (pcb_pstk_export_compat_pad(ps, &x1, &y1, &x2, &y2, &thickness, &clearance, &mask, &square, &nopaste)) {
		if ((x1 != x2) && (y1 != y2)) {
			pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padstack-rot", "can not export rotated rectangular/line pin/pad/via", "shape must be axis-aligned");
			return 0;
		}
		/* convert to the same format as via */
		cshape = square ? PCB_PSTK_COMPAT_SQUARE : PCB_PSTK_COMPAT_RRECT;

		/* calculate center and disable hole */
		x = (x1+x2)/2;
		y = (y1+y2)/2;
		w = x2 - x1;
		h = y2 - y1;
		drill_dia = 0;

TODO(": need to figure which side the padstack is on!")
		alayer = 1;
	}
	else {
		pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padstack-shape", "can not export complex pin/pad/via", "use uniform shaped pins/vias and simpler pads, with simple shapes (no generic polygons)");
		return 0;
	}

	if ((cshape != PCB_PSTK_COMPAT_ROUND) && (ps->rot != 0)) {
		pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padstack-rot", "can not export rotated pin/pad/via", "remove rotation, shapes must be axis-aligned");
		return 0;
	}

	switch((int)cshape) {
		case PCB_PSTK_COMPAT_ROUND:   ashape = 1; break;
		case PCB_PSTK_COMPAT_SQUARE:  ashape = 2; break;
		case PCB_PSTK_COMPAT_OCTAGON: ashape = 3; break;
		case PCB_PSTK_COMPAT_RRECT:   ashape = 4; break;
		default:
			pcb_io_incompat_save(ps->parent.data, (pcb_any_obj_t *)ps, "padstack-shape", "can not export: invalid pad shape", "use circle, octagon, rectangle or round rectangle");
			return 0;
	}

	if (in_subc)
		fputs("CP ", ctx->f);
	else
		fputs("FP ", ctx->f);

TODO(": figure which is the gnd and which is the power plane")
TODO(": add checks for thermals: only gnd/pwr can have them, warn for others")

	pcb_fprintf(ctx->f, "%.0ml %.0ml %.0ml %.0ml %d %.0ml 1 %d\r\n",
		x+dx, PCB->MaxHeight - (y+dy), w, h,
		ashape, drill_dia, alayer);

	fputs(name, ctx->f);
	fputs("\r\n", ctx->f);
	return 0;
}

int wrax_data(wctx_t *ctx, pcb_data_t *data, pcb_coord_t dx, pcb_coord_t dy);

static int wrax_vias(wctx_t *ctx, pcb_data_t *Data, pcb_coord_t dx, pcb_coord_t dy, pcb_bool in_subc)
{
	gdl_iterator_t it;
	pcb_pstk_t *ps;
	int res = 0;

	padstacklist_foreach(&Data->padstack, &it, ps)
		res |= wrax_padstack(ctx, ps, dx, dy, in_subc);

	return res;
}

/* writes generic autotrax track descriptor line for components and layouts  */
static int wrax_line(wctx_t *ctx, pcb_line_t *line, pcb_cardinal_t layer, pcb_coord_t dx, pcb_coord_t dy)
{
	int user_routed = 1;
	pcb_fprintf(ctx->f, "%.0ml %.0ml %.0ml %.0ml %.0ml %d %d\r\n", line->Point1.X+dx, PCB->MaxHeight - (line->Point1.Y+dy), line->Point2.X+dx, PCB->MaxHeight - (line->Point2.Y+dy), line->Thickness, layer, user_routed);
	return 0;
}

/* writes autotrax track descriptor for a pair of polyline vertices */
static int wrax_pline_segment(wctx_t *ctx, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t Thickness, pcb_cardinal_t layer)
{
	int user_routed = 1;
	pcb_fprintf(ctx->f, "FT\r\n%.0ml %.0ml %.0ml %.0ml %.0ml %d %d\r\n", x1, PCB->MaxHeight - y1, x2, PCB->MaxHeight - y2, Thickness, layer, user_routed);
	return 0;
}

typedef struct {
	wctx_t *wctx;
	pcb_cardinal_t layer;
	pcb_coord_t dx, dy;
	pcb_coord_t thickness;
} autotrax_hatch_ctx_t;


static void autotrax_hatch_cb(void *ctx_, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	autotrax_hatch_ctx_t *hctx = (autotrax_hatch_ctx_t *) ctx_;
	wrax_pline_segment(
		hctx->wctx,
		x1+hctx->dx, y1+hctx->dy, x2+hctx->dx, y2+hctx->dy,
		hctx->thickness, hctx->layer);
}

/* generates autotrax tracks to cross hatch a complex polygon being exported */
static void autotrax_cpoly_hatch_lines(wctx_t *ctx, const pcb_poly_t *src, pcb_cpoly_hatchdir_t dir, pcb_coord_t period, pcb_coord_t thickness, pcb_cardinal_t layer, pcb_coord_t dx, pcb_coord_t dy)
{
	autotrax_hatch_ctx_t hctx;

	hctx.wctx = ctx;
	hctx.layer = layer;
	hctx.thickness = thickness;
	hctx.dx = dx;
	hctx.dy = dy;
	pcb_cpoly_hatch(src, dir, (thickness / 2) + 1, period, &hctx, autotrax_hatch_cb);
}

/* generates an autotrax arc "segments" value to approximate an arc being exported   */
static int pcb_rnd_arc_to_autotrax_segments(pcb_angle_t arc_start, pcb_angle_t arc_delta)
{
	int arc_segments = 0; /* start with no arc segments */
	/* 15 = circle, bit 1 = LUQ, bit 2 = LLQ, bit 3 = LRQ, bit 4 = URQ */
	if (arc_delta == -360) /* it's a circle */
		arc_delta = 360;
	if (arc_delta < 0) {
		arc_delta = -arc_delta;
		arc_start -= arc_delta;
	}

TODO("TODO arc segments less than 90 degrees do not convert well.")

	while(arc_start < 0)
		arc_start += 360;
	while(arc_start > 360)
		arc_start -= 360;

	if (arc_delta >= 360) { /* it's a circle */
		arc_segments |= 0x0F;
	}
	else {
		if (arc_start <= 0.0 && (arc_start + arc_delta) >= 90.0)
			arc_segments |= 0x04; /* LLQ */
		if (arc_start <= 90.0 && (arc_start + arc_delta) >= 180.0)
			arc_segments |= 0x08; /* LRQ */
		if (arc_start <= 180.0 && (arc_start + arc_delta) >= 270.0)
			arc_segments |= 0x01; /* URQ */
		if (arc_start <= 270.0 && (arc_start + arc_delta) >= 360.0)
			arc_segments |= 0x02; /* ULQ */
		if (arc_start <= 360.0 && (arc_start + arc_delta) >= 450.0)
			arc_segments |= 0x04; /* LLQ */
		if (arc_start <= 360.0 && (arc_start + arc_delta) >= 540.0)
			arc_segments |= 0x08; /* LRQ */
		if (arc_start <= 360.0 && (arc_start + arc_delta) >= 630.0)
			arc_segments |= 0x01; /* URQ */
	}
	return arc_segments;
}

/* writes generic autotrax arc descriptor line for components and layouts */
static int wrax_arc(wctx_t *ctx, pcb_arc_t *arc, int current_layer, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_coord_t radius;
	if (arc->Width > arc->Height) {
		radius = arc->Height;
	}
	else {
		radius = arc->Width;
	}
	pcb_fprintf(ctx->f, "%.0ml %.0ml %.0ml %d %.0ml %d\r\n", arc->X+dx, PCB->MaxHeight - (arc->Y+dy), radius, pcb_rnd_arc_to_autotrax_segments(arc->StartAngle, arc->Delta), arc->Thickness, current_layer);
	return 0;
}

/* writes netlist data in autotrax format */
static int wrax_equipotential_netlists(wctx_t *ctx)
{
	int show_status = 0;
	/* show status can be 0 or 1 for a net:
	   0 hide rats nest
	   1 show rats nest */
	/* now we step through any available netlists and generate descriptors */

	if (PCB->netlist[PCB_NETLIST_EDITED].used > 0) {
		htsp_entry_t *e;

		for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e)) {
			pcb_net_term_t *t;
			pcb_net_t *net = e->value;

			fprintf(ctx->f, "NETDEF\r\n");
			pcb_fprintf(ctx->f, "%s\r\n", net->name);
			pcb_fprintf(ctx->f, "%d\r\n", show_status);
			fprintf(ctx->f, "(\r\n");
			for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t))
				pcb_fprintf(ctx->f, "%s-%s\r\n", t->refdes, t->term);
			fprintf(ctx->f, ")\r\n");
		}
	}
	return 0;
}


static int wrax_lines(wctx_t *ctx, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t dx, pcb_coord_t dy, pcb_bool in_subc)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_cardinal_t current_layer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		int local_flag = 0;
		linelist_foreach(&layer->Line, &it, line) {
			if (in_subc)
				pcb_fprintf(ctx->f, "CT\r\n");
			else
				pcb_fprintf(ctx->f, "FT\r\n");
			wrax_line(ctx, line, current_layer, dx, dy);
			local_flag |= 1;
		}
		return local_flag;
	}

	return 0;
}

/* writes autotrax arcs for layouts */
static int wrax_arcs(wctx_t *ctx, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t dx, pcb_coord_t dy, pcb_bool in_subc)
{
	gdl_iterator_t it;
	pcb_arc_t *arc;
	pcb_cardinal_t current_layer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer)) { /*|| (layer->name && *layer->name)) { */
		int local_flag = 0;
		arclist_foreach(&layer->Arc, &it, arc) {
			if (in_subc)
				pcb_fprintf(ctx->f, "CA\r\n");
			else
				pcb_fprintf(ctx->f, "FA\r\n");
			wrax_arc(ctx, arc, current_layer, dx, dy);
			local_flag |= 1;
		}
		return local_flag;
	}

	return 0;
}

/* writes generic autotrax text descriptor line layouts onl, since no text in .fp */
static int wrax_text(wctx_t *ctx, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t dx, pcb_coord_t dy, pcb_bool in_subc)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mHeight = myfont->MaxHeight; /* autotrax needs the width of the widest letter */
	int autotrax_mirrored = 0; /* 0 is not mirrored, +16  is mirrored */

	pcb_coord_t default_stroke_thickness, strokeThickness, textHeight;
	int rotation;
	int local_flag;

	gdl_iterator_t it;
	pcb_text_t *text;
	pcb_cardinal_t current_layer = number;

	int index = 0;

TODO(": why do we hardwire this here?")
	default_stroke_thickness = 200000;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer)) { /*|| (layer->name && *layer->name)) { */
		local_flag = 0;
		textlist_foreach(&layer->Text, &it, text) {
			int direction;
			
			if (pcb_text_old_direction(&direction, text->rot) != 0) {
TODO(": indicate save incompatibility")
			}

			if (current_layer < 9) { /* copper or silk layer text */
				if (in_subc)
					fputs("CS\r\n", ctx->f);
				else
					fputs("FS\r\n", ctx->f);
				strokeThickness = PCB_SCALE_TEXT(default_stroke_thickness, text->Scale / 2);
				textHeight = PCB_SCALE_TEXT(mHeight, text->Scale);
				rotation = 0;
				if (current_layer == 6 || current_layer == 8) /* back copper or silk */
					autotrax_mirrored = 16; /* mirrored */
				if (direction == 3) /*vertical down */
					rotation = 3;
				else if (direction == 2) /*upside down */
					rotation = 2;
				else if (direction == 1) /*vertical up */
					rotation = 1;
				else if (direction == 0) /*normal text */
					rotation = 0;

				pcb_fprintf(ctx->f, "%.0ml %.0ml %.0ml %d %.0ml %d\r\n", text->X+dx, PCB->MaxHeight - (text->Y+dy), textHeight, rotation + autotrax_mirrored, strokeThickness, current_layer);
				for(index = 0; index < 32; index++) {
					if (text->TextString[index] == '\0')
						index = 32;
					else if (text->TextString[index] < 32 || text->TextString[index] > 126)
						fputc(' ', ctx->f); /* replace non alphanum with space */
					else /* need to truncate to 32 alphanumeric chars */
						fputc(text->TextString[index], ctx->f);
				}
				pcb_fprintf(ctx->f, "\r\n");
			}
			local_flag |= 1;
		}
		return local_flag;
	}

	return 0;
}

static const char *or_empty(const char *s)
{
	if (s == NULL) return "";
	return s;
}

static int wrax_subc(wctx_t *ctx, pcb_subc_t *subc)
{
	int res, on_bottom = 0, silk_layer;
	pcb_box_t *box = &subc->BoundingBox;
	pcb_coord_t xPos, yPos, yPos2, yPos3;

TODO(": do not hardcode things like this, especially when actual data is available")
	pcb_coord_t text_offset = PCB_MIL_TO_COORD(400); /* this gives good placement of refdes relative to element */

TODO(": rename these variables to something more expressive")
TODO(": instead of hardwiring coords, just read existing dyntex coords")
	xPos = (box->X1 + box->X2) / 2;
	yPos = PCB->MaxHeight - (box->Y1 - text_offset);
	yPos2 = yPos - PCB_MIL_TO_COORD(200);
	yPos3 = yPos2 - PCB_MIL_TO_COORD(200);

	pcb_subc_get_side(subc, &on_bottom);

TODO(": do not hardwire these layers, even if the autotrax format hardwires them - look them up from the static table, let the hardwiring happen only at one place")
	if (on_bottom)
		silk_layer = 8;
	else
		silk_layer = 7;

	fprintf(ctx->f, "COMP\r\n%s\r\n", or_empty(subc->refdes));
	fprintf(ctx->f, "%s\r\n", or_empty(pcb_attribute_get(&subc->Attributes, "footprint")));
	fprintf(ctx->f, "%s\r\n", or_empty(pcb_attribute_get(&subc->Attributes, "value")));
	pcb_fprintf(ctx->f, "%.0ml %.0ml 100 0 10 %d\r\n", xPos, yPos, silk_layer); /* designator */
	pcb_fprintf(ctx->f, "%.0ml %.0ml 100 0 10 %d\r\n", xPos, yPos2, silk_layer); /* pattern */
	pcb_fprintf(ctx->f, "%.0ml %.0ml 100 0 10 %d\r\n", xPos, yPos3, silk_layer); /* comment field */

	res = wrax_data(ctx, subc->data, 0, 0);

	fprintf(ctx->f, "ENDCOMP\r\n");
	return res;
}

static int wrax_subcs(wctx_t *ctx, pcb_data_t *data)
{
	gdl_iterator_t sit;
	pcb_subc_t *subc;
	int res = 0;

	subclist_foreach(&data->subc, &sit, subc)
		res |= wrax_subc(ctx, subc);

	return res;
}

/* writes polygon data in autotrax fill (rectangle) format for use in a layout .PCB file */
static int wrax_polygons(wctx_t *ctx, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t dx, pcb_coord_t dy, pcb_bool in_subc)
{
	int i;
	gdl_iterator_t it;
	pcb_poly_t *polygon;
	pcb_cardinal_t current_layer = number;

	pcb_poly_it_t poly_it;
	pcb_polyarea_t *pa;

	pcb_coord_t minx, miny, maxx, maxy;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer)) { /*|| (layer->name && *layer->name)) { */
		int local_flag = 0;

		polylist_foreach(&layer->Polygon, &it, polygon) {
			if (pcb_cpoly_is_simple_rect(polygon)) {
				pcb_trace(" simple rectangular polyogon\n");

TODO(": why do we recalculate the bounding box here?")
				minx = maxx = polygon->Points[0].X;
				miny = maxy = polygon->Points[0].Y;

				/* now the fill zone outline is defined by a rectangle enclosing the poly */
				/* hmm. or, could use a bounding box... */
				for(i = 0; i < polygon->PointN; i++) {
					if (minx > polygon->Points[i].X)
						minx = polygon->Points[i].X;
					if (maxx < polygon->Points[i].X)
						maxx = polygon->Points[i].X;
					if (miny > polygon->Points[i].Y)
						miny = polygon->Points[i].Y;
					if (maxy < polygon->Points[i].Y)
						maxy = polygon->Points[i].Y;
				}
				pcb_fprintf(ctx->f, "%cF\r\n%.0ml %.0ml %.0ml %.0ml %d\r\n", (in_subc ? 'C' : 'F'), minx+dx, PCB->MaxHeight - (miny+dy), maxx+dx, PCB->MaxHeight - (maxy+dy), current_layer);

				local_flag |= 1;
/* here we need to test for non rectangular polygons to flag imperfect export to easy/autotrax

			if (helper_clipped_polygon_type_function(clipped_thing)) {
				pcb_message(PCB_MSG_ERROR, "Polygon exported as a bounding box only.\n");
			}*/
			}
			else {
				pcb_coord_t Thickness;
				Thickness = PCB_MIL_TO_COORD(10);
				autotrax_cpoly_hatch_lines(ctx, polygon, PCB_CPOLY_HATCH_HORIZONTAL | PCB_CPOLY_HATCH_VERTICAL, Thickness * 3, Thickness, current_layer, dx, dy);
TODO(": do we really need to reimplement this, can not cpoly_hatch_lines handle it?")
				for(pa = pcb_poly_island_first(polygon, &poly_it); pa != NULL; pa = pcb_poly_island_next(&poly_it)) {
					/* now generate cross hatch lines for polygon island export */
					pcb_pline_t *pl, *track;
					/* check if we have a contour for the given island */
					pl = pcb_poly_contour(&poly_it);
					if (pl != NULL) {
						const pcb_vnode_t *v, *n;
						track = pcb_pline_dup_offset(pl, -((Thickness / 2) + 1));
						v = &track->head;
						do {
							n = v->next;
							wrax_pline_segment(ctx, v->point[0]+dx, v->point[1]+dy, n->point[0]+dx, n->point[1]+dy, Thickness, current_layer);
						} while((v = v->next) != &track->head);
						pcb_poly_contour_del(&track);

						/* iterate over all holes within this island */
						for(pl = pcb_poly_hole_first(&poly_it); pl != NULL; pl = pcb_poly_hole_next(&poly_it)) {
							const pcb_vnode_t *v, *n;
							track = pcb_pline_dup_offset(pl, -((Thickness / 2) + 1));
							v = &track->head;
							do {
								n = v->next;
								wrax_pline_segment(ctx, v->point[0]+dx, v->point[1]+dy, n->point[0]+dx, n->point[1]+dy, Thickness, current_layer);
							} while((v = v->next) != &track->head);
							pcb_poly_contour_del(&track);
						}
					}
				}
			}
		}
		return local_flag;
	}

	return 0;
}

int wrax_data(wctx_t *ctx, pcb_data_t *data, pcb_coord_t dx, pcb_coord_t dy)
{
	int n;
	pcb_bool in_subc = (data->parent_type == PCB_PARENT_SUBC);

	for(n = 0; n < data->LayerN; n++) {
		pcb_layer_t *ly = &data->Layer[n];
		int alid = wrax_layer2id(ctx, ly); /* autotrax layer ID */
		if (alid == 0) {
			char tmp[256];
			pcb_snprintf(tmp, sizeof(tmp), "Ignoring unmapped layer: %s", ly->name);
			pcb_io_incompat_save(data, NULL, "layer", tmp, NULL);
			continue;
		}
		wrax_lines(ctx, alid, ly, dx, dy, in_subc);
		wrax_arcs(ctx, alid, ly, dx, dy, in_subc);
		wrax_text(ctx, alid, ly, dx, dy, in_subc);
		wrax_polygons(ctx, alid, ly, dx, dy, in_subc);
	}

	wrax_subcs(ctx, data);
	wrax_vias(ctx, data, dx, dy, in_subc);
	return 0;
}

/* writes autotrax PCB to file */
int io_autotrax_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	wctx_t wctx;

	/* autotrax expects layout dimensions to be specified in mils */
	int max_width_mil = 32000;
	int max_height_mil = 32000;

	memset(&wctx, 0, sizeof(wctx));
	wctx.f = FP;
	wctx.pcb = PCB;

TODO(": this is a bug - exporting to a file shall not change the content we are exporting")
	if (pcb_board_normalize(PCB) < 0) {
		pcb_message(PCB_MSG_ERROR, "Unable to normalise layout prior to attempting export.\n");
		return -1;
	}

	if (wrax_map_layers(&wctx) != 0)
		return -1;

	fputs("PCB FILE 4\r\n", FP); /*autotrax header */

	/* we sort out if the layout dimensions exceed the autotrax maxima */
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > max_width_mil || PCB_COORD_TO_MIL(PCB->MaxHeight) > max_height_mil) {
		pcb_message(PCB_MSG_ERROR, "Layout size exceeds protel autotrax 32000 mil x 32000 mil maximum.");
		return -1;
	}

	wrax_data(&wctx, PCB->Data, 0, 0);

	/* last are the autotrax netlist descriptors */
	wrax_equipotential_netlists(&wctx);

	fputs("ENDPCB\r\n", FP); /*autotrax footer */

	return 0;
}
