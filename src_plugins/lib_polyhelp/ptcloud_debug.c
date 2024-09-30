
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

/* Debug draw ptcloud to svg */

#include <librnd/core/safe_fs.h>

/* Draw a single polyline (no recurse to pl->next) */
static void debug_draw_pline1(FILE *F, const rnd_pline_t *pl)
{
	const rnd_vnode_t *v;

	v = pl->head;
	rnd_fprintf(F, "\n	M %.06mm,%.06mm L", v->point[0], v->point[1]);
	do {
		rnd_fprintf(F, "    %.06mm,%.06mm", v->point[0], v->point[1]);
	} while((v = v->next) != pl->head);
	fprintf(F, "z");
}

/* Draw a whole island with contour and holes */
RND_INLINE void debug_draw_pline(FILE *F, const rnd_pline_t *pl, const char *clr, double fill_opacity)
{
	static const rnd_coord_t poly_bloat = RND_MM_TO_COORD(0.02);

	rnd_fprintf(F, "\n<g fill-rule=\"even-odd\" stroke-width=\"%.06mm\" stroke=\"%s\" fill=\"%s\" fill-opacity=\"%.3f\" stroke-opacity=\"%.3f\">\n<path d=\"", poly_bloat, clr, clr, fill_opacity, fill_opacity*2);

	/* draw outer contour */
	debug_draw_pline1(F, pl);

	/* draw holes */
	for(pl = pl->next; pl != NULL; pl = pl->next)
		debug_draw_pline1(F, pl);

	fprintf(F, "\n\"/></g>\n");
}

RND_INLINE void debug_draw_points(FILE *F, pcb_ptcloud_ctx_t *ctx, const char *clr)
{
	long gidx;
	pcb_ptcloud_pt_t *pt;

	for(gidx = 0; gidx < ctx->glen; gidx++) {
		for(pt = gdl_first(&(ctx->grid[gidx])); pt != NULL; pt = pt->link.next) {
			rnd_fprintf(F, " <circle cx=\"%06mm\" cy=\"%06mm\" r=\"0.05\" stroke=\"none\" fill=\"%s\" />\n",
				pt->x, pt->y, clr);
		}
	}
}

RND_INLINE void ptcloud_debug_draw(pcb_ptcloud_ctx_t *ctx, const char *fn)
{
	FILE *F = rnd_fopen(NULL, fn, "w");

	if (F == NULL) {
		fprintf(stderr, "ptcloud_debug_draw(): failed to open '%s' for write\n", fn);
		return;
	}


	fprintf(F, "<?xml version=\"1.0\"?>\n");
	rnd_fprintf(F, "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"1000\" height=\"1000\" viewBox=\"%.06mm %.06mm %.06mm %.06mm\">\n", ctx->pl->xmin, ctx->pl->ymin, ctx->pl->xmax, ctx->pl->ymax);

	debug_draw_pline(F, ctx->pl, "#FF0000", 0.3);
	debug_draw_points(F, ctx, "#000000");

	fprintf(F, "</svg>\n");
	fclose(F);

}

