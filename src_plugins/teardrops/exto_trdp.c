/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Extended object for teardrops; edit: line; places teardrops on end-of-line pstk
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

/* included from teardrops.c */

#include "draw.h"
#include "extobj.h"
#include "extobj_helper.h"

#define LID_EDIT 0

typedef struct {
	int dummy;
} trdp;

static void pcb_trdp_del_pre(pcb_subc_t *subc)
{
	trdp *trdp = subc->extobj_data;
	rnd_trace("Trdp del_pre\n");

	free(trdp);
	subc->extobj_data = NULL;
}

static void trdp_unpack(pcb_subc_t *obj)
{
}

/* remove all existing padstacks from the subc */
static void trdp_clear(pcb_subc_t *subc)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];
	pcb_arc_t *a, *next;

	for(a = arclist_first(&ly->Arc); a != NULL; a = next) {
		next = arclist_next(a);
		if (PCB_FLAG_TEST(PCB_FLAG_FLOATER, a)) continue; /* do not free the floater */
		pcb_arc_free(a);
	}
}

static rnd_r_dir_t trdp_pstk_cb(const rnd_box_t *box, void *cl)
{
	teardrop_t *tr = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)box;

	if (teardrops_init_pstk(tr, ps, tr->line->parent.layer) == 0)
		teardrop_line(tr, tr->line);

	return RND_R_DIR_FOUND_CONTINUE;
}

/* create new teardrops on a line endpoint */
static void trdp_gen_line_pt(pcb_board_t *pcb, pcb_subc_t *subc, pcb_line_t *line, int second, rnd_coord_t *jx, rnd_coord_t *jy)
{
	teardrop_t t;
	rnd_coord_t x, y;
	rnd_box_t spot;

	if (second) {
		x = line->Point2.X;
		y = line->Point2.Y;
	}
	else {
		x = line->Point1.X;
		y = line->Point1.Y;
	}

	t.pcb = pcb;
	t.new_arcs = 0;
	t.line = line;
	t.flags = line->Flags;
	t.flags.f &= ~PCB_FLAG_FLOATER;

	spot.X1 = x - 10;
	spot.Y1 = y - 10;
	spot.X2 = x + 10;
	spot.Y2 = y + 10;

	rnd_r_search(pcb->Data->padstack_tree, &spot, NULL, trdp_pstk_cb, &t, NULL);

	if (t.new_arcs > 0) {
		*jx = t.jx;
		*jy = t.jy;
	}
}

static int trdp_gen(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_EDIT];
	pcb_line_t *edit_line;
	pcb_board_t *pcb = pcb_data_get_top(subc->data);
	rnd_coord_t jx, jy;

	if (subc->extobj_data == NULL)
		trdp_unpack(subc);

TODO("enable this if we have data");
#if 0
	trdp *trdp;
	trdp = subc->extobj_data;
#endif

	pcb_exto_regen_begin(subc);


	edit_line = linelist_first(&ly->Line);

	jx = (edit_line->Point1.X + edit_line->Point2.X) / 2;
	jy = (edit_line->Point1.Y + edit_line->Point2.Y) / 2;

	trdp_gen_line_pt(pcb, subc, edit_line, 0, &jx, &jy);
	trdp_gen_line_pt(pcb, subc, edit_line, 1, &jx, &jy);

	pcb_subc_move_origin_to(subc, jx, jy, 0); 

	return pcb_exto_regen_end(subc);
}

static void pcb_trdp_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	pcb_exto_draw_mark(info, subc);
}


static void pcb_trdp_float_pre(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	rnd_trace("Trdp: edit pre %ld %ld\n", subc->ID, edit_obj->ID);
	trdp_clear(subc);
}

static void pcb_trdp_float_geo(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	rnd_trace("Trdp: edit geo %ld %ld\n", subc->ID, edit_obj == NULL ? -1 : edit_obj->ID);
	trdp_gen(subc, edit_obj);
}

static pcb_extobj_new_t pcb_trdp_float_new(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	rnd_trace("Trdp: float new %ld %ld\n", subc->ID, floater->ID);
	return PCB_EXTONEW_FLOATER;
}

static pcb_extobj_del_t pcb_trdp_float_del(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	return PCB_EXTODEL_SUBC;
}


static void pcb_trdp_chg_attr(pcb_subc_t *subc, const char *key, const char *value)
{
	rnd_trace("Trdp chg_attr\n");
	if (strncmp(key, "extobj::", 8) == 0) {
		trdp_clear(subc);
		trdp_unpack(subc);
		trdp_gen(subc, NULL);
	}
}


static pcb_subc_t *pcb_trdp_conv_objs(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from)
{
	pcb_subc_t *subc;
	pcb_line_t *l;
	pcb_layer_t *ly;
	pcb_dflgmap_t layers[] = {
		{"edit", PCB_LYT_DOC, "extobj", 0, 0},
		{NULL, 0, NULL, 0, 0}
	};

	rnd_trace("Trdp: conv_objs\n");

	if (objs->used != 1)
		return NULL; /* there must be a single line */

	/* refuse anything that's not a line */
	l = objs->array[0];
	if (l->type != PCB_OBJ_LINE)
		return NULL;

	/* use the layer of the first object */
	layers[0].lyt = pcb_layer_flags_(l->parent.layer);
	pcb_layer_purpose_(l->parent.layer, &layers[0].purpose);

	subc = pcb_exto_create(dst, "teardrop", layers, l->Point1.X, l->Point1.Y, 0, copy_from);
	if (copy_from == NULL) {
		/* set up defaults */
/*		pcb_attribute_put(&subc->Attributes, "extobj::pitch", "4mm");*/
	}

	/* create edit-objects */
	ly = &subc->data->Layer[LID_EDIT];
	l = pcb_line_dup(ly, objs->array[0]);
	PCB_FLAG_SET(PCB_FLAG_FLOATER, l);
	PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, l);
	pcb_attribute_put(&l->Attributes, "extobj::role", "edit");

	trdp_unpack(subc);
	trdp_gen(subc, NULL);
	return subc;
}

static pcb_extobj_t pcb_trdp = {
	"teardrop",
	pcb_trdp_draw_mark,
	pcb_trdp_float_pre,
	pcb_trdp_float_geo,
	pcb_trdp_float_new,
	pcb_trdp_float_del,
	pcb_trdp_chg_attr,
	pcb_trdp_del_pre,
	pcb_trdp_conv_objs,
	NULL
};
