/*
 * jostle plug-in for PCB.
 *
 * Copyright (C) 2007 Ben Jackson <ben@ben.com>
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 *
 *
 * Pushes lines out of the way.
 */

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include <librnd/core/hid.h>
#include <librnd/poly/rtree.h>
#include "undo.h"
#include "polygon.h"
#include <librnd/poly/polygon1_gen.h>
#include "remove.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include "layer.h"
#include "conf_core.h"
#include <librnd/core/misc_util.h>
#include "obj_line.h"
#include <librnd/core/event.h>

/*#define DEBUG_pcb_polyarea_t*/

double rnd_vect_dist2(rnd_vector_t v1, rnd_vector_t v2);
#define Vcpy2(r,a)              {(r)[0] = (a)[0]; (r)[1] = (a)[1];}
#define Vswp2(a,b) { long t; \
        t = (a)[0], (a)[0] = (b)[0], (b)[0] = t; \
        t = (a)[1], (a)[1] = (b)[1], (b)[1] = t; \
}

enum {
	JNORTH,
	JNORTHEAST,
	JEAST,
	JSOUTHEAST,
	JSOUTH,
	JSOUTHWEST,
	JWEST,
	JNORTHWEST
};

const char *dirnames[] = {
	"JNORTH",
	"JNORTHEAST",
	"JEAST",
	"JSOUTHEAST",
	"JSOUTH",
	"JSOUTHWEST",
	"JWEST",
	"JNORTHWEST"
};

#define ARG(n) (argc > (n) ? argv[n] : 0)

/* DEBUG */
static void Debugpcb_polyarea_t(rnd_polyarea_t * s, char *color)
{
	int *x, *y, n, i = 0;
	rnd_pline_t *pl;
	rnd_vnode_t *v;
	rnd_polyarea_t *p;

#ifndef DEBUG_pcb_polyarea_t
	return;
#endif

/* TODO: rewrite this to UI layers if needed
	ddraw = rnd_gui->request_debug_draw();
	ddgc = ddraw->make_gc();
*/

	p = s;
	do {
		for (pl = p->contours; pl; pl = pl->next) {
			n = pl->Count;
			x = (int *) malloc(n * sizeof(int));
			y = (int *) malloc(n * sizeof(int));
			for (v = pl->head; i < n; v = v->next) {
				x[i] = v->point[0];
				y[i++] = v->point[1];
			}
/*			if (1) {
				rnd_render->set_color(ddgc, color ? color : conf_core.appearance.color.connected);
				rnd_hid_set_line_width(ddgc, 1);
				for (i = 0; i < n - 1; i++) {
					rnd_render->draw_line(ddgc, x[i], y[i], x[i + 1], y[i + 1]);
					rnd_render->fill_circle (ddgc, x[i], y[i], 30);
				}
				rnd_render->draw_line(ddgc, x[n - 1], y[n - 1], x[0], y[0]);
			}*/
			free(x);
			free(y);
		}
	} while ((p = p->f) != s);
/*	ddraw->flush_debug_draw();*/
/*	rnd_hid_busy(PCB, 1); */
/*	sleep(3);
	ddraw->finish_debug_draw();*/
/*	rnd_hid_busy(PCB, 0); */
}

/* Find the bounding box of a rnd_polyarea_t.
 *
 * pcb_polyarea_ts linked by ->f/b are outlines.\n
 * n->contours->next would be the start of the inner holes (irrelevant
 * for bounding box). */
static rnd_box_t pcb_polyarea_t_boundingBox(rnd_polyarea_t * a)
{
	rnd_polyarea_t *n;
	rnd_pline_t *pa;
	rnd_box_t box;
	int first = 1;

	n = a;
	do {
		pa = n->contours;
		if (first) {
			box.X1 = pa->xmin;
			box.X2 = pa->xmax + 1;
			box.Y1 = pa->ymin;
			box.Y2 = pa->ymax + 1;
			first = 0;
		}
		else {
			RND_MAKE_MIN(box.X1, pa->xmin);
			RND_MAKE_MAX(box.X2, pa->xmax + 1);
			RND_MAKE_MIN(box.Y1, pa->ymin);
			RND_MAKE_MAX(box.Y2, pa->ymax + 1);
		}
	} while ((n = n->f) != a);
	return box;
}

/* Given a polygon and a side of it (a direction north/northeast/etc), find a
   line tangent to that side, offset by clearance, and return it as a pair of
   vectors PQ. Make it long so it will intersect everything in the area. */
static void pcb_polyarea_t_findXmostLine(rnd_polyarea_t * a, int side, rnd_vector_t p, rnd_vector_t q, int clearance)
{
	int extra;
	p[0] = p[1] = 0;
	q[0] = q[1] = 0;
	extra = a->contours->xmax - a->contours->xmin + a->contours->ymax - a->contours->ymin;
	switch (side) {
	case JNORTH:
		p[1] = q[1] = a->contours->ymin - clearance;
		p[0] = a->contours->xmin - extra;
		q[0] = a->contours->xmax + extra;
		break;
	case JSOUTH:
		p[1] = q[1] = a->contours->ymax + clearance;
		p[0] = a->contours->xmin - extra;
		q[0] = a->contours->xmax + extra;
		break;
	case JEAST:
		p[0] = q[0] = a->contours->xmax + clearance;
		p[1] = a->contours->ymin - extra;
		q[1] = a->contours->ymax + extra;
		break;
	case JWEST:
		p[0] = q[0] = a->contours->xmin - clearance;
		p[1] = a->contours->ymin - extra;
		q[1] = a->contours->ymax + extra;
		break;
	default:											/* diagonal case */
		{
			int kx, ky, minmax, dq, ckx, cky;
			rnd_coord_t mm[2] = { RND_MAX_COORD, -RND_MAX_COORD };
			rnd_vector_t mmp[2];
			rnd_vnode_t *v;

			switch (side) {
			case JNORTHWEST:
				kx = 1;									/* new_x = kx * x + ky * y */
				ky = 1;
				dq = -1;								/* extend line in +x, dq*y */
				ckx = cky = -1;					/* clear line in ckx*clear, cky*clear */
				minmax = 0;							/* min or max */
				break;
			case JSOUTHWEST:
				kx = 1;
				ky = -1;
				dq = 1;
				ckx = -1;
				cky = 1;
				minmax = 0;
				break;
			case JNORTHEAST:
				kx = 1;
				ky = -1;
				dq = 1;
				ckx = 1;
				cky = -1;
				minmax = 1;
				break;
			case JSOUTHEAST:
				kx = ky = 1;
				dq = -1;
				ckx = cky = 1;
				minmax = 1;
				break;
			default:
				rnd_message(RND_MSG_ERROR, "jostle: aiee, what side?");
				return;
			}
			v = a->contours->head;
			do {
				int test = kx * v->point[0] + ky * v->point[1];
				if (test < mm[0]) {
					mm[0] = test;
					mmp[0][0] = v->point[0];
					mmp[0][1] = v->point[1];
				}
				if (test > mm[1]) {
					mm[1] = test;
					mmp[1][0] = v->point[0];
					mmp[1][1] = v->point[1];
				}
			} while ((v = v->next) != a->contours->head);
			Vcpy2(p, mmp[minmax]);
			/* add clearance in the right direction */
			clearance *= 0.707123;		/* = cos(45) = sqrt(2)/2 */
			p[0] += ckx * clearance;
			p[1] += cky * clearance;
			/* now create a tangent line to that point */
			Vcpy2(q, p);
			p[0] += -extra;
			p[1] += -extra * dq;
			q[0] += extra;
			q[1] += extra * dq;
		}
	}
}

/* Given a 'side' from the JNORTH/JSOUTH/etc enum, rotate it by n. */
static int rotateSide(int side, int n)
{
	return (side + n + 8) % 8;
}

/* Wrapper for CreateNewLineOnLayer that takes vectors and deals with Undo */
static pcb_line_t *Createpcb_vector_tLineOnLayer(pcb_layer_t * layer, rnd_vector_t a, rnd_vector_t b, int thickness, int clearance, pcb_flag_t flags)
{
	pcb_line_t *line;

	line = pcb_line_new(layer, a[0], a[1], b[0], b[1], thickness, clearance, flags);
	if (line) {
		pcb_undo_add_obj_to_create(PCB_OBJ_LINE, layer, line, line);
	}
	return line;
}

static pcb_line_t *MakeBypassLine(pcb_layer_t * layer, rnd_vector_t a, rnd_vector_t b, pcb_line_t * orig, rnd_polyarea_t ** expandp)
{
	pcb_line_t *line;

	line = Createpcb_vector_tLineOnLayer(layer, a, b, orig->Thickness, orig->Clearance, orig->Flags);
	if (line && expandp) {
		rnd_polyarea_t *p = pcb_poly_from_pcb_line(line, line->Thickness + line->Clearance);
		rnd_polyarea_boolean_free(*expandp, p, expandp, RND_PBO_UNITE);
	}
	return line;
}

/* Given a 'brush' that's pushing things out of the way (possibly already
 * cut down to just the part relevant to our line) and a line that
 * intersects it on some layer, find the 45/90 lines required to go around
 * the brush on the named side.  Create them and remove the original.
 *
 * Imagine side = north:
                 /      \
            ----b##FLAT##c----
               Q          P
   lA-ORIG####a            d####ORIG-lB
             /              \
 * First find the extended three lines that go around the brush.
 * Then intersect them with each other and the original to find
 * points a, b, c, d.  Finally connect the dots and remove the
 * old straight line. */
static int MakeBypassingLines(rnd_polyarea_t * brush, pcb_layer_t * layer, pcb_line_t * line, int side, rnd_polyarea_t ** expandp)
{
	rnd_vector_t pA, pB, flatA, flatB, qA, qB;
	rnd_vector_t lA, lB;
	rnd_vector_t a, b, c, d, junk;
	int hits;

	PCB_FLAG_SET(PCB_FLAG_DRC, line);			/* will cause sublines to inherit */
	lA[0] = line->Point1.X;
	lA[1] = line->Point1.Y;
	lB[0] = line->Point2.X;
	lB[1] = line->Point2.Y;

	pcb_polyarea_t_findXmostLine(brush, side, flatA, flatB, line->Thickness / 2);
	pcb_polyarea_t_findXmostLine(brush, rotateSide(side, 1), pA, pB, line->Thickness / 2);
	pcb_polyarea_t_findXmostLine(brush, rotateSide(side, -1), qA, qB, line->Thickness / 2);
	hits = rnd_vect_inters2(lA, lB, qA, qB, a, junk) +
		rnd_vect_inters2(qA, qB, flatA, flatB, b, junk) +
		rnd_vect_inters2(pA, pB, flatA, flatB, c, junk) + rnd_vect_inters2(lA, lB, pA, pB, d, junk);
	if (hits != 4) {
		return 0;
	}
	/* flip the line endpoints to match up with a/b */
	if (rnd_vect_dist2(lA, d) < rnd_vect_dist2(lA, a)) {
		Vswp2(lA, lB);
	}
	MakeBypassLine(layer, lA, a, line, NULL);
	MakeBypassLine(layer, a, b, line, expandp);
	MakeBypassLine(layer, b, c, line, expandp);
	MakeBypassLine(layer, c, d, line, expandp);
	MakeBypassLine(layer, d, lB, line, NULL);
	pcb_line_destroy(layer, line);
	return 1;
}

struct info {
	rnd_box_t box;
	rnd_polyarea_t *brush;
	pcb_layer_t *layer;
	rnd_polyarea_t *smallest; /* after cutting brush with line, the smallest chunk, which we will go around on 'side'. */
	pcb_line_t *line;
	int side;
	double centroid; /* smallest difference between slices of brush after cutting with line, trying to find the line closest to the centroid to process first */
};

/* Process lines that intersect our 'brush'. */
static rnd_r_dir_t jostle_callback(const rnd_box_t * targ, void *private)
{
	pcb_line_t *line = (pcb_line_t *) targ;
	struct info *info = private;
	rnd_polyarea_t *lp, *copy, *tmp, *n, *smallest = NULL;
	rnd_vector_t p;
	int inside = 0, side, r;
	double small, big;
	int nocentroid = 0;

	if (PCB_FLAG_TEST(PCB_FLAG_DRC, line)) {
		return 0;
	}
	fprintf(stderr, "hit! %p\n", (void *)line);
	p[0] = line->Point1.X;
	p[1] = line->Point1.Y;
	if (rnd_poly_contour_inside(info->brush->contours, p)) {
		rnd_fprintf(stderr, "\tinside1 %ms,%ms\n", p[0], p[1]);
		inside++;
	}
	p[0] = line->Point2.X;
	p[1] = line->Point2.Y;
	if (rnd_poly_contour_inside(info->brush->contours, p)) {
		rnd_fprintf(stderr, "\tinside2 %ms,%ms\n", p[0], p[1]);
		inside++;
	}
	lp = pcb_poly_from_pcb_line(line, line->Thickness);
	if (!rnd_polyarea_touching(lp, info->brush)) {
		/* not a factor */
		return 0;
	}
	rnd_polyarea_free(&lp);
	if (inside) {
		/* XXX not done!
		   XXX if this is part of a series of lines passing
		   XXX through, need to process as a group.
		   XXX if it just ends in here, shorten it?? */
		return 0;
	}
	/*
	 * Cut the brush with the line to figure out which side to go
	 * around.  Use a very fine line.  XXX can still graze.
	 */
	lp = pcb_poly_from_pcb_line(line, 1);
	if (!rnd_polyarea_m_copy0(&copy, info->brush))
		return 0;
	r = rnd_polyarea_boolean_free(copy, lp, &tmp, RND_PBO_SUB);
	if (r != rnd_err_ok) {
		rnd_fprintf(stderr, "Error while jostling RND_PBO_SUB: %d\n", r);
		return 0;
	}
	if (tmp == tmp->f) {
		/* it didn't slice, must have glanced. intersect instead
		 * to get the glancing sliver??
		 */
		rnd_fprintf(stderr, "try isect??\n");
		lp = pcb_poly_from_pcb_line(line, line->Thickness);
		r = rnd_polyarea_boolean_free(tmp, lp, &tmp, RND_PBO_ISECT);
		if (r != rnd_err_ok) {
			fprintf(stderr, "Error while jostling RND_PBO_ISECT: %d\n", r);
			return 0;
		}
		nocentroid = 1;
	}
	/* XXX if this operation did not create two chunks, bad things are about to happen */
	if (!tmp)
		return 0;
	n = tmp;
	small = big = tmp->contours->area;
	do {
		rnd_fprintf(stderr, "\t\tarea %g, %ms,%ms %ms,%ms\n", n->contours->area, n->contours->xmin, n->contours->ymin,
								n->contours->xmax, n->contours->ymax);
		if (n->contours->area <= small) {
			smallest = n;
			small = n->contours->area;
		}
		if (n->contours->area >= big) {
			big = n->contours->area;
		}
	} while ((n = n->f) != tmp);
	if (line->Point1.X == line->Point2.X) {	/* | */
		if (info->box.X2 - smallest->contours->xmax > smallest->contours->xmin - info->box.X1) {
			side = JWEST;
		}
		else {
			side = JEAST;
		}
	}
	else if (line->Point1.Y == line->Point2.Y) {	/* - */
		if (info->box.Y2 - smallest->contours->ymax > smallest->contours->ymin - info->box.Y1) {
			side = JNORTH;
		}
		else {
			side = JSOUTH;
		}
	}
	else if ((line->Point1.X > line->Point2.X) == (line->Point1.Y > line->Point2.Y)) {	/* \ */
		if (info->box.X2 - smallest->contours->xmax > smallest->contours->xmin - info->box.X1) {
			side = JSOUTHWEST;
		}
		else {
			side = JNORTHEAST;
		}
	}
	else {												/* / */
		if (info->box.X2 - smallest->contours->xmax > smallest->contours->xmin - info->box.X1) {
			side = JNORTHWEST;
		}
		else {
			side = JSOUTHEAST;
		}
	}
	rnd_fprintf(stderr, "\t%s\n", dirnames[side]);
	if (info->line == NULL || (!nocentroid && (big - small) < info->centroid)) {
		rnd_fprintf(stderr, "\tkeep it!\n");
		if (info->smallest) {
			rnd_polyarea_free(&info->smallest);
		}
		info->centroid = nocentroid ? DBL_MAX : (big - small);
		info->side = side;
		info->line = line;
		info->smallest = smallest;
		return 1;
	}
	return 0;
}

static const char pcb_acts_jostle[] = "Jostle(diameter)";
static const char pcb_acth_jostle[] = "Make room by moving wires away.";
static fgw_error_t pcb_act_jostle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t x, y;
	rnd_polyarea_t *expand;
	float value = conf_core.design.via_thickness + (conf_core.design.bloat + 1) * 2 + 50;
	struct info info;
	int found;

	RND_ACT_MAY_CONVARG(1, FGW_KEYWORD, jostle, value = fgw_keyword(&argv[1]));

	x = pcb_crosshair.X;
	y = pcb_crosshair.Y;
	fprintf(stderr, "%d, %d, %f\n", (int) x, (int) y, value);
	info.brush = rnd_poly_from_circle(x, y, value / 2);
	info.layer = PCB_CURRLAYER(PCB);
	PCB_LINE_LOOP(info.layer);
	{
		PCB_FLAG_CLEAR(PCB_FLAG_DRC, line);
	}
	PCB_END_LOOP;
	do {
		info.box = pcb_polyarea_t_boundingBox(info.brush);
		Debugpcb_polyarea_t(info.brush, NULL);
		rnd_fprintf(stderr, "search (%ms,%ms)->(%ms,%ms):\n", info.box.X1, info.box.Y1, info.box.X2, info.box.Y2);
		info.line = NULL;
		info.smallest = NULL;
		rnd_r_search(info.layer->line_tree, &info.box, NULL, jostle_callback, &info, &found);
		if (found) {
			expand = NULL;
			MakeBypassingLines(info.smallest, info.layer, info.line, info.side, &expand);
			rnd_polyarea_free(&info.smallest);
			rnd_polyarea_boolean_free(info.brush, expand, &info.brush, RND_PBO_UNITE);
		}
	} while (found);
	pcb_board_set_changed_flag(PCB_ACT_BOARD, rnd_true);
	pcb_undo_inc_serial();

	RND_ACT_IRES(0);
	return 0;
}

static rnd_action_t jostle_action_list[] = {
	{"jostle", pcb_act_jostle, pcb_acth_jostle, pcb_acts_jostle}
};

char *jostle_cookie = "jostle plugin";

int pplg_check_ver_jostle(int ver_needed) { return 0; }

void pplg_uninit_jostle(void)
{
	rnd_remove_actions_by_cookie(jostle_cookie);
}

int pplg_init_jostle(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(jostle_action_list, jostle_cookie);
	return 0;
}
