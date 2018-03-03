/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include "mesh.h"
#include "layer.h"
#include "layer_ui.h"
#include "board.h"
#include "data.h"

static pcb_mesh_t mesh;

static void mesh_add_edge(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t crd)
{
	vtc0_append(&mesh->line[dir].edge, crd);
}

static void mesh_add_range(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t c1, pcb_coord_t c2, pcb_coord_t dens)
{
	pcb_range_t *r = vtr0_alloc_append(&mesh->line[dir].dens, 1);
	r->begin = c1;
	r->end = c2;
	r->data[0].c = dens;
}

static void mesh_add_obj(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t c1, pcb_coord_t c2, int aligned)
{
	if (aligned) {
		mesh_add_edge(mesh, dir, c1 - mesh->dens_obj * 2 / 3);
		mesh_add_edge(mesh, dir, c1 + mesh->dens_obj * 1 / 3);
		mesh_add_edge(mesh, dir, c2 - mesh->dens_obj * 1 / 3);
		mesh_add_edge(mesh, dir, c2 + mesh->dens_obj * 2 / 3);
	}
	mesh_add_range(mesh, dir, c1, c2, mesh->dens_obj);
}

/* generate edges and ranges looking at objects on the given layer */
static int mesh_gen_obj(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	pcb_line_t *line;
	pcb_line_t *arc;
	pcb_poly_t *poly;
	gdl_iterator_t it;
	

	linelist_foreach(&mesh->layer->Line, &it, line) {
		pcb_coord_t tmp, x1 = line->Point1.X, y1 = line->Point1.Y, x2 = line->Point2.X, y2 = line->Point2.Y;
		int aligned = (x1 == x2) || (y1 == y2);

		switch(dir) {
			case PCB_MESH_HORIZONTAL:
				if (y1 < y2)
					mesh_add_obj(mesh, dir, y1 - line->Thickness/2, y2 + line->Thickness/2, aligned);
				else
					mesh_add_obj(mesh, dir, y2 - line->Thickness/2, y1 + line->Thickness/2, aligned);
				break;
			case PCB_MESH_VERTICAL:
				if (x1 < x2)
					mesh_add_obj(mesh, dir, x1 - line->Thickness/2, x2 + line->Thickness/2, aligned);
				else
					mesh_add_obj(mesh, dir, x2 - line->Thickness/2, x1 + line->Thickness/2, aligned);
				break;
			default: break;
		}
	}

	arclist_foreach(&mesh->layer->Arc, &it, arc) {
		/* no point in encorcinf 1/3 2/3 rule, just set the range */
		switch(dir) {
			case PCB_MESH_HORIZONTAL: mesh_add_range(mesh, dir, arc->BoundingBox.Y1 + arc->Clearance/2, arc->BoundingBox.Y2 - arc->Clearance/2, mesh->dens_obj); break;
			case PCB_MESH_VERTICAL:   mesh_add_range(mesh, dir, arc->BoundingBox.X1 + arc->Clearance/2, arc->BoundingBox.X2 - arc->Clearance/2, mesh->dens_obj); break;
			default: break;
		}
	}

#warning TODO: text and padstacks

	polylist_foreach(&mesh->layer->Polygon, &it, poly) {
		pcb_poly_it_t it;
		pcb_polyarea_t *pa;

		for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it)) {
			pcb_coord_t x, y;
			pcb_pline_t *pl;
			int go;

			pl = pcb_poly_contour(&it);
			if (pl != NULL) {
				pcb_coord_t lx, ly, minx, miny, maxx, maxy;
				
				pcb_poly_vect_first(&it, &minx, &miny);
				maxx = minx;
				maxy = miny;
				pcb_poly_vect_peek_prev(&it, &lx, &ly);
				/* find axis aligned contour edges for the 2/3 1/3 rule */
				for(go = pcb_poly_vect_first(&it, &x, &y); go; go = pcb_poly_vect_next(&it, &x, &y)) {
					switch(dir) {
						case PCB_MESH_HORIZONTAL:
							if (y == ly) {
								int sign = (x < lx) ? +1 : -1;
								mesh_add_edge(mesh, dir, y - sign * mesh->dens_obj * 2 / 3);
								mesh_add_edge(mesh, dir, y + sign * mesh->dens_obj * 1 / 3);
							}
							break;
						case PCB_MESH_VERTICAL:
							if (x == lx) {
								int sign = (y < ly) ? +1 : -1;
								mesh_add_edge(mesh, dir, x - sign * mesh->dens_obj * 2 / 3);
								mesh_add_edge(mesh, dir, x + sign * mesh->dens_obj * 1 / 3);
							}
							break;
						default: break;
					}
					lx = x;
					ly = y;
					if (x < minx) minx = x;
					if (y < miny) miny = y;
					if (x > maxx) maxx = x;
					if (y > maxy) maxy = y;
				}
				switch(dir) {
					case PCB_MESH_HORIZONTAL: mesh_add_range(mesh, dir, miny, maxy, mesh->dens_obj); break;
					case PCB_MESH_VERTICAL:   mesh_add_range(mesh, dir, minx, maxx, mesh->dens_obj); break;
					default: break;
				}
				/* Note: holes can be ignored: holes are sorrunded by polygons, the grid is dense over them already */
			}
		}
	}
	return 0;
}

static int cmp_coord(const void *v1, const void *v2)
{
	const pcb_coord_t *c1 = v1, *c2 = v2;
	return *c1 < *c2 ? -1 : +1;
}

static int cmp_range(const void *v1, const void *v2)
{
	const pcb_range_t *c1 = v1, *c2 = v2;
	return c1->begin < c2->begin ? -1 : +1;
}

static int mesh_sort(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;

	qsort(mesh->line[dir].edge.array, vtc0_len(&mesh->line[dir].edge), sizeof(pcb_coord_t), cmp_coord);
	qsort(mesh->line[dir].dens.array, vtr0_len(&mesh->line[dir].dens), sizeof(pcb_range_t), cmp_range);

	/* merge overlapping ranges of the same density */
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens)-1; n++) {
		pcb_range_t *r1 = &mesh->line[dir].dens.array[n], *r2 = &mesh->line[dir].dens.array[n+1];
		if (r1->data[0].c != r2->data[0].c) continue;
		if (r2->begin < r1->end) {
			if (r2->end > r1->end)
				r1->end = r2->end;
			vtr0_remove(&mesh->line[dir].dens, n+1, 1);
		}
	}

	return 0;
}

static void mesh_draw_line(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t at, pcb_coord_t aux1, pcb_coord_t aux2, pcb_coord_t thick)
{
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_line_new(mesh->ui_layer, aux1, at, aux2, at, thick, 0, pcb_no_flags());
	else
		pcb_line_new(mesh->ui_layer, at, aux1, at, aux2, thick, 0, pcb_no_flags());
}

static void mesh_draw_range(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t at1, pcb_coord_t at2, pcb_coord_t aux, pcb_coord_t thick)
{
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_line_new(mesh->ui_layer, aux, at1, aux, at2, thick, 0, pcb_no_flags());
	else
		pcb_line_new(mesh->ui_layer, at1, aux, at2, aux, thick, 0, pcb_no_flags());
}

static void mesh_draw_label(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t aux, const char *label)
{
	aux -= PCB_MM_TO_COORD(0.6);
	if (dir == PCB_MESH_HORIZONTAL)
		pcb_text_new(mesh->ui_layer, pcb_font(PCB, 0, 0), aux, 0, 1, 75, label, pcb_no_flags());
	else
		pcb_text_new(mesh->ui_layer, pcb_font(PCB, 0, 0), 0, aux, 0, 75, label, pcb_no_flags());

}

static int mesh_vis(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	size_t n;

	mesh_draw_label(mesh, dir, PCB_MM_TO_COORD(0.1), "object edge");

	pcb_trace("%s edges:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	for(n = 0; n < vtc0_len(&mesh->line[dir].edge); n++) {
		pcb_trace(" %mm", mesh->line[dir].edge.array[n]);
		mesh_draw_line(mesh, dir, mesh->line[dir].edge.array[n], PCB_MM_TO_COORD(0.1), PCB_MM_TO_COORD(0.5), PCB_MM_TO_COORD(0.1));
		mesh_draw_line(mesh, dir, mesh->line[dir].edge.array[n], 0, PCB->MaxHeight, PCB_MM_TO_COORD(0.03));
	}
	pcb_trace("\n");

	mesh_draw_label(mesh, dir, PCB_MM_TO_COORD(2), "density ranges");

	pcb_trace("%s ranges:\n", dir == PCB_MESH_HORIZONTAL ? "horizontal" : "vertical");
	for(n = 0; n < vtr0_len(&mesh->line[dir].dens); n++) {
		pcb_range_t *r = &mesh->line[dir].dens.array[n];
		pcb_trace(" [%mm..%mm=%mm]", r->begin, r->end, r->data[0].c);
		mesh_draw_range(mesh, dir, r->begin, r->end, PCB_MM_TO_COORD(2)+r->data[0].c/10, PCB_MM_TO_COORD(0.05));
	}
	pcb_trace("\n");

}


int mesh_auto(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	vtc0_truncate(&mesh->line[dir].edge, 0);
	vtr0_truncate(&mesh->line[dir].dens, 0);

	mesh_gen_obj(mesh, dir);
	mesh_sort(mesh, dir);

	if (mesh->ui_layer != NULL)
		mesh_vis(mesh, dir);

	return 0;
}

static const char *mesh_ui_cookie = "mesh ui layer cookie";

const char pcb_acts_mesh[] = "mesh(AllRats|SelectedRats)";
const char pcb_acth_mesh[] = "generate a mesh for simulation";
int pcb_act_mesh(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	mesh.layer = CURRENT;
	mesh.ui_layer = pcb_uilayer_alloc(mesh_ui_cookie, "mesh", "#007733");

	mesh.dens_obj = PCB_MM_TO_COORD(0.2);
	mesh.dens_gap = PCB_MM_TO_COORD(0.8);

	mesh_auto(&mesh, PCB_MESH_VERTICAL);
	return 0;
}
