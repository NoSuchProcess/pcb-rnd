/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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

#include <ctype.h>
#include "config.h"
#include "const.h"
#include "data.h"
#include "props.h"
#include "propsel.h"
#include "change.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "undo.h"
#include "rotate.h"

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
		strcpy(nm+2, list->List[i].name);
		map_add_prop(ctx, nm, String, list->List[i].value);
	}
	if (big != NULL)
		free(big);
}

static void map_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	map_chk_skip(ctx, line);
	map_add_prop(ctx, "p/trace/thickness", Coord, line->Thickness);
	map_add_prop(ctx, "p/trace/clearance", Coord, line->Clearance);
	map_attr(ctx, &line->Attributes);
}

static void map_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
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

static void map_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, TextType *text)
{
	map_chk_skip(ctx, text);
	map_add_prop(ctx, "p/text/scale", int, text->Scale);
	map_add_prop(ctx, "p/text/rotation", int, text->Direction);
	map_add_prop(ctx, "p/text/string", String, text->TextString);
	map_attr(ctx, &text->Attributes);
}

static void map_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_polygon_t *poly)
{
	map_chk_skip(ctx, poly);
	map_attr(ctx, &poly->Attributes);
}

static void map_eline_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_line_t *line)
{
	map_chk_skip(ctx, line);
	map_line_cb(ctx, pcb, NULL, line);
	map_attr(ctx, &line->Attributes);
}

static void map_earc_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_arc_t *arc)
{
	map_chk_skip(ctx, arc);
	map_arc_cb(ctx, pcb, NULL, arc);
	map_attr(ctx, &arc->Attributes);
}

static void map_etext_cb(void *ctx, pcb_board_t *pcb, ElementType *element, TextType *text)
{
	map_chk_skip(ctx, text);
	map_text_cb(ctx, pcb, NULL, text);
	map_attr(ctx, &text->Attributes);
}

static void map_epin_cb(void *ctx, pcb_board_t *pcb, ElementType *element, PinType *pin)
{
	map_chk_skip(ctx, pin);
	map_add_prop(ctx, "p/pin/thickness", Coord, pin->Thickness);
	map_add_prop(ctx, "p/pin/clearance", Coord, pin->Clearance);
	map_add_prop(ctx, "p/pin/mask",      Coord, pin->Mask);
	map_add_prop(ctx, "p/pin/hole",      Coord, pin->DrillingHole);
	map_attr(ctx, &pin->Attributes);
}

static void map_epad_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_pad_t *pad)
{
	map_chk_skip(ctx, pad);
	map_add_prop(ctx, "p/pad/mask",      Coord, pad->Mask);
	map_attr(ctx, &pad->Attributes);
}



static void map_via_cb(void *ctx, pcb_board_t *pcb, PinType *via)
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

/*******************/

typedef struct set_ctx_s {
	const char *name;
	const char *value;
	Coord c;
	double d;
	pcb_bool c_absolute, d_absolute, c_valid, d_valid, is_trace, is_attr;

	int set_cnt;
} set_ctx_t;

static void set_attr(set_ctx_t *st, AttributeListType *list)
{
	const char *key = st->name+2;
	const char *orig = AttributeGetFromList(list, key);

	if ((orig != NULL) && (strcmp(orig, st->value) == 0))
		return;

	AttributePutToList(list, key, st->value, 1);
	st->set_cnt++;
}

#define set_chk_skip(ctx, obj) \
	if (!TEST_FLAG(PCB_FLAG_SELECTED, obj)) return;

#define DONE { st->set_cnt++; RestoreUndoSerialNumber(); return; }

static void set_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 8;

	set_chk_skip(st, line);

	if (st->is_attr) {
		set_attr(st, &line->Attributes);
		return;
	}

	if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    ChangeObject1stSize(PCB_TYPE_LINE, layer, line, NULL, st->c, st->c_absolute)) DONE;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    ChangeObjectClearSize(PCB_TYPE_LINE, layer, line, NULL, st->c, st->c_absolute)) DONE;
}

static void set_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 8;

	set_chk_skip(st, arc);

	if (st->is_attr) {
		set_attr(st, &arc->Attributes);
		return;
	}

	if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    ChangeObject1stSize(PCB_TYPE_ARC, layer, arc, NULL, st->c, st->c_absolute)) DONE;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    ChangeObjectClearSize(PCB_TYPE_ARC, layer, arc, NULL, st->c, st->c_absolute)) DONE;

	pn = st->name + 6;

	if (!st->is_trace && st->c_valid && (strcmp(pn, "width") == 0) &&
	    ChangeObjectRadius(PCB_TYPE_ARC, layer, arc, NULL, 0, st->c, st->c_absolute)) DONE;

	if (!st->is_trace && st->c_valid && (strcmp(pn, "height") == 0) &&
	    ChangeObjectRadius(PCB_TYPE_ARC, layer, arc, NULL, 1, st->c, st->c_absolute)) DONE;

	if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/start") == 0) &&
	    ChangeObjectAngle(PCB_TYPE_ARC, layer, arc, NULL, 0, st->d, st->d_absolute)) DONE;

	if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/delta") == 0) &&
	    ChangeObjectAngle(PCB_TYPE_ARC, layer, arc, NULL, 1, st->d, st->d_absolute)) DONE;
}

static void set_text_cb_any(void *ctx, pcb_board_t *pcb, int type, void *layer_or_element, TextType *text)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 7;
	char *old;

	set_chk_skip(st, text);

	if (st->is_attr) {
		set_attr(st, &text->Attributes);
		return;
	}

	if (st->d_valid && (strcmp(pn, "scale") == 0) &&
	    ChangeObjectSize(type, layer_or_element, text, text, PCB_MIL_TO_COORD(st->d), st->d_absolute)) DONE;

	if ((strcmp(pn, "string") == 0) &&
	    (old = ChangeObjectName(type, layer_or_element, text, NULL, pcb_strdup(st->value)))) {
		free(old);
		DONE;
	}

	if (st->d_valid && (strcmp(pn, "rotation") == 0)) {
		int delta;
		if (st->d_absolute) {
			delta = (st->d - text->Direction);
			if (delta < 0)
				delta += 4;
			delta = (unsigned int)delta & 3;
		}
		else
			delta = st->d;
		if (RotateObject(type, layer_or_element, text, text, text->X, text->Y, delta) != NULL)
			DONE;
	}
}

static void set_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, TextType *text)
{
	set_text_cb_any(ctx, pcb, PCB_TYPE_TEXT, layer, text);
}


static void set_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_polygon_t *poly)
{
	set_ctx_t *st = (set_ctx_t *)ctx;

	set_chk_skip(st, poly);

	if (st->is_attr) {
		set_attr(st, &poly->Attributes);
		return;
	}
}

static void set_eline_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_line_t *line)
{
	set_ctx_t *st = (set_ctx_t *)ctx;

	set_chk_skip(st, line);

	if (st->is_attr) {
		set_attr(st, &line->Attributes);
		return;
	}
}

static void set_earc_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_arc_t *arc)
{
	set_ctx_t *st = (set_ctx_t *)ctx;

	set_chk_skip(st, arc);

	if (st->is_attr) {
		set_attr(st, &arc->Attributes);
		return;
	}
}

static void set_etext_cb(void *ctx, pcb_board_t *pcb, ElementType *element, TextType *text)
{
	set_text_cb_any(ctx, pcb, PCB_TYPE_ELEMENT_NAME, element, text);
}

static void set_epin_cb(void *ctx, pcb_board_t *pcb, ElementType *element, PinType *pin)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 6;

	set_chk_skip(st, pin);

	if (st->is_attr) {
		set_attr(st, &pin->Attributes);
		return;
	}

	if (st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    ChangeObject1stSize(PCB_TYPE_PIN, pin->Element, pin, NULL, st->c, st->c_absolute)) DONE;

	if (st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    ChangeObjectClearSize(PCB_TYPE_PIN, pin->Element, pin, NULL, st->c, st->c_absolute)) DONE;

	if (st->c_valid && (strcmp(pn, "mask") == 0) &&
	    ChangeObjectMaskSize(PCB_TYPE_PIN, pin->Element, pin, NULL, st->c, st->c_absolute)) DONE;

	if (st->c_valid && (strcmp(pn, "hole") == 0) &&
	    ChangeObject2ndSize(PCB_TYPE_PIN, pin->Element, pin, NULL, st->c, st->c_absolute, pcb_false)) DONE;
}

static void set_epad_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_pad_t *pad)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 6;

	set_chk_skip(st, pad);

	if (st->is_attr) {
		set_attr(st, &pad->Attributes);
		return;
	}

	if (st->c_valid && (strcmp(pn, "mask") == 0) &&
	    ChangeObjectMaskSize(PCB_TYPE_PAD, pad->Element, pad, NULL, st->c, st->c_absolute)) DONE;
}

static void set_via_cb(void *ctx, pcb_board_t *pcb, PinType *via)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 6;

	set_chk_skip(st, via);

	if (st->is_attr) {
		set_attr(st, &via->Attributes);
		return;
	}

	if (st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    ChangeObject1stSize(PCB_TYPE_VIA, via, via, NULL, st->c, st->c_absolute)) DONE;

	if (st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    ChangeObjectClearSize(PCB_TYPE_VIA, via, via, NULL, st->c, st->c_absolute)) DONE;

	if (st->c_valid && (strcmp(pn, "mask") == 0) &&
	    ChangeObjectMaskSize(PCB_TYPE_VIA, via, via, NULL, st->c, st->c_absolute)) DONE;

	if (st->c_valid && (strcmp(pn, "hole") == 0) &&
	    ChangeObject2ndSize(PCB_TYPE_VIA, via, via, NULL, st->c, st->c_absolute, pcb_false)) DONE;
}

/* use the callback if trc is true or prop matches a prefix or we are setting attributes, else NULL */
#define MAYBE_PROP(trc, prefix, cb) \
	(((ctx.is_attr) || (trc) || (strncmp(prop, (prefix), sizeof(prefix)-1) == 0) || (prop[0] == 'a')) ? (cb) : NULL)

#define MAYBE_ATTR(cb) \
	((ctx.is_attr) ? (cb) : NULL)

int pcb_propsel_set(const char *prop, const char *value)
{
	set_ctx_t ctx;
	char *end;
	const char *start;

	/* sanity checks for invalid props */
	if (prop[1] != '/')
		return 0;
	if ((prop[0] != 'a') && (prop[0] != 'p'))
		return 0;

	ctx.is_trace = (strncmp(prop, "p/trace/", 8) == 0);
	ctx.is_attr = (prop[0] == 'a');
	ctx.name = prop;
	ctx.value = value;
	ctx.c = GetValueEx(value, NULL, &ctx.c_absolute, NULL, NULL, &ctx.c_valid);
	ctx.d = strtod(value, &end);
	ctx.d_valid = (*end == '\0');
	start = value;
	while(isspace(*start)) start++;
	ctx.d_absolute = ((*start != '-') && (*start != '+'));
	ctx.set_cnt = 0;

	SaveUndoSerialNumber();

	pcb_loop_all(&ctx,
		NULL,
		MAYBE_PROP(ctx.is_trace, "p/line/", set_line_cb),
		MAYBE_PROP(ctx.is_trace, "p/arc/", set_arc_cb),
		MAYBE_PROP(0, "p/text/", set_text_cb),
		MAYBE_ATTR(set_poly_cb),
		NULL,
		MAYBE_ATTR(set_eline_cb),
		MAYBE_ATTR(set_earc_cb),
		MAYBE_PROP(0, "p/text/", set_etext_cb),
		MAYBE_PROP(0, "p/pin/", set_epin_cb),
		MAYBE_PROP(0, "p/pad/", set_epad_cb),
		MAYBE_PROP(0, "p/via/", set_via_cb)
	);
	IncrementUndoSerialNumber();
	return ctx.set_cnt;
}

#undef MAYBE_PROP
#undef MAYBE_ATTR

/*******************/

typedef struct del_ctx_s {
	const char *key;
	int del_cnt;
} del_ctx_t;

static void del_attr(void *ctx, AttributeListType *list)
{
	del_ctx_t *st = (del_ctx_t *)ctx;
	if (AttributeRemoveFromList(list, st->key+2))
		st->del_cnt++;
}

static void del_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	map_chk_skip(ctx, line);
	del_attr(ctx, &line->Attributes);
}

static void del_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	map_chk_skip(ctx, arc);
	del_attr(ctx, &arc->Attributes);
}

static void del_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, TextType *text)
{
	map_chk_skip(ctx, text);
	del_attr(ctx, &text->Attributes);
}

static void del_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_polygon_t *poly)
{
	map_chk_skip(ctx, poly);
	del_attr(ctx, &poly->Attributes);
}

static void del_eline_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_line_t *line)
{
	map_chk_skip(ctx, line);
	del_attr(ctx, &line->Attributes);
}

static void del_earc_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_arc_t *arc)
{
	map_chk_skip(ctx, arc);
	del_attr(ctx, &arc->Attributes);
}

static void del_etext_cb(void *ctx, pcb_board_t *pcb, ElementType *element, TextType *text)
{
	map_chk_skip(ctx, text);
	del_attr(ctx, &text->Attributes);
}

static void del_epin_cb(void *ctx, pcb_board_t *pcb, ElementType *element, PinType *pin)
{
	map_chk_skip(ctx, pin);
	del_attr(ctx, &pin->Attributes);
}

static void del_epad_cb(void *ctx, pcb_board_t *pcb, ElementType *element, pcb_pad_t *pad)
{
	map_chk_skip(ctx, pad);
	del_attr(ctx, &pad->Attributes);
}

static void del_via_cb(void *ctx, pcb_board_t *pcb, PinType *via)
{
	map_chk_skip(ctx, via);
	del_attr(ctx, &via->Attributes);
}

int pcb_propsel_del(const char *key)
{
	del_ctx_t st;

	if ((key[0] != 'a') || (key[1] != '/')) /* do not attempt to remove anything but attributes */
		return 0;

	st.key = key;
	st.del_cnt = 0;

	pcb_loop_all(&st,
		NULL, del_line_cb, del_arc_cb, del_text_cb, del_poly_cb,
		NULL, del_eline_cb, del_earc_cb, del_etext_cb, del_epin_cb, del_epad_cb,
		del_via_cb
	);
	return st.del_cnt;
}


