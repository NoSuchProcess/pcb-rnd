/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "thermal.h"

#include <librnd/core/compat_misc.h>
#include <librnd/core/rnd_printf.h>
#include "data.h"
#include "draw.h"
#include "undo.h"
#include "conf_core.h"
#include "obj_common.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_pinvia_therm.h"
#include "polygon.h"
#include "funchash_core.h"

static const char core_thermal_cookie[] = "core/thermal.c";

rnd_cardinal_t pcb_themal_style_new2old(unsigned char t)
{
	switch(t) {
		case 0: return 0;
		case PCB_THERMAL_ON | PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL: return 1;
		case PCB_THERMAL_ON | PCB_THERMAL_SHARP: return 2;
		case PCB_THERMAL_ON | PCB_THERMAL_SOLID: return 3;
		case PCB_THERMAL_ON | PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL: return 4;
		case PCB_THERMAL_ON | PCB_THERMAL_ROUND: return 5;
	}
	return 0;
}

unsigned char pcb_themal_style_old2new(rnd_cardinal_t t)
{
	switch(t) {
		case 0: return 0;
		case 1: return PCB_THERMAL_ON | PCB_THERMAL_SHARP | PCB_THERMAL_DIAGONAL;
		case 2: return PCB_THERMAL_ON | PCB_THERMAL_SHARP;
		case 3: return PCB_THERMAL_ON | PCB_THERMAL_SOLID;
		case 4: return PCB_THERMAL_ON | PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL;
		case 5: return PCB_THERMAL_ON | PCB_THERMAL_ROUND;
	}
	return 0;
}

pcb_thermal_t pcb_thermal_str2bits(const char *str)
{
	/* shape */
	if (strcmp(str, "noshape") == 0) return PCB_THERMAL_NOSHAPE;
	if (strcmp(str, "round") == 0)   return PCB_THERMAL_ROUND;
	if (strcmp(str, "sharp") == 0)   return PCB_THERMAL_SHARP;
	if (strcmp(str, "solid") == 0)   return PCB_THERMAL_SOLID;

	/* orientation */
	if (strcmp(str, "diag") == 0)   return PCB_THERMAL_DIAGONAL;

	if (strcmp(str, "on") == 0)     return PCB_THERMAL_ON;

	return 0;
}

const char *pcb_thermal_bits2str(pcb_thermal_t *bit)
{
	if ((*bit) & PCB_THERMAL_ON) {
		*bit &= ~PCB_THERMAL_ON;
		return "on";
	}
	if ((*bit) & PCB_THERMAL_DIAGONAL) {
		*bit &= ~PCB_THERMAL_DIAGONAL;
		return "diag";
	}
	if ((*bit) & 3) {
		int shape = (*bit) & 3;
		*bit &= ~3;
		switch(shape) {
			case PCB_THERMAL_NOSHAPE: return "noshape";
			case PCB_THERMAL_ROUND: return "round";
			case PCB_THERMAL_SHARP: return "sharp";
			case PCB_THERMAL_SOLID: return "solid";
			default: return NULL;
		}
	}
	return NULL;
}


/* generate a round-cap line polygon */
static rnd_polyarea_t *pa_line_at(double x1, double y1, double x2, double y2, rnd_coord_t clr, rnd_bool square)
{
	pcb_line_t ltmp;

	if (square)
		ltmp.Flags = pcb_flag_make(PCB_FLAG_SQUARE);
	else
		ltmp.Flags = pcb_no_flags();
	ltmp.Point1.X = rnd_round(x1); ltmp.Point1.Y = rnd_round(y1);
	ltmp.Point2.X = rnd_round(x2); ltmp.Point2.Y = rnd_round(y2);
	return pcb_poly_from_pcb_line(&ltmp, clr);
}

/* generate a round-cap arc polygon knowing the center and endpoints */
static rnd_polyarea_t *pa_arc_at(double cx, double cy, double r, double e1x, double e1y, double e2x, double e2y, rnd_coord_t clr, double max_span_angle)
{
	double sa, ea, da;
	pcb_arc_t atmp;

	sa = atan2(-(e1y - cy), e1x - cx) * RND_RAD_TO_DEG + 180.0;
	ea = atan2(-(e2y - cy), e2x - cx) * RND_RAD_TO_DEG + 180.0;

/*	rnd_trace("sa=%f ea=%f diff=%f\n", sa, ea, ea-sa);*/

	atmp.Flags = pcb_no_flags();
	atmp.X = rnd_round(cx);
	atmp.Y = rnd_round(cy);

	da = ea-sa;
	if ((da < max_span_angle) && (da > -max_span_angle)) {
		atmp.StartAngle = sa;
		atmp.Delta = ea-sa;
	}
	else {
		if (ea < sa) {
			double tmp = ea;
			ea = sa;
			sa = tmp;
		}
		atmp.StartAngle = ea;
		atmp.Delta = 360-ea+sa;
	}
	atmp.Width = atmp.Height = r;
	return pcb_poly_from_pcb_arc(&atmp, clr);
}

rnd_polyarea_t *pcb_thermal_area_line(pcb_board_t *pcb, pcb_line_t *line, rnd_layer_id_t lid, pcb_poly_t *in_poly)
{
	rnd_polyarea_t *pa, *pb, *pc;
	double dx, dy, len, vx, vy, nx, ny, clr, clrth, x1, y1, x2, y2, mx, my;
	rnd_coord_t th, lclr;

	if ((line->Point1.X == line->Point2.X) && (line->Point1.Y == line->Point2.Y)) {
		/* conrer case zero-long line is a circle: do the same as for vias */
TODO("thermal TODO")
		abort();
	}

	x1 = line->Point1.X;
	y1 = line->Point1.Y;
	x2 = line->Point2.X;
	y2 = line->Point2.Y;
	mx = (x1+x2)/2.0;
	my = (y1+y2)/2.0;
	dx = x1 - x2;
	dy = y1 - y2;

	len = sqrt(dx*dx + dy*dy);
	vx = dx / len;
	vy = dy / len;
	nx = -vy;
	ny = vx;

	lclr = pcb_obj_clearance_o05(line, in_poly);
	clr = lclr;
	clrth = (lclr + line->Thickness) / 2;

	assert(line->thermal & PCB_THERMAL_ON); /* caller should have checked this */
	switch(line->thermal & 3) {
		case PCB_THERMAL_NOSHAPE:
		case PCB_THERMAL_SOLID: return 0;

		case PCB_THERMAL_ROUND:
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, line)) {

				if (line->thermal & PCB_THERMAL_DIAGONAL) {
					/* side clear lines */
					pa = pa_line_at(
						x1 - clrth * nx + clrth * vx - clr*vx, y1 - clrth * ny + clrth * vy - clr*vy,
						x2 - clrth * nx - clrth * vx + clr*vx, y2 - clrth * ny - clrth * vy + clr*vy,
						clr, rnd_false);
					pb = pa_line_at(
						x1 + clrth * nx + clrth * vx - clr*vx, y1 + clrth * ny + clrth * vy - clr*vy,
						x2 + clrth * nx - clrth * vx + clr*vx, y2 + clrth * ny - clrth * vy + clr*vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					/* cross clear lines */
					pa = pc; pc = NULL;
					pb = pa_line_at(
						x1 - clrth * nx + clrth * vx + clr*nx, y1 - clrth * ny + clrth * vy + clr*ny,
						x1 + clrth * nx + clrth * vx - clr*nx, y1 + clrth * ny + clrth * vy - clr*ny,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						x2 - clrth * nx - clrth * vx + clr*nx, y2 - clrth * ny - clrth * vy + clr*ny,
						x2 + clrth * nx - clrth * vx - clr*nx, y2 + clrth * ny - clrth * vy - clr*ny,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);
				}
				else {
					/* side clear lines */
					pa = pa_line_at(
						x1 - clrth * nx + clrth * vx, y1 - clrth * ny + clrth * vy,
						mx - clrth * nx + clr * vx, my - clrth * ny + clr * vy,
						clr, rnd_false);
					pb = pa_line_at(
						x1 + clrth * nx + clrth * vx, y1 + clrth * ny + clrth * vy,
						mx + clrth * nx + clr * vx, my + clrth * ny + clr * vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						x2 - clrth * nx - clrth * vx, y2 - clrth * ny - clrth * vy,
						mx - clrth * nx - clr * vx, my - clrth * ny - clr * vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						x2 + clrth * nx - clrth * vx, y2 + clrth * ny - clrth * vy,
						mx + clrth * nx - clr * vx, my + clrth * ny - clr * vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					/* cross clear lines */
					pa = pc; pc = NULL;
					pb = pa_line_at(
						x1 - clrth * nx + clrth * vx, y1 - clrth * ny + clrth * vy,
						x1 - clr * nx + clrth * vx, y1 - clr * ny + clrth * vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						x1 + clrth * nx + clrth * vx, y1 + clrth * ny + clrth * vy,
						x1 + clr * nx + clrth * vx, y1 + clr * ny + clrth * vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						x2 - clrth * nx - clrth * vx, y2 - clrth * ny - clrth * vy,
						x2 - clr * nx - clrth * vx, y2 - clr * ny - clrth * vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						x2 + clrth * nx - clrth * vx, y2 + clrth * ny - clrth * vy,
						x2 + clr * nx - clrth * vx, y2 + clr * ny - clrth * vy,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);
				}
			}
			else {
				/* round cap line: arcs! */
				if (line->thermal & PCB_THERMAL_DIAGONAL) {
					/* side clear lines */
					pa = pa_line_at(
						x1 - clrth * nx - clr * vx * 0.75, y1 - clrth * ny - clr * vy * 0.75,
						x2 - clrth * nx + clr * vx * 0.75, y2 - clrth * ny + clr * vy * 0.75,
						clr, rnd_false);
					pb = pa_line_at(
						x1 + clrth * nx - clr * vx * 0.75, y1 + clrth * ny - clr * vy * 0.75,
						x2 + clrth * nx + clr * vx * 0.75, y2 + clrth * ny + clr * vy * 0.75,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					/* x1;y1 cap arc */
					pa = pc; pc = NULL;
					pb = pa_arc_at(x1, y1, clrth,
						x1 - clrth * nx + clr * vx * 2.0, y1 - clrth * ny + clr * vy * 2.0,
						x1 + clrth * nx + clr * vx * 2.0, y1 + clrth * ny + clr * vy * 2.0,
						clr, 180.0);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					/* x2;y2 cap arc */
					pa = pc; pc = NULL;
					pb = pa_arc_at(x2, y2, clrth,
						x2 - clrth * nx - clr * vx * 2.0, y2 - clrth * ny - clr * vy * 2.0,
						x2 + clrth * nx - clr * vx * 2.0, y2 + clrth * ny - clr * vy * 2.0,
						clr, 180.0);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);
				}
				else { /* non-diagonal */
					/* split side lines */
					pa = pa_line_at(
						x1 - clrth * nx - clr * vx * 0.00, y1 - clrth * ny - clr * vy * 0.00,
						mx - clrth * nx + clr * vx * 1.00, my - clrth * ny + clr * vy * 1.00,
						clr, rnd_false);
					pb = pa_line_at(
						x1 + clrth * nx - clr * vx * 0.00, y1 + clrth * ny - clr * vy * 0.00,
						mx + clrth * nx + clr * vx * 1.00, my + clrth * ny + clr * vy * 1.00,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						mx - clrth * nx - clr * vx * 1.00, my - clrth * ny - clr * vy * 1.00,
						x2 - clrth * nx + clr * vx * 0.00, y2 - clrth * ny + clr * vy * 0.00,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_line_at(
						mx + clrth * nx - clr * vx * 1.00, my + clrth * ny - clr * vy * 1.00,
						x2 + clrth * nx + clr * vx * 0.00, y2 + clrth * ny + clr * vy * 0.00,
						clr, rnd_false);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					/* split round cap, x1;y1 */
					pa = pc; pc = NULL;
					pb = pa_arc_at(x1, y1, clrth,
						x1 - clrth * nx, y1 - clrth * ny,
						x1 + clrth * vx - clr * nx, y1 + clrth * vy - clr * ny,
						clr, 180.0);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_arc_at(x1, y1, clrth,
						x1 + clrth * nx, y1 + clrth * ny,
						x1 + clrth * vx + clr * nx, y1 + clrth * vy + clr * ny,
						clr, 180.0);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					/* split round cap, x2;y2 */
					pa = pc; pc = NULL;
					pb = pa_arc_at(x2, y2, clrth,
						x2 - clrth * nx, y2 - clrth * ny,
						x2 - clrth * vx - clr * nx, y2 - clrth * vy - clr * ny,
						clr, 180.0);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);

					pa = pc; pc = NULL;
					pb = pa_arc_at(x2, y2, clrth,
						x2 + clrth * nx, y2 + clrth * ny,
						x2 - clrth * vx + clr * nx, y2 - clrth * vy + clr * ny,
						clr, 180.0);
					rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_UNITE);
				}
			}
			return pc;
		case PCB_THERMAL_SHARP:
			pa = pcb_poly_from_pcb_line(line, line->Thickness + pcb_obj_clearance_p2(line, in_poly));
			th = line->Thickness/2 < clr ? line->Thickness/2 : clr;
			clrth *= 2;
			if (line->thermal & PCB_THERMAL_DIAGONAL) {
				/* x1;y1 V-shape */
				pb = pa_line_at(x1, y1, x1-nx*clrth+vx*clrth, y1-ny*clrth+vy*clrth, th, rnd_false);
				rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_SUB);

				pa = pc; pc = NULL;
				pb = pa_line_at(x1, y1, x1+nx*clrth+vx*clrth, y1+ny*clrth+vy*clrth, th, rnd_false);
				rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_SUB);

				/* x2;y2 V-shape */
				pa = pc; pc = NULL;
				pb = pa_line_at(x2, y2, x2-nx*clrth-vx*clrth, y2-ny*clrth-vy*clrth, th, rnd_false);
				rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_SUB);

				pa = pc; pc = NULL;
				pb = pa_line_at(x2, y2, x2+nx*clrth-vx*clrth, y2+ny*clrth-vy*clrth, th, rnd_false);
				rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_SUB);
			}
			else {
				/* perpendicular */
				pb = pa_line_at(mx-nx*clrth, my-ny*clrth, mx+nx*clrth, my+ny*clrth, th, rnd_true);
				rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_SUB);

				/* straight */
				pa = pc; pc = NULL;
				pb = pa_line_at(x1+vx*clrth, y1+vy*clrth, x2-vx*clrth, y2-vy*clrth, th, rnd_true);
				rnd_polyarea_boolean_free(pa, pb, &pc, RND_PBO_SUB);
			}
			return pc;
	}
	return NULL;
}

/* combine a base poly-area into the result area (pres) */
static void polytherm_base(rnd_polyarea_t **pres, const rnd_polyarea_t *src)
{
	rnd_polyarea_t *p;

	if (*pres != NULL) {
		rnd_polyarea_boolean(src, *pres, &p, RND_PBO_UNITE);
		rnd_polyarea_free(pres);
		*pres = p;
	}
	else
		rnd_polyarea_copy0(pres, src);
}

#define CONG_MAX 256

static int cong_map(char *cong, pcb_poly_it_t *it, rnd_coord_t clr)
{
	int n, go, first = 1;
	rnd_coord_t cx, cy;
	double cl2 = (double)clr * (double)clr * 1.5;
	double px, py, x, y;

	for(n = 0, go = pcb_poly_vect_first(it, &cx, &cy); go; go = pcb_poly_vect_next(it, &cx, &cy), n++) {
		x = cx; y = cy;
		if (first) {
/*rnd_trace("prev %mm;%mm\n", cx, cy);*/
			pcb_poly_vect_peek_prev(it, &cx, &cy);
/*rnd_trace("     %mm;%mm\n", cx, cy);*/
			px = cx; py = cy;
			first = 0;
		}
		if (n >= CONG_MAX-1) {
			n++;
			break;
		}
		cong[n] = (rnd_distance2(x, y, px, py) < cl2);
		px = x; py = y;
	}
	return n;
}

/* combine a round clearance line set into pres; "it" is an iterator
   already initialized to a polyarea contour */
static void polytherm_round(rnd_polyarea_t **pres, pcb_poly_it_t *it, rnd_coord_t clr, rnd_bool is_diag, double tune)
{
	rnd_polyarea_t *ptmp, *p;
	double fact = 0.5, fact_ortho=0.75;
	rnd_coord_t cx, cy;
	double px, py, x, y, dx, dy, vx, vy, nx, ny, mx, my, len;
	int n, go, first = 1;
	char cong[CONG_MAX];

	clr -= 2;
	if (tune > 0) {
		fact *= tune;
		fact_ortho *= tune;
	}

	cong_map(cong, it, clr);

	/* iterate over the vectors of the contour */
	for(n = 0, go = pcb_poly_vect_first(it, &cx, &cy); go; go = pcb_poly_vect_next(it, &cx, &cy), n++) {
		x = cx; y = cy;
		if (first) {
			pcb_poly_vect_peek_prev(it, &cx, &cy);
			px = cx; py = cy;

			first = 0;
		}

/*rnd_trace("[%d] %d %mm;%mm\n", n, cong[n], (rnd_coord_t)x, (rnd_coord_t)y);*/


		dx = x - px;
		dy = y - py;
		mx = (x+px)/2.0;
		my = (y+py)/2.0;

		len = sqrt(dx*dx + dy*dy);
		vx = dx / len;
		vy = dy / len;

		nx = -vy;
		ny = vx;

		/* skip points too dense */
		if ((n >= CONG_MAX) || (cong[n])) {
			if (!is_diag) {
				/* have to draw a short clearance for the small segments */
				ptmp = pa_line_at(x - nx * clr/2, y - ny * clr/2, px - nx * clr/2, py - ny * clr/2, clr+4, rnd_false);

				rnd_polyarea_boolean(ptmp, *pres, &p, RND_PBO_UNITE);
				rnd_polyarea_free(pres);
				rnd_polyarea_free(&ptmp);
				*pres = p;
			}
			px = x; py = y;
			continue;
		}

		/* cheat: clr-2+4 to guarantee some overlap with the poly cutout */
		if (is_diag) {
			/* one line per edge, slightly shorter than the edge */
			ptmp = pa_line_at(x - vx * clr * fact - nx * clr/2, y - vy * clr * fact - ny * clr/2, px + vx * clr *fact - nx * clr/2, py + vy * clr * fact - ny * clr/2, clr+4, rnd_false);

			rnd_polyarea_boolean(ptmp, *pres, &p, RND_PBO_UNITE);
			rnd_polyarea_free(pres);
			rnd_polyarea_free(&ptmp);
			*pres = p;
		}
		else {
			/* two half lines per edge */
			ptmp = pa_line_at(x - nx * clr/2 , y - ny * clr/2, mx + vx * clr * fact_ortho - nx * clr/2, my + vy * clr * fact_ortho - ny * clr/2, clr+4, rnd_false);
			rnd_polyarea_boolean(ptmp, *pres, &p, RND_PBO_UNITE);
			rnd_polyarea_free(pres);
			rnd_polyarea_free(&ptmp);
			*pres = p;

			ptmp = pa_line_at(px - nx * clr/2, py - ny * clr/2, mx - vx * clr * fact_ortho - nx * clr/2, my - vy * clr * fact_ortho - ny * clr/2, clr+4, rnd_false);
			rnd_polyarea_boolean(ptmp, *pres, &p, RND_PBO_UNITE);
			rnd_polyarea_free(pres);
			rnd_polyarea_free(&ptmp);
			*pres = p;

			/* optical tuning: make sure the clearance is large enough around corners
			   even if lines didn't meet - just throw in a big circle */
			ptmp = rnd_poly_from_circle(x, y, clr);
			rnd_polyarea_boolean(ptmp, *pres, &p, RND_PBO_UNITE);
			rnd_polyarea_free(pres);
			rnd_polyarea_free(&ptmp);
			*pres = p;
		}
		px = x;
		py = y;
	}
}

static void polytherm_sharp(rnd_polyarea_t **pres, pcb_poly_it_t *it, rnd_coord_t clr, rnd_bool is_diag)
{
	rnd_polyarea_t *ptmp, *p;
	rnd_coord_t cx, cy, x2c, y2c;
	double px, py, x, y, dx, dy, vx, vy, nx, ny, mx, my, len, x2, y2, vx2, vy2, len2, dx2, dy2;
	int n, go, first = 1;
	char cong[CONG_MAX], cong2[CONG_MAX];

	if (is_diag) {
		int start = -1, v = cong_map(cong2, it, clr);
		memset(cong, 1, sizeof(cong));

		/* in case of sharp-diag, draw the bridge in the middle of each congestion;
		   run *2 and do module v so congestions spanning over the end are no special */
		for(n = 0; n < v*2; n++) {
			if (cong2[n % v] == 0) {
				if (start >= 0)
					cong[((start+n)/2) % v] = 0;
				start = n;
			}
		}
	}
	else {
		/* normal congestion logic: use long edges only */
		cong_map(cong, it, clr);
	}

	/* iterate over the vectors of the contour */
	for(n = 0, go = pcb_poly_vect_first(it, &cx, &cy); go; go = pcb_poly_vect_next(it, &cx, &cy), n++) {
		x = cx; y = cy;
		if (first) {
			pcb_poly_vect_peek_prev(it, &cx, &cy);
			px = cx; py = cy;
			first = 0;
		}

/*rnd_trace("[%d] %d %mm;%mm\n", n, cong[n], (rnd_coord_t)x, (rnd_coord_t)y);*/

		/* skip points too dense */
		if ((n >= CONG_MAX) || (cong[n])) {
			px = x; py = y;
			continue;
		}


		dx = x - px;
		dy = y - py;

		len = sqrt(dx*dx + dy*dy);
		vx = dx / len;
		vy = dy / len;

		if (is_diag) {
			pcb_poly_vect_peek_next(it, &x2c, &y2c);
			x2 = x2c; y2 = y2c;
			dx2 = x - x2;
			dy2 = y - y2;
			len2 = sqrt(dx2*dx2 + dy2*dy2);
			vx2 = dx2 / len2;
			vy2 = dy2 / len2;

			nx = -(vy2 - vy);
			ny = vx2 - vx;

			/* line from each corner at the average angle of the two edges from the corner */
			ptmp = pa_line_at(x-nx*clr*0.2, y-ny*clr*0.2, x + nx*clr*4, y + ny*clr*4, clr/2, rnd_false);
			rnd_polyarea_boolean(*pres, ptmp, &p, RND_PBO_SUB);
			rnd_polyarea_free(pres);
			rnd_polyarea_free(&ptmp);
			*pres = p;
		}
		else {
			nx = -vy;
			ny = vx;
			mx = (x+px)/2.0;
			my = (y+py)/2.0;

			/* perpendicular line from the middle of each edge */
			ptmp = pa_line_at(mx, my, mx - nx*clr, my - ny*clr, clr/2, rnd_true);
			rnd_polyarea_boolean(*pres, ptmp, &p, RND_PBO_SUB);
			rnd_polyarea_free(pres);
			rnd_polyarea_free(&ptmp);
			*pres = p;
		}
		px = x;
		py = y;
	}
}

/* generate round thermal around a polyarea specified by the iterator */
static void pcb_thermal_area_pa_round(rnd_polyarea_t **pres, pcb_poly_it_t *it, rnd_coord_t clr, rnd_bool_t is_diag)
{
	rnd_pline_t *pl;

/* cut out the poly so terminals will be displayed proerply */
	polytherm_base(pres, it->pa);

	/* generate the clear-lines */
	pl = pcb_poly_contour(it);
	if (pl != NULL)
		polytherm_round(pres, it, clr, is_diag, conf_core.design.thermal.poly_scale);
}

/* generate sharp thermal around a polyarea specified by the iterator */
static void pcb_thermal_area_pa_sharp(rnd_polyarea_t **pres, pcb_poly_it_t *it, rnd_coord_t clr, rnd_bool_t is_diag)
{
	rnd_pline_t *pl;

	/* add the usual clearance glory around the polygon */
	pcb_poly_pa_clearance_construct(pres, it, clr);

	pl = pcb_poly_contour(it);
	if (pl != NULL)
		polytherm_sharp(pres, it, clr, is_diag);

	/* trim internal stubs */
	polytherm_base(pres, it->pa);
}

rnd_polyarea_t *pcb_thermal_area_poly(pcb_board_t *pcb, pcb_poly_t *poly, rnd_layer_id_t lid, pcb_poly_t *in_poly)
{
	rnd_polyarea_t *pa, *pres = NULL;
	rnd_coord_t clr = pcb_obj_clearance_o05(poly, in_poly);
	pcb_poly_it_t it;

	assert(poly->thermal & PCB_THERMAL_ON); /* caller should have checked this */
	switch(poly->thermal & 3) {
		case PCB_THERMAL_NOSHAPE:
		case PCB_THERMAL_SOLID: return NULL;

		case PCB_THERMAL_ROUND:
			for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it))
				pcb_thermal_area_pa_round(&pres, &it, clr, (poly->thermal & PCB_THERMAL_DIAGONAL));
			return pres;

		case PCB_THERMAL_SHARP:
			for(pa = pcb_poly_island_first(poly, &it); pa != NULL; pa = pcb_poly_island_next(&it))
				pcb_thermal_area_pa_sharp(&pres, &it, clr, (poly->thermal & PCB_THERMAL_DIAGONAL));
			return pres;
	}

	return NULL;
}

/* Generate a clearance around a padstack shape, with no thermal */
static rnd_polyarea_t *pcb_thermal_area_pstk_nothermal(pcb_board_t *pcb, pcb_pstk_t *ps, rnd_layer_id_t lid, pcb_pstk_shape_t *shp, rnd_coord_t clearance)
{
	pcb_poly_it_t it;
	rnd_polyarea_t *pres = NULL;
	pcb_pstk_shape_t tmpshp;

	retry:;
	switch(shp->shape) {
		case PCB_PSSH_HSHADOW:
			shp = pcb_pstk_hshadow_shape(ps, shp, &tmpshp);
			if (shp == NULL)
				return NULL;
			goto retry;
		case PCB_PSSH_CIRC:
			return rnd_poly_from_circle(ps->x + shp->data.circ.x, ps->y + shp->data.circ.y, shp->data.circ.dia/2 + clearance);
		case PCB_PSSH_LINE:
			return pa_line_at(ps->x + shp->data.line.x1, ps->y + shp->data.line.y1, ps->x + shp->data.line.x2, ps->y + shp->data.line.y2, shp->data.line.thickness + clearance*2, shp->data.line.square);
		case PCB_PSSH_POLY:
			if (shp->data.poly.pa == NULL)
				pcb_pstk_shape_update_pa(&shp->data.poly);
			if (shp->data.poly.pa == NULL)
				return NULL;
			pcb_poly_iterate_polyarea(shp->data.poly.pa, &it);
			pcb_poly_pa_clearance_construct(&pres, &it, clearance);
			rnd_polyarea_move(pres, ps->x, ps->y);
			return pres;
	}
	return NULL;
}

rnd_polyarea_t *pcb_thermal_area_pstk(pcb_board_t *pcb, pcb_pstk_t *ps, rnd_layer_id_t lid, pcb_poly_t *in_poly)
{
	unsigned char thr;
	pcb_pstk_shape_t *shp, tmpshp;
	rnd_polyarea_t *pres = NULL;
	pcb_layer_t *layer;
	rnd_coord_t clearance, gclearance = ps->Clearance; /* this is a legal direct access to the Clearance field, poly->enforce_clearance is applied later */

	/* thermal needs a stackup - only a PCB has a stackup, buffers don't */
	if (pcb == NULL)
		return NULL;

	/* if we have no clearance, there's no reason to do anything;
	   ps->Clearance == 0 doesn't mean no clearance because of the per shape clearances */
	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, ps))
		return NULL;

	layer = pcb_get_layer(pcb->Data, lid);

	/* retrieve shape; assume 0 (no shape) for layers not named */
	if (lid < ps->thermals.used)
		thr = ps->thermals.shape[lid];
	else
		thr = 0;

	shp = pcb_pstk_shape_at_(pcb, ps, layer, 1);
	if (shp == NULL) {
		rnd_layergrp_id_t gid;
		pcb_layergrp_t *grp;
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
		if (proto->hdia < 0)
			return NULL;

		/* corner case: subtracting a hole from a mech/drill poly (e.g. gcode) */
		gid = pcb_layer_get_group_(layer);
		grp = pcb_get_layergrp(pcb, gid);
		if (PCB_LAYER_IS_DRILL(grp->ltype, grp->purpi) || ((grp->ltype & PCB_LYT_MECH) && (grp->purpi == (proto->hplated ? F_proute : F_uroute)))) {
			thr = 0;
			shp = &tmpshp;
			shp->shape = PCB_PSSH_CIRC;
			shp->data.circ.x = 0;
			shp->data.circ.y = 0;
			shp->data.circ.dia = proto->hdia;
		}
		else
			return NULL;
	}

	if (gclearance <= 0)
		clearance = shp->clearance/2;
	else
		clearance = gclearance;

	/* apply polygon enforced clearance on top of the per shape clearance */
	clearance = RND_MAX(clearance, in_poly->enforce_clearance);

	if (clearance <= 0) {
		rnd_layergrp_id_t gid = pcb_layer_get_group_(layer);
		pcb_layergrp_t *grp = pcb_get_layergrp(pcb, gid);
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
		if (grp == NULL) return NULL;
		if (PCB_LAYER_IS_DRILL(grp->ltype, grp->purpi) || ((grp->ltype & PCB_LYT_MECH) && (grp->purpi == (proto->hplated ? F_proute : F_uroute)))) {
			/* shape on a slot layer - do it, but without termals! */
			thr = 0;
		}
		else
			return NULL;
	}

	if (!(thr & PCB_THERMAL_ON))
		return pcb_thermal_area_pstk_nothermal(pcb, ps, lid, shp, clearance);

	switch(thr & 3) {
		case PCB_THERMAL_NOSHAPE:
			tmpshp.shape = PCB_PSSH_HSHADOW;
			tmpshp.clearance = shp->clearance;
			return pcb_thermal_area_pstk_nothermal(pcb, ps, lid, &tmpshp, clearance);
		case PCB_THERMAL_SOLID: return NULL;
		case PCB_THERMAL_ROUND:
		case PCB_THERMAL_SHARP:
			retry:;
			switch(shp->shape) {
				case PCB_PSSH_HSHADOW:
					shp = pcb_pstk_hshadow_shape(ps, shp, &tmpshp);
					if (shp == NULL)
						return NULL;
					goto retry;
				case PCB_PSSH_CIRC:
					return ThermPoly_(pcb, ps->x + shp->data.circ.x, ps->y + shp->data.circ.y, shp->data.circ.dia, clearance*2, pcb_themal_style_new2old(thr));
				case PCB_PSSH_LINE:
				{
					pcb_line_t ltmp;
					if (shp->data.line.square)
						ltmp.Flags = pcb_flag_make(PCB_FLAG_SQUARE);
					else
						ltmp.Flags = pcb_no_flags();
					ltmp.Point1.X = ps->x + shp->data.line.x1;
					ltmp.Point1.Y = ps->y + shp->data.line.y1;
					ltmp.Point2.X = ps->x + shp->data.line.x2;
					ltmp.Point2.Y = ps->y + shp->data.line.y2;
					ltmp.Thickness = shp->data.line.thickness;
					ltmp.Clearance = clearance*2;
					ltmp.thermal = thr;
					pres = pcb_thermal_area_line(pcb, &ltmp, lid, in_poly);
				}
				return pres;

				case PCB_PSSH_POLY:
				{
					pcb_poly_it_t it;
					if (shp->data.poly.pa == NULL)
						pcb_pstk_shape_update_pa(&shp->data.poly);
					if (shp->data.poly.pa == NULL)
						return NULL;
					pcb_poly_iterate_polyarea(shp->data.poly.pa, &it);
					if (thr & PCB_THERMAL_ROUND)
						pcb_thermal_area_pa_round(&pres, &it, clearance, (thr & PCB_THERMAL_DIAGONAL));
					else
						pcb_thermal_area_pa_sharp(&pres, &it, clearance, (thr & PCB_THERMAL_DIAGONAL));

					if (pres != NULL)
						rnd_polyarea_move(pres, ps->x, ps->y);
				}
				return pres;
			}

	}

	return NULL;
}

rnd_polyarea_t *pcb_thermal_area(pcb_board_t *pcb, pcb_any_obj_t *obj, rnd_layer_id_t lid, pcb_poly_t *in_poly)
{
	switch(obj->type) {

		case PCB_OBJ_LINE:
			return pcb_thermal_area_line(pcb, (pcb_line_t *)obj, lid, in_poly);

		case PCB_OBJ_POLY:
			return pcb_thermal_area_poly(pcb, (pcb_poly_t *)obj, lid, in_poly);

		case PCB_OBJ_PSTK:
			return pcb_thermal_area_pstk(pcb, (pcb_pstk_t *)obj, lid, in_poly);

		case PCB_OBJ_ARC:
			break;

		default: break;
	}

	return NULL;
}


/*** undoable thermal change ***/
typedef struct {
	pcb_board_t *pcb;
	pcb_any_obj_t *obj;
	unsigned char shape;
} undo_anyobj_thermal_t;

static int undo_anyobj_thermal_swap(void *udata)
{
	undo_anyobj_thermal_t *t = udata;
	unsigned char old, *th = pcb_obj_common_get_thermal(t->obj, 0, 1);

	if (th != NULL) {
		pcb_poly_restore_to_poly(t->pcb->Data, t->obj->type, t->obj->parent.layer, t->obj);

		old = *th;
		*th = t->shape;
		t->shape = old;

		pcb_poly_clear_from_poly(t->pcb->Data, t->obj->type, t->obj->parent.layer, t->obj);
		pcb_draw_invalidate(t->obj);
	}
	return 0;
}

static void undo_anyobj_thermal_print(void *udata, char *dst, size_t dst_len)
{
	undo_anyobj_thermal_t *t = udata;
	rnd_snprintf(dst, dst_len, "anyobj_thermal: #%ld %d", t->obj->ID, t->shape);
}

static const uundo_oper_t undo_anyobj_thermal = {
	core_thermal_cookie,
	NULL, /* free */
	undo_anyobj_thermal_swap,
	undo_anyobj_thermal_swap,
	undo_anyobj_thermal_print
};


int pcb_anyobj_set_thermal(pcb_any_obj_t *obj, unsigned char shape, int undoable)
{
	undo_anyobj_thermal_t *t;
	pcb_board_t *pcb = NULL;

	if (obj->type == PCB_OBJ_PSTK)
		return -1; /* needs a layer */

	if (undoable) {
		assert(obj->parent_type == PCB_PARENT_LAYER);
		pcb = pcb_data_get_top(obj->parent.layer->parent.data);
	}

	if (!undoable || (pcb == NULL)) {
		unsigned char *th = pcb_obj_common_get_thermal(obj, 0, 1);
		if (th != NULL)
			*th = shape;
		return 0;
	}

	t = pcb_undo_alloc(pcb, &undo_anyobj_thermal, sizeof(undo_anyobj_thermal_t));
	t->pcb = pcb;
	t->obj = obj;
	t->shape = shape;
	undo_anyobj_thermal_swap(t);
	return 0;
}

void *pcb_anyop_change_thermal(pcb_opctx_t *ctx, pcb_any_obj_t *obj)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, obj))
		return NULL;

	if (pcb_anyobj_set_thermal(obj, ctx->chgtherm.style, 1) != 0)
		return NULL;

	return obj;
}

