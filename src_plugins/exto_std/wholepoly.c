/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Extended object for a wholepoly; edit: first polygon; explicit polygon object for each island
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#define LID_WHP 0

typedef struct {
	RND_DAD_DECL_NOINIT(dlg)
	int gui_active;
} wholepoly;

static void pcb_wholepoly_del_pre(pcb_subc_t *subc)
{
	wholepoly *whp = subc->extobj_data;
	rnd_trace("WhP del_pre\n");

	if ((whp != NULL) && (whp->gui_active))
		RND_DAD_FREE(whp->dlg);

	free(whp);
	subc->extobj_data = NULL;
}

static void wholepoly_unpack(pcb_subc_t *obj)
{
#if 0
/* no settings yet */
	wholepoly *whp;

	if (obj->extobj_data == NULL)
		obj->extobj_data = calloc(sizeof(wholepoly), 1);
	whp = obj->extobj_data;
#endif
}

/* remove all existing padstacks from the subc */
static void wholepoly_clear(pcb_subc_t *subc)
{
	pcb_poly_t *p, *back = NULL;
	pcb_layer_t *ly = &subc->data->Layer[LID_WHP];

	for(p = polylist_first(&ly->Polygon); p != NULL; p = polylist_next(p)) {
		if (PCB_FLAG_TEST(PCB_FLAG_FLOATER, p)) {
			/* do not free the floater, but remember as the last kept floater */
			back = p;
		}
		else {
			pcb_poly_free(p);
			p = back;
			if (p == NULL)
				p = polylist_first(&ly->Polygon);
		}
	}
}

static void wholepoly_gen_poly(pcb_board_t *pcb, pcb_subc_t *subc, pcb_poly_t *poly)
{
	pcb_poly_it_t it;
	rnd_polyarea_t *pa;
	double best = 0;
	rnd_pline_t *best_pl;

	/* figure the biggest area island - that one shouldn't be traced */
	for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		rnd_pline_t *pl = pcb_poly_contour(&it);
		if (pl != NULL) {
			if (pl->area > best) {
				best = pl->area;
				best_pl = pl;
			}
		}
	}

	/* trace each missing island with a new poly */
	for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
		int go;
		rnd_coord_t x, y;
		rnd_pline_t *pl = pcb_poly_contour(&it);
		pcb_poly_t *np;

		if ((pl == NULL) || (pl == best_pl))
			continue;

		np = pcb_poly_new(poly->parent.layer, poly->Clearance, poly->Flags);
		PCB_FLAG_CLEAR(PCB_FLAG_FLOATER, np);
		for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y))
			pcb_poly_point_new(np, x, y);
		pcb_add_poly_on_layer(poly->parent.layer, np);
	}
}

static int wholepoly_gen(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	pcb_poly_t *poly;
	pcb_board_t *pcb = pcb_data_get_top(subc->data);
	pcb_layer_t *ly = &subc->data->Layer[LID_WHP];
	wholepoly *whp;

	if (subc->extobj_data == NULL)
		wholepoly_unpack(subc);

	whp = subc->extobj_data;

	pcb_exto_regen_begin(subc);
	for(poly = polylist_first(&ly->Polygon); poly != NULL; poly = polylist_next(poly))
		wholepoly_gen_poly(pcb, subc, poly);

	return pcb_exto_regen_end(subc);
}

static void pcb_wholepoly_draw_mark(pcb_draw_info_t *info, pcb_subc_t *subc)
{
	pcb_poly_t *poly;
	pcb_layer_t *ly = &subc->data->Layer[LID_WHP];

	if (subc->extobj_data == NULL)
		wholepoly_unpack(subc);

	printf("mark!!!\n");

	pcb_exto_draw_mark(info, subc);
}


static void pcb_wholepoly_float_pre(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	rnd_trace("WhP: edit pre %ld %ld\n", subc->ID, edit_obj->ID);
	wholepoly_clear(subc);
}

static void pcb_wholepoly_float_geo(pcb_subc_t *subc, pcb_any_obj_t *edit_obj)
{
	rnd_trace("WhP: edit geo %ld %ld\n", subc->ID, edit_obj == NULL ? -1 : edit_obj->ID);
	wholepoly_gen(subc, edit_obj);
}

static pcb_extobj_new_t pcb_wholepoly_float_new(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	rnd_trace("WhP: float new %ld %ld\n", subc->ID, floater->ID);
	return PCB_EXTONEW_FLOATER;
}

static pcb_extobj_del_t pcb_wholepoly_float_del(pcb_subc_t *subc, pcb_any_obj_t *floater)
{
	pcb_layer_t *ly = &subc->data->Layer[LID_WHP];
	long len = polylist_length(&ly->Polygon);

	rnd_trace("WhP: float del %ld %ld edit-objs=%ld\n", subc->ID, floater->ID, len);

	return len == 1 ? PCB_EXTODEL_SUBC : PCB_EXTODEL_FLOATER; /* removing the last floater should remove the subc */
}


static void pcb_wholepoly_chg_attr(pcb_subc_t *subc, const char *key, const char *value)
{
	rnd_trace("WhP chg_attr\n");
	if (strncmp(key, "extobj::", 8) == 0) {
		wholepoly_clear(subc);
		wholepoly_unpack(subc);
		wholepoly_gen(subc, NULL);
	}
}


static pcb_subc_t *pcb_wholepoly_conv_objs(pcb_data_t *dst, vtp0_t *objs, pcb_subc_t *copy_from)
{
	long n;
	rnd_coord_t cx, cy;
	pcb_subc_t *subc;
	pcb_poly_t *p;
	pcb_layer_t *ly;
	pcb_dflgmap_t layers[] = {
		{"edit", PCB_LYT_DOC, "extobj", 0, 0},
		{NULL, 0, NULL, 0, 0}
	};

	rnd_trace("WhP: conv_objs\n");

	/* refuse anything that's not a polygon */
	for(n = 0; n < objs->used; n++) {
		p = objs->array[n];
		if (p->type != PCB_OBJ_POLY)
			return NULL;
	}

	/* use the layer of the first object */
	p = objs->array[0];
	layers[0].lyt = pcb_layer_flags_(p->parent.layer);
	pcb_layer_purpose_(p->parent.layer, &layers[0].purpose);

	cx = cy = 0;
	for(n = 0; n < p->PointN; n++) {
		cx += p->Points[n].X;
		cy += p->Points[n].Y;
	}
	cx /= (double)p->PointN;
	cy /= (double)p->PointN;
rnd_printf("Center: %mm;%mm\n", cx, cy);
	subc = pcb_exto_create(dst, "wholepoly", layers, cx, cy, 0, copy_from);

#if 0
	if (copy_from == NULL)
		pcb_attribute_put(&subc->Attributes, "extobj::pitch", "4mm");
#endif

	/* create edit-objects */
	ly = &subc->data->Layer[LID_WHP];
	for(n = 0; n < objs->used; n++) {
		p = pcb_poly_dup(ly, objs->array[n]);
		PCB_FLAG_SET(PCB_FLAG_FLOATER, p);
		PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, p);
		pcb_attribute_put(&p->Attributes, "extobj::role", "edit");
	}

	wholepoly_unpack(subc);
	wholepoly_gen(subc, NULL);
	return subc;
}


static void pcb_wholepoly_close_cb(void *caller_data, rnd_hid_attr_ev_t ev)
{
	pcb_subc_t *subc = caller_data;
	wholepoly *whp = subc->extobj_data;

	RND_DAD_FREE(whp->dlg);
	whp->gui_active = 0;
}

static void pcb_wholepoly_gui_propedit(pcb_subc_t *subc)
{
	rnd_hid_dad_buttons_t clbtn[] = {{"Close", 0}, {NULL, 0}};
	wholepoly *whp;

	rnd_trace("WhP: gui propedit\n");

	if (subc->extobj_data == NULL)
		wholepoly_unpack(subc);
	whp = subc->extobj_data;

	rnd_trace("WhP: active=%d\n", whp->gui_active);
	if (whp->gui_active)
		return; /* do not open another */

	RND_DAD_BEGIN_VBOX(whp->dlg);
		RND_DAD_COMPFLAG(whp->dlg, RND_HATF_EXPFILL);
		RND_DAD_LABEL(whp->dlg, "The whole poly extended object\ndoes not have settings");
		RND_DAD_BUTTON_CLOSES(whp->dlg, clbtn);
	RND_DAD_END(whp->dlg);

	/* set up the context */
	whp->gui_active = 1;

	RND_DAD_NEW("wholepoly", whp->dlg, "Whole polygons", subc, rnd_false, pcb_wholepoly_close_cb);
}

static pcb_extobj_t pcb_wholepoly = {
	"wholepoly",
	pcb_wholepoly_draw_mark,
	pcb_wholepoly_float_pre,
	pcb_wholepoly_float_geo,
	pcb_wholepoly_float_new,
	pcb_wholepoly_float_del,
	pcb_wholepoly_chg_attr,
	pcb_wholepoly_del_pre,
	pcb_wholepoly_conv_objs,
	pcb_wholepoly_gui_propedit
};
