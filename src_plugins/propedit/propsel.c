/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2018,2019,2020 Tibor 'Igor2' Palinkas
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
#include "draw.h"
#include <librnd/core/misc_util.h>
#include "flag_str.h"
#include <librnd/core/compat_misc.h>
#include "undo.h"
#include "rotate.h"
#include "obj_pstk_inlines.h"
#include <librnd/core/rnd_printf.h>
#include "conf_core.h"
#include <librnd/core/hidlib_conf.h>
#include "netlist.h"

#include "obj_line_op.h"
#include "obj_arc_op.h"
#include "obj_text_op.h"
#include "obj_pstk_op.h"
#include "obj_subc_op.h"

/*********** map ***********/
#define type2field_String string
#define type2field_rnd_coord_t coord
#define type2field_rnd_angle_t angle
#define type2field_double d
#define type2field_int i
#define type2field_bool i
#define type2field_color clr

#define type2TYPE_String PCB_PROPT_STRING
#define type2TYPE_rnd_coord_t PCB_PROPT_COORD
#define type2TYPE_rnd_angle_t PCB_PROPT_ANGLE
#define type2TYPE_double PCB_PROPT_DOUBLE
#define type2TYPE_int PCB_PROPT_INT
#define type2TYPE_bool PCB_PROPT_BOOL
#define type2TYPE_color PCB_PROPT_COLOR

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
		map_add_prop(ctx, name, bool, PCB_FLAG_TEST(bit, obj));
	}
}

static void map_board(pcb_propedit_t *ctx, pcb_board_t *pcb)
{
	map_add_prop(ctx, "p/board/name",   String, pcb->hidlib.name);
	map_add_prop(ctx, "p/board/filename",   String, pcb->hidlib.filename);
	map_add_prop(ctx, "p/board/width", rnd_coord_t, pcb->hidlib.size_x);
	map_add_prop(ctx, "p/board/height", rnd_coord_t, pcb->hidlib.size_y);
	map_attr(ctx, &pcb->Attributes);
}

static void map_layer(pcb_propedit_t *ctx, pcb_layer_t *layer)
{
	if (layer == NULL)
		return;
	map_add_prop(ctx, "p/layer/name", String, layer->name);
	map_add_prop(ctx, "p/layer/comb/negative", bool, !!(layer->comb & PCB_LYC_SUB));
	map_add_prop(ctx, "p/layer/comb/auto", bool, !!(layer->comb & PCB_LYC_AUTO));
	if (!layer->is_bound)
		map_add_prop(ctx, "p/layer/color", color, layer->meta.real.color);
	map_attr(ctx, &layer->Attributes);
}

static void map_layergrp(pcb_propedit_t *ctx, pcb_layergrp_t *grp)
{
	const char *s;

	if (grp == NULL)
		return;

	map_add_prop(ctx, "p/layer_group/name", String, grp->name);
	map_add_prop(ctx, "p/layer_group/purpose", String, grp->purpose);

	s = pcb_layer_type_bit2str(grp->ltype & PCB_LYT_ANYWHERE);
	if (s == NULL) s = "global";
	map_add_prop(ctx, "p/layer_group/location", String, s);

	map_add_prop(ctx, "p/layer_group/main_type", String, pcb_layer_type_bit2str(grp->ltype & PCB_LYT_ANYTHING));
	map_add_prop(ctx, "p/layer_group/prop_type", String, pcb_layer_type_bit2str(grp->ltype & PCB_LYT_ANYPROP));

	map_attr(ctx, &grp->Attributes);
}

static void map_net(pcb_propedit_t *ctx, const char *netname)
{
	pcb_net_t *net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_EDITED], netname, 0);
	if (net == NULL)
		return;
	map_attr(ctx, &net->Attributes);
}

static void map_line(pcb_propedit_t *ctx, pcb_line_t *line)
{
	map_add_prop(ctx, "p/trace/thickness", rnd_coord_t, line->Thickness);
	map_add_prop(ctx, "p/trace/clearance", rnd_coord_t, line->Clearance/2);
	map_common(ctx, (pcb_any_obj_t *)line);
	map_attr(ctx, &line->Attributes);
	if (ctx->geo) {
		map_add_prop(ctx, "p/line/x1", rnd_coord_t, line->Point1.X);
		map_add_prop(ctx, "p/line/y1", rnd_coord_t, line->Point1.Y);
		map_add_prop(ctx, "p/line/x2", rnd_coord_t, line->Point2.X);
		map_add_prop(ctx, "p/line/y2", rnd_coord_t, line->Point2.Y);
	}
}

static void map_arc(pcb_propedit_t *ctx, pcb_arc_t *arc)
{
	map_add_prop(ctx, "p/trace/thickness", rnd_coord_t, arc->Thickness);
	map_add_prop(ctx, "p/trace/clearance", rnd_coord_t, arc->Clearance/2);
	map_add_prop(ctx, "p/arc/width",       rnd_coord_t, arc->Width);
	map_add_prop(ctx, "p/arc/height",      rnd_coord_t, arc->Height);
	map_add_prop(ctx, "p/arc/angle/start", rnd_angle_t, arc->StartAngle);
	map_add_prop(ctx, "p/arc/angle/delta", rnd_angle_t, arc->Delta);
	map_common(ctx, (pcb_any_obj_t *)arc);
	map_attr(ctx, &arc->Attributes);
	if (ctx->geo) {
		map_add_prop(ctx, "p/arc/x", rnd_coord_t, arc->X);
		map_add_prop(ctx, "p/arc/y", rnd_coord_t, arc->Y);
	}
}

static void map_gfx(pcb_propedit_t *ctx, pcb_gfx_t *gfx)
{
	map_add_prop(ctx, "p/gfx/sx",      rnd_coord_t, gfx->sx);
	map_add_prop(ctx, "p/gfx/sy",      rnd_coord_t, gfx->sy);
	map_add_prop(ctx, "p/gfx/rot",     rnd_angle_t, gfx->rot);
	map_common(ctx, (pcb_any_obj_t *)gfx);
	map_attr(ctx, &gfx->Attributes);
	if (ctx->geo) {
		map_add_prop(ctx, "p/gfx/cx",      rnd_coord_t, gfx->cx);
		map_add_prop(ctx, "p/gfx/cy",      rnd_coord_t, gfx->cy);
	}
}

static void map_text(pcb_propedit_t *ctx, pcb_text_t *text)
{
	map_add_prop(ctx, "p/text/scale", int, text->Scale);
	map_add_prop(ctx, "p/text/scale_x", double, text->scale_x);
	map_add_prop(ctx, "p/text/scale_y", double, text->scale_y);
	map_add_prop(ctx, "p/text/fid", int, text->fid);
	map_add_prop(ctx, "p/text/rotation",  rnd_angle_t, text->rot);
	map_add_prop(ctx, "p/text/thickness", rnd_coord_t, text->thickness);
	map_add_prop(ctx, "p/text/string", String, text->TextString);
	map_common(ctx, (pcb_any_obj_t *)text);
	map_attr(ctx, &text->Attributes);
	if (ctx->geo) {
		map_add_prop(ctx, "p/text/x", rnd_coord_t, text->X);
		map_add_prop(ctx, "p/text/y", rnd_coord_t, text->Y);
	}
}

static void map_poly(pcb_propedit_t *ctx, pcb_poly_t *poly)
{
	map_attr(ctx, &poly->Attributes);
	map_common(ctx, (pcb_any_obj_t *)poly);
	map_add_prop(ctx, "p/trace/clearance", rnd_coord_t, poly->Clearance/2);
	map_add_prop(ctx, "p/trace/enforce_clearance", rnd_coord_t, poly->enforce_clearance);
}

static void map_pstk(pcb_propedit_t *ctx, pcb_pstk_t *ps)
{
	pcb_pstk_proto_t *proto;

	map_add_prop(ctx, "p/padstack/xmirror", rnd_coord_t, ps->xmirror);
	map_add_prop(ctx, "p/padstack/smirror", rnd_coord_t, ps->smirror);
	map_add_prop(ctx, "p/padstack/rotation", rnd_angle_t, ps->rot);
	map_add_prop(ctx, "p/padstack/proto", int, ps->proto);

	proto = pcb_pstk_get_proto(ps);
	map_add_prop(ctx, "p/padstack/clearance", rnd_coord_t, ps->Clearance);
	map_add_prop(ctx, "p/padstack/hole", rnd_coord_t, proto->hdia);
	map_add_prop(ctx, "p/padstack/plated", int, proto->hplated);
	map_add_prop(ctx, "p/padstack/htop", int, proto->htop);
	map_add_prop(ctx, "p/padstack/hbottom", int, proto->hbottom);

	map_attr(ctx, &ps->Attributes);
	map_common(ctx, (pcb_any_obj_t *)ps);

	if (ctx->geo) {
		map_add_prop(ctx, "p/padstack/x", rnd_coord_t, ps->x);
		map_add_prop(ctx, "p/padstack/y", rnd_coord_t, ps->y);
	}
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

	if (ctx->geo) {
		rnd_coord_t x, y;
		if (pcb_subc_get_origin(msubc, &x, &y) == 0) {
			map_add_prop(ctx, "p/subcircuit/x", rnd_coord_t, x);
			map_add_prop(ctx, "p/subcircuit/y", rnd_coord_t, y);
		}
	}
}

static void map_any(pcb_propedit_t *ctx, pcb_any_obj_t *o)
{
	if (o == NULL)
		return;
	switch(o->type) {
		case PCB_OBJ_ARC:  map_arc(ctx, (pcb_arc_t *)o); break;
		case PCB_OBJ_GFX:  map_gfx(ctx, (pcb_gfx_t *)o); break;
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
		map_layer(ctx, pcb_get_layer(ctx->data, ctx->layers.array[n]));

	for(n = 0; n < vtl0_len(&ctx->layergrps); n++)
		map_layergrp(ctx, pcb_get_layergrp(ctx->pcb, ctx->layergrps.array[n]));

	for(idp = pcb_idpath_list_first(&ctx->objs); idp != NULL; idp = pcb_idpath_list_next(idp))
		map_any(ctx, pcb_idpath2obj_in(ctx->data, idp));

	if (ctx->nets_inited) {
		htsp_entry_t *e;
		for(e = htsp_first(&ctx->nets); e != NULL; e = htsp_next(&ctx->nets, e))
			map_net(ctx, e->key);
	}

	if (ctx->selection) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		TODO("TODO#28: should be rtree based for recursion");
		for(o = pcb_data_first(&it, ctx->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it))
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))
				map_any(ctx, o);
	}

	if (ctx->board)
		map_board(ctx, ctx->pcb);
}

/*******************/

static int attr_key_has_side_effect(const char *key)
{
	if (strcmp(key, "tight_clearance") == 0) return 1;
	if (strcmp(key, "mirror_x") == 0) return 1;
	return 0;
}

static void toggle_attr(pcb_propset_ctx_t *st, pcb_attribute_list_t *list, int undoable, pcb_any_obj_t *obj)
{
	const char *key = st->name+2, *newval = NULL;
	const char *orig = pcb_attribute_get(list, key);
	int side_effect = 0;

	if (orig == NULL) {
		if (st->toggle_create) {
			newval = "true";
			goto do_set;
		}
		/* else do not create non-existing attributes */
		return;
	}
	if (rnd_istrue(orig)) {
		newval = "false";
		goto do_set;
	}
	else if (rnd_isfalse(orig)) {
		newval = "true";
		goto do_set;
	}
	return;

	do_set:;
	side_effect = attr_key_has_side_effect(key);
	if ((obj != NULL) && side_effect)
		pcb_obj_pre(obj);
	if (undoable)
		pcb_uchg_attr(st->pcb, obj, key, newval);
	else
		pcb_attribute_put(list, key, newval);
	if ((obj != NULL) && side_effect) {
		pcb_obj_update_bbox(st->pcb, obj);
		pcb_obj_post(obj);
		pcb_draw_invalidate(obj);
	}
	st->set_cnt++;
}

static void set_attr_raw(pcb_propset_ctx_t *st, pcb_attribute_list_t *list)
{
	const char *key = st->name+2;
	const char *orig;

	if (st->toggle) {
		toggle_attr(st, list, 0, NULL);
		return;
	}

	orig = pcb_attribute_get(list, key);
	if ((orig != NULL) && (strcmp(orig, st->s) == 0))
		return;

	pcb_attribute_put(list, key, st->s);
	st->set_cnt++;
}

static void set_attr_obj(pcb_propset_ctx_t *st, pcb_any_obj_t *obj)
{
	const char *key = st->name+2;
	int side_effect, res;

	if (st->toggle) {
		toggle_attr(st, &obj->Attributes, 1, obj);
		return;
	}

	side_effect = attr_key_has_side_effect(key);

	if (side_effect)
		pcb_obj_pre(obj);
	res = pcb_uchg_attr(st->pcb, obj, key, st->s);
	if (side_effect) {
		pcb_obj_update_bbox(st->pcb, obj);
		pcb_obj_post(obj);
		pcb_draw_invalidate(obj);
	}


	if (res != 0)
		return;

	st->set_cnt++;
}

#define DONE { st->set_cnt++; pcb_undo_restore_serial(); return; }
#define DONE0 { st->set_cnt++; pcb_undo_restore_serial(); return 0; }
#define DONE1 { st->set_cnt++; pcb_undo_restore_serial(); return 1; }

static int set_common(pcb_propset_ctx_t *st, pcb_any_obj_t *obj)
{
	const pcb_flag_bits_t *i = pcb_strflg_name(st->name + 8, obj->type);

	if (i != NULL) {
		if (i->mask & PCB_FLAG_CLEARLINE)
			pcb_obj_pre(obj);

		pcb_undo_add_obj_to_flag(obj);

		if (st->toggle)
			pcb_flag_change(st->pcb, PCB_CHGFLG_TOGGLE, i->mask, obj->type, obj->parent.any, obj, obj);
		else
			pcb_flag_change(st->pcb, st->c ? PCB_CHGFLG_SET : PCB_CHGFLG_CLEAR, i->mask, obj->type, obj->parent.any, obj, obj);
		if (i->mask & PCB_FLAG_CLEARLINE)
			pcb_obj_post(obj);
		DONE1;
	}

	return 0;
}

static int brd_resize(rnd_coord_t w, rnd_coord_t h)
{
	pcb_board_resize(w, h, 0);
	return 1;
}

static void set_board(pcb_propset_ctx_t *st, pcb_board_t *pcb)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr_raw(st, &pcb->Attributes);
		return;
	}

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/board/", 8) == 0) {
		if ((strcmp(pn, "name") == 0) &&
		    (pcb_board_change_name(rnd_strdup(st->s)))) DONE;

		if ((strcmp(pn, "filename") == 0)) {
			free(pcb->hidlib.filename);
			pcb->hidlib.filename = rnd_strdup(st->s);
			DONE;
		}

		if (st->c_valid && (strcmp(pn, "width") == 0) &&
		    brd_resize(st->c, PCB->hidlib.size_y)) DONE;

		if (st->c_valid && (strcmp(pn, "height") == 0) &&
		    brd_resize(PCB->hidlib.size_x,st->c)) DONE;
	}
}

static int layer_recolor(pcb_layer_t *layer, const char *clr)
{
	rnd_color_t c;
	if (rnd_color_load_str(&c, clr) != 0)
		return -1;
	return pcb_layer_recolor_(layer, &c, 1);
}

static int layer_recomb(pcb_layer_t *layer, int val, pcb_layer_combining_t bit)
{
	if (val)
		return pcb_layer_recomb(layer, layer->comb | bit, 1);
	return pcb_layer_recomb(layer, layer->comb & ~bit, 1);
}

static int set_layer(pcb_propset_ctx_t *st, pcb_layer_t *layer)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)layer);
		return 0;
	}

	if (st->toggle)
		return 0; /* can't toggle anything else */

	if (strncmp(st->name, "p/layer/", 8) == 0) {
		if ((strcmp(pn, "name") == 0) &&
		    (pcb_layer_rename_(layer, rnd_strdup(st->s), 1) == 0)) DONE0;

		if ((strcmp(pn, "color") == 0) &&
		    (layer_recolor(layer, st->color.str) == 0)) DONE0;

		if ((strcmp(pn, "comb/negative") == 0) &&
		    (layer_recomb(layer, st->c, PCB_LYC_SUB) == 0)) DONE0;

		if ((strcmp(pn, "comb/auto") == 0) &&
		    (layer_recomb(layer, st->c, PCB_LYC_AUTO) == 0)) DONE0;
	}

	return 0;
}


static void set_layergrp(pcb_propset_ctx_t *st, pcb_layergrp_t *grp)
{
	const char *pn = st->name + 14;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)grp);
		return;
	}

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/layer_group/", 14) == 0) {
		if ((strcmp(pn, "name") == 0) &&
		    (pcb_layergrp_rename_(grp, rnd_strdup(st->s), 1) == 0)) DONE;

		if ((strcmp(pn, "purpose") == 0) &&
		    (pcb_layergrp_set_purpose(grp, st->s, 1) == 0)) DONE;
	}
}

static void set_net(pcb_propset_ctx_t *st, const char *netname)
{
	pcb_net_t *net = pcb_net_get(st->pcb, &st->pcb->netlist[PCB_NETLIST_EDITED], netname, 0);
	if (net == NULL)
		return;

	if (st->toggle)
		return; /* can't toggle anything else */

	if (st->is_attr) {
		const char *key = st->name+2;
		const char *orig = pcb_attribute_get(&net->Attributes, key);

		if ((orig != NULL) && (strcmp(orig, st->s) == 0))
			return;

		st->set_cnt++;
		pcb_ratspatch_append_optimize(st->pcb, RATP_CHANGE_ATTRIB, netname, key, st->s);
		return;
	}
}

static void set_line(pcb_propset_ctx_t *st, pcb_line_t *line)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)line);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)line)) return;

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/trace/", 8) == 0) {
		if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
		    pcb_chg_obj_1st_size(PCB_OBJ_LINE, line->parent.layer, line, NULL, st->c, st->c_absolute)) DONE;

		if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
		    pcb_chg_obj_clear_size(PCB_OBJ_LINE, line->parent.layer, line, NULL, st->c*2, st->c_absolute)) DONE;
	}

	if (strncmp(st->name, "p/line/", 7) == 0) {
		pcb_opctx_t op;
		memset(&op, 0, sizeof(op));
		op.move.pcb = st->pcb;
		pn = st->name + 7;
		if (st->c_valid && (strcmp(pn, "x1") == 0)) {
			op.move.dx = st->c - line->Point1.X;
			pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, line->parent.layer, line, &line->Point1, op.move.dx, op.move.dy);
			if (pcb_lineop_move_point(&op, line->parent.layer, line, &line->Point1) != NULL)
				DONE;
		}
		if (st->c_valid && (strcmp(pn, "y1") == 0)) {
			op.move.dy = st->c - line->Point1.Y;
			pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, line->parent.layer, line, &line->Point1, op.move.dx, op.move.dy);
			if (pcb_lineop_move_point(&op, line->parent.layer, line, &line->Point1) != NULL)
				DONE;
		}
		if (st->c_valid && (strcmp(pn, "x2") == 0)) {
			op.move.dx = st->c - line->Point2.X;
			pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, line->parent.layer, line, &line->Point2, op.move.dx, op.move.dy);
			if (pcb_lineop_move_point(&op, line->parent.layer, line, &line->Point2) != NULL)
				DONE;
		}
		if (st->c_valid && (strcmp(pn, "y2") == 0)) {
			op.move.dy = st->c - line->Point2.Y;
			pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT, line->parent.layer, line, &line->Point2, op.move.dx, op.move.dy);
			if (pcb_lineop_move_point(&op, line->parent.layer, line, &line->Point2) != NULL)
				DONE;
		}
	}
}

static void set_arc(pcb_propset_ctx_t *st, pcb_arc_t *arc)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)arc);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)arc)) return;

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/trace/", 8) == 0)  {
		if (st->is_trace && st->c_valid && (strcmp(pn, "thickness") == 0) &&
		    pcb_chg_obj_1st_size(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, st->c, st->c_absolute)) DONE;

		if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
		    pcb_chg_obj_clear_size(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, st->c*2, st->c_absolute)) DONE;
	}

	pn = st->name + 6;
	if (strncmp(st->name, "p/arc/", 6) == 0) {
		if (!st->is_trace && st->c_valid && (strcmp(pn, "width") == 0) &&
		    pcb_chg_obj_radius(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 0, st->c, st->c_absolute)) DONE;

		if (!st->is_trace && st->c_valid && (strcmp(pn, "height") == 0) &&
		    pcb_chg_obj_radius(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 1, st->c, st->c_absolute)) DONE;

		if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/start") == 0) &&
		    pcb_chg_obj_angle(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 0, st->d, st->d_absolute)) DONE;

		if (!st->is_trace && st->d_valid && (strcmp(pn, "angle/delta") == 0) &&
		    pcb_chg_obj_angle(PCB_OBJ_ARC, arc->parent.layer, arc, NULL, 1, st->d, st->d_absolute)) DONE;
	}

	if (strncmp(st->name, "p/arc/", 6) == 0) {
		pcb_opctx_t op;
		memset(&op, 0, sizeof(op));
		op.move.pcb = st->pcb;
		pn = st->name + 6;
		if (st->c_valid && (strcmp(pn, "x") == 0)) {
			op.move.dx = st->c - arc->X;
			pcb_undo_add_obj_to_move(PCB_OBJ_ARC, arc->parent.layer, arc, arc, op.move.dx, op.move.dy);
			if (pcb_arcop_move(&op, arc->parent.layer, arc) != NULL)
				DONE;
		}
		if (st->c_valid && (strcmp(pn, "y") == 0)) {
			op.move.dy = st->c - arc->Y;
			pcb_undo_add_obj_to_move(PCB_OBJ_ARC, arc->parent.layer, arc, arc, op.move.dx, op.move.dy);
			if (pcb_arcop_move(&op, arc->parent.layer, arc) != NULL)
				DONE;
		}
	}
}

static void set_gfx(pcb_propset_ctx_t *st, pcb_gfx_t *gfx)
{
	const char *pn = st->name + 6;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)gfx);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)gfx)) return;

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/gfx/", 6) == 0) {
		if (st->c_valid && (strcmp(pn, "sx") == 0)) {
			pcb_gfx_chg_geo(gfx, gfx->cx, gfx->cy, st->c, gfx->sy, gfx->rot, 1); DONE;
		}
		if (st->c_valid && (strcmp(pn, "sy") == 0)) {
			pcb_gfx_chg_geo(gfx, gfx->cx, gfx->cy, gfx->sx, st->c, gfx->rot, 1); DONE;
		}
		if (st->d_valid && (strcmp(pn, "rot") == 0)) {
			pcb_gfx_chg_geo(gfx, gfx->cx, gfx->cy, gfx->sx, gfx->sy, st->d, 1); DONE;
		}
	}
}

static void set_text(pcb_propset_ctx_t *st, pcb_text_t *text)
{
	const char *pn = st->name + 7;
	char *old;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)text);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)text)) return;

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/text/", 7) == 0) {
		pcb_opctx_t op;

		if (st->c_valid && (strcmp(pn, "scale") == 0) &&
		    pcb_chg_obj_size(PCB_OBJ_TEXT, text->parent.layer, text, text, RND_MIL_TO_COORD(st->c), st->c_absolute)) DONE;

		if (st->d_valid && (strcmp(pn, "scale_x") == 0) &&
		    (pcb_text_chg_scale(text, st->d, st->d_absolute, text->scale_y, 1, 1) == 0)) DONE;

		if (st->d_valid && (strcmp(pn, "scale_y") == 0) &&
		    (pcb_text_chg_scale(text, text->scale_x, 1, st->d, st->d_absolute, 1) == 0)) DONE;

		if (st->c_valid && (strcmp(pn, "fid") == 0)) {
			pcb_text_set_font(text, st->c);
			DONE;
		}

		if ((strcmp(pn, "string") == 0) &&
		    (old = pcb_chg_obj_name(PCB_OBJ_TEXT, text->parent.layer, text, NULL, rnd_strdup(st->s)))) {
			free(old);
			DONE;
		}

		if (st->d_valid && (strcmp(pn, "rotation") == 0) &&
			pcb_chg_obj_rot(PCB_OBJ_TEXT, text->parent.layer, text, text, st->d, st->d_absolute, rnd_true)) DONE;

		if (st->c_valid && (strcmp(pn, "thickness") == 0) &&
			pcb_chg_obj_2nd_size(PCB_OBJ_TEXT, text->parent.layer, text, text, st->c, st->c_absolute, rnd_true)) DONE;

		memset(&op, 0, sizeof(op));
		op.move.pcb = st->pcb;
		if (st->c_valid && (strcmp(pn, "x") == 0)) {
			op.move.dx = st->c - text->X;
			pcb_undo_add_obj_to_move(PCB_OBJ_TEXT, text->parent.layer, text, text, op.move.dx, op.move.dy);
			if (pcb_textop_move(&op, text->parent.layer, text) != NULL)
				DONE;
		}
		if (st->c_valid && (strcmp(pn, "y") == 0)) {
			op.move.dy = st->c - text->Y;
			pcb_undo_add_obj_to_move(PCB_OBJ_TEXT, text->parent.layer, text, text, op.move.dx, op.move.dy);
			if (pcb_textop_move(&op, text->parent.layer, text) != NULL)
				DONE;
		}
	}
}

static void set_poly(pcb_propset_ctx_t *st, pcb_poly_t *poly)
{
	const char *pn = st->name + 8;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)poly);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)poly)) return;

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/trace/", 8) == 0) {
		if (st->is_trace && st->c_valid && (strcmp(pn, "clearance") == 0) &&
		    pcb_chg_obj_clear_size(PCB_OBJ_POLY, poly->parent.layer, poly, NULL, st->c*2, st->c_absolute)) DONE;
		if (st->is_trace && st->c_valid && (strcmp(pn, "enforce_clearance") == 0) &&
		    pcb_chg_obj_enforce_clear_size(PCB_OBJ_POLY, poly->parent.layer, poly, NULL, st->c, st->c_absolute)) DONE;
	}
}

static void set_pstk(pcb_propset_ctx_t *st, pcb_pstk_t *ps)
{
	const char *pn = st->name + 11;
	int i;
	rnd_cardinal_t ca;
	pcb_pstk_proto_t *proto;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)ps);
		return;
	}

	if (set_common(st, (pcb_any_obj_t *)ps)) return;

	if (st->toggle)
		return; /* can't toggle anything else */

	ca = st->c;
	i = (st->c != 0);
	proto = pcb_pstk_get_proto(ps);

	if (strncmp(st->name, "p/padstack/", 11) == 0) {
		pcb_opctx_t op;

		if (st->c_valid && (strcmp(pn, "clearance") == 0) &&
		    pcb_chg_obj_clear_size(PCB_OBJ_PSTK, ps, ps, NULL, st->c*2, st->c_absolute)) DONE;
		if (st->d_valid && (strcmp(pn, "rotation") == 0)) {
			if (st->d_absolute) {
				if (pcb_obj_rotate(st->pcb, (pcb_any_obj_t *)ps, ps->x, ps->y, st->d - ps->rot)) DONE;
			}
			else {
				if (pcb_obj_rotate(st->pcb, (pcb_any_obj_t *)ps, ps->x, ps->y, st->d)) DONE;
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

		memset(&op, 0, sizeof(op));
		op.move.pcb = st->pcb;
		if (st->c_valid && (strcmp(pn, "x") == 0)) {
			op.move.dx = st->c - ps->x;
			pcb_undo_add_obj_to_move(PCB_OBJ_PSTK, ps->parent.data, ps, ps, op.move.dx, op.move.dy);
			if (pcb_pstkop_move(&op, ps) != NULL)
				DONE;
		}
		if (st->c_valid && (strcmp(pn, "y") == 0)) {
			op.move.dy = st->c - ps->y;
			pcb_undo_add_obj_to_move(PCB_OBJ_PSTK, ps->parent.data, ps, ps, op.move.dx, op.move.dy);
			if (pcb_pstkop_move(&op, ps) != NULL)
				DONE;
		}
	}
}

static void set_subc(pcb_propset_ctx_t *st, pcb_subc_t *ssubc)
{

	if (((st->name[0] != 'a') && (st->name[0] != 'f')) || (st->name[1] != '/')) { /* attributes and flags are not recursive */
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
	}

	if (set_common(st, (pcb_any_obj_t *)ssubc)) return;

	if (st->is_attr) {
		set_attr_obj(st, (pcb_any_obj_t *)ssubc);
		return;
	}

	if (st->toggle)
		return; /* can't toggle anything else */

	if (strncmp(st->name, "p/subcircuit/", 13) == 0) {
		pcb_opctx_t op;
		rnd_coord_t x = 0, y = 0;
		const char *pn = st->name + 13;

		pcb_subc_get_origin(ssubc, &x, &y);
		memset(&op, 0, sizeof(op));
		op.move.pcb = st->pcb;
		if (st->c_valid && (strcmp(pn, "x") == 0)) {
			op.move.dx = st->c - x;
			pcb_undo_add_obj_to_move(PCB_OBJ_SUBC, ssubc->parent.data, ssubc, ssubc, op.move.dx, op.move.dy);
			if (pcb_subcop_move(&op, ssubc) != NULL)
				DONE;
		}
		if (st->c_valid && (strcmp(pn, "y") == 0)) {
			op.move.dy = st->c - y;
			pcb_undo_add_obj_to_move(PCB_OBJ_SUBC, ssubc->parent.data, ssubc, ssubc, op.move.dx, op.move.dy);
			if (pcb_subcop_move(&op, ssubc) != NULL)
				DONE;
		}

	}
}

static void set_any(pcb_propset_ctx_t *ctx, pcb_any_obj_t *o)
{
	if (o == NULL)
		return;
	switch(o->type) {
		case PCB_OBJ_ARC:  set_arc(ctx, (pcb_arc_t *)o); break;
		case PCB_OBJ_GFX:  set_gfx(ctx, (pcb_gfx_t *)o); break;
		case PCB_OBJ_LINE: set_line(ctx, (pcb_line_t *)o); break;
		case PCB_OBJ_POLY: set_poly(ctx, (pcb_poly_t *)o); break;
		case PCB_OBJ_TEXT: set_text(ctx, (pcb_text_t *)o); break;
		case PCB_OBJ_SUBC: set_subc(ctx, (pcb_subc_t *)o); break;
		case PCB_OBJ_PSTK: set_pstk(ctx, (pcb_pstk_t *)o); break;
		default: break;
	}
}

int pcb_propsel_set(pcb_propedit_t *ctx, const char *prop, pcb_propset_ctx_t *sctx)
{
	size_t n;
	pcb_idpath_t *idp;

	sctx->pcb = ctx->pcb;
	sctx->data = ctx->data;
	sctx->is_trace = (strncmp(prop, "p/trace/", 8) == 0);
	sctx->is_attr = (prop[0] == 'a');
	sctx->name = prop;

	pcb_undo_save_serial();

	for(n = 0; n < vtl0_len(&ctx->layers); n++)
		set_layer(sctx, pcb_get_layer(ctx->data, ctx->layers.array[n]));

	for(n = 0; n < vtl0_len(&ctx->layergrps); n++)
		set_layergrp(sctx, pcb_get_layergrp(ctx->pcb, ctx->layergrps.array[n]));

	for(idp = pcb_idpath_list_first(&ctx->objs); idp != NULL; idp = pcb_idpath_list_next(idp)) {
		pcb_any_obj_t *obj = pcb_idpath2obj_in(ctx->data, idp);
		if (obj == NULL) /* workaround: idp sometimes points into the buffer */
			obj = pcb_idpath2obj(PCB, idp);
		set_any(sctx, obj);
	}

	if (ctx->nets_inited) {
		long old_set_cnt = sctx->set_cnt;
		htsp_entry_t *e;
		for(e = htsp_first(&ctx->nets); e != NULL; e = htsp_next(&ctx->nets, e))
			set_net(sctx, e->key);
		if (old_set_cnt != sctx->set_cnt)
			pcb_ratspatch_make_edited(sctx->pcb);
	}

	if (ctx->selection) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		TODO("TODO#28: should be rtree based for recursion");
		for(o = pcb_data_first(&it, ctx->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it))
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))
				set_any(sctx, o);
	}

	if (ctx->board)
		set_board(sctx, ctx->pcb);

	pcb_board_set_changed_flag(ctx->pcb, rnd_true);
	pcb_undo_inc_serial();
	return sctx->set_cnt;
}


int pcb_propsel_set_str(pcb_propedit_t *ctx, const char *prop, const char *value)
{
	pcb_propset_ctx_t sctx;
	char *end;
	const char *start;

	/* sanity checks for invalid props */
	if ((prop[1] != '/') || ((prop[0] != 'a') && (prop[0] != 'p'))) {
		rnd_message(RND_MSG_ERROR, "Invalid property path: '%s':\n must start with p/ for property or a/ for attribute\n", prop);
		return 0;
	}

	memset(&sctx, 0, sizeof(sctx));

	if (value == NULL)
		value = "";
	sctx.s = value;
	start = value;
	while(isspace(*start)) start++;
	if (*start == '#') {
		sctx.d_absolute = 1;
		start++;
	}
	else
		sctx.d_absolute = ((*start != '-') && (*start != '+'));
	sctx.toggle = 0;
	sctx.c = rnd_get_value_ex(start, NULL, &sctx.c_absolute, NULL, NULL, &sctx.c_valid);
	sctx.d = strtod(start, &end);
	sctx.d_valid = (*end == '\0');
	sctx.set_cnt = 0;

	return pcb_propsel_set(ctx, prop, &sctx);
}

int pcb_propsel_toggle(pcb_propedit_t *ctx, const char *prop, rnd_bool create)
{
	pcb_propset_ctx_t sctx;

	/* sanity checks for invalid props */
	if (prop[1] != '/')
		return 0;
	if ((prop[0] != 'a') && (prop[0] != 'p'))
		return 0;

	memset(&sctx, 0, sizeof(sctx));

	sctx.toggle = 1;
	sctx.toggle_create = create;
	sctx.set_cnt = 0;

	return pcb_propsel_set(ctx, prop, &sctx);
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

static long del_net(pcb_propedit_t *ctx, const char *netname, const char *key)
{
	pcb_net_t *net = pcb_net_get(ctx->pcb, &ctx->pcb->netlist[PCB_NETLIST_EDITED], netname, 0);
	if ((net == NULL) || (pcb_attribute_get(&net->Attributes, key) == NULL))
		return 0;

	pcb_ratspatch_append_optimize(ctx->pcb, RATP_CHANGE_ATTRIB, netname, key, NULL);
	return 1;
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
		del_cnt += del_layer(ctx, pcb_get_layer(ctx->data, ctx->layers.array[n]), key);

	for(n = 0; n < vtl0_len(&ctx->layergrps); n++)
		del_cnt += del_layergrp(ctx, pcb_get_layergrp(ctx->pcb, ctx->layergrps.array[n]), key);

	for(idp = pcb_idpath_list_first(&ctx->objs); idp != NULL; idp = pcb_idpath_list_next(idp))
		del_cnt += del_any(ctx, pcb_idpath2obj_in(ctx->data, idp), key);

	if (ctx->nets_inited) {
		htsp_entry_t *e;
		long old_del_cnt = del_cnt;
		for(e = htsp_first(&ctx->nets); e != NULL; e = htsp_next(&ctx->nets, e))
			del_cnt += del_net(ctx, e->key, key);
		if (old_del_cnt != del_cnt)
			pcb_ratspatch_make_edited(ctx->pcb);
	}

	if (ctx->selection) {
		pcb_any_obj_t *o;
		pcb_data_it_t it;
		TODO("TODO#28: should be rtree based for recursion");
		for(o = pcb_data_first(&it, ctx->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it))
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, o))
				del_cnt += del_any(ctx, o, key);
	}

	if (ctx->board)
		del_cnt += del_board(&ctx, ctx->pcb, key);

	pcb_board_set_changed_flag(ctx->pcb, rnd_true);
	return del_cnt;
}


char *pcb_propsel_printval(pcb_prop_type_t type, const pcb_propval_t *val)
{
	switch(type) {
		case PCB_PROPT_STRING: return val->string == NULL ? rnd_strdup("") : rnd_strdup(val->string);
		case PCB_PROPT_COORD:  return rnd_strdup_printf("%m+%.02mS", rnd_conf.editor.grid_unit->allow, val->coord);
		case PCB_PROPT_ANGLE:  return rnd_strdup_printf("%f", val->angle);
		case PCB_PROPT_DOUBLE: return rnd_strdup_printf("%f", val->d);
		case PCB_PROPT_INT:    return rnd_strdup_printf("%d", val->i);
		case PCB_PROPT_BOOL:   return rnd_strdup(val->i ? "true" : "false");
		case PCB_PROPT_COLOR:  return rnd_strdup_printf("#%02x%02x%02x", val->clr.r, val->clr.g, val->clr.b);
		default:
			return rnd_strdup("<error>");
	}
}
