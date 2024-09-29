/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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

/* convert a set of board cutouts into a polyarea */

/* Add an rnd_polyarea_t *src into rnd_polyarea_t **dst if src is in contour */
#define SOLID_ADD_PA(dst, srcpa) \
do { \
	rnd_polyarea_t *__res__, *__src__ = srcpa; \
	if (*dst != NULL) { \
			rnd_polyarea_boolean_free(*dst, __src__, &__res__, RND_PBO_UNITE); \
			*dst = __res__; \
	} \
	else \
		*dst = __src__; \
} while(0)


/* Same but src is a pcb_poly_t */
#define SOLID_ADD_POLY(dst, src) \
do { \
	rnd_bool need_full; \
	SOLID_ADD_PA(dst, pcb_poly_to_polyarea(src, &need_full)); \
	pcb_poly_free(src); \
} while(0)


/* Add all layer-object-drawn solids (lines, arcs, etc) */
static void topoly_solid_add_layerobjs(pcb_board_t *pcb, vtp0_t *solids_per_layer, pcb_dynf_t df)
{
	rnd_layer_id_t lid;

	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		pcb_layer_t *layer = &pcb->Data->Layer[lid];
		pcb_line_t *line;
		pcb_arc_t *arc;
		pcb_poly_t *poly;
		rnd_rtree_it_t it;
		rnd_polyarea_t **solids = (rnd_polyarea_t **)&solids_per_layer->array[lid];

		if (layer->line_tree != NULL) {
			for(line = rnd_rtree_all_first(&it, layer->line_tree); line != NULL; line = rnd_rtree_all_next(&it)) {
				if (PCB_DFLAG_TEST(&line->Flags, df)) {
					rnd_polyarea_t *pa = pcb_poly_from_pcb_line(line, line->Thickness);
					SOLID_ADD_PA(solids, pa);
				}
			}
		}

		if (layer->arc_tree != NULL) {
			for(arc = rnd_rtree_all_first(&it, layer->arc_tree); arc != NULL; arc = rnd_rtree_all_next(&it)) {
				if (PCB_DFLAG_TEST(&arc->Flags, df)) {
					rnd_polyarea_t *pa = pcb_poly_from_pcb_arc(arc, arc->Thickness);
					SOLID_ADD_PA(solids, pa);
				}
			}
		}

		if (layer->polygon_tree != NULL)
			for(poly = rnd_rtree_all_first(&it, layer->polygon_tree); poly != NULL; poly = rnd_rtree_all_next(&it))
				if (PCB_DFLAG_TEST(&poly->Flags, df))
					SOLID_ADD_POLY(solids, poly);

		TODO("add text as well");
	}
}

static void topoly_solid_add_pstk(pcb_board_t *pcb, pcb_pstk_t *pstk, rnd_layer_id_t lid, vtp0_t *solids_per_layer)
{
	pcb_pstk_shape_t *shp;
	rnd_polyarea_t **solids = (rnd_polyarea_t **)&solids_per_layer->array[lid];
	pcb_layer_t *layer = &pcb->Data->Layer[lid];

	shp = pcb_pstk_shape_at(pcb, pstk, layer);
	if (shp == NULL)
		return;

	switch(shp->shape) {
		case PCB_PSSH_POLY:
		{
			rnd_polyarea_t *pa;
			rnd_vector_t v;
			rnd_pline_t *pl;
			pcb_pstk_poly_t *poly = &shp->data.poly;
			long n;

			v[0] = pstk->x + poly->x[0]; v[1] = pstk->y + poly->y[0];
			pl = rnd_poly_contour_new(v);
			for(n = 1; n < poly->len; n++) {
				v[0] = pstk->x + poly->x[n]; v[1] = pstk->y + poly->y[n];
				rnd_poly_vertex_include(pl->head->prev, rnd_poly_node_create(v));
			}
			rnd_poly_contour_pre(pl, 1);

			pa = rnd_polyarea_create();
			rnd_polyarea_contour_include(pa, pl);

			if (!rnd_poly_valid(pa)) {
				rnd_polyarea_free(&pa);
				pa = rnd_polyarea_create();
				rnd_poly_contour_inv(pl);
				rnd_polyarea_contour_include(pa, pl);
			}

			SOLID_ADD_PA(solids, pa);
			break;
		}

		case PCB_PSSH_CIRC:
			{
				rnd_polyarea_t *pac;
				pac = rnd_poly_from_circle(
					pstk->x + shp->data.circ.x, pstk->y + shp->data.circ.y,
					rnd_round((double)shp->data.circ.dia / 2.0));
				SOLID_ADD_PA(solids, pac);
			}
			break;


		case PCB_PSSH_HSHADOW: return;

		case PCB_PSSH_LINE:
			{
				pcb_line_t l = {0};
				l.Point1.X = pstk->x + shp->data.line.x1; l.Point1.Y = pstk->y + shp->data.line.y1;
				l.Point2.X = pstk->x + shp->data.line.x2; l.Point2.Y = pstk->y + shp->data.line.y2;
				l.Thickness = shp->data.line.thickness;
				if (shp->data.line.square)
					PCB_FLAG_SET(PCB_FLAG_SQUARE, &l);
				SOLID_ADD_PA(solids, pcb_poly_from_pcb_line(&l, l.Thickness));
			}
			break;
	}
}

/* Add all padstack based solids */
static void topoly_solid_add_pstks(pcb_board_t *pcb, vtp0_t *solids_per_layer, pcb_dynf_t df)
{
	rnd_rtree_it_t it;
	rnd_box_t *n;

	if (pcb->Data->padstack_tree == NULL)
		return;

	for(n = rnd_rtree_all_first(&it, pcb->Data->padstack_tree); n != NULL; n = rnd_rtree_all_next(&it)) {
		pcb_pstk_t *ps = (pcb_pstk_t *)n;
		if (PCB_DFLAG_TEST(&ps->Flags, df)) {
			rnd_layer_id_t lid;
			for(lid = 0; lid < pcb->Data->LayerN; lid++)
				topoly_solid_add_pstk(pcb, ps, lid, solids_per_layer);
		}
	}
}

vtp0_t *pcb_topoly_solids_in(pcb_board_t *pcb, pcb_dynf_t df)
{
	vtp0_t *res = calloc(sizeof(vtp0_t), 1);

	vtp0_enlarge(res, pcb->Data->LayerN-1);

	topoly_solid_add_layerobjs(pcb, res, df);
	topoly_solid_add_pstks(pcb, res, df);

	return res;
}

