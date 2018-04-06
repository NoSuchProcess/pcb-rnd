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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <ctype.h>
#include "config.h"
#include "data.h"
#include "props.h"
#include "propsel.h"
#include "change.h"
#include "misc_util.h"
#include "compat_misc.h"
#include "undo.h"
#include "rotate.h"
#include "obj_pstk_inlines.h"

/*********** map ***********/
typedef struct {
	htsp_t *props;
} map_ctx_t;

#define map_chk_skip(ctx, obj) \
	if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj)) return;

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
	pcb_props_add(((map_ctx_t *)ctx)->props, name, type2TYPE_ ## type, v); \
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
#warning TODO: flags
}

static int map_layer_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, int enter)
{
	/* layers can not be selected so do not check for skip */
	map_add_prop(ctx, "p/layer/name", String, layer->name);
	map_add_prop(ctx, "p/layer/comb/negative", int, !!(layer->comb & PCB_LYC_SUB));
	map_add_prop(ctx, "p/layer/comb/auto", int, !!(layer->comb & PCB_LYC_AUTO));
#warning layer TODO: when hardwired layer colors are gone make color selection possible
#if 0
	if (!layer->is_bound) {
		map_add_prop(ctx, "p/layer/color", int, layer->meta.real.color);
		map_add_prop(ctx, "p/layer/selected_color", int, layer->meta.real.selected_color);
	}
#endif
	map_attr(ctx, &layer->Attributes);
	layer->propedit = 1;
	return 0;
}

static void map_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	map_chk_skip(ctx, line);
	map_add_prop(ctx, "p/trace/thickness", pcb_coord_t, line->Thickness);
	map_add_prop(ctx, "p/trace/clearance", pcb_coord_t, line->Clearance/2);
	map_common(ctx, (pcb_any_obj_t *)line);
	map_attr(ctx, &line->Attributes);
}

static void map_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	map_chk_skip(ctx, arc);
	map_add_prop(ctx, "p/trace/thickness", pcb_coord_t, arc->Thickness);
	map_add_prop(ctx, "p/trace/clearance", pcb_coord_t, arc->Clearance/2);
	map_add_prop(ctx, "p/arc/width",       pcb_coord_t, arc->Width);
	map_add_prop(ctx, "p/arc/height",      pcb_coord_t, arc->Height);
	map_add_prop(ctx, "p/arc/angle/start", pcb_angle_t, arc->StartAngle);
	map_add_prop(ctx, "p/arc/angle/delta", pcb_angle_t, arc->Delta);
	map_common(ctx, (pcb_any_obj_t *)arc);
	map_attr(ctx, &arc->Attributes);
}

static void map_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_text_t *text)
{
	map_chk_skip(ctx, text);
	map_add_prop(ctx, "p/text/scale", int, text->Scale);
	map_add_prop(ctx, "p/text/rotation", int, text->Direction);
	map_add_prop(ctx, "p/text/string", String, text->TextString);
	map_common(ctx, (pcb_any_obj_t *)text);
	map_attr(ctx, &text->Attributes);
}

static void map_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_poly_t *poly)
{
	map_chk_skip(ctx, poly);
	map_attr(ctx, &poly->Attributes);
	map_common(ctx, (pcb_any_obj_t *)poly);
	map_add_prop(ctx, "p/trace/clearance", pcb_coord_t, poly->Clearance/2);
}

static void map_subc_cb_(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc)
{
	map_chk_skip(ctx, subc);
	map_attr(ctx, &subc->Attributes);
	map_common(ctx, (pcb_any_obj_t *)subc);
}

static int map_subc_cb(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc, int enter)
{
	map_subc_cb_(ctx, pcb, subc);
	return 0;
}

static void map_pstk_cb(void *ctx, pcb_board_t *pcb, pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto;
	map_chk_skip(ctx, ps);

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

void pcb_propsel_map_core(htsp_t *props)
{
	map_ctx_t ctx;

	ctx.props = props;
	
	pcb_loop_all(PCB, &ctx,
		NULL, map_line_cb, map_arc_cb, map_text_cb, map_poly_cb,
		map_subc_cb,
		map_pstk_cb
	);
}

void pcb_propsel_map_layers(pe_ctx_t *pe, pcb_layer_t *ly)
{
	map_ctx_t ctx;

	ctx.props = pe->core_props;

	if (ly == NULL) {
		pcb_loop_all(PCB, &ctx, map_layer_cb,
			NULL, NULL, NULL, NULL, NULL, NULL);
	}
	else
		map_layer_cb(&ctx, PCB, ly, 1);
}

/*******************/

typedef struct set_ctx_s {
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

#define set_chk_skip(ctx, obj) \
	if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj)) return;

#define DONE { st->set_cnt++; pcb_undo_restore_serial(); return; }

static int set_common(set_ctx_t *st, pcb_any_obj_t *obj)
{
#warning TODO: flags
	return 0;
}

static void set_layer_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 8;

	if (!layer->propedit)
		return;

	if (st->is_attr) {
		set_attr(st, &layer->Attributes);
		return;
	}

	if ((strcmp(pn, "name") == 0) &&
	    (pcb_layer_rename_(layer, pcb_strdup(st->value)) == 0)) DONE;

	pcb_message(PCB_MSG_ERROR, "This property can not be changed from the property editor.\n");
}


static void set_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 8;

	set_chk_skip(st, line);

	if (st->is_attr) {
		set_attr(st, &line->Attributes);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)line)) return;

	if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    pcb_chg_obj_1st_size(PCB_OBJ_LINE, layer, line, NULL, st->c, st->c_absolute)) DONE;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_LINE, layer, line, NULL, st->c*2, st->c_absolute)) DONE;
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

	if (set_common(st, (pcb_any_obj_t *)arc)) return;

	if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
	    pcb_chg_obj_1st_size(PCB_OBJ_ARC, layer, arc, NULL, st->c, st->c_absolute)) DONE;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_ARC, layer, arc, NULL, st->c*2, st->c_absolute)) DONE;

	pn = st->name + 6;

	if (!st->is_trace && st->c_valid && (strcmp(pn, "width") == 0) &&
	    pcb_chg_obj_radius(PCB_OBJ_ARC, layer, arc, NULL, 0, st->c, st->c_absolute)) DONE;

	if (!st->is_trace && st->c_valid && (strcmp(pn, "height") == 0) &&
	    pcb_chg_obj_radius(PCB_OBJ_ARC, layer, arc, NULL, 1, st->c, st->c_absolute)) DONE;

	if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/start") == 0) &&
	    pcb_chg_obj_angle(PCB_OBJ_ARC, layer, arc, NULL, 0, st->d, st->d_absolute)) DONE;

	if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/delta") == 0) &&
	    pcb_chg_obj_angle(PCB_OBJ_ARC, layer, arc, NULL, 1, st->d, st->d_absolute)) DONE;
}

static void set_text_cb_any(void *ctx, pcb_board_t *pcb, int type, void *layer_or_element, pcb_text_t *text)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 7;
	char *old;

	set_chk_skip(st, text);

	if (st->is_attr) {
		set_attr(st, &text->Attributes);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)text)) return;

	if (st->d_valid && (strcmp(pn, "scale") == 0) &&
	    pcb_chg_obj_size(type, layer_or_element, text, text, PCB_MIL_TO_COORD(st->d), st->d_absolute)) DONE;

	if ((strcmp(pn, "string") == 0) &&
	    (old = pcb_chg_obj_name(type, layer_or_element, text, NULL, pcb_strdup(st->value)))) {
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
		if (pcb_obj_rotate90(type, layer_or_element, text, text, text->X, text->Y, delta) != NULL)
			DONE;
	}
}

static void set_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_text_t *text)
{
	set_text_cb_any(ctx, pcb, PCB_OBJ_TEXT, layer, text);
}


static void set_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_poly_t *poly)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 8;

	set_chk_skip(st, poly);

	if (set_common(st, (pcb_any_obj_t *)poly)) return;

	if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_POLY, layer, poly, NULL, st->c*2, st->c_absolute)) DONE;

	if (st->is_attr) {
		set_attr(st, &poly->Attributes);
		return;
	}
}

static void set_subc_cb_(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc)
{
	set_ctx_t *st = (set_ctx_t *)ctx;

	set_chk_skip(st, subc);

	if (set_common(st, (pcb_any_obj_t *)subc)) return;

	if (st->is_attr) {
		set_attr(st, &subc->Attributes);
		return;
	}
}

static int set_subc_cb(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc, int enter)
{
	set_subc_cb_(ctx, pcb, subc);
	return 0;
}

static void set_pstk_cb(void *ctx, pcb_board_t *pcb, pcb_pstk_t *ps)
{
	set_ctx_t *st = (set_ctx_t *)ctx;
	const char *pn = st->name + 11;
	int i;
	pcb_cardinal_t ca;
	pcb_pstk_proto_t *proto;

	set_chk_skip(st, ps);

	if (st->is_attr) {
		set_attr(st, &ps->Attributes);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)ps)) return;

	ca = i = (st->c != 0);
	proto = pcb_pstk_get_proto(ps);

	if (st->c_valid && (strcmp(pn, "clearance") == 0) &&
	    pcb_chg_obj_clear_size(PCB_OBJ_PSTK, ps, ps, NULL, st->c, st->c_absolute)) DONE;
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
	start = value;
	while(isspace(*start)) start++;
	if (*start == '#') {
		ctx.d_absolute = 1;
		start++;
	}
	else
		ctx.d_absolute = ((*start != '-') && (*start != '+'));
	ctx.c = pcb_get_value_ex(start, NULL, &ctx.c_absolute, NULL, NULL, &ctx.c_valid);
	ctx.d = strtod(start, &end);
	ctx.d_valid = (*end == '\0');
	ctx.set_cnt = 0;

	pcb_undo_save_serial();

	pcb_loop_all(PCB, &ctx,
		MAYBE_PROP(0, "p/layer/", set_layer_cb),
		MAYBE_PROP(ctx.is_trace, "p/line/", set_line_cb),
		MAYBE_PROP(ctx.is_trace, "p/arc/", set_arc_cb),
		MAYBE_PROP(0, "p/text/", set_text_cb),
		MAYBE_PROP(ctx.is_trace, "p/poly/", set_poly_cb),
		MAYBE_PROP(0, "p/subc/", set_subc_cb),
		MAYBE_PROP(0, "p/padstack/", set_pstk_cb)
	);

	pcb_undo_inc_serial();
	return ctx.set_cnt;
}

#undef MAYBE_PROP
#undef MAYBE_ATTR

/*******************/

typedef struct del_ctx_s {
	const char *key;
	int del_cnt;
} del_ctx_t;

static void del_attr(void *ctx, pcb_attribute_list_t *list)
{
	del_ctx_t *st = (del_ctx_t *)ctx;
	if (pcb_attribute_remove(list, st->key+2))
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

static void del_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_text_t *text)
{
	map_chk_skip(ctx, text);
	del_attr(ctx, &text->Attributes);
}

static void del_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_poly_t *poly)
{
	map_chk_skip(ctx, poly);
	del_attr(ctx, &poly->Attributes);
}

static void del_subc_cb_(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc)
{
	map_chk_skip(ctx, subc);
	del_attr(ctx, &subc->Attributes);
}

static int del_subc_cb(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc, int enter)
{
	del_subc_cb_(ctx, pcb, subc);
	return 0;
}

static void del_pstk_cb(void *ctx, pcb_board_t *pcb, pcb_pstk_t *ps)
{
	map_chk_skip(ctx, ps);
	del_attr(ctx, &ps->Attributes);
}

int pcb_propsel_del(const char *key)
{
	del_ctx_t st;

	if ((key[0] != 'a') || (key[1] != '/')) /* do not attempt to remove anything but attributes */
		return 0;

	st.key = key;
	st.del_cnt = 0;

	pcb_loop_all(PCB, &st,
		NULL, del_line_cb, del_arc_cb, del_text_cb, del_poly_cb,
		del_subc_cb,
		del_pstk_cb
	);
	return st.del_cnt;
}


