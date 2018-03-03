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
#include "board.h"
#include "data.h"

static pcb_mesh_t mesh;

static void mesh_add_edge(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t crd)
{
pcb_printf("EDGE: %mm\n", crd);
	vtc0_append(&mesh->line[dir].edge, crd);
}

static void mesh_add_range(pcb_mesh_t *mesh, pcb_mesh_dir_t dir, pcb_coord_t c1, pcb_coord_t c2, pcb_coord_t dens)
{
	pcb_range_t *r = vtr0_alloc_append(&mesh->line[dir].dens, 1);
	r->begin = c1;
	r->end = c2;
	r->data[0].c = dens;

pcb_printf("range: %mm..%mm\n", c1, c2);
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
	pcb_poly_t *poly;
	gdl_iterator_t it;
	

	linelist_foreach(&mesh->layer->Line, &it, line) {
		int aligned = (line->Point1.Y == line->Point2.Y) || (line->Point1.X == line->Point2.X);
		switch(dir) {
			case PCB_MESH_HORIZONTAL: mesh_add_obj(mesh, dir, line->Point1.Y - line->Thickness/2, line->Point2.Y + line->Thickness/2, aligned); break;
			case PCB_MESH_VERTICAL: mesh_add_obj(mesh, dir, line->Point1.X - line->Thickness/2, line->Point2.X + line->Thickness/2, aligned); break;
			default: break;
		}
	}

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

int mesh_auto(pcb_mesh_t *mesh, pcb_mesh_dir_t dir)
{
	vtc0_truncate(&mesh->line[dir].edge, 0);
	vtr0_truncate(&mesh->line[dir].dens, 0);
	mesh_gen_obj(mesh, dir);

	return 0;
}

const char pcb_acts_mesh[] = "mesh(AllRats|SelectedRats)";
const char pcb_acth_mesh[] = "generate a mesh for simulation";
int pcb_act_mesh(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	mesh.layer = CURRENT;
	mesh.dens_obj = PCB_MM_TO_COORD(0.2);
	mesh.dens_gap = PCB_MM_TO_COORD(0.8);
	mesh_auto(&mesh, PCB_MESH_VERTICAL);
	return 0;
}
