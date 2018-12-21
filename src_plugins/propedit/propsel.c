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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <ctype.h>
#include "config.h"
#include "data.h"
#include "data_it.h"
#include "props.h"
#include "propsel.h"
#include "change.h"
#include "misc_util.h"
#include "flag_str.h"
#include "compat_misc.h"
#include "undo.h"
#include "rotate.h"
#include "obj_pstk_inlines.h"

/*********** map ***********/
#define type2field_String string
#define type2field_pcb_coord_t coord
#define type2field_pcb_angle_t angle
#define type2field_int i

#define type2TYPE_String PCB_PROPT_STRING
#define type2TYPE_pcb_coord_t PCB_PROPT_COORD
#define type2TYPE_pcb_angle_t PCB_PROPT_ANGLE
#define type2TYPE_int PCB_PROPT_INT

#define map_add_prop(ctx, name, type, val) \
do { \
	pcb_propval_t v; \
	v.type2field_ ## type = (val);  \
	pcb_props_add(ctx, name, type2TYPE_ ## type, v); \
} while(0)

static void map_attr(void *ctx, const pcb_attribute_list_t *list)
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

static void map_common(void *ctx, pcb_any_obj_t *obj)
{
	unsigned long bit;
	char name[256], *end;

	strcpy(name, "p/flags/");
	end = name + 8;
	for(bit = 1; bit < (1UL<<31); bit <<= 1) {
		const pcb_flag_bits_t *i;
		if ((bit == PCB_FLAG_SELECTED) || (bit == PCB_FLAG_RUBBEREND))
			continue;
		i = pcb_strflg_1bit(bit, obj->type);
		if (i == NULL)
			continue;
		strcpy(end, i->name);
		map_add_prop(ctx, name, int, PCB_FLAG_TEST(bit, obj));
	}
}

static void map_board(pcb_propedit_t *ctx, pcb_board_t *pcb)
{
	map_add_prop(ctx, "p/board/name",   String, pcb->Name);
	map_add_prop(ctx, "p/board/width", pcb_coord_t, pcb->MaxWidth);
	map_add_prop(ctx, "p/board/height", pcb_coord_t, pcb->MaxHeight);
	map_attr(ctx, &pcb->Attributes);
}

static void map_layer(pcb_propedit_t *ctx, pcb_layer_t *layer)
{
	if (layer == NULL)
		return;
	map_add_prop(ctx, "p/layer/name", String, layer->name);
	map_add_prop(ctx, "p/layer/comb/negative", int, !!(layer->comb & PCB_LYC_SUB));
	map_add_prop(ctx, "p/layer/comb/auto", int, !!(layer->comb & PCB_LYC_AUTO));
	if (!layer->is_bound)
		map_add_prop(ctx, "p/layer/color", String, layer->meta.real.color.str);
	map_attr(ctx, &layer->Attributes);
}

static void map_layergrp(pcb_propedit_t *ctx, pcb_layergrp_t *grp)
{
	if (grp == NULL)
		return;
	map_add_prop(ctx, "p/layer_group/name", String, grp->name);
	map_add_prop(ctx, "p/layer_group/purpose", String, grp->purpose);
	map_attr(ctx, &grp->Attributes);
}

static void map_line(pcb_propedit_t *ctx, pcb_line_t *line)
{
	map_add_prop(ctx, "p/trace/thickness", pcb_coord_t, line->Thickness);
	map_add_prop(ctx, "p/trace/clearance", pcb_coord_t, line->Clearance/2);
	map_common(ctx, (pcb_any_obj_t *)line);
	map_attr(ctx, &line->Attributes);
}

static void map_arc(pcb_propedit_t *ctx, pcb_arc_t *arc)
{
	map_add_prop(ctx, "p/trace/thickness", pcb_coord_t, arc->Thickness);
	map_add_prop(ctx, "p/trace/clearance", pcb_coord_t, arc->Clearance/2);
	map_add_prop(ctx, "p/arc/width",       pcb_coord_t, arc->Width);
	map_add_prop(ctx, "p/arc/height",      pcb_coord_t, arc->Height);
	map_add_prop(ctx, "p/arc/angle/start", pcb_angle_t, arc->StartAngle);
	map_add_prop(ctx, "p/arc/angle/delta", pcb_angle_t, arc->Delta);
	map_common(ctx, (pcb_any_obj_t *)arc);
	map_attr(ctx, &arc->Attributes);
}

static void map_text(pcb_propedit_t *ctx, pcb_text_t *text)
{
	map_add_prop(ctx, "p/text/scale", int, text->Scale);
	map_add_prop(ctx, "p/text/rotation",  pcb_angle_t, text->rot);
	map_add_prop(ctx, "p/text/thickness", pcb_coord_t, text->thickness);
	map_add_prop(ctx, "p/text/string", String, text->TextString);
	map_common(ctx, (pcb_any_obj_t *)text);
	map_attr(ctx, &text->Attributes);
}

static void map_poly(pcb_propedit_t *ctx, pcb_poly_t *poly)
{
	map_attr(ctx, &poly->Attributes);
	map_common(ctx, (pcb_any_obj_t *)poly);
	map_add_prop(ctx, "p/trace/clearance", pcb_coord_t, poly->Clearance/2);
}

static void map_pstk(pcb_propedit_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto;

	map_add_prop(ctx, "p/padstack/xmirror", pcb_coord_t, ps->xmirror);
	map_add_prop(ctx, "p/padstack/smirror", pcb_coord_t, ps->smirror);
	map_add_prop(ctx, "p/padstack/rotation", pcb_angle_t, ps->rot);
	map_add_prop(ctx, "p/padstack/proto", pcb_coord_t, ps->proto);

	proto = pcb_pstk_get_proto(ps);
	map_add_prop(ctx, "p/padstack/clearance", pcb_coord_t, ps->Clearance);
	map_add_prop(ctx, "p/padstack/hole", pcb_coord_t, proto->hdia);
	map_add_prop(ctx, "p/padstack/plated", int, proto->hplated);
	map_add_prop(ctx, "p/padstack/htop", int, proto->htop);
	map_add_prop(ctx, "p/padstack/hbottom", int, proto->hbottom);

	map_attr(ctx, &ps->Attributes);
	map_common(ctx, (pcb_any_obj_t *)ps);
}

static void map_subc(pcb_propedit_t *ctx, pcb_subc_t *msubc)
{
	PCB_ARC_ALL_LOOP(msubc->data); {
		if (pcb_subc_part_editable(ctx->pcb, arc))
			map_arc(ctx, arc);
	} PCB_ENDALL_LOOP;
	PCB_LINE_ALL_LOOP(msubc->data); {
		if (pcb_subc_part_editable(ctx->pcb, line))
			map_line(ctx, line);
	} PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(msubc->data); {
		if (pcb_subc_part_editable(ctx->pcb, polygon))
			map_poly(ctx, polygon);
	} PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(msubc->data); {
		if (pcb_subc_part_editable(ctx->pcb, text))
			map_text(ctx, text);
	} PCB_ENDALL_LOOP;
	PCB_PADSTACK_LOOP(msubc->data); {
		if (pcb_subc_part_editable(ctx->pcb, padstack))
			map_pstk(ctx, padstack);
	} PCB_END_LOOP;
	PCB_SUBC_LOOP(msubc->data); {
		if (pcb_subc_part_editable(ctx->pcb, subc))
			map_subc(ctx, subc);
	} PCB_END_LOOP;
	map_attr(ctx, &msubc->Attributes);
	map_common(ctx, (pcb_any_obj_t *)msubc);
}

static void map_any(pcb_propedit_t *ctx, pcb_any_obj_t *o)
{
	if (o == NULL)
		return;
	switch(o->type) {
		case PCB_OBJ_ARC:  map_arc(ctx, (pcb_arc_t *)o); break;
		case PCB_OBJ_LINE: map_line(ctx, (pcb_line_t *)o); break;
		case PCB_OBJ_POLY: map_poly(ctx, (pcb_poly_t *)o); break;
		case PCB_OBJ_TEXT: map_text(ctx, (pcb_text_t *)o); break;
		case PCB_OBJ_SUBC: map_subc(ctx, (pcb_subc_t *)o); break;
		case PCB_OBJ_PSTK: map_pstk(ctx, (pcb_pstk_t *)o); break;
		default: break;
	}
}

void pcb_propsel_map_core(pcb_propedit_t *ctx)
{
	pcb_idpath_t *idp;
	size_t n;

	for(n = 0; n < vtl0_len(&ctx->layers); n++)
		map_layer(ctx, pcb_get_layer(ctx->pcb->Data, ctx->layers.array[n]));

	for(n = 0; n < vtl0_len(&ctx->layergrps); n++)
		map_layergrp(ctx, pcb_get_layergrp(ctx->pcb, ctx->layergrps.array[n]));

	for(idp = pcb_idpath_list_first(&ctx->objs); idp != NULL; pcb_idpath_list_next(idp))
		map_any(ctx, pcb_idpath2obj(ctx->pcb->Data, idp));

	if (ctx->selection) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		for(o = pcb_data_first(&it, ctx->pcb->Data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it))
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))
				map_any(ctx, o);
	}

	if (ctx->board)
		map_board(ctx, ctx->pcb);
}

/*******************/

typedef struct set_ctx_s {
	pcb_board_t *pcb;
	const char *name;
	const char *value;
	pcb_coord_t c;
	double d;
	pcb_bool c_absolute, d_absolute, c_valid, d_valid, is_trace, is_attr;

	int set_cnt;
} set_ctx_t;

static void set_attr(set_ctx_t *st, pcb_attribute_list_t *list)
{
	const char *key = st->name+2;
	const char *orig = pcb_attribute_get(list, key);

	if ((orig != NULL) && (strcmp(orig, st->value) == 0))
		return;

	pcb_attribute_put(list, key, st->value);
	st->set_cnt++;
}

#define DONE { st->set_cnt++; pcb_undo_restore_serial(); return; }
#define DONE0 { st->set_cnt++; pcb_undo_restore_serial(); return 0; }
#define DONE1 { st->set_cnt++; pcb_undo_restore_serial(); return 1; }

static int set_common(set_ctx_t *st, pcb_any_obj_t *obj)
{
	const pcb_flag_bits_t *i = pcb_strflg_name(st->name + 8, obj->type);

	if (i != NULL) {
		pcb_flag_change(st->pcb, st->c ? PCB_CHGFLG_SET : PCB_CHGFLG_CLEAR, i->mask, obj->type, obj->parent.any, obj, obj);
		DONE1;
	}

	return 0;
}

static int brd_resize(pcb_coord_t w, pcb_coord_t h)
{
	pcb_board_resize(w, h);
	return 1;
}

static void set_board(set_ctx_t *st, pcb_board_t *pcb)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr(st, &pcb->Attributes);
		return;
	}

	if ((strcmp(pn, "name") == 0) &&
	    (pcb_board_change_name(pcb_strdup(st->value)))) DONE;

	if (st->c_valid && (strcmp(pn, "width") == 0) &&
	    brd_resize(st->c, PCB->MaxHeight)) DONE;

	if (st->c_valid && (strcmp(pn, "height") == 0) &&
	    brd_resize(PCB->MaxWidth,st->c)) DONE;
}

static int layer_recolor(pcb_layer_t *layer, const char *clr)
{
	pcb_color_t c;
	if (pcb_color_load_str(&c, clr) != 0)
		return -1;
	return pcb_layer_recolor_(layer, &c);
}

static int set_layer(set_ctx_t *st, pcb_layer_t *layer)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr(st, &layer->Attributes);
		return 0;
	}

	if ((strcmp(pn, "name") == 0) &&
	    (pcb_layer_rename_(layer, pcb_strdup(st->value)) == 0)) DONE0;

	if ((strcmp(pn, "color") == 0) &&
	    (layer_recolor(layer, st->value) == 0)) DONE0;

	return 0;
}


static void set_layergrp(set_ctx_t *st, pcb_layergrp_t *grp)
{
	const char *pn = st->name + 14;

	if (st->is_attr) {
		set_attr(st, &grp->Attributes);
		return;
	}

	if ((strcmp(pn, "name") == 0) &&
	    (pcb_layergrp_rename_(grp, pcb_strdup(st->value)) == 0)) DONE;

	if ((strcmp(pn, "purpose") == 0) &&
	    (pcb_layergrp_set_purpose(grp, st->value) == 0)) DONE;
}


static void set_line(set_ctx_t *st, pcb_line_t *line)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr(st, &line->Attributes);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)line)) return;

	if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    pcb_chg_obj_1st_size(PCB_OBJ_LINE, line->parent.layer, line, NULL, st->c, st->c_absolute)) DONE;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_LINE, line->parent.layer, line, NULL, st->c*2, st->c_absolute)) DONE;
}

static void set_arc(set_ctx_t *st, pcb_arc_t *arc)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr(st, &arc->Attributes);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)arc)) return;

	if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    pcb_chg_obj_1st_size(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, st->c, st->c_absolute)) DONE;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, st->c*2, st->c_absolute)) DONE;

	pn = st->name + 6;

	if (!st->is_trace && st->c_valid && (strcmp(pn, "width") == 0) &&
	    pcb_chg_obj_radius(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 0, st->c, st->c_absolute)) DONE;

	if (!st->is_trace && st->c_valid && (strcmp(pn, "height") == 0) &&
	    pcb_chg_obj_radius(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 1, st->c, st->c_absolute)) DONE;

	if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/start") == 0) &&
	    pcb_chg_obj_angle(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 0, st->d, st->d_absolute)) DONE;

	if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/delta") == 0) &&
	    pcb_chg_obj_angle(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 1, st->d, st->d_absolute)) DONE;
}

static void set_text(set_ctx_t *st, pcb_text_t *text)
{
	const char *pn = st->name + 7;
	char *old;

	if (st->is_attr) {
		set_attr(st, &text->Attributes);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)text)) return;

	if (st->d_valid && (strcmp(pn, "scale") == 0) &&
	    pcb_chg_obj_size(PCB_OBJ_TEXT, text->parent.layer, text, text, PCB_MIL_TO_COORD(st->d), st->d_absolute)) DONE;

	if ((strcmp(pn, "string") == 0) &&
	    (old = pcb_chg_obj_name(PCB_OBJ_TEXT, text->parent.layer, text, NULL, pcb_strdup(st->value)))) {
		free(old);
		DONE;
	}

	if (st->d_valid && (strcmp(pn, "rotation") == 0) &&
		pcb_chg_obj_rot(PCB_OBJ_TEXT, text->parent.layer, text, text, st->d, st->d_absolute, pcb_true)) DONE;

	if (st->c_valid && (strcmp(pn, "thickness") == 0) &&
		pcb_chg_obj_2nd_size(PCB_OBJ_TEXT, text->parent.layer, text, text, st->c, st->c_absolute, pcb_true)) DONE;
}

static void set_poly(set_ctx_t *st, pcb_poly_t *poly)
{
	const char *pn = st->name + 8;

	if (set_common(st, (pcb_any_obj_t *)poly)) return;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_POLY, poly->parent.layer, poly, NULL, st->c*2, st->c_absolute)) DONE;

	if (st->is_attr) {
		set_attr(st, &poly->Attributes);
		return;
	}
}

static void set_pstk(set_ctx_t *st, pcb_pstk_t *ps)
{
	const char *pn = st->name + 11;
	int i;
	pcb_cardinal_t ca;
	pcb_pstk_proto_t *proto;

	if (st->is_attr) {
		set_attr(st, &ps->Attributes);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)ps)) return;

	ca = i = (st->c != 0);
	proto = pcb_pstk_get_proto(ps);

	if (st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_PSTK, ps, ps, NULL, st->c*2, st->c_absolute)) DONE;
	if (st->d_valid && (strcmp(pn, "rotation") == 0)) {
		if (st->d_absolute) {
			if (pcb_obj_rotate(PCB_OBJ_PSTK, ps, ps, NULL, ps->x, ps->y, st->d - ps->rot)) DONE;
		}
		else {
			if (pcb_obj_rotate(PCB_OBJ_PSTK, ps, ps, NULL, ps->x, ps->y, st->d)) DONE;
		}
		return;
	}
	if (st->c_valid && (strcmp(pn, "xmirror") == 0) &&
	    (pcb_pstk_change_instance(ps, NULL, NULL, NULL, &i, NULL) == 0)) DONE;
	if (st->c_valid && (strcmp(pn, "smirror") == 0) &&
	    (pcb_pstk_change_instance(ps, NULL, NULL, NULL, NULL, &i) == 0)) DONE;
	if (st->c_valid && (strcmp(pn, "proto") == 0) &&
	    (pcb_pstk_change_instance(ps, &ca, NULL, NULL, NULL, NULL) == 0)) DONE;
	if (st->c_valid && (strcmp(pn, "hole") == 0) &&
	    (pcb_pstk_proto_change_hole(proto, NULL, &st->c, NULL, NULL) == 0)) DONE;
	if (st->c_valid && (strcmp(pn, "plated") == 0) &&
	    (pcb_pstk_proto_change_hole(proto, &i, NULL, NULL, NULL) == 0)) DONE;
	if (st->c_valid && (strcmp(pn, "htop") == 0) &&
	    (pcb_pstk_proto_change_hole(proto, NULL, NULL, &i, NULL) == 0)) DONE;
	if (st->c_valid && (strcmp(pn, "hbottom") == 0) &&
	    (pcb_pstk_proto_change_hole(proto, NULL, NULL, NULL, &i) == 0)) DONE;
}

static void set_subc(set_ctx_t *st, pcb_subc_t *ssubc)
{
	PCB_ARC_ALL_LOOP(ssubc->data); {
		if (pcb_subc_part_editable(st->pcb, arc))
			set_arc(st, arc);
	} PCB_ENDALL_LOOP;
	PCB_LINE_ALL_LOOP(ssubc->data); {
		if (pcb_subc_part_editable(st->pcb, line))
			set_line(st, line);
	} PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(ssubc->data); {
		if (pcb_subc_part_editable(st->pcb, polygon))
			set_poly(st, polygon);
	} PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(ssubc->data); {
		if (pcb_subc_part_editable(st->pcb, text))
			set_text(st, text);
	} PCB_ENDALL_LOOP;
	PCB_PADSTACK_LOOP(ssubc->data); {
		if (pcb_subc_part_editable(st->pcb, padstack))
			set_pstk(st, padstack);
	} PCB_END_LOOP;
	PCB_SUBC_LOOP(ssubc->data); {
		if (pcb_subc_part_editable(st->pcb, subc))
			set_subc(st, subc);
	} PCB_END_LOOP;

	if (set_common(st, (pcb_any_obj_t *)ssubc)) return;

	if (st->is_attr) {
		set_attr(st, &ssubc->Attributes);
		return;
	}
}

static void set_any(set_ctx_t *ctx, pcb_any_obj_t *o)
{
	if (o == NULL)
		return;
	switch(o->type) {
		case PCB_OBJ_ARC:  set_arc(ctx, (pcb_arc_t *)o); break;
		case PCB_OBJ_LINE: set_line(ctx, (pcb_line_t *)o); break;
		case PCB_OBJ_POLY: set_poly(ctx, (pcb_poly_t *)o); break;
		case PCB_OBJ_TEXT: set_text(ctx, (pcb_text_t *)o); break;
		case PCB_OBJ_SUBC: set_subc(ctx, (pcb_subc_t *)o); break;
		case PCB_OBJ_PSTK: set_pstk(ctx, (pcb_pstk_t *)o); break;
		default: break;
	}
}

int pcb_propsel_set(pcb_propedit_t *ctx, const char *prop, const char *value)
{
	set_ctx_t sctx;
	char *end;
	const char *start;
	pcb_idpath_t *idp;
	size_t n;

	/* sanity checks for invalid props */
	if (prop[1] != '/')
		return 0;
	if ((prop[0] != 'a') && (prop[0] != 'p'))
		return 0;

	sctx.pcb = ctx->pcb;
	sctx.is_trace = (strncmp(prop, "p/trace/", 8) == 0);
	sctx.is_attr = (prop[0] == 'a');
	sctx.name = prop;
	sctx.value = value;
	start = value;
	while(isspace(*start)) start++;
	if (*start == '#') {
		sctx.d_absolute = 1;
		start++;
	}
	else
		sctx.d_absolute = ((*start != '-') && (*start != '+'));
	sctx.c = pcb_get_value_ex(start, NULL, &sctx.c_absolute, NULL, NULL, &sctx.c_valid);
	sctx.d = strtod(start, &end);
	sctx.d_valid = (*end == '\0');
	sctx.set_cnt = 0;

	pcb_undo_save_serial();

	for(n = 0; n < vtl0_len(&ctx->layers); n++)
		set_layer(&sctx, pcb_get_layer(ctx->pcb->Data, ctx->layers.array[n]));

	for(n = 0; n < vtl0_len(&ctx->layergrps); n++)
		set_layergrp(&sctx, pcb_get_layergrp(ctx->pcb, ctx->layergrps.array[n]));

	for(idp = pcb_idpath_list_first(&ctx->objs); idp != NULL; pcb_idpath_list_next(idp))
		set_any(&sctx, pcb_idpath2obj(ctx->pcb->Data, idp));

	if (ctx->selection) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		for(o = pcb_data_first(&it, ctx->pcb->Data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it))
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))
				set_any(&sctx, o);
	}

	if (ctx->board)
		set_board(&sctx, ctx->pcb);

	pcb_undo_inc_serial();
	return sctx.set_cnt;
}

/*******************/

static long del_attr(void *ctx, pcb_attribute_list_t *list, const char *key)
{
	if (pcb_attribute_remove(list, key))
		return 1;
	return 0;
}

static long del_layer(void *ctx, pcb_layer_t *ly, const char *key)
{
	return del_attr(ctx, &ly->Attributes, key);
}

static long del_layergrp(void *ctx, pcb_layergrp_t *grp, const char *key)
{
	return del_attr(ctx, &grp->Attributes, key);
}

static long del_any(void *ctx, pcb_any_obj_t *o, const char *key)
{
	return del_attr(ctx, &o->Attributes, key);
}

static long del_board(void *ctx, pcb_board_t *pcb, const char *key)
{
	return del_attr(ctx, &pcb->Attributes, key);
}

int pcb_propsel_del(pcb_propedit_t *ctx, const char *key)
{
	long del_cnt = 0;
	pcb_idpath_t *idp;
	size_t n;

	if ((key[0] != 'a') || (key[1] != '/')) /* do not attempt to remove anything but attributes */
		return 0;

	key += 2;

	for(n = 0; n < vtl0_len(&ctx->layers); n++)
		del_cnt += del_layer(ctx, pcb_get_layer(ctx->pcb->Data, ctx->layers.array[n]), key);

	for(n = 0; n < vtl0_len(&ctx->layergrps); n++)
		del_cnt += del_layergrp(ctx, pcb_get_layergrp(ctx->pcb, ctx->layergrps.array[n]), key);

	for(idp = pcb_idpath_list_first(&ctx->objs); idp != NULL; pcb_idpath_list_next(idp))
		del_cnt += del_any(ctx, pcb_idpath2obj(ctx->pcb->Data, idp), key);

	if (ctx->selection) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		for(o = pcb_data_first(&it, ctx->pcb->Data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it))
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))
				del_cnt += del_any(ctx, o, key);
	}

	if (ctx->board)
		del_cnt += del_board(&ctx, ctx->pcb, key);

	return del_cnt;
}


