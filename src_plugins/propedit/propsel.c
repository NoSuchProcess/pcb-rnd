/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design - pcb-rnd
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include "data.h"
#include "props.h"
#include "propsel.h"

/*********** map ***********/
typedef struct {
	htsp_t *props;
} map_ctx_t;

#define map_chk_skip(ctx, obj) \
	if (!TEST_FLAG(PCB_FLAG_SELECTED, obj)) return;

#define type2field_Coord coord
#define type2field_Angle angle
#define type2field_int i
#define type2TYPE_Coord PCB_PROPT_COORD
#define type2TYPE_Angle PCB_PROPT_ANGLE
#define type2TYPE_int PCB_PROPT_INT

#define map_add_prop(ctx, name, type, val) \
do { \
	pcb_propval_t v; \
	v.type2field_ ## type = (val);  \
	pcb_props_add(((map_ctx_t *)ctx)->props, name, type2TYPE_ ## type, v); \
} while(0)

static void map_line_cb(void *ctx, PCBType *pcb, LayerType *layer, LineType *line)
{
	map_chk_skip(ctx, line);
	map_add_prop(ctx, "trace/thickness", Coord, line->Thickness);
	map_add_prop(ctx, "trace/clearance", Coord, line->Clearance);
}

static void map_arc_cb(void *ctx, PCBType *pcb, LayerType *layer, ArcType *arc)
{
	map_chk_skip(ctx, arc);
	map_add_prop(ctx, "trace/thickness", Coord, arc->Thickness);
	map_add_prop(ctx, "trace/clearance", Coord, arc->Clearance);
	map_add_prop(ctx, "arc/width",       Coord, arc->Width);
	map_add_prop(ctx, "arc/height",      Coord, arc->Height);
	map_add_prop(ctx, "arc/angle/start", Angle, arc->StartAngle);
	map_add_prop(ctx, "arc/angle/delta", Angle, arc->Delta);
}

static void map_text_cb(void *ctx, PCBType *pcb, LayerType *layer, TextType *text)
{
	map_chk_skip(ctx, text);
	map_add_prop(ctx, "text/scale", int, text->Scale);
	map_add_prop(ctx, "text/rotation", int, text->Direction);
}

static void map_poly_cb(void *ctx, PCBType *pcb, LayerType *layer, PolygonType *poly)
{
	map_chk_skip(ctx, poly);
}

static void map_eline_cb(void *ctx, PCBType *pcb, ElementType *element, LineType *line)
{
	map_chk_skip(ctx, line);
	map_line_cb(ctx, pcb, NULL, line);
}

static void map_earc_cb(void *ctx, PCBType *pcb, ElementType *element, ArcType *arc)
{
	map_chk_skip(ctx, arc);
	map_arc_cb(ctx, pcb, NULL, arc);
}

static void map_etext_cb(void *ctx, PCBType *pcb, ElementType *element, TextType *text)
{
	map_chk_skip(ctx, text);
	map_text_cb(ctx, pcb, NULL, text);
}

static void map_epin_cb(void *ctx, PCBType *pcb, ElementType *element, PinType *pin)
{
	map_chk_skip(ctx, pin);
	map_add_prop(ctx, "pin/thickness", Coord, pin->Thickness);
	map_add_prop(ctx, "pin/clearance", Coord, pin->Clearance);
	map_add_prop(ctx, "pin/mask",      Coord, pin->Mask);
	map_add_prop(ctx, "pin/hole",      Coord, pin->DrillingHole);
}

static void map_epad_cb(void *ctx, PCBType *pcb, ElementType *element, PadType *pad)
{
	map_chk_skip(ctx, pad);
	map_add_prop(ctx, "pad/mask",      Coord, pad->Mask);
}



static void map_via_cb(void *ctx, PCBType *pcb, PinType *via)
{
	map_chk_skip(ctx, via);
	map_add_prop(ctx, "via/thickness", Coord, via->Thickness);
	map_add_prop(ctx, "via/clearance", Coord, via->Clearance);
	map_add_prop(ctx, "via/mask",      Coord, via->Mask);
	map_add_prop(ctx, "via/hole",      Coord, via->DrillingHole);
}

void pcb_propsel_map_core(htsp_t *props)
{
	map_ctx_t ctx;

	ctx.props = props;
	
	pcb_loop_all(&ctx,
		NULL, map_line_cb, map_arc_cb, map_text_cb, map_poly_cb,
		NULL, map_eline_cb, map_earc_cb, map_etext_cb, map_epin_cb, map_epad_cb,
		map_via_cb
	);
}


/*
int pcb_propsel_apply();
*/
