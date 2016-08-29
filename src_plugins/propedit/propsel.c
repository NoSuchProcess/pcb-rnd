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

#define type2field_String string
#define type2field_Coord coord
#define type2field_Angle angle
#define type2field_int i

#define type2TYPE_String PCB_PROPT_STRING
#define type2TYPE_Coord PCB_PROPT_COORD
#define type2TYPE_Angle PCB_PROPT_ANGLE
#define type2TYPE_int PCB_PROPT_INT

#define map_add_prop(ctx, name, type, val) \
do { \
	pcb_propval_t v; \
	v.type2field_ ## type = (val);  \
	pcb_props_add(((map_ctx_t *)ctx)->props, name, type2TYPE_ ## type, v); \
} while(0)

static void map_attr(void *ctx, const AttributeListType *list)
{
	int i, bl = 0;
	char small[256];
	char *big = NULL;

	small[0] = 'a';
	small[1] = '/';

	for (i = 0; i < list->Number; i++) {
		int len = strlen(list->List[i].name);
		char *nm;
		if (len >= sizeof(small)-3) {
			if (len > bl) {
				bl = len + 128;
				if (big != NULL)
					free(big);
				big = malloc(bl);
				big[0] = 'a';
				big[1] = '/';
			}
			nm = big;
		}
		else
			nm = small;

		map_add_prop(ctx, nm, String, list->List[i].value);
	}
	if (big != NULL)
		free(big);
}

static void map_line_cb(void *ctx, PCBType *pcb, LayerType *layer, LineType *line)
{
	map_chk_skip(ctx, line);
	map_add_prop(ctx, "p/trace/thickness", Coord, line->Thickness);
	map_add_prop(ctx, "p/trace/clearance", Coord, line->Clearance);
	map_attr(ctx, &line->Attributes);
}

static void map_arc_cb(void *ctx, PCBType *pcb, LayerType *layer, ArcType *arc)
{
	map_chk_skip(ctx, arc);
	map_add_prop(ctx, "p/trace/thickness", Coord, arc->Thickness);
	map_add_prop(ctx, "p/trace/clearance", Coord, arc->Clearance);
	map_add_prop(ctx, "p/arc/width",       Coord, arc->Width);
	map_add_prop(ctx, "p/arc/height",      Coord, arc->Height);
	map_add_prop(ctx, "p/arc/angle/start", Angle, arc->StartAngle);
	map_add_prop(ctx, "p/arc/angle/delta", Angle, arc->Delta);
	map_attr(ctx, &arc->Attributes);
}

static void map_text_cb(void *ctx, PCBType *pcb, LayerType *layer, TextType *text)
{
	map_chk_skip(ctx, text);
	map_add_prop(ctx, "p/text/scale", int, text->Scale);
	map_add_prop(ctx, "p/text/rotation", int, text->Direction);
	map_attr(ctx, &text->Attributes);
}

static void map_poly_cb(void *ctx, PCBType *pcb, LayerType *layer, PolygonType *poly)
{
	map_chk_skip(ctx, poly);
	map_attr(ctx, &poly->Attributes);
}

static void map_eline_cb(void *ctx, PCBType *pcb, ElementType *element, LineType *line)
{
	map_chk_skip(ctx, line);
	map_line_cb(ctx, pcb, NULL, line);
	map_attr(ctx, &line->Attributes);
}

static void map_earc_cb(void *ctx, PCBType *pcb, ElementType *element, ArcType *arc)
{
	map_chk_skip(ctx, arc);
	map_arc_cb(ctx, pcb, NULL, arc);
	map_attr(ctx, &arc->Attributes);
}

static void map_etext_cb(void *ctx, PCBType *pcb, ElementType *element, TextType *text)
{
	map_chk_skip(ctx, text);
	map_text_cb(ctx, pcb, NULL, text);
	map_attr(ctx, &text->Attributes);
}

static void map_epin_cb(void *ctx, PCBType *pcb, ElementType *element, PinType *pin)
{
	map_chk_skip(ctx, pin);
	map_add_prop(ctx, "p/pin/thickness", Coord, pin->Thickness);
	map_add_prop(ctx, "p/pin/clearance", Coord, pin->Clearance);
	map_add_prop(ctx, "p/pin/mask",      Coord, pin->Mask);
	map_add_prop(ctx, "p/pin/hole",      Coord, pin->DrillingHole);
	map_attr(ctx, &pin->Attributes);
}

static void map_epad_cb(void *ctx, PCBType *pcb, ElementType *element, PadType *pad)
{
	map_chk_skip(ctx, pad);
	map_add_prop(ctx, "p/pad/mask",      Coord, pad->Mask);
	map_attr(ctx, &pad->Attributes);
}



static void map_via_cb(void *ctx, PCBType *pcb, PinType *via)
{
	map_chk_skip(ctx, via);
	map_add_prop(ctx, "p/via/thickness", Coord, via->Thickness);
	map_add_prop(ctx, "p/via/clearance", Coord, via->Clearance);
	map_add_prop(ctx, "p/via/mask",      Coord, via->Mask);
	map_add_prop(ctx, "p/via/hole",      Coord, via->DrillingHole);
	map_attr(ctx, &via->Attributes);
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
