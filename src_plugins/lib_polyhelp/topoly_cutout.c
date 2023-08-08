/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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
#define CUTOUT_ADD_PA(dst, srcpa, contour) \
do { \
	rnd_polyarea_t *__res__, *__src__ = srcpa; \
	if (rnd_polyarea_touching(__src__, (rnd_polyarea_t *)contour)) { \
		if (*dst != NULL) { \
				rnd_polyarea_boolean_free(*dst, __src__, &__res__, RND_PBO_UNITE); \
				*dst = __res__; \
		} \
		else \
			*dst = __src__; \
	} \
} while(0)


/* Same but src is a pcb_poly_t */
#define CUTOUT_ADD_POLY(dst, src, contour) \
do { \
	rnd_bool need_full; \
	CUTOUT_ADD_PA(dst, pcb_poly_to_polyarea(src, &need_full), contour); \
	pcb_poly_free(src); \
} while(0)

#define PCB_LAYER_IS_SLOT(lyt, purpi) (((lyt) & (PCB_LYT_MECH)) && (((purpi) == F_uroute) || ((purpi) == F_proute)))



/* Add all layer-object-drawn cutouts (lines, arcs, etc) */
static void topoly_cutout_add_layerobjs(pcb_board_t *pcb, rnd_polyarea_t **cutouts, pcb_dynf_t df, const rnd_polyarea_t *contour)
{
	rnd_layer_id_t lid;

	for(lid = 0; lid < pcb->Data->LayerN; lid++) {
		pcb_layer_type_t lyt = pcb_layer_flags(pcb, lid);
		int purpi = pcb_layer_purpose(pcb, lid, NULL);
		pcb_layer_t *layer = &pcb->Data->Layer[lid];
		pcb_poly_t *poly;
		pcb_line_t *line;
		pcb_arc_t *arc;
		rnd_rtree_it_t it;
		

		if (!PCB_LAYER_IS_ROUTE(lyt, purpi)) continue;
/*rnd_trace("Outline [%ld]\n", lid);*/

		if (layer->line_tree != NULL) {
			for(line = rnd_rtree_all_first(&it, layer->line_tree); line != NULL; line = rnd_rtree_all_next(&it)) {
				if (PCB_DFLAG_TEST(&line->Flags, df)) continue; /* object already found - either as outline or as a cutout */

				if (PCB_LAYER_IS_SLOT(lyt, purpi)) {
					/* slot layer: go with per object cutout */
					rnd_polyarea_t *pa = pcb_poly_from_pcb_line(line, line->Thickness);
					CUTOUT_ADD_PA(cutouts, pa, contour);
				}
				else {
					/* boundary layer: go with centerline poly of closed loops */
					poly = pcb_topoly_conn_with(pcb, (pcb_any_obj_t *)line, PCB_TOPOLY_FLOATING, df);
					if (poly != NULL)
					CUTOUT_ADD_POLY(cutouts, poly, contour);
					else
						rnd_message(RND_MSG_ERROR, "Cutout error: need closed loops; cutout omitted\n(Hint: use the wireframe draw mode to see broken connections; use a coarse grid and snap to fix them up!)\n");
/*rnd_trace(" line: %ld %d -> %p\n", line->ID, PCB_DFLAG_TEST(&line->Flags, df), poly);*/
				}
			}
		}

		if (layer->arc_tree != NULL) {
			for(arc = rnd_rtree_all_first(&it, layer->arc_tree); arc != NULL; arc = rnd_rtree_all_next(&it)) {
				if (PCB_DFLAG_TEST(&arc->Flags, df)) continue; /* object already found - either as outline or as a cutout */

				if (PCB_LAYER_IS_SLOT(lyt, purpi)) {
					/* slot layer: go with per object cutout */
					rnd_polyarea_t *pa = pcb_poly_from_pcb_arc(arc, arc->Thickness);
					CUTOUT_ADD_PA(cutouts, pa, contour);
				}
				else {
					poly = pcb_topoly_conn_with(pcb, (pcb_any_obj_t *)arc, PCB_TOPOLY_FLOATING, df);
					if (poly != NULL)
						CUTOUT_ADD_POLY(cutouts, poly, contour);
					else
						rnd_message(RND_MSG_ERROR, "Cutout error: need closed loops; cutout omitted\n(Hint: use the wireframe draw mode to see broken connections; use a coarse grid and snap to fix them up!)\n");
/*rnd_trace(" arc: %ld %d -> %p\n", arc->ID, PCB_DFLAG_TEST(&arc->Flags, df), poly);*/
				}
			}
		}
	}
}

static void topoly_cutout_add_pstk(pcb_board_t *pcb, pcb_pstk_t *pstk, pcb_layer_t *layer, rnd_polyarea_t **cutouts, const pcb_topoly_cutout_opts_t *opts, const rnd_polyarea_t *contour)
{
	pcb_pstk_shape_t *shp, tmp;

	shp = pcb_pstk_shape_mech_or_hole_at(pcb, pstk, layer, &tmp);
	if (shp == NULL)
		return;

	switch(shp->shape) {
		case PCB_PSSH_POLY:
			if (!opts->pstk_omit_slot_poly) {
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

				CUTOUT_ADD_PA(cutouts, pa, contour);
			}
			break;

		case PCB_PSSH_CIRC:
			if (shp->data.circ.dia >= opts->pstk_min_drill_dia) {
				rnd_polyarea_t *pac;
				pac = rnd_poly_from_circle(
					pstk->x + shp->data.circ.x, pstk->y + shp->data.circ.y,
					rnd_round((double)shp->data.circ.dia / 2.0));
				CUTOUT_ADD_PA(cutouts, pac, contour);
			}
			break;


		case PCB_PSSH_HSHADOW: return;

		case PCB_PSSH_LINE:
			if (shp->data.line.thickness >= opts->pstk_min_line_thick) {
				pcb_line_t l = {0};
				l.Point1.X = pstk->x + shp->data.line.x1; l.Point1.Y = pstk->y + shp->data.line.y1;
				l.Point2.X = pstk->x + shp->data.line.x2; l.Point2.Y = pstk->y + shp->data.line.y2;
				l.Thickness = shp->data.line.thickness;
				if (shp->data.line.square)
					PCB_FLAG_SET(PCB_FLAG_SQUARE, &l);
				CUTOUT_ADD_PA(cutouts, pcb_poly_from_pcb_line(&l, l.Thickness), contour);
			}
			break;
	}
}

/* Add all padstack based cutouts */
static void topoly_cutout_add_pstks(pcb_board_t *pcb, rnd_polyarea_t **cutouts, const pcb_topoly_cutout_opts_t *opts, pcb_dynf_t df, const rnd_polyarea_t *contour)
{
	rnd_rtree_it_t it;
	rnd_box_t *n;
	rnd_layer_id_t lid = -1;
	pcb_layer_t *toply;

	if ((pcb_layer_list(pcb, PCB_LYT_COPPER | PCB_LYT_TOP, &lid, 1) != 1) && (pcb_layer_list(PCB, PCB_LYT_COPPER | PCB_LYT_BOTTOM, &lid, 1) != 1)) {
		rnd_message(RND_MSG_ERROR, "A top or bottom copper layer is required for pcb_topoly_cutouts*() - padstack cutouts are omitted\n");
		return;
	}
	toply = pcb_get_layer(pcb->Data, lid);

	if (pcb->Data->padstack_tree != NULL) {
		for(n = rnd_rtree_all_first(&it, pcb->Data->padstack_tree); n != NULL; n = rnd_rtree_all_next(&it)) {
			pcb_pstk_t *ps = (pcb_pstk_t *)n;
			if (!PCB_DFLAG_TEST(&ps->Flags, df)) /* if a padstack is marked, it is on the contour and it should already be subtracted from the contour poly, skip it */
				topoly_cutout_add_pstk(pcb, ps, toply, cutouts, opts, contour);
		}
	}
}

rnd_polyarea_t *pcb_topoly_cutouts_in(pcb_board_t *pcb, pcb_dynf_t df, pcb_poly_t *contour, const pcb_topoly_cutout_opts_t *opts)
{
	rnd_polyarea_t *res = NULL;

	topoly_cutout_add_layerobjs(pcb, &res, df, contour->Clipped);
	if (!opts->omit_pstks)
		topoly_cutout_add_pstks(pcb, &res, opts, df, contour->Clipped);

	return res;
}

