/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016,2018 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016 Erich S. Heinzle
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include <math.h>
#include "config.h"
#include "compat_misc.h"
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "uniq_name.h"
#include "data.h"
#include "write.h"
#include "layer.h"
#include "const.h"
#include "netlist.h"
#include "obj_all.h"
#include "obj_pstk_inlines.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"

/* writes the buffer to file */
int io_kicad_write_buffer(pcb_plug_io_t *ctx, FILE *FP, pcb_buffer_t *buff, pcb_bool elem_only)
{
	pcb_message(PCB_MSG_ERROR, "can't save buffer in s-expr yet, please use kicad legacy for this\n");
	return -1;
}

/* if implicit outline needs to be drawn, use lines of this thickness */
#define KICAD_OUTLINE_THICK (PCB_MIL_TO_COORD(10))

#define KICAD_MAX_LAYERS 64
typedef struct {
	FILE *f;                     /* output file */
	pcb_board_t *pcb;            /* board being exported */
	pcb_coord_t ox, oy;          /* move every object by this origin */
	struct {
		pcb_layergrp_t *grp;
		char name[32];             /* kicad layer name */
		const char *param;
		int is_sig;
	} layer[KICAD_MAX_LAYERS];   /* pcb-rnd layer groups indexed by kicad's stackup */
	int num_layers;              /* number of groups in use */
} wctx_t;

typedef enum {
	FLP_COP_FIRST,
	FLP_COP_INT,
	FLP_COP_LAST,
	FLP_FIXED,
	FLP_MISC /* auto-number from the last known layer */
} fixed_layer_place_t;

typedef struct {
	int fixed_layernum;
	char *name;
	char *param;
	int is_sig;
	pcb_layer_type_t type;
	fixed_layer_place_t place;
} fixed_layer_t;

static fixed_layer_t fixed_layers[] = {
	{0,  "F.Cu",       "signal", 1, PCB_LYT_COPPER | PCB_LYT_TOP,    FLP_COP_FIRST },
	{0,  "Inner%d.Cu", "signal", 1, PCB_LYT_COPPER | PCB_LYT_INTERN, FLP_COP_INT },
	{0,  "B.Cu",       "signal", 1, PCB_LYT_COPPER | PCB_LYT_BOTTOM, FLP_COP_LAST },
/*	{16, "B.Adhes",    "user",   0, PCB_LYT_ADHES | PCB_LYT_BOTTOM,  FLP_FIXED },
	{17, "F.Adhes",    "user",   0, PCB_LYT_ADHES | PCB_LYT_TOP,     FLP_FIXED },*/
	{18, "B.Paste",    "user",   0, PCB_LYT_PASTE | PCB_LYT_BOTTOM,  FLP_FIXED },
	{19, "F.Paste",    "user",   0, PCB_LYT_PASTE | PCB_LYT_TOP,     FLP_FIXED },
	{20, "B.SilkS",    "user",   0, PCB_LYT_SILK | PCB_LYT_BOTTOM,   FLP_FIXED },
	{21, "F.SilkS",    "user",   0, PCB_LYT_SILK | PCB_LYT_TOP,      FLP_FIXED },
	{22, "B.Mask",     "user",   0, PCB_LYT_MASK | PCB_LYT_BOTTOM,   FLP_FIXED },
	{23, "F.Mask",     "user",   0, PCB_LYT_MASK | PCB_LYT_TOP,      FLP_FIXED },
	{28, "Edge.Cuts",  "user",   0, PCB_LYT_OUTLINE,                 FLP_FIXED },
	{NULL, NULL, 0}
};

/* number of copper layers, where to start inners, how to increment iners */
#define KICAD_COPPERS       16
#define KICAD_FIRST_INNER   1
#define KICAD_NEXT_INNER   +1
#define KICAD_FIRST_MISC    29

#define grp_idx(grp) ((grp) - ctx->pcb->LayerGroups->grp)

/* Make the mapping between pcb-rnd layer groups and kicad layer stackup
   Assumption: the first KICAD_COPPERS layers are the copper layers */
static int kicad_map_layers(wctx_t *ctx)
{

	fixed_layer_t *fl;
	int n;
	int inner = KICAD_FIRST_INNER;
	int misc = KICAD_FIRST_MISC;
	char *mapped;

	mapped = calloc(ctx->pcb->LayerGroups.len, 1);
	ctx->num_layers = -1;

	for(fl = fixed_layers; fl->name != NULL; fl++) {
		pcb_layergrp_id_t gid[KICAD_COPPERS];
		int idx, ngrp;

		gid[0] = -1;
		ngrp = pcb_layergrp_list(ctx->pcb, fl->type, gid, KICAD_COPPERS);
		if ((fl->place != FLP_COP_INT) && (ngrp > 1))
			pcb_io_incompat_save(ctx->pcb->Data, NULL, "Multiple layer groups of the same type - exporting only the first", "Merge the groups");

		switch(fl->place) {
			case FLP_COP_FIRST:
				idx = 0;
				break;
			case FLP_COP_INT:
				/* iterate over all internal groups */
				for(n = 0; n < ngrp; n++) {
					idx = inner;
					if (inner >= KICAD_COPPERS-1) {
						pcb_io_incompat_save(ctx->pcb->Data, NULL, "Too many internal layer groups, omitting some", "Use fewer groups");
						continue;
					}
					sprintf(ctx->layer[idx].name, fl->name, idx);
					if (gid[n] >= 0) {
						inner += KICAD_NEXT_INNER;
						ctx->layer[idx].grp = &ctx->pcb->LayerGroups.grp[gid[n]];
						ctx->layer[idx].param = fl->param;
						ctx->layer[idx].is_sig = fl->is_sig;
						mapped[gid[n]]++;
						if (idx > ctx->num_layers)
							ctx->num_layers = idx;
					}
				}
				continue; /* the normal layer insertion does only a single group, skip it */
			case FLP_COP_LAST:
				idx = KICAD_COPPERS - 1;
				break;

			case FLP_FIXED:
				idx = fl->fixed_layernum;
				break;

			case FLP_MISC:
				idx = misc;
				if (gid[0] >= 0)
					misc++;
				if (misc >= KICAD_MAX_LAYERS-1) {
					pcb_io_incompat_save(ctx->pcb->Data, NULL, "Too many internal layer groups, omitting some", "Use fewer groups");
					continue;
				}
				break;
		}

		/* normal, single group mapping */
		if (gid[0] >= 0) {
			strcpy(ctx->layer[idx].name, fl->name);
			ctx->layer[idx].grp = &ctx->pcb->LayerGroups.grp[gid[0]];
			ctx->layer[idx].param = fl->param;
			ctx->layer[idx].is_sig = fl->is_sig;
			mapped[gid[0]]++;
			if (idx > ctx->num_layers)
				ctx->num_layers = idx;
		}
	}

	for(n = 0; n < ctx->pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[n];
		if ((mapped[n] == 0) && !(grp->type & PCB_LYT_SUBSTRATE))
			pcb_io_incompat_save(ctx->pcb->Data, NULL, "Failed to map layer for export - layer omitted", grp->name);
		else if (mapped[n] > 1)
			pcb_io_incompat_save(ctx->pcb->Data, NULL, "Mapped layer for export multiple times", grp->name);
	}

	free(mapped);
	ctx->num_layers++;

	if (ctx->num_layers < 3)
		pcb_io_incompat_save(ctx->pcb->Data, NULL, "Warning: low output layer count", NULL);
	return 0;
}

static const char *kicad_sexpr_layer_to_text(wctx_t *ctx, int layer)
{
	if ((layer < 0) || (layer >= ctx->num_layers)) {
		assert(!"invalid layer");
		return "";
	}
	return ctx->layer[layer].name;
}

static void kicad_print_layers(wctx_t *ctx, int ind)
{
	int n;

	fprintf(ctx->f, "\n%*s(layers\n", ind, "");
	for(n = 0; n < ctx->num_layers; n++)
		if (ctx->layer[n].param != NULL)
			fprintf(ctx->f, "%*s(%d %s %s)\n", ind+2, "", n, ctx->layer[n].name, ctx->layer[n].param);
	fprintf(ctx->f, "%*s)\n", ind, "");
}



typedef struct {
	int klayer;       /* kicad layer ID, index of wctx->layer */
	const char *name; /* kicad layer name (cached from wctx) */
	pcb_layer_t *ly;
	pcb_layer_type_t lyt;
	enum {
		KLYT_COPPER,    /* board: copper/signal/trace objects */
		KLYT_GR,        /* board: "grpahical" objects: silk and misc */
		KLYT_FP         /* module: any */
	} type;
	int skip_term;    /* do not print terminals on this layer */
} klayer_t;

static void kicad_print_line(const wctx_t *ctx, const klayer_t *kly, pcb_line_t *line, int ind)
{
	const char *cmd[] = {"segment", "gr_line", "fp_line"};

	fprintf(ctx->f, "%*s", ind, "");
	pcb_fprintf(ctx->f,
		"(%s (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
		cmd[kly->type],
		line->Point1.X + ctx->ox, line->Point1.Y + ctx->oy,
		line->Point2.X + ctx->ox, line->Point2.Y + ctx->oy,
		kly->name, line->Thickness);
		/* neglect (net ___ ) for now */
}

static void kicad_print_arc(const wctx_t *ctx, const klayer_t *kly, pcb_arc_t *arc, int ind)
{
	pcb_arc_t localArc = *arc; /* for converting ellipses to circular arcs */
	int kicadArcShape; /* 3 = circle, and 2 = arc, 1= rectangle used in eeschema only */
	pcb_coord_t copperStartX, copperStartY; /* used for mapping geda copper arcs onto kicad copper lines */
	pcb_coord_t radius, xStart, yStart, xEnd, yEnd;

	if (arc->Width > arc->Height) {
		radius = arc->Height;
		localArc.Width = radius;
	}
	else {
		radius = arc->Width;
		localArc.Height = radius;
	}

	/* Return the x;y coordinate of the endpoint of an arc; if which is 0, return
	   the endpoint that corresponds to StartAngle, else return the end angle's. */
	if ((arc->Delta == 360.0) || (arc->Delta == -360.0))
		kicadArcShape = 3; /* it's a circle */
	else
		kicadArcShape = 2;  /* it's an arc */

	xStart = localArc.X + ctx->ox;
	yStart = localArc.Y + ctx->oy;
	pcb_arc_get_end(&localArc, 1, &xEnd, &yEnd);
	xEnd += ctx->ox;
	yEnd += ctx->oy;
	pcb_arc_get_end(&localArc, 0, &copperStartX, &copperStartY);
	copperStartX += ctx->ox;
	copperStartY += ctx->oy;

	fprintf(ctx->f, "%*s", ind, "");

	switch(kly->type) {
		case KLYT_COPPER:
#warning TODO: this should be a proper line approximation using a helper (to be written)
			pcb_fprintf(ctx->f, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n", copperStartX, copperStartY, xEnd, yEnd, kly->name, arc->Thickness); /* neglect (net ___ ) for now */
			pcb_io_incompat_save(ctx->pcb->Data, (pcb_any_obj_t *)arc, "Kicad does not support copper arcs; using line approximation", NULL);
			break;
		case KLYT_GR:
			pcb_fprintf(ctx->f, "(gr_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n", xStart, yStart, xEnd, yEnd, arc->Delta, kly->name, arc->Thickness);
			break;
		case KLYT_FP:
			pcb_fprintf(ctx->f, "(fp_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n", xStart, yStart, xEnd, yEnd, arc->Delta, kly->name, arc->Thickness);
			break;
	}
}

static void kicad_print_text(const wctx_t *ctx, const klayer_t *kly, pcb_text_t *text, int ind)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mWidth = myfont->MaxWidth; /* kicad needs the width of the widest letter */
	pcb_coord_t defaultStrokeThickness = 100 * 2540; /* use 100 mil as default 100% stroked font line thickness */
	int kicadMirrored = 1; /* 1 is not mirrored, 0  is mirrored */
	pcb_coord_t defaultXSize;
	pcb_coord_t defaultYSize;
	pcb_coord_t strokeThickness;
	int rotation;
	pcb_coord_t textOffsetX;
	pcb_coord_t textOffsetY;
	pcb_coord_t halfStringWidth;
	pcb_coord_t halfStringHeight;

	if (!(kly->lyt & PCB_LYT_COPPER) && !(kly->lyt & PCB_LYT_SILK)) {
		pcb_io_incompat_save(ctx->pcb->Data, (pcb_any_obj_t *)text, "Kicad supports text only on copper or silk - omitting text object on misc layer", NULL);
		return;
	}

	fprintf(ctx->f, "%*s", ind, "");
	pcb_fprintf(ctx->f, "(gr_text %[4] ", text->TextString);
	defaultXSize = 5 * PCB_SCALE_TEXT(mWidth, text->Scale) / 6; /* IIRC kicad treats this as kerned width of upper case m */
	defaultYSize = defaultXSize;
	strokeThickness = PCB_SCALE_TEXT(defaultStrokeThickness, text->Scale / 2);
	rotation = 0;
	textOffsetX = 0;
	textOffsetY = 0;
	halfStringWidth = (text->BoundingBox.X2 - text->BoundingBox.X1) / 2;
	if (halfStringWidth < 0) {
		halfStringWidth = -halfStringWidth;
	}
	halfStringHeight = (text->BoundingBox.Y2 - text->BoundingBox.Y1) / 2;
	if (halfStringHeight < 0) {
		halfStringHeight = -halfStringHeight;
	}
	if (text->Direction == 3) { /*vertical down */
		if (kly->lyt & PCB_LYT_BOTTOM) { /* back copper or silk */
			rotation = 2700;
			kicadMirrored = 0; /* mirrored */
			textOffsetY -= halfStringHeight;
			textOffsetX -= 2 * halfStringWidth; /* was 1*hsw */
		}
		else { /* front copper or silk */
			rotation = 2700;
			kicadMirrored = 1; /* not mirrored */
			textOffsetY = halfStringHeight;
			textOffsetX -= halfStringWidth;
		}
	}
	else if (text->Direction == 2) { /*upside down */
		if (kly->lyt & PCB_LYT_BOTTOM) { /* back copper or silk */
			rotation = 0;
			kicadMirrored = 0; /* mirrored */
			textOffsetY += halfStringHeight;
		}
		else { /* front copper or silk */
			rotation = 1800;
			kicadMirrored = 1; /* not mirrored */
			textOffsetY -= halfStringHeight;
		}
		textOffsetX = -halfStringWidth;
	}
	else if (text->Direction == 1) { /*vertical up */
		if (kly->lyt & PCB_LYT_BOTTOM) { /* back copper or silk */
			rotation = 900;
			kicadMirrored = 0; /* mirrored */
			textOffsetY = halfStringHeight;
			textOffsetX += halfStringWidth;
		}
		else { /* front copper or silk */
			rotation = 900;
			kicadMirrored = 1; /* not mirrored */
			textOffsetY = -halfStringHeight;
			textOffsetX = 0; /* += halfStringWidth; */
		}
	}
	else if (text->Direction == 0) { /*normal text */
		if (kly->lyt & PCB_LYT_BOTTOM) { /* back copper or silk */
			rotation = 1800;
			kicadMirrored = 0; /* mirrored */
			textOffsetY -= halfStringHeight;
		}
		else { /* front copper or silk */
			rotation = 0;
			kicadMirrored = 1; /* not mirrored */
			textOffsetY += halfStringHeight;
		}
		textOffsetX = halfStringWidth;
	}
	pcb_fprintf(ctx->f, "(at %.3mm %.3mm", text->X + ctx->ox + textOffsetX, text->Y + ctx->oy + textOffsetY);
	if (rotation != 0) {
		fprintf(ctx->f, " %d", rotation / 10); /* convert decidegrees to degrees */
	}
	pcb_fprintf(ctx->f, ") (layer %s)\n", kly->name);
	fprintf(ctx->f, "%*s", ind + 2, "");
	pcb_fprintf(ctx->f, "(effects (font (size %.3mm %.3mm) (thickness %.3mm))", defaultXSize, defaultYSize, strokeThickness); /* , rotation */
	if (kicadMirrored == 0) {
		fprintf(ctx->f, " (justify mirror)");
	}
	fprintf(ctx->f, ")\n%*s)\n", ind, "");
}


static void kicad_print_poly(const wctx_t *ctx, const klayer_t *kly, pcb_poly_t *polygon, int ind)
{
	int i, j;

	if (polygon->HoleIndexN != 0) {
#warning TODO: does kicad suppor holes? of so, use them; else (and only else) there is a polygon.h call that can split up a holed poly into a set of hole-free polygons
		pcb_io_incompat_save(ctx->pcb->Data, (pcb_any_obj_t *)polygon, "can't export polygon with holes", NULL);
		return;
	}

	/* preliminaries for zone settings */
#warning TODO: never hardwire tstamp
#warning TODO: do not hardwire thicknesses and gaps and hatch values!
	fprintf(ctx->f, "%*s(zone (net 0) (net_name \"\") (layer %s) (tstamp 478E3FC8) (hatch edge 0.508)\n", ind, "", kly->name);
	fprintf(ctx->f, "%*s(connect_pads no (clearance 0.508))\n", ind + 2, "");
	fprintf(ctx->f, "%*s(min_thickness 0.4826)\n", ind + 2, "");
	fprintf(ctx->f, "%*s(fill (arc_segments 32) (thermal_gap 0.508) (thermal_bridge_width 0.508))\n", ind + 2, "");
	fprintf(ctx->f, "%*s(polygon\n", ind + 2, "");
	fprintf(ctx->f, "%*s(pts\n", ind + 4, "");

	/* now the zone outline is defined */
	for(i = 0; i < polygon->PointN; i = i + 5) { /* kicad exports five coords per line in s-expr files */
		fprintf(ctx->f, "%*s", ind + 6, ""); /* pcb_fprintf does not support %*s   */
		for(j = 0; (j < polygon->PointN) && (j < 5); j++) {
			pcb_fprintf(ctx->f, "(xy %.3mm %.3mm)", polygon->Points[i + j].X + ctx->ox, polygon->Points[i + j].Y + ctx->oy);
			if ((j < 4) && ((i + j) < (polygon->PointN - 1))) {
				fputs(" ", ctx->f);
			}
		}
		fputs("\n", ctx->f);
	}
	fprintf(ctx->f, "%*s)\n", ind + 4, "");
	fprintf(ctx->f, "%*s)\n", ind + 2, "");
	fprintf(ctx->f, "%*s)\n", ind, "");
}


/* Print all objects of a kicad layer; if skip_term is true, ignore the objects
   with term ID set */
static void kicad_print_layer(wctx_t *ctx, pcb_layer_t *ly, const klayer_t *kly, int ind)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_text_t *text;
	pcb_poly_t *poly;
	pcb_arc_t *arc;

	linelist_foreach(&ly->Line, &it, line)
		if ((line->term == NULL) || !kly->skip_term)
			kicad_print_line(ctx, kly, line, ind);

	arclist_foreach(&ly->Arc, &it, arc)
		if ((arc->term == NULL) || !kly->skip_term)
			kicad_print_arc(ctx, kly, arc, ind);

	textlist_foreach(&ly->Text, &it, text) {
		if (kly->type == KLYT_FP) {
			if (!PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, text)) {
				pcb_io_incompat_save(ctx->pcb->Data, (pcb_any_obj_t *)text, "can't export custom text in footprint", NULL);
				break;
			}
			else
				continue;
		}
		if ((text->term == NULL) || !kly->skip_term)
			kicad_print_text(ctx, kly, text, ind);
	}

	polylist_foreach(&ly->Polygon, &it, poly)
		if ((poly->term == NULL) || !kly->skip_term)
			kicad_print_poly(ctx, kly, poly, ind);
}

/* writes kicad format via data
   For a track segment: Position shape Xstart Ystart Xend Yend width
   Description layer 0 netcode timestamp status; Shape parameter is set to 0 (reserved for future) */
static void kicad_print_pstks(wctx_t *ctx, pcb_data_t *Data, int ind)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	pcb_pstk_t *ps;
	int is_subc = Data->parent_type == PCB_PARENT_SUBC;

	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		fprintf(ctx->f, "%*s", ind, "");
		if (is_subc && (via->term == NULL)) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)via, "can't export non-terminal via in subcircuit, omitting the object", NULL);
			continue;
		}
		if (!is_subc && (via->term != NULL)) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)via, "can't export terminal info for a via outside of a subcircuit (omitting terminal info)", NULL);
			continue;
		}
		if (is_subc) {
			pcb_fprintf(ctx->f, "(pad %s thru_hole %s (at %.3mm %.3mm) (size %.3mm %.3mm) (drill %.3mm) (layers %s %s))\n",
				via->term, (PCB_FLAG_TEST(PCB_FLAG_SQUARE, via) ? "rect" : "oval"),
				via->X + ctx->ox, via->Y + ctx->oy,
				via->Thickness, via->Thickness,
				via->DrillingHole,
				kicad_sexpr_layer_to_text(ctx, 0), kicad_sexpr_layer_to_text(ctx, 15)); /* skip (net 0) for now */
		}
		else {
			pcb_fprintf(ctx->f, "(via (at %.3mm %.3mm) (size %.3mm) (layers %s %s))\n",
				via->X + ctx->ox, via->Y + ctx->oy,
				via->Thickness,
				kicad_sexpr_layer_to_text(ctx, 0), kicad_sexpr_layer_to_text(ctx, 15)); /* skip (net 0) for now */
		}
	}
	padstacklist_foreach(&Data->padstack, &it, ps) {
		int klayer_from = 0, klayer_to = 15;
		pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask, x1, y1, x2, y2, thickness;
		pcb_pstk_compshape_t cshape;
		pcb_bool plated, square, nopaste;

		if (is_subc && (ps->term == NULL)) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "can't export non-terminal padstack in subcircuit, omitting the object", NULL);
			continue;
		}
		if (!is_subc && (ps->term != NULL)) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "can't export terminal info for a padstack outside of a subcircuit (omitting terminal info)", NULL);
			continue;
		}

		if (is_subc) {
			if (pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
				pcb_fprintf(ctx->f, "(pad %s thru_hole %s (at %.3mm %.3mm) (size %.3mm %.3mm) (drill %.3mm) (layers %s %s))\n",
					via->term, (PCB_FLAG_TEST(PCB_FLAG_SQUARE, via) ? "rect" : "oval"),
					via->X + ctx->ox, via->Y + ctx->oy,
					via->Thickness, via->Thickness,
					kicad_sexpr_layer_to_text(ctx, 0), kicad_sexpr_layer_to_text(ctx, 15)); /* skip (net 0) for now */
			}
			else if (pcb_pstk_export_compat_pad(ps, &x1, &y1, &x2, &y2, &thickness, &clearance, &mask, &square, &nopaste)) {
				/* the above check only makes sure this is a plain padstack, get the geometry from the copper layer shape */
				const char *shape_str, *side_str = "F.";
				int n, has_mask = 0;
				pcb_pstk_proto_t *proto = pcb_pstk_get_proto_(Data, ps->proto);
				pcb_pstk_tshape_t *tshp = &proto->tr.array[0];
				pcb_coord_t w, h;

				for(n = 0; n < tshp->len; n++) {
					if (tshp->shape[n].layer_mask & PCB_LYT_COPPER) {
						pcb_line_t line;
						pcb_box_t bx;
						pcb_pstk_shape_t *shape = &tshp->shape[n];
						
						if (tshp->shape[n].layer_mask & PCB_LYT_BOTTOM)
							side_str = "B.";

						switch(shape->shape) {
							case PCB_PSSH_POLY:
								bx.X1 = bx.X2 = shape->data.poly.x[0];
								bx.Y1 = bx.Y2 = shape->data.poly.y[0];
								for(n = 1; n < shape->data.poly.len; n++)
									pcb_box_bump_point(&bx, shape->data.poly.x[n], shape->data.poly.y[n]);
								w = (bx.X2 - bx.X1);
								h = (bx.Y2 - bx.Y1);
								shape_str = "rect";
								break;
							case PCB_PSSH_LINE:
								line.Point1.X = shape->data.line.x1;
								line.Point1.Y = shape->data.line.y1;
								line.Point2.X = shape->data.line.x2;
								line.Point2.Y = shape->data.line.y2;
								line.Thickness = shape->data.line.thickness;
								line.Clearance = 0;
								line.Flags = pcb_flag_make(shape->data.line.square ? PCB_FLAG_SQUARE : 0);
								pcb_line_bbox(&line);
								w = (line.BoundingBox.X2 - line.BoundingBox.X1);
								h = (line.BoundingBox.Y2 - line.BoundingBox.Y1);
								shape_str = shape->data.line.square ? "rect" : "oval";
								break;
							case PCB_PSSH_CIRC:
								w = h = shape->data.circ.dia;
								shape_str = "oval";
								break;
						}
					}
					if (tshp->shape[n].layer_mask & PCB_LYT_MASK)
						has_mask = 1;
				}


				pcb_fprintf(ctx->f, "(pad %s smd %s (at %.3mm %.3mm) (size %.3mm %.3mm) (layers",
					ps->term, shape_str,
					ps->x + ctx->ox, ps->y + ctx->oy,
					w, h,
					kicad_sexpr_layer_to_text(ctx, 0), kicad_sexpr_layer_to_text(ctx, 15)); /* skip (net 0) for now */
				
				fprintf(ctx->f, " %sCu", side_str); /* always has copper */
				if (has_mask)
					fprintf(ctx->f, " %sMask", side_str);
				if (!nopaste)
					fprintf(ctx->f, " %sPaste", side_str);
				fprintf(ctx->f, "))\n");
			}
			else
				pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "can't export terminal info for a padstack outside of a subcircuit (omitting terminal info)", NULL);
		}
		else { /* global via */
			if (!pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
				pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "Can not convert padstack to old-style via", "Use round, uniform-shaped vias only");
				continue;
			}

			if (cshape != PCB_PSTK_COMPAT_ROUND) {
				pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "Can not convert padstack to via", "only round vias are supported");
				continue;
			}
#warning TODO: set klayer_from and klayer_to using bb span of ps

			fprintf(ctx->f, "%*s", ind, "");
			pcb_fprintf(ctx->f, "(via (at %.3mm %.3mm) (size %.3mm) (layers %s %s))\n",
				x + ctx->ox, y + ctx->oy, pad_dia,
				kicad_sexpr_layer_to_text(ctx, klayer_from), kicad_sexpr_layer_to_text(ctx, klayer_to)
				); /* skip (net 0) for now */
		}
	}
}



void kicad_print_data(wctx_t *ctx, pcb_data_t *data, int ind)
{
	int n, klayer;

	for(n = 0; n < data->LayerN; n++) {
		pcb_layer_t *ly = &data->Layer[n];
		pcb_layergrp_id_t gid = pcb_layer_get_group_(ly);
		pcb_layergrp_t *grp;
		int found = 0;
		klayer_t kly;


		if (gid < 0)
			continue; /* unbound, should not be exported */

		grp = &ctx->pcb->LayerGroups.grp[gid];

		for(klayer = 0; klayer < ctx->num_layers; klayer++) {
			if (ctx->layer[klayer].grp == grp) {
				found = 1;
				break;
			}
		}

		if (!found) {
			pcb_io_incompat_save(data, NULL, "unmapped layer on data export", NULL);
			continue;
		}

#warning TODO: this should be a safe lookup, merged with kicad_sexpr_layer_to_text()
		kly.name = kicad_sexpr_layer_to_text(ctx, klayer);
		kly.ly = ly;
		kly.lyt = pcb_layer_flags_(ly);
		if (data->parent_type != PCB_PARENT_SUBC) {
			kly.type = ctx->layer[klayer].is_sig ? KLYT_COPPER : KLYT_GR;
			kly.skip_term = pcb_false;
		}
		else {
			kly.type = KLYT_FP;
			kly.skip_term = pcb_true;
		}

		kicad_print_layer(ctx, ly, &kly, ind);
	}

	kicad_print_pstks(ctx, data, ind);
}

static int kicad_print_subcs(wctx_t *ctx, pcb_data_t *Data, pcb_cardinal_t ind)
{
	gdl_iterator_t sit;
	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc rendering */
	pcb_coord_t xPos, yPos;
	pcb_subc_t *subc;
	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	const char *currentElementName;
	const char *currentElementRef;
	const char *currentElementVal;

	elementlist_dedup_initializer(ededup);
	/* Now initialize the group with defaults */
	unm_init(&group1);

	subclist_foreach(&Data->subc, &sit, subc) {
		gdl_iterator_t it;
		pcb_coord_t xPos, yPos;
		int on_bottom;
		double rot;


#warning TODO: get this from data table (see also #1)
		int silkLayer = 21; /* hard coded default, 20 is bottom silk */
		int copperLayer = 15; /* hard coded default, 0 is bottom copper */

		/* elementlist_dedup_skip(ededup, element);  */
#warning TODO: why?
		/* let's not skip duplicate elements for layout export */

		if (pcb_subc_get_origin(subc, &xPos, &yPos) != 0) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)subc, "Failed to get origin of subcircuit", "fix the missing subc-aux layer");
			continue;
		}
		if (pcb_subc_get_side(subc, &on_bottom) != 0) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)subc, "Failed to get placement side of subcircuit", "fix the missing subc-aux layer");
			continue;
		}

		xPos += ctx->ox;
		yPos += ctx->oy;

		if (on_bottom) {
			silkLayer = 20;
			copperLayer = 0;
		}

#warning TODO: we should probably do unm_name() on the refdes, not on footprint-name?
#warning TODO: the unique name makes no sense if we override it with unknown - if the unique name is NULL, it's more likely a save-incompatibility error
		currentElementName = unm_name(&group1, pcb_attribute_get(&subc->Attributes, "footprint"), subc);
		if (currentElementName == NULL) {
			currentElementName = "unknown";
		}
		currentElementRef = pcb_attribute_get(&subc->Attributes, "refdes");
		if (currentElementRef == NULL) {
			currentElementRef = "unknown";
		}
		currentElementVal = pcb_attribute_get(&subc->Attributes, "value");
		if (currentElementVal == NULL) {
			currentElementVal = "unknown";
		}

#warning TODO: why the heck do we hardwire timestamps?!!?!?!
		fprintf(ctx->f, "%*s", ind, "");
		pcb_fprintf(ctx->f, "(module %[4] (layer %s) (tedit 4E4C0E65) (tstamp 5127A136)\n", currentElementName, kicad_sexpr_layer_to_text(ctx, copperLayer));
		fprintf(ctx->f, "%*s", ind + 2, "");
		pcb_fprintf(ctx->f, "(at %.3mm %.3mm)\n", xPos, yPos);

		fprintf(ctx->f, "%*s", ind + 2, "");
		pcb_fprintf(ctx->f, "(descr %[4])\n", currentElementName);

		fprintf(ctx->f, "%*s", ind + 2, "");

#warning TODO: do not hardwire these coords, look up the first silk dyntext coords instead
		pcb_fprintf(ctx->f, "(fp_text reference %[4] (at 0.0 -2.56) ", currentElementRef);
		pcb_fprintf(ctx->f, "(layer %s)\n", kicad_sexpr_layer_to_text(ctx, silkLayer));

#warning TODO: do not hardwire font sizes here, look up the first silk dyntext sizes instead
		fprintf(ctx->f, "%*s", ind + 4, "");
		fprintf(ctx->f, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(ctx->f, "%*s)\n", ind + 2, "");

#warning TODO: do not hardwire these coords, look up the first silk dyntext coords instead
		fprintf(ctx->f, "%*s", ind + 2, "");
		pcb_fprintf(ctx->f, "(fp_text value %[4] (at 0.0 -1.27) ", currentElementVal);
		pcb_fprintf(ctx->f, "(layer %s)\n", kicad_sexpr_layer_to_text(ctx, silkLayer));

#warning TODO: do not hardwire font sizes here, look up the first silk dyntext sizes instead
		fprintf(ctx->f, "%*s", ind + 4, "");
		fprintf(ctx->f, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(ctx->f, "%*s)\n", ind + 2, "");

		kicad_print_data(ctx, subc->data, ind+2);

#warning TODO: export padstacks
#warning TODO: warn for vias
#warning TODO: warn for heavy terminals

		fprintf(ctx->f, "%*s)\n\n", ind, ""); /*  finish off module */
	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);

	return 0;
}


static int write_kicad_layout_via_drill_size(FILE *FP, pcb_cardinal_t indentation)
{
#warning TODO: do not hardwire the drill size here - does kicad support only one size, or what?
	fprintf(FP, "%*s", indentation, "");
	pcb_fprintf(FP, "(via_drill 0.635)\n"); /* mm format, default for now, ~= 0.635mm */
	return 0;
}



/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a .mod library
 */
int io_kicad_write_element(pcb_plug_io_t *ctx, FILE *FP, pcb_data_t *Data)
{
	if (elementlist_length(&Data->Element) > 1) {
		pcb_message(PCB_MSG_ERROR, "Can't save multiple modules (footprints) in a single s-experssion mod file\n");
		return -1;
	}

#warning TODO: make this initialization a common function with write_kicad_layout()
	pcb_printf_slot[4] = "%{\\()\t\r\n \"}mq";

	return write_kicad_layout_elements(FP, PCB, Data, 0, 0, 0);
}


/* ---------------------------------------------------------------------------
 * writes netlist data in kicad legacy format for use in a layout .brd file
 */

int write_kicad_equipotential_netlists(FILE *FP, pcb_board_t *Layout, pcb_cardinal_t indentation)
{
	int n; /* code mostly lifted from netlist.c */
	int netNumber;
	pcb_lib_menu_t *menu;
	pcb_lib_entry_t *netlist;

	/* first we write a default netlist for the 0 net, which is for unconnected pads in pcbnew */
	fprintf(FP, "\n%*s(net 0 \"\")\n", indentation, "");

	/* now we step through any available netlists and generate descriptors */
	for(n = 0, netNumber = 1; n < Layout->NetlistLib[PCB_NETLIST_EDITED].MenuN; n++, netNumber++) {
		menu = &Layout->NetlistLib[PCB_NETLIST_EDITED].Menu[n];
		netlist = &menu->Entry[0];
		if (netlist != NULL) {
			fprintf(FP, "%*s(net %d ", indentation, "", netNumber); /* netlist 0 was used for unconnected pads  */
			pcb_fprintf(FP, "%[4])\n", pcb_netlist_name(menu));
		}
	}
	return 0;
}

/* may need to export a netclass or two 
(net_class Default "Ceci est la Netclass par dÃ©faut"
(clearance 0.254)
(trace_width 0.254)
(via_dia 0.889)
(via_drill 0.635)
(uvia_dia 0.508)
(uvia_drill 0.127)
(add_net "")
*/


/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a layout .brd file
 */
int write_kicad_layout_elements(FILE *FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{

	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc rendering */
	pcb_coord_t xPos, yPos;

	pcb_element_t *element;
	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	const char *currentElementName;
	const char *currentElementRef;
	const char *currentElementVal;

	pcb_lib_menu_t *current_pin_menu;
	pcb_lib_menu_t *current_pad_menu;

	int silkLayer = 21; /* hard coded default, 20 is bottom silk */
	int copperLayer = 15; /* hard coded default, 0 is bottom copper */

	elementlist_dedup_initializer(ededup);
	/* Now initialize the group with defaults */
	unm_init(&group1);

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		/* elementlist_dedup_skip(ededup, element);  */
		/* let's not skip duplicate elements for layout export */

		/* TOOD: Footprint name element->Name[0].TextString */

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;
		/* the coordinates and text-flags are the same for
		 * both names of an element
		 */

		xPos = element->MarkX + xOffset;
		yPos = element->MarkY + yOffset;
		silkLayer = 21; /* default */
		copperLayer = 15; /* default */

		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element)) {
			silkLayer = 20;
			copperLayer = 0;
		}

		currentElementName = unm_name(&group1, element->Name[0].TextString, element);
		if (currentElementName == NULL) {
			currentElementName = "unknown";
		}
		currentElementRef = element->Name[PCB_ELEMNAME_IDX_REFDES].TextString;
		if (currentElementRef == NULL) {
			currentElementRef = "unknown";
		}
		currentElementVal = element->Name[PCB_ELEMNAME_IDX_VALUE].TextString;
		if (currentElementVal == NULL) {
			currentElementVal = "unknown";
		}

		fprintf(FP, "%*s", indentation, "");
		pcb_fprintf(FP, "(module %[4] (layer %s) (tedit 4E4C0E65) (tstamp 5127A136)\n", currentElementName, kicad_sexpr_layer_to_text(NULL, copperLayer));
		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(at %.3mm %.3mm)\n", xPos, yPos);

		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(descr %[4])\n", currentElementName);

		fprintf(FP, "%*s", indentation + 2, "");

		pcb_fprintf(FP, "(fp_text reference %[4] (at 0.0 -2.56) ", currentElementRef);
		pcb_fprintf(FP, "(layer %s)\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

		fprintf(FP, "%*s", indentation + 2, "");
		printf("Element SilkLayer: %s\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

		pcb_fprintf(FP, "(fp_text value %[4] (at 0.0 -1.27) ", currentElementVal);
		pcb_fprintf(FP, "(layer %s)\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

		linelist_foreach(&element->Line, &it, line) {
			fprintf(FP, "%*s", indentation + 2, "");
			pcb_fprintf(FP, "(fp_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
									line->Point1.X - element->MarkX, line->Point1.Y - element->MarkY, line->Point2.X - element->MarkX, line->Point2.Y - element->MarkY, kicad_sexpr_layer_to_text(NULL, silkLayer), line->Thickness);
		}

		arclist_foreach(&element->Arc, &it, arc) {

			pcb_arc_get_end(arc, 0, &arcStartX, &arcStartY);
			pcb_arc_get_end(arc, 1, &arcEndX, &arcEndY);

			if ((arc->Delta == 360.0) || (arc->Delta == -360.0)) { /* it's a circle */
				fprintf(FP, "%*s", indentation + 2, "");
				pcb_fprintf(FP, "(fp_circle (center %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n", arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcStartX - element->MarkX, /* x on circle */
										arcStartY - element->MarkY, /* y on circle */
										kicad_sexpr_layer_to_text(NULL, silkLayer), arc->Thickness); /* stroke thickness */
			}
			else {
				fprintf(FP, "%*s", indentation + 2, "");
				pcb_fprintf(FP, "(fp_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n", arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcEndX - element->MarkX, /* x on arc */
										arcEndY - element->MarkY, /* y on arc */
										arc->Delta, /* CW delta angle in decidegrees */
										kicad_sexpr_layer_to_text(NULL, silkLayer), arc->Thickness); /* stroke thickness */
			}
		}


		pinlist_foreach(&element->Pin, &it, pin) {
			fprintf(FP, "%*s", indentation + 2, "");
			fputs("(pad ", FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pin->Number));
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
				fputs(" thru_hole rect ", FP); /* square */
			}
			else {
				fputs(" thru_hole circle ", FP); /* circular */
			}
			pcb_fprintf(FP, "(at %.3mm %.3mm)", /* positions of pad */
									pin->X - element->MarkX, pin->Y - element->MarkY);
			/* pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number)); */
			pcb_fprintf(FP, " (size %.3mm %.3mm)", pin->Thickness, pin->Thickness); /* height = width */
			/* drill details; size */
			pcb_fprintf(FP, " (drill %.3mm)\n", pin->DrillingHole);
			fprintf(FP, "%*s", indentation + 4, "");
			fprintf(FP, "(layers *.Cu *.Mask)\n"); /* define included layers for pin */
			current_pin_menu = pcb_netlist_find_net4pin(Layout, pin);
			fprintf(FP, "%*s", indentation + 4, "");
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				pcb_fprintf(FP, "(net %d %[4])\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			}
			else {
				fprintf(FP, "(net 0 \"\")\n"); /* unconnected pads have zero for net */
			}
			fprintf(FP, "%*s)\n", indentation + 2, "");
			/*
			   pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
			   fprintf(FP, " %s\n", F2S(pin, PCB_TYPE_PIN));
			 */
		}
		padlist_foreach(&element->Pad, &it, pad) {
			fprintf(FP, "%*s", indentation + 2, "");
			fputs("(pad ", FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pad->Number));
			fputs(" smd rect ", FP); /* square for now */
			pcb_fprintf(FP, "(at %.3mm %.3mm)", /* positions of pad */
									(pad->Point1.X + pad->Point2.X) / 2 - element->MarkX, (pad->Point1.Y + pad->Point2.Y) / 2 - element->MarkY);
			pcb_fprintf(FP, " (size ");
			if ((pad->Point1.X - pad->Point2.X) <= 0 && (pad->Point1.Y - pad->Point2.Y) <= 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point2.X - pad->Point1.X + pad->Thickness, /* width */
										pad->Point2.Y - pad->Point1.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) <= 0 && (pad->Point1.Y - pad->Point2.Y) > 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point2.X - pad->Point1.X + pad->Thickness, /* width */
										pad->Point1.Y - pad->Point2.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) > 0 && (pad->Point1.Y - pad->Point2.Y) > 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point1.X - pad->Point2.X + pad->Thickness, /* width */
										pad->Point1.Y - pad->Point2.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) > 0 && (pad->Point1.Y - pad->Point2.Y) <= 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point1.X - pad->Point2.X + pad->Thickness, /* width */
										pad->Point2.Y - pad->Point1.Y + pad->Thickness); /* height */
			}

			fprintf(FP, "%*s", indentation + 4, "");
			if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad)) {
				fprintf(FP, "(layers B.Cu B.Paste B.Mask)\n"); /* May break if odd layer names */
			}
			else {
				fprintf(FP, "(layers F.Cu F.Paste F.Mask)\n"); /* May break if odd layer names */
			}

			current_pin_menu = pcb_netlist_find_net4pad(Layout, pad);
			fprintf(FP, "%*s", indentation + 4, "");
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				pcb_fprintf(FP, "(net %d %[4])\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			}
			else {
				fprintf(FP, "(net 0 \"\")\n"); /* unconnected pads have zero for net */
			}
			fprintf(FP, "%*s)\n", indentation + 2, "");

		}

		fprintf(FP, "%*s)\n\n", indentation, ""); /*  finish off module */
	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);

	return 0;
}

static void kicad_paper(wctx_t *ctx, int ind)
{
	pcb_coord_t LayoutXOffset;
	pcb_coord_t LayoutYOffset;

	/* Kicad expects a layout "sheet" size to be specified in mils, and A4, A3 etc... */
	int A4HeightMil = 8267;
	int A4WidthMil = 11700;
	int sheetHeight = A4HeightMil;
	int sheetWidth = A4WidthMil;
	int paperSize = 4; /* default paper size is A4 */

#warning TODO: rewrite this: rather have a table and a loop that hardwired calculations in code
	/* we sort out the needed kicad sheet size here, using A4, A3, A2, A1 or A0 size as needed */
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > A4WidthMil || PCB_COORD_TO_MIL(PCB->MaxHeight) > A4HeightMil) {
		sheetHeight = A4WidthMil; /* 11.7" */
		sheetWidth = 2 * A4HeightMil; /* 16.5" */
		paperSize = 3; /* this is A3 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth || PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 2 * A4HeightMil; /* 16.5" */
		sheetWidth = 2 * A4WidthMil; /* 23.4" */
		paperSize = 2; /* this is A2 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth || PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 2 * A4WidthMil; /* 23.4" */
		sheetWidth = 4 * A4HeightMil; /* 33.1" */
		paperSize = 1; /* this is A1 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth || PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 4 * A4HeightMil; /* 33.1" */
		sheetWidth = 4 * A4WidthMil; /* 46.8"  */
		paperSize = 0; /* this is A0 size; where would you get it made ?!?! */
	}
	fprintf(ctx->f, "\n%*s(page A%d)\n", ind, "", paperSize);


	/* we now sort out the offsets for centring the layout in the chosen sheet size here */
	if (sheetWidth > PCB_COORD_TO_MIL(PCB->MaxWidth)) { /* usually A4, bigger if needed */
		/* fprintf(ctx->f, "%d ", sheetWidth);  legacy kicad: elements decimils, sheet size mils */
		LayoutXOffset = PCB_MIL_TO_COORD(sheetWidth) / 2 - PCB->MaxWidth / 2;
	}
	else { /* the layout is bigger than A0; most unlikely, but... */
		/* pcb_fprintf(ctx->f, "%.0ml ", PCB->MaxWidth); */
		LayoutXOffset = 0;
	}
	if (sheetHeight > PCB_COORD_TO_MIL(PCB->MaxHeight)) {
		/* fprintf(ctx->f, "%d", sheetHeight); */
		LayoutYOffset = PCB_MIL_TO_COORD(sheetHeight) / 2 - PCB->MaxHeight / 2;
	}
	else { /* the layout is bigger than A0; most unlikely, but... */
		/* pcb_fprintf(ctx->f, "%.0ml", PCB->MaxHeight); */
		LayoutYOffset = 0;
	}

	ctx->ox = LayoutXOffset;
	ctx->oy = LayoutYOffset;
}

static void kicad_print_implicit_outline(wctx_t *ctx, const char *lynam, pcb_coord_t thick, int ind)
{
	fprintf(ctx->f, "%*s", ind, "");
	pcb_fprintf(ctx->f, "(gr_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
		ctx->ox, ctx->oy,
		ctx->pcb->MaxWidth + ctx->ox, ctx->oy,
		lynam, thick);
	fprintf(ctx->f, "%*s", ind, "");
	pcb_fprintf(ctx->f, "(gr_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
		ctx->pcb->MaxWidth + ctx->ox, ctx->oy,
		ctx->pcb->MaxWidth + ctx->ox, ctx->pcb->MaxHeight + ctx->oy,
		lynam, thick);
	fprintf(ctx->f, "%*s", ind, "");
	pcb_fprintf(ctx->f, "(gr_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
		ctx->pcb->MaxWidth + ctx->ox, ctx->pcb->MaxHeight + ctx->oy,
		ctx->ox, ctx->pcb->MaxHeight + ctx->oy,
		lynam, thick);
	fprintf(ctx->f, "%*s", ind, "");
	pcb_fprintf(ctx->f, "(gr_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
		ctx->ox, ctx->pcb->MaxHeight + ctx->oy,
		ctx->ox, ctx->oy,
		lynam, thick);
}

static void kicad_fixup_outline(wctx_t *ctx, int ind)
{
	int n;
	fixed_layer_t *l;

	/* if any outline layer is non-empty, don't draw the implicit outline */
	for(n = 0; n < ctx->pcb->Data->LayerN; n++) {
		if (!(pcb_layer_flags(ctx->pcb, n) & PCB_LYT_OUTLINE))
			continue;
		if (!pcb_layer_is_pure_empty(&ctx->pcb->Data->Layer[n]))
			return;
	}

	/* find the first kicad outline layer */
	for(l = fixed_layers; l->name != NULL; l++) {
		if (l->type & PCB_LYT_OUTLINE) {
			kicad_print_implicit_outline(ctx, l->name, KICAD_OUTLINE_THICK, ind);
			return;
		}
	}

	pcb_message(PCB_MSG_ERROR, "io_kicad: internal error: can not find output outline layer for drawing the implicit outline\n");
}

/* ---------------------------------------------------------------------------
 * writes PCB to file in s-expression format
 */
int io_kicad_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	wctx_t wctx;
	int baseSExprIndent = 2;
	pcb_cardinal_t i;
	int layer = 0;

	memset(&wctx, 0, sizeof(wctx));
	wctx.pcb = PCB;
	wctx.f = FP;

	/* Kicad string quoting pattern: protect parenthesis, whitespace, quote and backslash */
	pcb_printf_slot[4] = "%{\\()\t\r\n \"}mq";

	fprintf(FP, "(kicad_pcb (version 3) (host pcb-rnd \"(%s %s)\")", PCB_VERSION, PCB_REVISION);

	fprintf(FP, "\n%*s(general\n", baseSExprIndent, "");
	fprintf(FP, "%*s)\n", baseSExprIndent, "");

	kicad_paper(&wctx, baseSExprIndent);
	kicad_map_layers(&wctx);
	kicad_print_layers(&wctx, baseSExprIndent);

	/* setup section */
	fprintf(FP, "\n%*s(setup\n", baseSExprIndent, "");
	write_kicad_layout_via_drill_size(FP, baseSExprIndent + 2);
	fprintf(FP, "%*s)\n", baseSExprIndent, "");

	/* now come the netlist "equipotential" descriptors */

	write_kicad_equipotential_netlists(FP, PCB, baseSExprIndent);

	/* module descriptions come next */
	fputs("\n", FP);

	write_kicad_layout_elements(FP, PCB, PCB->Data, wctx.ox, wctx.oy, baseSExprIndent);
	kicad_print_subcs(&wctx, PCB->Data, baseSExprIndent);
	kicad_print_data(&wctx, PCB->Data, baseSExprIndent);

	kicad_fixup_outline(&wctx, baseSExprIndent);

	fputs(")\n", FP); /* finish off the board */

	return 0;
}

