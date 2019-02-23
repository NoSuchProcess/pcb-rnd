/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include "write.h"

#include "board.h"
#include "data.h"
#include "compat_misc.h"
#include "polygon.h"
#include "obj_subc_parent.h"
#include "obj_pstk_inlines.h"
#include "src_plugins/lib_netmap/netmap.h"
#include "funchash_core.h"
#include <genht/htpi.h>
#include <genht/hash.h>

typedef struct hyp_wr_s {
	pcb_board_t *pcb;
	FILE *f;
	const char *fn;

	const char *ln_top, *ln_bottom;	/* "layer name" for top and bottom groups */
	char *elem_name;
	size_t elem_name_len;
	pcb_cardinal_t poly_id;
	htpi_t pstk_cache;
	htpp_t grp_names;
	long pstk_cache_next;
	struct {
		unsigned elliptic:1;
	} warn;
} hyp_wr_t;

/* pcb-rnd y-axis points down; hyperlynx y-axis points up */
static pcb_coord_t flip(pcb_coord_t y)
{
	return (PCB->MaxHeight - y);
}

static void hyp_grp_init(hyp_wr_t *wr)
{
	htpp_init(&wr->grp_names, ptrhash, ptrkeyeq);
}

static void hyp_grp_uninit(hyp_wr_t *wr)
{
	htpp_entry_t *e;
	for (e = htpp_first(&wr->grp_names); e; e = htpp_next(&wr->grp_names, e))
		free(e->value);
	htpp_uninit(&wr->grp_names);
}

static const char *hyp_grp_name(hyp_wr_t *wr, pcb_layergrp_t *grp, const char *sugg_name)
{
	const char *name;
	name = htpp_get(&wr->grp_names, grp);
	if (name == NULL) {
		int n, dup = 0;
		name = sugg_name;
		if (name == NULL)
			name = grp->name;
		for(n = 0; n < wr->pcb->LayerGroups.len; n++) {
			pcb_layergrp_t *g = &wr->pcb->LayerGroups.grp[n];
			if ((g != grp) && (g->name != NULL) && (strcmp(g->name, name) == 0)) {
				dup = 1;
				break;
			}
		}
		if (dup)
			name = pcb_strdup_printf("%s___%d", name, n);
		else
			name = pcb_strdup(name);
		htpp_set(&wr->grp_names, grp, (char *)name);
	}
	return name;
}

static const char *safe_subc_name(hyp_wr_t *wr, pcb_subc_t *subc)
{
	const char *orig = subc->refdes;
	char *s;
	int len;

	if (orig == NULL)
		return orig;

	if (strchr(orig, '.') == NULL)
		return orig;

	len = strlen(orig);
	if (wr->elem_name_len < len) {
		wr->elem_name = realloc(wr->elem_name, len + 1);
		wr->elem_name_len = len;
	}
	memcpy(wr->elem_name, orig, len + 1);
	for (s = wr->elem_name; *s != '\0'; s++)
		if (*s == '.')
			*s = '_';

	return wr->elem_name;
}

static int write_hdr(hyp_wr_t * wr)
{
	char dt[128];

	pcb_print_utc(dt, sizeof(dt), 0);

	fprintf(wr->f, "* %s exported by pcb-rnd " PCB_VERSION " (" PCB_REVISION ") on %s\n", wr->pcb->Filename, dt);
	fprintf(wr->f, "* Board: %s\n", wr->pcb->Name);
	fprintf(wr->f, "{VERSION=2.0}\n");
	fprintf(wr->f, "{DATA_MODE=DETAILED}\n");
	fprintf(wr->f, "{UNITS=METRIC LENGTH}\n");
	return 0;
}

static int write_foot(hyp_wr_t * wr)
{
	fprintf(wr->f, "{END}\n");
	return 0;
}

static const char *get_layer_name(hyp_wr_t * wr, pcb_parenttype_t pt, pcb_layer_t * l)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;

	if (pt != PCB_PARENT_LAYER)
		return NULL;

	gid = pcb_layer_get_group_(l);
	if (gid < 0)
		return NULL;

	g = pcb_get_layergrp(wr->pcb, gid);
	return g->name;
}

static void write_pr_line(hyp_wr_t * wr, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_fprintf(wr->f, "  (PERIMETER_SEGMENT X1=%me Y1=%me X2=%me Y2=%me)\n", x1, flip(y1), x2, flip(y2));
}

static void hyp_pstk_init(hyp_wr_t *wr)
{
	htpi_init(&wr->pstk_cache, ptrhash, ptrkeyeq);
	wr->pstk_cache_next = 1;
}

static void hyp_pstk_uninit(hyp_wr_t *wr)
{
	htpi_uninit(&wr->pstk_cache);
}

void hyp_pstk_shape(hyp_wr_t *wr, const char *lynam, const pcb_pstk_shape_t *shp)
{
	pcb_coord_t sx, sy, minx, miny, maxx, maxy;
	int shnum = 0, n;
TODO(": this ignores rotation")
	switch(shp->shape) {
		case PCB_PSSH_HSHADOW:
TODO("hshadow TODO")
			break;
		case PCB_PSSH_CIRC:
			sx = sy = shp->data.circ.dia;
			shnum = 0;
			break;
		case PCB_PSSH_POLY:
TODO(": check if it is a rectangle")
			minx = maxx = shp->data.poly.x[0];
			miny = maxy = shp->data.poly.y[0];
			for(n = 1; n < shp->data.poly.len; n++) {
				if (shp->data.poly.x[n] < minx) minx = shp->data.poly.x[n];
				if (shp->data.poly.y[n] < miny) miny = shp->data.poly.y[n];
				if (shp->data.poly.x[n] > maxx) maxx = shp->data.poly.x[n];
				if (shp->data.poly.y[n] > maxy) maxy = shp->data.poly.y[n];
			}
			sx = maxx - minx;
			sy = maxy - miny;
			shnum = 1;
			break;
		case PCB_PSSH_LINE:
			sx = shp->data.line.x2 - shp->data.line.x1;
			sy = shp->data.line.y2 - shp->data.line.y1;
			if (sx < 0) sx = -sx;
			if (sy < 0) sy = -sy;
			if (shp->data.line.square)
				shnum = 1;
			else
				shnum = 2;
			break;
	}
	pcb_fprintf(wr->f, "	(%s, %d, %me, %me, 0)\n", lynam, shnum, sx, sy);
}

/* WARNING: not reentrant! */
static const char *hyp_pstk_cache(hyp_wr_t *wr, pcb_pstk_proto_t *proto, int print)
{
	static char name[16];
	long id;

	id = htpi_get(&wr->pstk_cache, proto);
	if (id == 0) { /* not found */
		if (print) { /* auto-allocate */
			id = wr->pstk_cache_next++;
			htpi_set(&wr->pstk_cache, proto, id);
		}
		else
			pcb_message(PCB_MSG_ERROR, "Internal error: unknown padstack prototype\n");
	}

	sprintf(name, "proto_%ld", id);
	if (print) {
		pcb_pstk_tshape_t *tshp = &proto->tr.array[0];
		int n;

		if (proto->hdia > 0)
			pcb_fprintf(wr->f, "{PADSTACK=%s,%me\n", name, proto->hdia);
		else
			fprintf(wr->f, "{PADSTACK=%s\n", name);

		for(n = 0; n < tshp->len; n++) {
			pcb_pstk_shape_t *shp = &tshp->shape[n];
			pcb_layer_type_t loc;
			pcb_layer_id_t l;

			if (!(shp->layer_mask & PCB_LYT_COPPER))
				continue; /* hyp suppports copper only */

			loc = (shp->layer_mask & PCB_LYT_ANYWHERE);
			for(l = 0; l < wr->pcb->LayerGroups.len; l++) {
				pcb_layergrp_t *lg = &wr->pcb->LayerGroups.grp[l];
				pcb_layer_type_t lyt = lg->ltype;
				if ((lyt & PCB_LYT_COPPER) && (lyt & loc))
					hyp_pstk_shape(wr, hyp_grp_name(wr, lg, NULL), shp);
			}
		}

		fprintf(wr->f, "}\n");
	}
	return name;
}

static void write_pstk(hyp_wr_t *wr, pcb_pstk_t *ps)
{
	pcb_subc_t *subc = pcb_gobj_parent_subc(ps->parent_type, &ps->parent);
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

	if ((subc != NULL) && (ps->term != NULL))
		pcb_fprintf(wr->f, "  (PIN X=%me Y=%me R=\"%s.%s\" P=%[4])\n", ps->x, flip(ps->y), safe_subc_name(wr, subc), ps->term, hyp_pstk_cache(wr, proto, 0));
	else
		pcb_fprintf(wr->f, "  (VIA X=%me Y=%me P=%[4])\n", ps->x, flip(ps->y), hyp_pstk_cache(wr, proto, 0));
}

static void write_poly(hyp_wr_t * wr, pcb_poly_t * poly)
{
	pcb_pline_t *pl;
	pcb_vnode_t *v;

	if (poly->Clipped == NULL) {
		pcb_layer_t *l = poly->parent.layer;
		pcb_poly_init_clip(l->parent.data, l, poly);
	}

	if (poly->Clipped == NULL)
		return;

	pl = poly->Clipped->contours;
	do {
		v = pl->head.next;

		if (pl == poly->Clipped->contours)
			pcb_fprintf(wr->f, "  {POLYGON L=%[4] T=POUR W=0.0 ID=%d X=%me Y=%me\n",
									get_layer_name(wr, poly->parent_type, poly->parent.layer), ++wr->poly_id, v->point[0], flip(v->point[1]));
		else
			/* hole. Use same ID as polygon. */
			pcb_fprintf(wr->f, "  {POLYVOID ID=%d X=%me Y=%me\n", wr->poly_id, v->point[0], flip(v->point[1]));

		for (v = v->next; v != pl->head.next; v = v->next)
			pcb_fprintf(wr->f, "    (LINE X=%me Y=%me)\n", v->point[0], flip(v->point[1]));

		v = pl->head.next;					/* repeat first point */
		pcb_fprintf(wr->f, "    (LINE X=%me Y=%me)\n", v->point[0], flip(v->point[1]));

		fprintf(wr->f, "  }\n");
	} while ((pl = pl->next) != NULL);
}


static void write_line(hyp_wr_t * wr, pcb_line_t * line)
{
	pcb_fprintf(wr->f, "  (SEG X1=%me Y1=%me X2=%me Y2=%me W=%me L=%[4])\n",
							line->Point1.X, flip(line->Point1.Y), line->Point2.X, flip(line->Point2.Y),
							line->Thickness, get_layer_name(wr, line->parent_type, line->parent.layer));
}

static void write_arc_(hyp_wr_t * wr, const char *cmd, pcb_arc_t * arc, const char *layer)
{
	pcb_coord_t x1, y1, x2, y2;

	if (arc->Width != arc->Height) {
		if (!wr->warn.elliptic) {
			pcb_message(PCB_MSG_WARNING, "Elliptic arcs are not supported - omitting all elliptic arcs\n");
			wr->warn.elliptic = 1;
		}
		return;
	}

	if ((arc->Delta >= 0) == (layer == NULL)) {
		pcb_arc_get_end(arc, 0, &x1, &y1);
		pcb_arc_get_end(arc, 1, &x2, &y2);
	}
	else {
		pcb_arc_get_end(arc, 1, &x1, &y1);
		pcb_arc_get_end(arc, 0, &x2, &y2);
	}

	pcb_fprintf(wr->f, "(%s X1=%me Y1=%me X2=%me Y2=%me XC=%me YC=%me R=%me", cmd, x1, flip(y1), x2, flip(y2), arc->X,
							flip(arc->Y), arc->Width);
	if (layer != NULL)
		pcb_fprintf(wr->f, " W=%me L=%[4]", arc->Thickness, layer);
	fprintf(wr->f, ")\n");
}

static void write_pr_arc(hyp_wr_t * wr, pcb_arc_t * arc)
{
	fprintf(wr->f, "  ");
	write_arc_(wr, "PERIMETER_ARC", arc, NULL);
}

static void write_arc(hyp_wr_t * wr, pcb_arc_t * arc)
{
	fprintf(wr->f, "  ");
	write_arc_(wr, "ARC", arc, get_layer_name(wr, arc->parent_type, arc->parent.layer));
}

static int write_board(hyp_wr_t * wr)
{
	int has_outline;
	pcb_layergrp_id_t i;
	pcb_layergrp_t *g;

	fprintf(wr->f, "{BOARD\n");

	has_outline = pcb_has_explicit_outline(PCB);
	if (!has_outline) {
		/* implicit outline */
		fprintf(wr->f, "* implicit outline derived from board width and height\n");
		write_pr_line(wr, 0, 0, PCB->MaxWidth, 0);
		write_pr_line(wr, 0, 0, 0, PCB->MaxHeight);
		write_pr_line(wr, PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
		write_pr_line(wr, 0, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
	}
	else { /* explicit outline */
		for(i = 0, g = PCB->LayerGroups.grp; i < PCB->LayerGroups.len; i++,g++) {
			int n;

			if (!PCB_LAYER_IS_OUTLINE(g->ltype, g->purpi))
				continue;

			for(n = 0; n < g->len; n++) {
				pcb_layer_t *l = pcb_get_layer(PCB->Data, g->lid[n]);
				gdl_iterator_t it;
				pcb_line_t *line;
				pcb_arc_t *arc;

				if (l == NULL)
					continue;

TODO("layer: refuse negative layers and warn for objects other than line/arc")

				linelist_foreach(&l->Line, &it, line) {
					write_pr_line(wr, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
				}
				arclist_foreach(&l->Arc, &it, arc) {
					write_pr_arc(wr, arc);
				}
			}
		}
	}
	fprintf(wr->f, "}\n");
	return 0;
}

static int write_lstack(hyp_wr_t * wr)
{
	int n;

	fprintf(wr->f, "{STACKUP\n");
	for (n = 0; n < wr->pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *grp = &wr->pcb->LayerGroups.grp[n];
		const char *name = grp->name;

		if (grp->ltype & PCB_LYT_COPPER) {
			pcb_fprintf(wr->f, "  (SIGNAL T=0.003500 L=%[4])\n", hyp_grp_name(wr, grp, name));
			if (grp->ltype & PCB_LYT_TOP)
				wr->ln_top = name;
			else if (grp->ltype & PCB_LYT_BOTTOM)
				wr->ln_bottom = name;
		}
		else if (grp->ltype & PCB_LYT_SUBSTRATE) {
			char tmp[128];
			if (name == NULL) {
				sprintf(tmp, "dielectric layer %d", n);
				name = tmp;
			}
			pcb_fprintf(wr->f, "  (DIELECTRIC T=0.160000 L=%[4])\n", hyp_grp_name(wr, grp, name));
		}
	}

	fprintf(wr->f, "}\n");
	return 0;
}

static int write_devices(hyp_wr_t * wr)
{
	gdl_iterator_t it;
	pcb_subc_t *subc;
	int cnt;

	fprintf(wr->f, "{DEVICES\n");

	cnt = 0;
	subclist_foreach(&wr->pcb->Data->subc, &it, subc) {
		const char *layer, *descr;
		int on_bottom = 0;

		pcb_subc_get_side(subc, &on_bottom);

		if (on_bottom)
			layer = wr->ln_bottom;
		else
			layer = wr->ln_top;
		descr = subc->refdes;
		if (descr == NULL)
			descr = "?";

		pcb_fprintf(wr->f, "  (? REF=%[4] NAME=%[4] L=%[4])\n", safe_subc_name(wr, subc), descr, layer);
		cnt++;
	}

	if (cnt == 0)
		pcb_message(PCB_MSG_WARNING,
								"There is no element on the board - this limites the use of the resulting .hyp file, as it won't be able to connect to a simulation\n");

	fprintf(wr->f, "}\n");
	return 0;
}

static int write_pstk_protos(hyp_wr_t *wr, pcb_data_t *data)
{
	gdl_iterator_t it;
	pcb_subc_t *subc;
	pcb_cardinal_t n, end;

	/* print the protos */
	end = pcb_vtpadstack_proto_len(&data->ps_protos);
	for(n = 0; n < end; n++)
		hyp_pstk_cache(wr, &data->ps_protos.array[n], 1);

	/* recurse on subcircuits */
	subclist_foreach(&data->subc, &it, subc)
		write_pstk_protos(wr, subc->data);

	return 0;
}

static int write_nets(hyp_wr_t * wr)
{
	htpp_entry_t *e;
	pcb_netmap_t map;

	pcb_netmap_init(&map, wr->pcb);
	for (e = htpp_first(&map.n2o); e != NULL; e = htpp_next(&map.n2o, e)) {
		dyn_obj_t *o;
		pcb_net_t *net = e->key;
		pcb_fprintf(wr->f, "{NET=%[4]\n", net->name);
		for (o = e->value; o != NULL; o = o->next) {
			switch (o->obj->type) {
			case PCB_OBJ_LINE:
				write_line(wr, (pcb_line_t *) o->obj);
				break;
			case PCB_OBJ_ARC:
				write_arc(wr, (pcb_arc_t *) o->obj);
				break;
			case PCB_OBJ_PSTK:
				write_pstk(wr, (pcb_pstk_t *) o->obj);
				break;

			case PCB_OBJ_POLY:
				write_poly(wr, (pcb_poly_t *) o->obj);
				break;

			case PCB_OBJ_RAT:
				break;									/* not yet done */

			case PCB_OBJ_TEXT:
			case PCB_OBJ_SUBC:
			case PCB_OBJ_NET:
			case PCB_OBJ_NET_TERM:
			case PCB_OBJ_LAYER:
			case PCB_OBJ_LAYERGRP:
			case PCB_OBJ_VOID:
				break;									/* silently ignore these */
			}
		}
		fprintf(wr->f, "}\n");
	}

	pcb_netmap_uninit(&map);
	return 0;
}

int io_hyp_write_pcb(pcb_plug_io_t * ctx, FILE * f, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	hyp_wr_t wr;

	memset(&wr, 0, sizeof(wr));
	wr.pcb = PCB;
	wr.f = f;
	wr.fn = new_filename;

	hyp_pstk_init(&wr);
	hyp_grp_init(&wr);

	pcb_printf_slot[4] = "%{{\\}\\()\t\r\n \"=}mq";

	if (write_hdr(&wr) != 0)
		goto err;

	if (write_board(&wr) != 0)
		goto err;

	if (write_lstack(&wr) != 0)
		goto err;

	if (write_devices(&wr) != 0)
		goto err;

	if (write_pstk_protos(&wr, wr.pcb->Data) != 0)
		goto err;

	if (write_nets(&wr) != 0)
		goto err;

	if (write_foot(&wr) != 0)
		goto err;

	hyp_pstk_uninit(&wr);
	hyp_grp_uninit(&wr);
	free(wr.elem_name);
	return 0;

err:;
	hyp_pstk_uninit(&wr);
	hyp_grp_uninit(&wr);
	free(wr.elem_name);
	return -1;
}
