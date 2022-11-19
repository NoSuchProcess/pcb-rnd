/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2003 DJ Delorie
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
 *
 *
 *  Old contact info:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#include "config.h"
#include "conf_core.h"
#include <librnd/core/rnd_conf.h>
#include "board.h"

#include <memory.h>
#include <limits.h>


#include "data.h"
#include "remove.h"
#include "move.h"
#include "draw.h"
#include "undo.h"
#include "flag_str.h"
#include "find.h"
#include "layer.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include "djopt_conf.h"
#include "obj_line.h"
#include <librnd/core/event.h>
#include "obj_pstk_inlines.h"

#include <librnd/hid/hid_menu.h>
#include "menu_internal.c"

conf_djopt_t conf_djopt;

static const char *djopt_cookie = "djopt";


#define dprintf if(0)rnd_printf

#define selected(x) PCB_FLAG_TEST(PCB_FLAG_SELECTED, (x))
#define autorouted(x) PCB_FLAG_TEST(PCB_FLAG_AUTO, (x))

#define SB (conf_core.design.bloat+1)

/* must be 2^N-1 */
#define INC 7

#define O_HORIZ		0x10
#define O_VERT		0x20
#define LEFT		0x11
#define RIGHT		0x12
#define UP		0x24
#define DOWN		0x28
#define DIAGONAL	0xf0
#define ORIENT(x) ((x) & 0xf0)
#define DIRECT(x) ((x) & 0x0f)

/* Manhattan length of the longest "freckle" */
#define LONGEST_FRECKLE	2

struct line_s;

typedef struct corner_s {
	int layer;
	struct corner_s *next;
	int x, y;
	int net;
	pcb_pstk_t *via, *pad, *pin;
	int miter;
	int n_lines;
	struct line_s **lines;
} corner_s;

typedef struct line_s {
	int layer;
	struct line_s *next;
	corner_s *s, *e;
	pcb_line_t *line;
	char is_pad;
} line_s;

typedef struct rect_s {
	int x1, y1, x2, y2;
} rect_s;

#define DELETE(q) (q)->layer = 0xdeadbeef
#define DELETED(q) ((q)->layer == 0xdeadbeef)

static corner_s *corners, *next_corner = 0;
static line_s *lines;

static int layer_groupings[PCB_MAX_LAYERGRP];
static char layer_type[PCB_MAX_LAYER];
#define LT_COMPONENT 1
#define LT_SOLDER 2

static const char *element_name_for(corner_s * c)
{
	PCB_SUBC_LOOP(PCB->Data);
	{
		PCB_PADSTACK_LOOP(subc->data);
		{
			if ((padstack == c->pin) || (padstack == c->pad))
				return subc->refdes;
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	return "unknown";
}

static const char *corner_name(corner_s * c)
{
	static char buf[4][100];
	static int bn = 0;
	char *bp;
	bn = (bn + 1) % 4;

	if (c->net == 0xf1eef1ee) {
		sprintf(buf[bn], "\033[31m[%p freed corner]\033[0m", (void *) c);
		return buf[bn];
	}

	sprintf(buf[bn], "\033[%dm[%p ", (c->pin || c->pad || c->via) ? 33 : 34, (void *) c);
	bp = buf[bn] + strlen(buf[bn]);

	if (c->pin)
		rnd_sprintf(bp, "pin %s:%s at %#mD", element_name_for(c), c->pin->term, c->x, c->y);
	else if (c->via)
		rnd_sprintf(bp, "via at %#mD", c->x, c->y);
	else if (c->pad) {
		rnd_sprintf(bp, "pad %s:%s at %#mD %#mD-%#mD",
								element_name_for(c), c->pad->term, c->x, c->y,
								c->pad->BoundingBox.X1, c->pad->BoundingBox.Y1, c->pad->BoundingBox.X2, c->pad->BoundingBox.Y2);
	}
	else
		rnd_sprintf(bp, "at %#mD", c->x, c->y);
	sprintf(bp + strlen(bp), " n%d l%d]\033[0m", c->n_lines, c->layer);
	return buf[bn];
}

static int solder_layer, component_layer;

static void dj_abort(const char *msg, ...)
{
	va_list a;
	va_start(a, msg);
	vprintf(msg, a);
	va_end(a);
	fflush(stdout);
	abort();
}

#if 1
#define check(c,l)
#else
#define check(c,l) check2(__LINE__,c,l)
static void check2(int srcline, corner_s * c, line_s * l)
{
	int saw_c = 0, saw_l = 0;
	corner_s *cc;
	line_s *ll;
	int i;

	for (cc = corners; cc; cc = cc->next) {
		if (DELETED(cc))
			continue;
		if (cc == c)
			saw_c = 1;
		for (i = 0; i < cc->n_lines; i++)
			if (cc->lines[i]->s != cc && cc->lines[i]->e != cc)
				dj_abort("check:%d: cc has line without backref\n", srcline);
		if (cc->via && (cc->x != cc->via->X || cc->y != cc->via->Y))
			dj_abort("check:%d: via not at corner\n", srcline);
		if (cc->pin && (cc->x != cc->pin->X || cc->y != cc->pin->Y))
			dj_abort("check:%d: pin not at corner\n", srcline);
	}
	if (c && !saw_c)
		dj_abort("check:%d: corner not in corners list\n", srcline);
	for (ll = lines; ll; ll = ll->next) {
		if (DELETED(ll))
			continue;
		if (ll == l)
			saw_l = 1;
		for (i = 0; i < ll->s->n_lines; i++)
			if (ll->s->lines[i] == ll)
				break;
		if (i == ll->s->n_lines)
			dj_abort("check:%d: ll->s has no backref\n", srcline);
		for (i = 0; i < ll->e->n_lines; i++)
			if (ll->e->lines[i] == ll)
				break;
		if (i == ll->e->n_lines)
			dj_abort("check:%d: ll->e has no backref\n", srcline);
		if (!ll->is_pad
				&& (ll->s->x != ll->line->Point1.X
						|| ll->s->y != ll->line->Point1.Y || ll->e->x != ll->line->Point2.X || ll->e->y != ll->line->Point2.Y)) {
			rnd_printf("line: %#mD to %#mD  pcbline: %#mD to %#mD\n",
								 ll->s->x, ll->s->y,
								 ll->e->x, ll->e->y, ll->line->Point1.X, ll->line->Point1.Y, ll->line->Point2.X, ll->line->Point2.Y);
			dj_abort("check:%d: line doesn't match pcbline\n", srcline);
		}
	}
	if (l && !saw_l)
		dj_abort("check:%d: line not in lines list\n", srcline);
}

#endif

#define SWAP(a,b) { a^=b; b^=a; a^=b; }

static int gridsnap(rnd_coord_t n)
{
	if (n <= 0)
		return 0;
	return n - n % (rnd_conf.editor.grid);
}

/* Avoid commonly used names. */

static int djabs(int x)
{
	return x > 0 ? x : -x;
}

static int djmax(int x, int y)
{
	return x > y ? x : y;
}

/*
 * Find distance between 2 points.  We use floating point math here
 * because we can fairly easily overflow a 32 bit integer here.  In
 * fact it only takes 0.46" to do so.
 */
static int dist(int x1, int y1, int x2, int y2)
{
	double dx1, dy1, dx2, dy2, d;

	dx1 = (double) x1;
	dy1 = (double) y1;
	dx2 = (double) x2;
	dy2 = (double) y2;

	d = sqrt((dx1 - dx2) * (dx1 - dx2) + (dy1 - dy2) * (dy1 - dy2));
	d = rnd_round(d);

	return (int) d;
}

static int line_length(line_s * l)
{
	if (l->s->x == l->e->x)
		return djabs(l->s->y - l->e->y);
	if (l->s->y == l->e->y)
		return djabs(l->s->x - l->e->x);
	return dist(l->s->x, l->s->y, l->e->x, l->e->y);
}

static int dist_ltp2(int dx, int y, int y1, int y2)
{
	if (y1 > y2)
		SWAP(y1, y2);
	if (y < y1)
		return dist(dx, y, 0, y1);
	if (y > y2)
		return dist(dx, y, 0, y2);
	return djabs(dx);
}

int sqr(int a)
{
	return a * a;
}

static int intersecting_layers(int l1, int l2)
{
	if (l1 == -1 || l2 == -1)
		return 1;
	if (l1 == l2)
		return 1;
	if (layer_groupings[l1] == layer_groupings[l2])
		return 1;
	return 0;
}

static int dist_line_to_point(line_s * l, corner_s * c)
{
	double len, r, d;
	/* We can do this quickly if l is vertical or horizontal.  */
	if (l->s->x == l->e->x)
		return dist_ltp2(l->s->x - c->x, c->y, l->s->y, l->e->y);
	if (l->s->y == l->e->y)
		return dist_ltp2(l->s->y - c->y, c->x, l->s->x, l->e->x);

	/* Do it the hard way.  See comments for pcb_is_point_on_line() in search.c */
	len = sqrt(sqr(l->s->x - l->e->x) + sqr(l->s->y - l->e->y));
	if (len == 0)
		return dist(l->s->x, l->s->y, c->x, c->y);
	r = (l->s->y - c->y) * (l->s->y - l->e->y) + (l->s->x - c->x) * (l->s->x - l->e->x);
	r /= len * len;
	if (r < 0)
		return dist(l->s->x, l->s->y, c->x, c->y);
	if (r > 1)
		return dist(l->e->x, l->e->y, c->x, c->y);
	d = (l->e->y - l->s->y) * (c->x * l->s->x) + (l->e->x - l->s->x) * (c->y - l->s->y);
	return (int) (d / len);
}

static int line_orient(line_s * l, corner_s * c)
{
	int x1, y1, x2, y2;
	if (c == l->s) {
		x1 = l->s->x;
		y1 = l->s->y;
		x2 = l->e->x;
		y2 = l->e->y;
	}
	else {
		x1 = l->e->x;
		y1 = l->e->y;
		x2 = l->s->x;
		y2 = l->s->y;
	}
	if (x1 == x2) {
		if (y1 < y2)
			return DOWN;
		return UP;
	}
	else if (y1 == y2) {
		if (x1 < x2)
			return RIGHT;
		return LEFT;
	}
	return DIAGONAL;
}

static corner_s *other_corner(line_s * l, corner_s * c)
{
	if (l->s == c)
		return l->e;
	if (l->e == c)
		return l->s;
	dj_abort("other_corner: neither corner passed\n");
	return NULL;
}

static corner_s *find_corner_if(int x, int y, int l)
{
	corner_s *c;
	for (c = corners; c; c = c->next) {
		if (DELETED(c))
			continue;
		if (c->x != x || c->y != y)
			continue;
		if (!(c->layer == -1 || intersecting_layers(c->layer, l)))
			continue;
		return c;
	}
	return 0;
}

static corner_s *find_corner(int x, int y, int l)
{
	corner_s *c;
	for (c = corners; c; c = c->next) {
		if (DELETED(c))
			continue;
		if (c->x != x || c->y != y)
			continue;
		if (!(c->layer == -1 || intersecting_layers(c->layer, l)))
			continue;
		return c;
	}
	c = (corner_s *) malloc(sizeof(corner_s));
	c->next = corners;
	corners = c;
	c->x = x;
	c->y = y;
	c->net = 0;
	c->via = 0;
	c->pad = 0;
	c->pin = 0;
	c->layer = l;
	c->n_lines = 0;
	c->lines = (line_s **) malloc(INC * sizeof(line_s *));
	return c;
}

static void add_line_to_corner(line_s * l, corner_s * c)
{
	int n;
	n = (c->n_lines + 1 + INC) & ~INC;
	c->lines = (line_s **) realloc(c->lines, n * sizeof(line_s *));
	c->lines[c->n_lines] = l;
	c->n_lines++;
	dprintf("add_line_to_corner %#mD\n", c->x, c->y);
}

static pcb_line_t *create_pcb_line(int layer, int x1, int y1, int x2, int y2, int thick, int clear, pcb_flag_t flags)
{
	char *from, *to;
	pcb_line_t *nl;
	pcb_layer_t *lyr = pcb_get_layer(PCB->Data, layer);

	from = (char *) linelist_first(&lyr->Line);
	nl = pcb_line_new(PCB->Data->Layer + layer, x1, y1, x2, y2, thick, clear, flags);
	pcb_undo_add_obj_to_create(PCB_OBJ_LINE, lyr, nl, nl);

	to = (char *) linelist_first(&lyr->Line);
	if (from != to) {
		line_s *lp;
		for (lp = lines; lp; lp = lp->next) {
			if (DELETED(lp))
				continue;
			if ((char *) (lp->line) >= from && (char *) (lp->line) <= from + linelist_length(&lyr->Line) * sizeof(pcb_line_t))
				lp->line = (pcb_line_t *) ((char *) (lp->line) + (to - from));
		}
	}
	return nl;
}

static void new_line(corner_s * s, corner_s * e, int layer, pcb_line_t * example)
{
	line_s *ls;

	if (!(pcb_layer_flags(PCB, layer) & PCB_LYT_COPPER))
		dj_abort("layer %d\n", layer);

	if (example == NULL)
		dj_abort("NULL example passed to new_line()\n", layer);

	if (s->x == e->x && s->y == e->y)
		return;

	ls = (line_s *) malloc(sizeof(line_s));
	ls->next = lines;
	lines = ls;
	ls->is_pad = 0;
	ls->s = s;
	ls->e = e;
	ls->layer = layer;
#if 0
	if ((example->Point1.X == s->x && example->Point1.Y == s->y && example->Point2.X == e->x && example->Point2.Y == e->y)
			|| (example->Point2.X == s->x && example->Point2.Y == s->y && example->Point1.X == e->x && example->Point1.Y == e->y)) {
		ls->line = example;
	}
	else
#endif
	{
		pcb_line_t *nl;
		dprintf
			("New line \033[35m%#mD to %#mD from l%d t%#mS c%#mS f%s\033[0m\n",
			 s->x, s->y, e->x, e->y, layer, example->Thickness, example->Clearance, pcb_strflg_f2s(example->Flags, PCB_OBJ_LINE, NULL, 0));
		nl = create_pcb_line(layer, s->x, s->y, e->x, e->y, example->Thickness, example->Clearance, example->Flags);

		if (!nl)
			dj_abort("can't create new line!");
		ls->line = nl;
	}
	add_line_to_corner(ls, s);
	add_line_to_corner(ls, e);
	check(s, ls);
	check(e, ls);
}

static line_s *other_line(corner_s * c, line_s * l)
{
	int i;
	line_s *rv = 0;
	if (c->pin || c->pad || c->via)
		return 0;
	for (i = 0; i < c->n_lines; i++) {
		if (c->lines[i] == l)
			continue;
		if (rv)
			return 0;
		rv = c->lines[i];
	}
	return rv;
}

static void empty_rect(rect_s * rect)
{
	rect->x1 = rect->y1 = INT_MAX;
	rect->x2 = rect->y2 = INT_MIN;
}

static void add_point_to_rect(rect_s * rect, int x, int y, int w)
{
	if (rect->x1 > x - w)
		rect->x1 = x - w;
	if (rect->x2 < x + w)
		rect->x2 = x + w;
	if (rect->y1 > y - w)
		rect->y1 = y - w;
	if (rect->y2 < y + w)
		rect->y2 = y + w;
}

static void add_line_to_rect(rect_s * rect, line_s * l)
{
	add_point_to_rect(rect, l->s->x, l->s->y, 0);
	add_point_to_rect(rect, l->e->x, l->e->y, 0);
}

static int pin_in_rect(rect_s * r, int x, int y, int w)
{
	if (x < r->x1 && x + w < r->x1)
		return 0;
	if (x > r->x2 && x - w > r->x2)
		return 0;
	if (y < r->y1 && y + w < r->y1)
		return 0;
	if (y > r->y2 && y - w > r->y2)
		return 0;
	return 1;
}

static int line_in_rect(rect_s * r, line_s * l)
{
	rect_s lr;
	empty_rect(&lr);
	add_point_to_rect(&lr, l->s->x, l->s->y, l->line->Thickness / 2);
	add_point_to_rect(&lr, l->e->x, l->e->y, l->line->Thickness / 2);
	dprintf("line_in_rect %#mD-%#mD vs %#mD-%#mD\n", r->x1, r->y1, r->x2, r->y2, lr.x1, lr.y1, lr.x2, lr.y2);
	/* simple intersection of rectangles */
	if (lr.x1 < r->x1)
		lr.x1 = r->x1;
	if (lr.x2 > r->x2)
		lr.x2 = r->x2;
	if (lr.y1 < r->y1)
		lr.y1 = r->y1;
	if (lr.y2 > r->y2)
		lr.y2 = r->y2;
	if (lr.x1 < lr.x2 && lr.y1 < lr.y2)
		return 1;
	return 0;
}

static int corner_radius(corner_s * c)
{
	pcb_pstk_t *p = NULL;
	int diam = 0;
	int i;

	if (c->pin) p = c->pin;
	if (c->via) p = c->via;
	if (c->pad) p = c->pad;
	if (p != NULL)
		diam = djmax(p->BoundingBox.X2 - p->BoundingBox.X1, p->BoundingBox.Y2 - p->BoundingBox.Y1);

	for (i = 0; i < c->n_lines; i++)
		if (c->lines[i]->line)
			diam = djmax(c->lines[i]->line->Thickness, diam);
	diam = (diam + 1) / 2;
	return diam;
}

static void add_corner_to_rect_if(rect_s * rect, corner_s * c, rect_s * e)
{
	int diam = corner_radius(c);
	if (!pin_in_rect(e, c->x, c->y, diam))
		return;
	if (c->x < e->x1 && c->y < e->y1 && dist(c->x, c->y, e->x1, e->y1) > diam)
		return;
	if (c->x > e->x2 && c->y < e->y1 && dist(c->x, c->y, e->x2, e->y1) > diam)
		return;
	if (c->x < e->x1 && c->y > e->y2 && dist(c->x, c->y, e->x1, e->y2) > diam)
		return;
	if (c->x > e->x2 && c->y > e->y2 && dist(c->x, c->y, e->x2, e->y2) > diam)
		return;

	/*rnd_printf("add point %#mD diam %#mS\n", c->x, c->y, diam); */
	add_point_to_rect(rect, c->x, c->y, diam);
}

static void remove_line(line_s * l)
{
	int i, j;
	pcb_layer_t *layer = &(PCB->Data->Layer[l->layer]);

	check(0, 0);

	if (l->line)
		pcb_line_destroy(layer, l->line);

	DELETE(l);

	for (i = 0, j = 0; i < l->s->n_lines; i++)
		if (l->s->lines[i] != l)
			l->s->lines[j++] = l->s->lines[i];
	l->s->n_lines = j;

	for (i = 0, j = 0; i < l->e->n_lines; i++)
		if (l->e->lines[i] != l)
			l->e->lines[j++] = l->e->lines[i];
	l->e->n_lines = j;
	check(0, 0);
}

static void move_line_to_layer(line_s * l, int layer)
{
	pcb_layer_t *ls, *ld;

	ls = pcb_get_layer(PCB->Data, l->layer);
	ld = pcb_get_layer(PCB->Data, layer);

	pcb_move_obj_to_layer(PCB_OBJ_LINE, ls, l->line, 0, ld, 0);
	l->layer = layer;
}

static void remove_via_at(corner_s * c)
{
	pcb_remove_object(PCB_OBJ_PSTK, c->via, 0, 0);
	c->via = 0;
}

static void remove_corner(corner_s * c2)
{
	corner_s *c;
	dprintf("remove corner %s\n", corner_name(c2));
	if (corners == c2)
		corners = c2->next;
	for (c = corners; c; c = c->next) {
		if (DELETED(c))
			continue;
		if (c->next == c2)
			c->next = c2->next;
	}
	if (next_corner == c2)
		next_corner = c2->next;
	free(c2->lines);
	c2->lines = 0;
	DELETE(c2);
}

static void merge_corners(corner_s * c1, corner_s * c2)
{
	int i;
	if (c1 == c2)
		abort();
	dprintf("merge corners %s %s\n", corner_name(c1), corner_name(c2));
	for (i = 0; i < c2->n_lines; i++) {
		add_line_to_corner(c2->lines[i], c1);
		if (c2->lines[i]->s == c2)
			c2->lines[i]->s = c1;
		if (c2->lines[i]->e == c2)
			c2->lines[i]->e = c1;
	}
	if (c1->via && c2->via)
		remove_via_at(c2);
	else if (c2->via)
		c1->via = c2->via;
	if (c2->pad)
		c1->pad = c2->pad;
	if (c2->pin)
		c1->pin = c2->pin;
	if (c2->layer != c1->layer)
		c1->layer = -1;

	remove_corner(c2);
}

static void move_corner(corner_s * c, int x, int y)
{
	pcb_pstk_t *via;
	int i;
	corner_s *pad;

	check(c, 0);
	if (c->pad || c->pin)
		dj_abort("move_corner: has pin or pad\n");
	dprintf("move_corner %p from %#mD to %#mD\n", (void *) c, c->x, c->y, x, y);
	pad = find_corner_if(x, y, c->layer);
	c->x = x;
	c->y = y;
	via = c->via;
	if (via) {
		pcb_move_obj(PCB_OBJ_PSTK, via, via, via, x - via->x, y - via->y);
		dprintf("via move %#mD to %#mD\n", via->x, via->y, x, y);
	}
	for (i = 0; i < c->n_lines; i++) {
		pcb_line_t *tl = c->lines[i]->line;
		if (tl) {
			if (c->lines[i]->s == c) {
				pcb_move_obj(PCB_OBJ_LINE_POINT, pcb_get_layer(PCB->Data, c->lines[i]->layer), tl, &tl->Point1, x - (tl->Point1.X), y - (tl->Point1.Y));
			}
			else {
				pcb_move_obj(PCB_OBJ_LINE_POINT, pcb_get_layer(PCB->Data, c->lines[i]->layer), tl, &tl->Point2, x - (tl->Point2.X), y - (tl->Point2.Y));
			}
			dprintf("Line %p moved to %#mD %#mD\n", (void *) tl, tl->Point1.X, tl->Point1.Y, tl->Point2.X, tl->Point2.Y);
		}
	}
	if (pad && pad != c)
		merge_corners(c, pad);
	else
		for (i = 0; i < c->n_lines; i++) {
			if (c->lines[i]->s->x == c->lines[i]->e->x && c->lines[i]->s->y == c->lines[i]->e->y) {
				corner_s *c2 = other_corner(c->lines[i], c);
				dprintf("move_corner: removing line %#mD %#mD %p %p\n", c->x, c->y, c2->x, c2->y, (void *) c, (void *) c2);

				remove_line(c->lines[i]);
				if (c != c2)
					merge_corners(c, c2);
				check(c, 0);
				i--;
				break;
			}
		}
	rnd_hid_progress(0, 0, 0);
	check(c, 0);
}

static int any_line_selected()
{
	line_s *l;
	for (l = lines; l; l = l->next) {
		if (DELETED(l))
			continue;
		if (l->line && selected(l->line))
			return 1;
	}
	return 0;
}

static int trim_step(int s, int l1, int l2)
{
	dprintf("trim %d %d %d\n", s, l1, l2);
	if (s > l1)
		s = l1;
	if (s > l2)
		s = l2;
	if (s != l1 && s != l2)
		s = gridsnap(s);
	return s;
}

static int canonicalize_line(line_s * l);

static int split_line(line_s * l, corner_s * c)
{
	int i;
	pcb_line_t *pcbline;
	line_s *ls;

	if (!intersecting_layers(l->layer, c->layer))
		return 0;
	if (l->is_pad)
		return 0;
	if (c->pad) {
		dprintf("split on pad!\n");
		if (l->s->pad == c->pad || l->e->pad == c->pad)
			return 0;
		dprintf("splitting...\n");
	}

	check(c, l);
	pcbline = create_pcb_line(l->layer, c->x, c->y, l->e->x, l->e->y, l->line->Thickness, l->line->Clearance, l->line->Flags);
	if (pcbline == 0)
		return 0;										/* already a line there */

	check(c, l);

	dprintf("split line from %#mD to %#mD at %#mD\n", l->s->x, l->s->y, l->e->x, l->e->y, c->x, c->y);
	ls = (line_s *) malloc(sizeof(line_s));

	ls->next = lines;
	lines = ls;
	ls->is_pad = 0;
	ls->s = c;
	ls->e = l->e;
	ls->line = pcbline;
	ls->layer = l->layer;
	for (i = 0; i < l->e->n_lines; i++)
		if (l->e->lines[i] == l)
			l->e->lines[i] = ls;
	l->e = c;
	add_line_to_corner(l, c);
	add_line_to_corner(ls, c);

	pcb_move_obj(PCB_OBJ_LINE_POINT, pcb_get_layer(PCB->Data, l->layer), l->line, &l->line->Point2,
						 c->x - (l->line->Point2.X), c->y - (l->line->Point2.Y));

	return 1;
}

static int canonicalize_line(line_s * l)
{
	/* This could be faster */
	corner_s *c;
	if (l->s->x == l->e->x) {
		int y1 = l->s->y;
		int y2 = l->e->y;
		int x1 = l->s->x - l->line->Thickness / 2;
		int x2 = l->s->x + l->line->Thickness / 2;
		if (y1 > y2) {
			int t = y1;
			y1 = y2;
			y2 = t;
		}
		for (c = corners; c; c = c->next) {
			if (DELETED(c))
				continue;
			if ((y1 < c->y && c->y < y2)
					&& intersecting_layers(l->layer, c->layer)) {
				if (c->x != l->s->x && c->x < x2 && c->x > x1 && !(c->pad || c->pin)) {
					move_corner(c, l->s->x, c->y);
				}
				if (c->x == l->s->x) {
					/* FIXME: if the line is split, we have to re-canonicalize
					   both segments. */
					return split_line(l, c);
				}
			}
		}
	}
	else if (l->s->y == l->e->y) {
		int x1 = l->s->x;
		int x2 = l->e->x;
		int y1 = l->s->y - l->line->Thickness / 2;
		int y2 = l->s->y + l->line->Thickness / 2;
		if (x1 > x2) {
			int t = x1;
			x1 = x2;
			x2 = t;
		}
		for (c = corners; c; c = c->next) {
			if (DELETED(c))
				continue;
			if ((x1 < c->x && c->x < x2)
					&& intersecting_layers(l->layer, c->layer)) {
				if (c->y != l->s->y && c->y < y2 && c->y > y1 && !(c->pad || c->pin)) {
					move_corner(c, c->x, l->s->y);
				}
				if (c->y == l->s->y) {
					/* FIXME: Likewise.  */
					return split_line(l, c);
				}
			}
		}
	}
	else {
		/* diagonal lines.  Let's try to split them at pins/vias
		   anyway.  */
		int x1 = l->s->x;
		int x2 = l->e->x;
		int y1 = l->s->y;
		int y2 = l->e->y;
		if (x1 > x2) {
			int t = x1;
			x1 = x2;
			x2 = t;
		}
		if (y1 > y2) {
			int t = y1;
			y1 = y2;
			y2 = t;
		}
		for (c = corners; c; c = c->next) {
			if (DELETED(c))
				continue;
			if (!c->via && !c->pin)
				continue;
			if ((x1 < c->x && c->x < x2)
					&& (y1 < c->y && c->y < y2)
					&& intersecting_layers(l->layer, c->layer)) {
				pcb_pstk_t *p = c->pin ? c->pin : c->via;
				int th = djmax(p->BoundingBox.X2 - p->BoundingBox.X1, p->BoundingBox.Y2 - p->BoundingBox.Y1);
				th /= 2;
				if (dist(l->s->x, l->s->y, c->x, c->y) > th
						&& dist(l->e->x, l->e->y, c->x, c->y) > th && pcb_isc_pstk_line(pcb_find0, p, l->line, rnd_false)) {
					return split_line(l, c);
				}
			}
		}
	}
	return 0;
}

/* Make sure all vias are at line end points */
static int canonicalize_lines()
{
	int changes = 0;
	int count;
	line_s *l;
	while (1) {
		count = 0;
		for (l = lines; l; l = l->next) {
			if (DELETED(l))
				continue;
			count += canonicalize_line(l);
		}
		changes += count;
		if (count == 0)
			break;
	}
	return changes;
}

static int simple_optimize_corner(corner_s * c)
{
	int i;
	int rv = 0;

	check(c, 0);
	if (c->via) {
		/* see if no via is needed */
		if (selected(c->via))
			dprintf("via check: line[0] layer %d at %#mD nl %d\n", c->lines[0]->layer, c->x, c->y, c->n_lines);
		/* We can't delete vias that connect to power planes, or vias
		   have terminal name (assume they're test points).  */
		if (!PCB_FLAG_THERM_TEST_ANY(c->via) && (c->via->term == NULL)) {
			for (i = 1; i < c->n_lines; i++) {
				if (selected(c->via))
					dprintf("           line[%d] layer %d %#mD to %#mD\n",
									i, c->lines[i]->layer, c->lines[i]->s->x, c->lines[i]->s->y, c->lines[i]->e->x, c->lines[i]->e->y);
				if (c->lines[i]->layer != c->lines[0]->layer)
					break;
			}
			if (i == c->n_lines) {
				if (selected(c->via))
					dprintf("           remove it\n");
				remove_via_at(c);
				rv++;
			}
		}
	}

	check(c, 0);
	if (c->n_lines == 2 && !c->via) {
		/* see if it is an unneeded corner */
		int o = line_orient(c->lines[0], c);
		corner_s *c2 = other_corner(c->lines[1], c);
		corner_s *c0 = other_corner(c->lines[0], c);
		if (o == line_orient(c->lines[1], c2) && o != DIAGONAL) {
			dprintf("straight %#mD to %#mD to %#mD\n", c0->x, c0->y, c->x, c->y, c2->x, c2->y);
			if (selected(c->lines[0]->line))
				PCB_FLAG_SET(PCB_FLAG_SELECTED, c->lines[1]->line);
			if (selected(c->lines[1]->line))
				PCB_FLAG_SET(PCB_FLAG_SELECTED, c->lines[0]->line);
			move_corner(c, c2->x, c2->y);
		}
	}
	check(c, 0);
	if (c->n_lines == 1 && !c->via) {
		corner_s *c0 = other_corner(c->lines[0], c);
		if (abs(c->x - c0->x) + abs(c->y - c0->y) <= LONGEST_FRECKLE) {
			/*
			 * Remove this line, as it is a "freckle".  A freckle is an extremely
			 * short line (around 0.01 thou) that is unconnected at one end.
			 * Freckles are almost insignificantly small, but are annoying as
			 * they prevent the mitering optimiser from working.
			 * Freckles sometimes arise because of a bug in the autorouter that
			 * causes it to create small overshoots (typically 0.01 thou) at the
			 * intersections of vertical and horizontal lines. These overshoots
			 * are converted to freckles as a side effect of canonicalize_line().
			 * Note that canonicalize_line() is not at fault, the bug is in the
			 * autorouter creating overshoots.
			 * The autorouter bug arose some time between the 20080202 and 20091103
			 * releases.
			 * This code is probably worth keeping even when the autorouter bug is
			 * fixed, as "freckles" could conceivably arise in other ways.
			 */
			dprintf("freckle %#mD to %#mD\n", c->x, c->y, c0->x, c0->y);
			move_corner(c, c0->x, c0->y);
		}
	}
	check(c, 0);
	return rv;
}

/* We always run these */
static int simple_optimizations()
{
	corner_s *c;
	int rv = 0;

	/* Look for corners that aren't */
	for (c = corners; c; c = c->next) {
		if (DELETED(c))
			continue;
		if (c->pad || c->pin)
			continue;
		rv += simple_optimize_corner(c);
	}
	return rv;
}

static int is_hole(corner_s * c)
{
	return c->pin || c->pad || c->via;
}

static int orthopull_1(corner_s * c, int fdir, int rdir, int any_sel)
{
	static corner_s **cs = 0;
	static int cm = 0;
	static line_s **ls = 0;
	static int lm = 0;
	int i, li, ln, cn, snap;
	line_s *l = 0;
	corner_s *c2, *cb;
	int adir = 0, sdir = 0, pull;
	int saw_sel = 0, saw_auto = 0;
	int max, len = 0, r1 = 0, r2;
	rect_s rr;
	int edir = 0, done;

	if (cs == 0) {
		cs = (corner_s **) malloc(10 * sizeof(corner_s));
		cm = 10;
		ls = (line_s **) malloc(10 * sizeof(line_s));
		lm = 10;
	}

	for (i = 0; i < c->n_lines; i++) {
		int o = line_orient(c->lines[i], c);
		if (o == rdir)
			return 0;
	}

	switch (fdir) {
	case RIGHT:
		adir = DOWN;
		sdir = UP;
		break;
	case DOWN:
		adir = RIGHT;
		sdir = LEFT;
		break;
	default:
		dj_abort("fdir not right or down\n");
	}

	c2 = c;
	cn = 0;
	ln = 0;
	pull = 0;
	while (c2) {
		if (c2->pad || c2->pin || c2->n_lines < 2)
			return 0;
		if (cn >= cm) {
			cm = cn + 10;
			cs = (corner_s **) realloc(cs, cm * sizeof(corner_s));
		}
		cs[cn++] = c2;
		r2 = corner_radius(c2);
		if (r1 < r2)
			r1 = r2;
		l = 0;
		for (i = 0; i < c2->n_lines; i++) {
			int o = line_orient(c2->lines[i], c2);
			if (o == DIAGONAL)
				return 0;
			if (o == fdir) {
				if (l)
					return 0;							/* we don't support overlapping lines yet */
				l = c2->lines[i];
			}
			if (o == rdir && c2->lines[i] != ls[ln - 1])
				return 0;								/* likewise */
			if (o == adir)
				pull++;
			if (o == sdir)
				pull--;
		}
		if (!l)
			break;
		if (selected(l->line))
			saw_sel = 1;
		if (autorouted(l->line))
			saw_auto = 1;
		if (ln >= lm) {
			lm = ln + 10;
			ls = (line_s **) realloc(ls, lm * sizeof(line_s));
		}
		ls[ln++] = l;
		c2 = other_corner(l, c2);
	}
	if (cn < 2 || pull == 0)
		return 0;
	if (any_sel && !saw_sel)
		return 0;
	if (!any_sel && conf_djopt.plugins.djopt.auto_only && !saw_auto)
		return 0;

	/* Ok, now look for other blockages. */

	empty_rect(&rr);
	add_point_to_rect(&rr, c->x, c->y, corner_radius(c));
	add_point_to_rect(&rr, c2->x, c2->y, corner_radius(c2));

	if (fdir == RIGHT && pull < 0)
		edir = UP;
	else if (fdir == RIGHT && pull > 0)
		edir = DOWN;
	else if (fdir == DOWN && pull < 0)
		edir = LEFT;
	else if (fdir == DOWN && pull > 0)
		edir = RIGHT;

	max = -1;
	for (i = 0; i < cn; i++)
		for (li = 0; li < cs[i]->n_lines; li++) {
			if (line_orient(cs[i]->lines[li], cs[i]) != edir)
				continue;
			len = line_length(cs[i]->lines[li]);
			if (max > len || max == -1)
				max = len;
		}
	dprintf("c %s %4#mD  cn %d pull %3d  max %4#mS\n", fdir == RIGHT ? "right" : "down ", c->x, c->y, cn, pull, max);

	switch (edir) {
	case UP:
		rr.y1 = c->y - r1 - max;
		break;
	case DOWN:
		rr.y2 = c->y + r1 + max;
		break;
	case LEFT:
		rr.x1 = c->x - r1 - max;
		break;
	case RIGHT:
		rr.x2 = c->x + r1 + max;
		break;
	}
	rr.x1 -= SB + 1;
	rr.x2 += SB + 1;
	rr.y1 -= SB + 1;
	rr.y2 += SB + 1;

	snap = 0;
	for (cb = corners; cb; cb = cb->next) {
		int sep;
		if (DELETED(cb))
			continue;
		r1 = corner_radius(cb);
		if (cb->net == c->net && !cb->pad)
			continue;
		if (!pin_in_rect(&rr, cb->x, cb->y, r1))
			continue;
		switch (edir) {
#define ECHK(X,Y,LT) \
	  for (i=0; i<cn; i++) \
	    { \
	      if (!intersecting_layers(cs[i]->layer, cb->layer)) \
		continue; \
	      r2 = corner_radius(cs[i]); \
	      if (cb->X + r1 <= cs[i]->X - r2 - SB - 1) \
		continue; \
	      if (cb->X - r1 >= cs[i]->X + r2 + SB + 1) \
		continue; \
	      if (cb->Y LT cs[i]->Y) \
		continue; \
	      sep = djabs(cb->Y - cs[i]->Y) - r1 - r2 - SB - 1; \
	      if (max > sep) \
		{ max = sep; snap = 1; }\
	    } \
	  for (i=0; i<ln; i++) \
	    { \
	      if (!intersecting_layers(ls[i]->layer, cb->layer)) \
		continue; \
	      if (cb->X <= cs[i]->X || cb->X >= cs[i+1]->X) \
		continue; \
	      sep = (djabs(cb->Y - cs[i]->Y) - ls[i]->line->Thickness/2 \
		     - r1 - SB - 1); \
	      if (max > sep) \
		{ max = sep; snap = 1; }\
	    }
		case UP:
			ECHK(x, y, >=);
			break;
		case DOWN:
			ECHK(x, y, <=);
			break;
		case LEFT:
			ECHK(y, x, >=);
			break;
		case RIGHT:
			ECHK(y, x, <=);
			break;
		}
	}

	/* We must now check every line segment against our corners.  */
	for (l = lines; l; l = l->next) {
		int o, x1, x2, y1, y2;
		if (DELETED(l))
			continue;
		dprintf("check line %#mD to %#mD\n", l->s->x, l->s->y, l->e->x, l->e->y);
		if (l->s->net == c->net) {
			dprintf("  same net\n");
			continue;
		}
		o = line_orient(l, 0);
		/* We don't need to check perpendicular lines, because their
		   corners already take care of it.  */
		if ((fdir == RIGHT && (o == UP || o == DOWN))
				|| (fdir == DOWN && (o == RIGHT || o == LEFT))) {
			dprintf("  perpendicular\n");
			continue;
		}

		/* Choose so that x1,y1 is closest to corner C */
		if ((fdir == RIGHT && l->s->x < l->e->x)
				|| (fdir == DOWN && l->s->y < l->e->y)) {
			x1 = l->s->x;
			y1 = l->s->y;
			x2 = l->e->x;
			y2 = l->e->y;
		}
		else {
			x1 = l->e->x;
			y1 = l->e->y;
			x2 = l->s->x;
			y2 = l->s->y;
		}

		/* Eliminate all lines outside our range */
		if ((fdir == RIGHT && (x2 < c->x || x1 > c2->x))
				|| (fdir == DOWN && (y2 < c->y || y1 > c2->y))) {
			dprintf("  outside our range\n");
			continue;
		}

		/* Eliminate all lines on the wrong side of us */
		if ((edir == UP && y1 > c->y && y2 > c->y)
				|| (edir == DOWN && y1 < c->y && y2 < c->y)
				|| (edir == LEFT && x1 > c->x && x2 > c->x)
				|| (edir == RIGHT && x1 < c->x && x2 < c->x)) {
			dprintf("  wrong side\n");
			continue;
		}

		/* For now, cheat on diagonals */
		switch (edir) {
		case RIGHT:
			if (x1 > x2)
				x1 = x2;
			break;
		case LEFT:
			if (x1 < x2)
				x1 = x2;
			break;
		case DOWN:
			if (y1 > y2)
				y1 = y2;
			break;
		case UP:
			if (y1 < y2)
				y1 = y2;
			break;
		}

		/* Ok, now see how far we can get for each of our corners. */
		for (i = 0; i < cn; i++) {
			int r = l->line->Thickness + SB + corner_radius(cs[i]) + 1;
			int len = 0;
			if ((fdir == RIGHT && (x2 < cs[i]->x || x1 > cs[i]->x))
					|| (fdir == DOWN && (y2 < cs[i]->y || y1 > cs[i]->y)))
				continue;
			if (!intersecting_layers(cs[i]->layer, l->layer))
				continue;
			switch (edir) {
			case RIGHT:
				len = x1 - c->x;
				break;
			case LEFT:
				len = c->x - x1;
				break;
			case DOWN:
				len = y1 - c->y;
				break;
			case UP:
				len = c->y - y1;
				break;
			}
			len -= r;
			dprintf("  len is %#mS vs corner at %#mD\n", len, cs[i]->x, cs[i]->y);
			if (len <= 0)
				return 0;
			if (max > len)
				max = len;
		}

	}

	/* We must make sure that if a segment isn't being completely
	   removed, that any vias and/or pads don't overlap.  */
	done = 0;
	while (!done) {
		done = 1;
		for (i = 0; i < cn; i++)
			for (li = 0; li < cs[i]->n_lines; li++) {
				line_s *l = cs[i]->lines[li];
				corner_s *oc = other_corner(l, cs[i]);
				if (line_orient(l, cs[i]) != edir)
					continue;
				len = line_length(l);
				if (!oc->pad || !cs[i]->via) {
					if (!is_hole(l->s) || !is_hole(l->e))
						continue;
					if (len == max)
						continue;
				}
				len -= corner_radius(l->s);
				len -= corner_radius(l->e);
				len -= SB + 1;
				if (max > len) {
					max = len;
					done = 0;
				}
			}
	}

	if (max <= 0)
		return 0;
	switch (edir) {
	case UP:
		len = c->y - max;
		break;
	case DOWN:
		len = c->y + max;
		break;
	case LEFT:
		len = c->x - max;
		break;
	case RIGHT:
		len = c->x + max;
		break;
	}
	if (snap && max > rnd_conf.editor.grid) {
		if (pull < 0)
			len += rnd_conf.editor.grid - 1;
		len = gridsnap(len);
	}
	if ((fdir == RIGHT && len == cs[0]->y) || (fdir == DOWN && len == cs[0]->x))
		return 0;
	for (i = 0; i < cn; i++) {
		if (fdir == RIGHT) {
			max = len - cs[i]->y;
			move_corner(cs[i], cs[i]->x, len);
		}
		else {
			max = len - cs[i]->x;
			move_corner(cs[i], len, cs[i]->y);
		}
	}
	return max * pull;
}

static int orthopull()
{
	/* Look for straight runs which could be moved to reduce total trace
	   length.  */
	int any_sel = any_line_selected();
	corner_s *c;
	int rv = 0;

	for (c = corners; c;) {
		if (DELETED(c))
			continue;
		if (c->pin || c->pad) {
			c = c->next;
			continue;
		}
		next_corner = c;
		rv += orthopull_1(c, RIGHT, LEFT, any_sel);
		if (c != next_corner) {
			c = next_corner;
			continue;
		}
		rv += orthopull_1(c, DOWN, UP, any_sel);
		if (c != next_corner) {
			c = next_corner;
			continue;
		}
		c = c->next;
	}
	if (rv)
		rnd_printf("orthopull: %ml mils saved\n", rv);
	return rv;
}

static int debumpify()
{
	/* Look for "U" shaped traces we can shorten (or eliminate) */
	int rv = 0;
	int any_selected = any_line_selected();
	line_s *l, *l1, *l2;
	corner_s *c, *c1, *c2;
	rect_s rr, rp;
	int o, o1, o2, step, w;
	for (l = lines; l; l = l->next) {
		if (DELETED(l))
			continue;
		if (!l->line)
			continue;
		if (any_selected && !selected(l->line))
			continue;
		if (!any_selected && conf_djopt.plugins.djopt.auto_only && !autorouted(l->line))
			continue;
		if (l->s->pin || l->s->pad || l->e->pin || l->e->pad)
			continue;
		o = line_orient(l, 0);
		if (o == DIAGONAL)
			continue;
		l1 = other_line(l->s, l);
		if (!l1)
			continue;
		o1 = line_orient(l1, l->s);
		l2 = other_line(l->e, l);
		if (!l2)
			continue;
		o2 = line_orient(l2, l->e);
		if (ORIENT(o) == ORIENT(o1) || o1 != o2 || o1 == DIAGONAL)
			continue;

		dprintf("\nline: %#mD to %#mD\n", l->s->x, l->s->y, l->e->x, l->e->y);
		w = l->line->Thickness / 2 + SB + 1;
		empty_rect(&rr);
		add_line_to_rect(&rr, l1);
		add_line_to_rect(&rr, l2);
		if (rr.x1 != l->s->x && rr.x1 != l->e->x)
			rr.x1 -= w;
		if (rr.x2 != l->s->x && rr.x2 != l->e->x)
			rr.x2 += w;
		if (rr.y1 != l->s->y && rr.y1 != l->e->y)
			rr.y1 -= w;
		if (rr.y2 != l->s->y && rr.y2 != l->e->y)
			rr.y2 += w;
		dprintf("range: x %#mS..%#mS y %#mS..%#mS\n", rr.x1, rr.x2, rr.y1, rr.y2);

		c1 = other_corner(l1, l->s);
		c2 = other_corner(l2, l->e);

		empty_rect(&rp);
		for (c = corners; c; c = c->next) {
			if (DELETED(c))
				continue;
			if (c->net != l->s->net && intersecting_layers(c->layer, l->s->layer))
				add_corner_to_rect_if(&rp, c, &rr);
		}
		if (rp.x1 == INT_MAX) {
			rp.x1 = rr.x2;
			rp.x2 = rr.x1;
			rp.y1 = rr.y2;
			rp.y2 = rr.y1;
		}
		dprintf("pin r: x %#mS..%#mS y %#mS..%#mS\n", rp.x1, rp.x2, rp.y1, rp.y2);

		switch (o1) {
		case LEFT:
			step = l->s->x - rp.x2 - w;
			step = gridsnap(step);
			if (step > l->s->x - c1->x)
				step = l->s->x - c1->x;
			if (step > l->s->x - c2->x)
				step = l->s->x - c2->x;
			if (step > 0) {
				dprintf("left step %#mS at %#mD\n", step, l->s->x, l->s->y);
				move_corner(l->s, l->s->x - step, l->s->y);
				move_corner(l->e, l->e->x - step, l->e->y);
				rv += step;
			}
			break;
		case RIGHT:
			step = rp.x1 - l->s->x - w;
			step = gridsnap(step);
			if (step > c1->x - l->s->x)
				step = c1->x - l->s->x;
			if (step > c2->x - l->s->x)
				step = c2->x - l->s->x;
			if (step > 0) {
				dprintf("right step %#mS at %#mD\n", step, l->s->x, l->s->y);
				move_corner(l->s, l->s->x + step, l->s->y);
				move_corner(l->e, l->e->x + step, l->e->y);
				rv += step;
			}
			break;
		case UP:
			if (rp.y2 == INT_MIN)
				rp.y2 = rr.y1;
			step = trim_step(l->s->y - rp.y2 - w, l->s->y - c1->y, l->s->y - c2->y);
			if (step > 0) {
				dprintf("up step %#mS at %#mD\n", step, l->s->x, l->s->y);
				move_corner(l->s, l->s->x, l->s->y - step);
				move_corner(l->e, l->e->x, l->e->y - step);
				rv += step;
			}
			break;
		case DOWN:
			step = rp.y1 - l->s->y - w;
			step = gridsnap(step);
			if (step > c1->y - l->s->y)
				step = c1->y - l->s->y;
			if (step > c2->y - l->s->y)
				step = c2->y - l->s->y;
			if (step > 0) {
				dprintf("down step %#mS at %#mD\n", step, l->s->x, l->s->y);
				move_corner(l->s, l->s->x, l->s->y + step);
				move_corner(l->e, l->e->x, l->e->y + step);
				rv += step;
			}
			break;
		}
		check(0, l);
	}

	rv += simple_optimizations();
	if (rv)
		rnd_printf("debumpify: %ml mils saved\n", rv / 50);
	return rv;
}

static int simple_corner(corner_s * c)
{
	int o1, o2;
	if (c->pad || c->pin || c->via)
		return 0;
	if (c->n_lines != 2)
		return 0;
	o1 = line_orient(c->lines[0], c);
	o2 = line_orient(c->lines[1], c);
	if (ORIENT(o1) == ORIENT(o2))
		return 0;
	if (ORIENT(o1) == DIAGONAL || ORIENT(o2) == DIAGONAL)
		return 0;
	return 1;
}

static int unjaggy_once()
{
	/* Look for sequences of simple corners we can reduce. */
	int rv = 0;
	corner_s *c, *c0, *c1, *cc;
	int l, w, sel = any_line_selected();
	int o0, o1, s0, s1;
	rect_s rr, rp;
	for (c = corners; c; c = c->next) {
		if (DELETED(c))
			continue;
		if (!simple_corner(c))
			continue;
		if (!c->lines[0]->line || !c->lines[1]->line)
			continue;
		if (sel && !(selected(c->lines[0]->line)
								 || selected(c->lines[1]->line)))
			continue;
		if (!sel && conf_djopt.plugins.djopt.auto_only && !(autorouted(c->lines[0]->line)
																		 || autorouted(c->lines[1]->line)))
			continue;
		dprintf("simple at %#mD\n", c->x, c->y);

		c0 = other_corner(c->lines[0], c);
		o0 = line_orient(c->lines[0], c);
		s0 = simple_corner(c0);

		c1 = other_corner(c->lines[1], c);
		o1 = line_orient(c->lines[1], c);
		s1 = simple_corner(c1);

		if (!s0 && !s1)
			continue;
		dprintf("simples at %#mD\n", c->x, c->y);

		w = 1;
		for (l = 0; l < c0->n_lines; l++)
			if (c0->lines[l] != c->lines[0]
					&& c0->lines[l]->layer == c->lines[0]->layer) {
				int o = line_orient(c0->lines[l], c0);
				if (o == o1)
					w = 0;
			}
		for (l = 0; l < c1->n_lines; l++)
			if (c1->lines[l] != c->lines[0]
					&& c1->lines[l]->layer == c->lines[0]->layer) {
				int o = line_orient(c1->lines[l], c1);
				if (o == o0)
					w = 0;
			}
		if (!w)
			continue;
		dprintf("orient ok\n");

		w = c->lines[0]->line->Thickness / 2 + SB + 1;
		empty_rect(&rr);
		add_line_to_rect(&rr, c->lines[0]);
		add_line_to_rect(&rr, c->lines[1]);
		if (c->x != rr.x1)
			rr.x1 -= w;
		else
			rr.x2 += w;
		if (c->y != rr.y1)
			rr.y1 -= w;
		else
			rr.y2 += w;

		empty_rect(&rp);
		for (cc = corners; cc; cc = cc->next) {
			if (DELETED(cc))
				continue;
			if (cc->net != c->net && intersecting_layers(cc->layer, c->layer))
				add_corner_to_rect_if(&rp, cc, &rr);
		}
		dprintf("rp x %#mS..%#mS  y %#mS..%#mS\n", rp.x1, rp.x2, rp.y1, rp.y2);
		if (rp.x1 <= rp.x2)					/* something triggered */
			continue;

		dprintf("unjaggy at %#mD layer %d\n", c->x, c->y, c->layer);
		if (c->x == c0->x)
			move_corner(c, c1->x, c0->y);
		else
			move_corner(c, c0->x, c1->y);
		rv++;
		check(c, 0);
	}
	rv += simple_optimizations();
	check(c, 0);
	return rv;
}

static int unjaggy()
{
	int i, r = 0, j;
	for (i = 0; i < 100; i++) {
		j = unjaggy_once();
		if (j == 0)
			break;
		r += j;
	}
	if (r)
		printf("%d unjagg%s    \n", r, r == 1 ? "y" : "ies");
	return r;
}

static int vianudge()
{
	/* Look for vias with all lines leaving the same way, try to nudge
	   via to eliminate one or more of them. */
	int rv = 0;
	corner_s *c, *c2, *c3;
	line_s *l;
	unsigned char directions[PCB_MAX_LAYER];
	unsigned char counts[PCB_MAX_LAYER];

	memset(directions, 0, sizeof(directions));
	memset(counts, 0, sizeof(counts));

	for (c = corners; c; c = c->next) {
		int o, i, vr, cr, oboth;
		int len = 0, saved = 0;

		if (DELETED(c))
			continue;

		if (!c->via)
			continue;

		memset(directions, 0, sizeof(directions));
		memset(counts, 0, sizeof(counts));

		for (i = 0; i < c->n_lines; i++) {
			o = line_orient(c->lines[i], c);
			counts[c->lines[i]->layer]++;
			directions[c->lines[i]->layer] |= o;
		}
		for (o = 0, i = 0; i < pcb_max_layer(PCB); i++) {
			if (!(pcb_layer_flags(PCB, i) & PCB_LYT_COPPER))
				continue;
			if (counts[i] == 1) {
				o = directions[i];
				break;
			}
		}
		switch (o) {
		case LEFT:
		case RIGHT:
			oboth = LEFT | RIGHT;
			break;
		case UP:
		case DOWN:
			oboth = UP | DOWN;
			break;
		default:
			continue;
		}
		for (i = 0; i < pcb_max_layer(PCB); i++) {
			if (!(pcb_layer_flags(PCB, i) & PCB_LYT_COPPER))
				continue;
			if (counts[i] && directions[i] != o && directions[i] != oboth)
				goto vianudge_continue;
		}
		c2 = 0;
		for (i = 0; i < c->n_lines; i++) {
			int ll = line_length(c->lines[i]);
			if (line_orient(c->lines[i], c) != o) {
				saved--;
				continue;
			}
			saved++;
			if (c2 == 0 || len > ll) {
				len = ll;
				c2 = other_corner(c->lines[i], c);
			}
		}
		if (c2->pad || c2->pin || c2->via)
			continue;

		/* Now look for clearance in the new position */
		vr = djmax(c->via->BoundingBox.X2 - c->via->BoundingBox.X1, c->via->BoundingBox.Y2 - c->via->BoundingBox.Y1);
		vr = vr / 2 + SB + 1;
		for (c3 = corners; c3; c3 = c3->next) {
			if (DELETED(c3))
				continue;
			if ((c3->net != c->net && (c3->pin || c3->via)) || c3->pad) {
				cr = corner_radius(c3);
				if (dist(c2->x, c2->y, c3->x, c3->y) < vr + cr)
					goto vianudge_continue;
			}
		}
		for (l = lines; l; l = l->next) {
			if (DELETED(l))
				continue;
			if (l->s->net != c->net) {
				int ld = dist_line_to_point(l, c2);
				if (ld < l->line->Thickness / 2 + vr)
					goto vianudge_continue;
			}
		}

		/* at this point, we know we can move it */

		dprintf("vianudge: nudging via at %#mD by %#mS saving %#mS\n", c->x, c->y, len, saved);
		rv += len * saved;
		move_corner(c, c2->x, c2->y);

		check(c, 0);

	vianudge_continue:
		continue;
	}

	if (rv)
		rnd_printf("vianudge: %ml mils saved\n", rv);
	return rv;
}

static int viatrim()
{
	/* Look for traces that can be moved to the other side of the board,
	   to reduce the number of vias needed.  For now, we look for simple
	   lines, not multi-segmented lines.  */
	line_s *l, *l2;
	int i, rv = 0, vrm = 0;
	int any_sel = any_line_selected();

	for (l = lines; l; l = l->next) {
		rect_s r;
		int my_layer, other_layer;

		if (DELETED(l))
			continue;
		if (!l->s->via)
			continue;
		if (!l->e->via)
			continue;
		if (any_sel && !selected(l->line))
			continue;
		if (!any_sel && conf_djopt.plugins.djopt.auto_only && !autorouted(l->line))
			continue;

		my_layer = l->layer;
		other_layer = -1;
		dprintf("line %p on layer %d from %#mD to %#mD\n", (void *) l, l->layer, l->s->x, l->s->y, l->e->x, l->e->y);
		for (i = 0; i < l->s->n_lines; i++)
			if (l->s->lines[i] != l) {
				if (other_layer == -1) {
					other_layer = l->s->lines[i]->layer;
					dprintf("noting other line %p on layer %d\n", (void *) (l->s->lines[i]), my_layer);
				}
				else if (l->s->lines[i]->layer != other_layer) {
					dprintf("saw other line %p on layer %d (not %d)\n", (void *) (l->s->lines[i]), l->s->lines[i]->layer, my_layer);
					other_layer = -1;
					goto viatrim_other_corner;
				}
			}
	viatrim_other_corner:
		if (other_layer == -1)
			for (i = 0; i < l->e->n_lines; i++)
				if (l->e->lines[i] != l) {
					if (other_layer == -1) {
						other_layer = l->s->lines[i]->layer;
						dprintf("noting other line %p on layer %d\n", (void *) (l->s->lines[i]), my_layer);
					}
					else if (l->e->lines[i]->layer != other_layer) {
						dprintf("saw end line on layer %d (not %d)\n", l->e->lines[i]->layer, other_layer);
						goto viatrim_continue;
					}
				}

		/* Now see if any other line intersects us.  We don't need to
		   check corners, because they'd either be pins/vias and
		   already conflict, or pads, which we'll check here anyway.  */
		empty_rect(&r);
		add_point_to_rect(&r, l->s->x, l->s->y, l->line->Thickness);
		add_point_to_rect(&r, l->e->x, l->e->y, l->line->Thickness);

		for (l2 = lines; l2; l2 = l2->next) {
			if (DELETED(l2))
				continue;
			if (l2->s->net != l->s->net && l2->layer == other_layer) {
				dprintf("checking other line %#mD to %#mD\n", l2->s->x, l2->s->y, l2->e->x, l2->e->y);
				if (line_in_rect(&r, l2)) {
					dprintf("line from %#mD to %#mD in the way\n", l2->s->x, l2->s->y, l2->e->x, l2->e->y);
					goto viatrim_continue;
				}
			}
		}

		if (l->layer == other_layer)
			continue;
		move_line_to_layer(l, other_layer);
		rv++;

	viatrim_continue:
		continue;
	}
	vrm = simple_optimizations();
	if (rv > 0)
		printf("viatrim: %d traces moved, %d vias removed\n", rv, vrm);
	return rv + vrm;
}

static int automagic()
{
	int more = 1, oldmore = 0;
	int toomany = 100;
	while (more != oldmore && --toomany) {
		oldmore = more;
		more += debumpify();
		more += unjaggy();
		more += orthopull();
		more += vianudge();
		more += viatrim();
	}
	return more - 1;
}

static int miter()
{
	corner_s *c;
	int done, progress;
	int sel = any_line_selected();
	int saved = 0;

	for (c = corners; c; c = c->next) {
		if (DELETED(c))
			continue;
		c->miter = 0;
		if (c->n_lines == 2 && !c->via && !c->pin) {
			int o1 = line_orient(c->lines[0], c);
			int o2 = line_orient(c->lines[1], c);
			if (ORIENT(o1) != ORIENT(o2)
					&& o1 != DIAGONAL && o2 != DIAGONAL && c->lines[0]->line->Thickness == c->lines[1]->line->Thickness)
				c->miter = -1;
		}
	}

	done = 0;
	progress = 1;
	while (!done && progress) {
		done = 1;
		progress = 0;
		for (c = corners; c; c = c->next) {
			if (DELETED(c))
				continue;
			if (c->miter == -1) {
				int max = line_length(c->lines[0]);
				int len = line_length(c->lines[1]);
				int bloat;
				int ref, dist;
				corner_s *closest_corner = 0, *c2, *oc1, *oc2;
				int mx = 0, my = 0, x, y;
				int o1 = line_orient(c->lines[0], c);
				int o2 = line_orient(c->lines[1], c);

				if (c->pad || c->pin || c->via) {
					c->miter = 0;
					progress = 1;
					continue;
				}

				oc1 = other_corner(c->lines[0], c);
				oc2 = other_corner(c->lines[1], c);
#if 0
				if (oc1->pad)
					oc1 = 0;
				if (oc2->pad)
					oc2 = 0;
#endif

				if ((sel && !(selected(c->lines[0]->line)
											|| selected(c->lines[1]->line)))
						|| (!sel && conf_djopt.plugins.djopt.auto_only && !(autorouted(c->lines[0]->line)
																						 || autorouted(c->lines[1]->line)))) {
					c->miter = 0;
					progress = 1;
					continue;
				}

				if (max > len)
					max = len;
				switch (o1) {
				case LEFT:
					mx = -1;
					break;
				case RIGHT:
					mx = 1;
					break;
				case UP:
					my = -1;
					break;
				case DOWN:
					my = 1;
					break;
				}
				switch (o2) {
				case LEFT:
					mx = -1;
					break;
				case RIGHT:
					mx = 1;
					break;
				case UP:
					my = -1;
					break;
				case DOWN:
					my = 1;
					break;
				}
				ref = c->x * mx + c->y * my;
				dist = max;

				bloat = (c->lines[0]->line->Thickness / 2 + SB + 1) * 3 / 2;

				for (c2 = corners; c2; c2 = c2->next) {
					if (DELETED(c2))
						continue;
					if (c2 != c && c2 != oc1 && c2 != oc2
							&& c->x * mx <= c2->x * mx
							&& c->y * my <= c2->y * my && c->net != c2->net && intersecting_layers(c->layer, c2->layer)) {
						int cr = corner_radius(c2);
						len = c2->x * mx + c2->y * my - ref - cr - bloat;
						if (c->x != c2->x && c->y != c2->y)
							len -= cr;
						if (len < dist || (len == dist && c->miter != -1)) {
							dist = len;
							closest_corner = c2;
						}
					}
				}

				if (closest_corner && closest_corner->miter == -1) {
					done = 0;
					continue;
				}

#if 0
				if (dist < rnd_conf.editor.grid) {
					c->miter = 0;
					progress = 1;
					continue;
				}

				dist -= dist % rnd_conf.editor.grid;
#endif
				if (dist <= 0) {
					c->miter = 0;
					progress = 1;
					continue;
				}

				x = c->x;
				y = c->y;
				switch (o1) {
				case LEFT:
					x -= dist;
					break;
				case RIGHT:
					x += dist;
					break;
				case UP:
					y -= dist;
					break;
				case DOWN:
					y += dist;
					break;
				}
				c2 = find_corner(x, y, c->layer);
				if (c2 != other_corner(c->lines[0], c))
					split_line(c->lines[0], c2);
				x = c->x;
				y = c->y;
				switch (o2) {
				case LEFT:
					x -= dist;
					break;
				case RIGHT:
					x += dist;
					break;
				case UP:
					y -= dist;
					break;
				case DOWN:
					y += dist;
					break;
				}
				move_corner(c, x, y);
				c->miter = 0;
				c2->miter = 0;
				progress = 1;
				saved++;
			}
		}
	}
	return saved;
}

static void classify_corner(corner_s * c, int this_net)
{
	int i;
	if (c->net == this_net)
		return;
	c->net = this_net;
	for (i = 0; i < c->n_lines; i++)
		classify_corner(other_corner(c->lines[i], c), this_net);
}

static void classify_nets()
{
	static int this_net = 1;
	corner_s *c;

	for (c = corners; c; c = c->next) {
		if (DELETED(c))
			continue;
		if (c->net)
			continue;
		classify_corner(c, this_net);
		this_net++;
	}
}

#if 0
static void nudge_corner(corner_s * c, int dx, int dy, corner_s * prev_corner)
{
	int ox = c->x;
	int oy = c->y;
	int l;
	if (prev_corner && (c->pin || c->pad))
		return;
	move_corner(c, ox + dx, oy + dy);
	for (l = 0; l < c->n_lines; l++) {
		corner_s *oc = other_corner(c->lines[l], c);
		if (oc == prev_corner)
			continue;
		if (dx && oc->x == ox)
			nudge_corner(oc, dx, 0, c);
		if (dy && oc->y == oy)
			nudge_corner(oc, 0, dy, c);
	}
}
#endif

static line_s *choose_example_line(corner_s * c1, corner_s * c2)
{
	int ci, li;
	corner_s *c[2];
	c[0] = c1;
	c[1] = c2;
	dprintf("choose_example_line\n");
	for (ci = 0; ci < 2; ci++)
		for (li = 0; li < c[ci]->n_lines; li++) {
			dprintf("  try[%d,%d] \033[36m<%#mD-%#mD t%#mS c%#mS f%s>\033[0m\n",
							ci, li,
							c[ci]->lines[li]->s->x, c[ci]->lines[li]->s->y,
							c[ci]->lines[li]->e->x, c[ci]->lines[li]->e->y,
							c[ci]->lines[li]->line->Thickness,
							c[ci]->lines[li]->line->Clearance, pcb_strflg_f2s(c[ci]->lines[li]->line->Flags, PCB_OBJ_LINE, NULL, 0));
			/* Pads are disqualified, as we want to mimic a trace line. */
			if (c[ci]->lines[li]->line == (pcb_line_t *) c[ci]->pad) {
				dprintf("  bad, pad\n");
				continue;
			}
			/* Lines on layers that don't connect to the other pad are bad too.  */
			if (!intersecting_layers(c[ci]->lines[li]->layer, c[1 - ci]->layer)) {
				dprintf("  bad, layers\n");
				continue;
			}
			dprintf("  good\n");
			return c[ci]->lines[li];
		}
	dprintf("choose_example_line: none found!\n");
	return 0;
}

static int connect_corners(corner_s * c1, corner_s * c2)
{
	int layer;
	line_s *ex = choose_example_line(c1, c2);
	pcb_line_t *example = ex->line;

	dprintf
		("connect_corners \033[32m%#mD to %#mD, example line %#mD to %#mD l%d\033[0m\n",
		 c1->x, c1->y, c2->x, c2->y, ex->s->x, ex->s->y, ex->e->x, ex->e->y, ex->layer);

	layer = ex->layer;

	/* Assume c1 is the moveable one.  */
	if (!(c1->pin || c1->pad || c1->via) && c1->n_lines == 1) {
		int nx, ny;
		/* Extend the line */
		if (c1->lines[0]->s->x == c1->lines[0]->e->x)
			nx = c1->x, ny = c2->y;
		else
			nx = c2->x, ny = c1->y;
		if (nx != c2->x || ny != c2->y) {
			move_corner(c1, nx, ny);
			new_line(c1, c2, layer, example);
			return 1;
		}
		else {
			move_corner(c1, nx, ny);
			return 1;
		}
	}
	else {
		corner_s *nc = find_corner(c1->x, c2->y, layer);
		new_line(c1, nc, layer, example);
		new_line(nc, c2, layer, example);
		return 0;
	}
}

static void pinsnap()
{
	corner_s *c;
	int best_dist[PCB_MAX_LAYER + 1];
	corner_s *best_c[PCB_MAX_LAYER + 1];
	int l, got_one;
	int left = 0, right = 0, top = 0, bottom = 0;
	pcb_pstk_t *pin;
	int again = 1;

	int close = 0;
	corner_s *c2;

	memset(best_c, 0, sizeof(best_c));
	memset(best_dist, 0, sizeof(best_dist));

	/* Look for pins that have no connections.  See if there's a corner
	   close by that should be connected to it.  This usually happens
	   when the MUCS router needs to route to an off-grid pin.  */
	while (again) {
		again = 0;
		for (c = corners; c; c = c->next) {
			if (DELETED(c))
				continue;
			if (!(c->pin || c->via || c->pad))
				continue;

			pin = NULL;

			dprintf("\ncorner %s\n", corner_name(c));
			if (c->pin) pin = c->pin;
			if (c->via) pin = c->via;
			if (c->pad) pin = c->pad;
			left = pin->BoundingBox.X1;
			right = pin->BoundingBox.X2;
			top = pin->BoundingBox.Y1;
			bottom = pin->BoundingBox.Y2;

			dprintf("%s x %#mS-%#mS y %#mS-%#mS\n", corner_name(c), left, right, bottom, top);
			for (l = 0; l <= pcb_max_layer(PCB); l++) {
				if (!(pcb_layer_flags(PCB, l) & PCB_LYT_COPPER))
					continue;
				best_dist[l] = close * 2;
				best_c[l] = 0;
			}
			got_one = 0;
			for (c2 = corners; c2; c2 = c2->next) {
				int lt;
				rnd_coord_t thick;

				if (DELETED(c2))
					continue;
				thick = djmax(pin->BoundingBox.X2 - pin->BoundingBox.X1, pin->BoundingBox.Y2 - pin->BoundingBox.Y1);
				lt = corner_radius(c2);
				if (c2->n_lines && c2 != c && !(c2->pin || c2->pad || c2->via)
						&& intersecting_layers(c->layer, c2->layer)
						&& c2->x >= left - lt && c2->x <= right + lt && c2->y >= bottom - lt && c2->y <= top + lt) {
					int d = dist(c->x, c->y, c2->x, c2->y);
					if (pin && d > thick / 2 + lt)
						continue;
					if (c2->n_lines == 1) {
						got_one++;
						dprintf("found orphan %s vs %s\n", corner_name(c2), corner_name(c));
						connect_corners(c, c2);
						again = 1;
						continue;
					}
					if (best_c[c2->layer] == 0 || c2->n_lines < best_c[c2->layer]->n_lines || (d < best_dist[c2->layer]
																																										 && c2->n_lines <=
																																										 best_c[c2->layer]->n_lines)) {
						best_dist[c2->layer] = d;
						best_c[c2->layer] = c2;
						dprintf("layer %d best now %s\n", c2->layer, corner_name(c2));
					}
				}
				if (!got_one && c->n_lines == (c->pad ? 1 : 0)) {
					for (l = 0; l <= pcb_max_layer(PCB); l++)
						if (best_c[l])
							dprintf("best[%d] = %s\n", l, corner_name(best_c[l]));
					for (l = 0; l <= pcb_max_layer(PCB); l++)
						if (best_c[l]) {
							dprintf("move %s to %s\n", corner_name(best_c[l]), corner_name(c));
							connect_corners(best_c[l], c);
							again = 1;
							continue;
						}
				}
			}
		}
	}

	/* Now look for line ends that don't connect, see if they need to be
	   extended to intersect another line.  */
	for (c = corners; c; c = c->next) {
		line_s *l, *t;
		int lo;

		if (DELETED(c))
			continue;
		if (c->pin || c->via || c->pad)
			continue;
		if (c->n_lines != 1)
			continue;

		l = c->lines[0];
		lo = line_orient(l, c);
		dprintf("line end %#mD orient %d\n", c->x, c->y, lo);

		for (t = lines; t; t = t->next) {
			if (DELETED(t))
				continue;
			if (t->layer != c->lines[0]->layer)
				continue;
			switch (lo) {							/* remember, orient is for the line relative to the corner */
			case LEFT:
				if (t->s->x == t->e->x
						&& c->x < t->s->x
						&& t->s->x < c->x + (l->line->Thickness + t->line->Thickness) / 2 && ((t->s->y < c->y && c->y < t->e->y)
																																									|| (t->e->y < c->y && c->y < t->s->y))) {
					dprintf("found %#mD - %#mD\n", t->s->x, t->s->y, t->e->x, t->e->y);
					move_corner(c, t->s->x, c->y);
				}
				break;
			case RIGHT:
				if (t->s->x == t->e->x
						&& c->x > t->s->x
						&& t->s->x > c->x - (l->line->Thickness + t->line->Thickness) / 2 && ((t->s->y < c->y && c->y < t->e->y)
																																									|| (t->e->y < c->y && c->y < t->s->y))) {
					dprintf("found %#mD - %#mD\n", t->s->x, t->s->y, t->e->x, t->e->y);
					move_corner(c, t->s->x, c->y);
				}
				break;
			case UP:
				if (t->s->y == t->e->y
						&& c->y < t->s->y
						&& t->s->y < c->y + (l->line->Thickness + t->line->Thickness) / 2 && ((t->s->x < c->x && c->x < t->e->x)
																																									|| (t->e->x < c->x && c->x < t->s->x))) {
					dprintf("found %#mD - %#mD\n", t->s->x, t->s->y, t->e->x, t->e->y);
					move_corner(c, c->x, t->s->y);
				}
				break;
			case DOWN:
				if (t->s->y == t->e->y
						&& c->y > t->s->y
						&& t->s->y > c->y - (l->line->Thickness + t->line->Thickness) / 2 && ((t->s->x < c->x && c->x < t->e->x)
																																									|| (t->e->x < c->x && c->x < t->s->x))) {
					dprintf("found %#mD - %#mD\n", t->s->x, t->s->y, t->e->x, t->e->y);
					move_corner(c, c->x, t->s->y);
				}
				break;
			}
		}
	}
}

static int pstk_orient(pcb_pstk_t *p)
{
	rnd_coord_t dx = p->BoundingBox.X2 - p->BoundingBox.X1;
	rnd_coord_t dy = p->BoundingBox.Y2 - p->BoundingBox.Y1;
	if (dx < dy * 3)
		return O_VERT;
	if (dx > dy * 3)
		return O_HORIZ;
	return DIAGONAL;
}

static void padcleaner_(pcb_data_t *data)
{
	line_s *l, *nextl;
	rect_s r;

	dprintf("\ndj: padcleaner\n");
	for (l = lines; l; l = nextl) {
		nextl = l->next;

		if (l->is_pad)
			continue;

		if (DELETED(l))
			continue;

		dprintf("dj: line %p\n", (void *) l);
		check(0, l);

		if (l->s->pad && l->s->pad == l->e->pad)
			continue;

		PCB_PADSTACK_LOOP(data);
		{
			pcb_pstk_shape_t *shape = NULL;

			/* check if the padstack has copper on the given side */
			switch(layer_type[l->layer]) {
				case LT_SOLDER:     shape = pcb_pstk_shape(padstack, PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0); break;
				case LT_COMPONENT:  shape = pcb_pstk_shape(padstack, PCB_LYT_TOP | PCB_LYT_COPPER, 0); break;
			}
			if (shape == NULL)
				continue;

			empty_rect(&r);
			add_point_to_rect(&r, padstack->BoundingBox.X1, padstack->BoundingBox.Y1, 0);
			add_point_to_rect(&r, padstack->BoundingBox.X2, padstack->BoundingBox.Y2, 0);
			if (pin_in_rect(&r, l->s->x, l->s->y, 0)
					&& pin_in_rect(&r, l->e->x, l->e->y, 0)
					&& ORIENT(line_orient(l, 0)) == pstk_orient(padstack)) {
				dprintf
					("padcleaner %#mD-%#mD vs line %#mD-%#mD %#mS\n",
					 padstack->BoundingBox.X1, padstack->BoundingBox.Y1, padstack->BoundingBox.X2, padstack->BoundingBox.Y2,
					 l->s->x, l->s->y, l->e->x, l->e->y, l->line->Thickness);
				remove_line(l);
				goto next_line;
			}
		}
		PCB_END_LOOP;
	next_line:;
	}
}

static void padcleaner(void)
{
	padcleaner_(PCB->Data);
	PCB_SUBC_LOOP(PCB->Data); {
		padcleaner_(subc->data);
	} PCB_END_LOOP;
}


static void grok_layer_groups()
{
	int i, j, f;
	pcb_layer_stack_t *l = &(PCB->LayerGroups);

	solder_layer = component_layer = -1;
	for (i = 0; i < pcb_max_layer(PCB); i++) {
		layer_type[i] = 0;
		layer_groupings[i] = 0;
	}
	for (i = 0; i < pcb_max_group(PCB); i++) {
		f = 0;
		for (j = 0; j < l->grp[i].len; j++) {
			unsigned int lflg = pcb_layer_flags(PCB, l->grp[i].lid[j]);
			if (lflg & PCB_LYT_BOTTOM)
				f |= LT_SOLDER;
			if (lflg & PCB_LYT_TOP)
				f |= LT_COMPONENT;
		}
		for (j = 0; j < l->grp[i].len; j++) {
			if (l->grp[i].lid[j] < pcb_max_layer(PCB)) {
				layer_type[l->grp[i].lid[j]] |= f;
				layer_groupings[l->grp[i].lid[j]] = i;
				if (solder_layer == -1 && f == LT_SOLDER)
					solder_layer = l->grp[i].lid[j];
				if (component_layer == -1 && f == LT_COMPONENT)
					component_layer = l->grp[i].lid[j];
			}
		}
	}
}

static const char pcb_acts_DJopt[] = "djopt(debumpify|unjaggy|simple|vianudge|viatrim|orthopull)\n" "djopt(auto) - all of the above\n" "djopt(miter)";
static const char pcb_acth_DJopt[] = "Perform various optimizations on the current board.";
/* DOC: djopt.html */
static fgw_error_t pcb_act_DJopt(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *arg = NULL;
	int layn, saved = 0;
	corner_s *c;

	RND_ACT_MAY_CONVARG(1, FGW_STR, DJopt, arg = argv[1].val.str);

#ifdef ENDIF
	SwitchDrawingWindow(PCB->Zoom, Output.drawing_area->window, conf_core.editor.show_solder_side, rnd_false);
#endif

	rnd_hid_busy(PCB, 1);

	lines = 0;
	corners = 0;

	grok_layer_groups();

	PCB_SUBC_LOOP(PCB->Data);
		PCB_PADSTACK_LOOP(subc->data);
		{
			pcb_pstk_proto_t *proto;
			proto = pcb_pstk_get_proto(padstack);
			if (proto == NULL)
				continue;

			c = find_corner(padstack->x, padstack->y, -1);
			if (!PCB_PSTK_PROTO_CUTS(proto))
				c->pad = padstack;
			else
				c->pin = padstack;
		}
		PCB_END_LOOP;
	PCB_END_LOOP;

	PCB_PADSTACK_LOOP(PCB->Data);
	/* hace don't mess with vias that have thermals */
	/* but then again don't bump into them if (!PCB_FLAG_TEST(ALLTHERMFLAGS, via)) */
	{
		c = find_corner(padstack->x, padstack->y, -1);
		c->via = padstack;
	}
	PCB_END_LOOP;

	check(0, 0);

	if (RND_NSTRCMP(arg, "splitlines") == 0) {
		if (canonicalize_lines())
			pcb_undo_inc_serial();
		rnd_hid_busy(PCB, 0);
		return 0;
	}

	for (layn = 0; layn < pcb_max_layer(PCB); layn++) {
		pcb_layer_t *layer = pcb_get_layer(PCB->Data, layn);

		if (!(pcb_layer_flags(PCB, layn) & PCB_LYT_COPPER))
			continue;

		PCB_LINE_LOOP(layer);
		{
			line_s *ls;

			if (conf_djopt.plugins.djopt.auto_only && !autorouted(line))
				continue;

			if (line->Point1.X == line->Point2.X && line->Point1.Y == line->Point2.Y) {
				pcb_line_destroy(layer, line);
				continue;
			}

			ls = (line_s *) malloc(sizeof(line_s));
			ls->next = lines;
			lines = ls;
			ls->is_pad = 0;
			ls->s = find_corner(line->Point1.X, line->Point1.Y, layn);
			ls->e = find_corner(line->Point2.X, line->Point2.Y, layn);
			ls->line = line;
			add_line_to_corner(ls, ls->s);
			add_line_to_corner(ls, ls->e);
			ls->layer = layn;
		}
		PCB_END_LOOP;
	}

	check(0, 0);
	pinsnap();
	canonicalize_lines();
	check(0, 0);
	classify_nets();
	/*dump_all(); */
	check(0, 0);

	if (RND_NSTRCMP(arg, "debumpify") == 0)
		saved += debumpify();
	else if (RND_NSTRCMP(arg, "unjaggy") == 0)
		saved += unjaggy();
	else if (RND_NSTRCMP(arg, "simple") == 0)
		saved += simple_optimizations();
	else if (RND_NSTRCMP(arg, "vianudge") == 0)
		saved += vianudge();
	else if (RND_NSTRCMP(arg, "viatrim") == 0)
		saved += viatrim();
	else if (RND_NSTRCMP(arg, "orthopull") == 0)
		saved += orthopull();
	else if (RND_NSTRCMP(arg, "auto") == 0)
		saved += automagic();
	else if (RND_NSTRCMP(arg, "miter") == 0)
		saved += miter();
	else {
		printf("unknown command: %s\n", arg);
		rnd_hid_busy(PCB, 0);
		return 1;
	}

	padcleaner();

	check(0, 0);
	if (saved)
		pcb_undo_inc_serial();

	rnd_hid_busy(PCB, 0);
	RND_ACT_IRES(0);
	return 0;
}

rnd_action_t djopt_action_list[] = {
	{"djopt", pcb_act_DJopt, pcb_acth_DJopt, pcb_acts_DJopt}
};

int pplg_check_ver_djopt(int ver_needed) { return 0; }

void pplg_uninit_djopt(void)
{
	rnd_remove_actions_by_cookie(djopt_cookie);
	rnd_hid_menu_unload(rnd_gui, djopt_cookie);
	rnd_conf_unreg_fields("plugins/djopt/");
}

int pplg_init_djopt(void)
{
	RND_API_CHK_VER;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_djopt, field,isarray,type_name,cpath,cname,desc,flags);
#include "djopt_conf_fields.h"

	RND_REGISTER_ACTIONS(djopt_action_list, djopt_cookie)

	rnd_hid_menu_load(rnd_gui, NULL, djopt_cookie, 175, NULL, 0, djopt_menu, "plugin: djopt");

	return 0;
}
