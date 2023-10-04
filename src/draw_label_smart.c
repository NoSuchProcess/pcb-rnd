/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include <librnd/poly/rtree.h>

typedef struct pcb_smart_label_s pcb_smart_label_t;

struct pcb_smart_label_s {
	rnd_box_t bbox;
	rnd_coord_t x;
	rnd_coord_t y;
	double scale;
	double rot;
	int prio;
	rnd_bool mirror;
	rnd_coord_t w;
	rnd_coord_t h;
	pcb_smart_label_t *next;
	char label[1]; /* dynamic length array */
};

static pcb_smart_label_t *smart_labels = NULL;

RND_INLINE void pcb_label_smart_add(pcb_draw_info_t *info, rnd_coord_t x, rnd_coord_t y, double scale, double rot, rnd_bool mirror, rnd_coord_t w, rnd_coord_t h, const char *label, int prio)
{
	int len = strlen(label);
	pcb_smart_label_t *l = malloc(sizeof(pcb_smart_label_t) + len + 1);

	l->x = x; l->y = y;
	l->scale = scale;
	l->rot = rot;
	l->mirror = mirror;
	l->w = w; l->h = h;
	l->prio = prio;
	memcpy(l->label, label, len+1);

	l->next = smart_labels;
	smart_labels = l;
}

/* returns 1 if x;y was suitable and the label could be placed */
RND_INLINE int lsm_shuffle_try(rnd_rtree_t *rt, pcb_smart_label_t *l, rnd_coord_t x, rnd_coord_t y)
{
	rnd_box_t b;
	rnd_rtree_it_t it;

	b.X1 = x; b.Y1 = y;
	b.X2 = x + l->w; b.Y2 = y + l->h;

	if (rnd_rtree_first(&it, rt, (rnd_rtree_box_t *)&b) != NULL)
		return 0;

	l->bbox = b;
	rnd_rtree_insert(rt, (void *)l, (rnd_rtree_box_t *)l);
	return 1;
}

RND_INLINE void lsm_shuffle(rnd_rtree_t *rt, pcb_smart_label_t *l)
{
	int nx, ny;
	rnd_coord_t dx, dy, step = RND_MM_TO_COORD(0.25);

	rnd_rtree_delete(rt, (void *)l, (rnd_rtree_box_t *)l);

	/* try 0;0, maybe other objects got moved out of the way */
	if (lsm_shuffle_try(rt, l, l->x, l->y))
		return;

	/* try a few locations as close as possible */
	for(ny = 1; ny < 16*2; ny++) {
		dy = step * ny / 2;
		if (ny % 2) dy = -dy;
		for(nx = 1; nx < 16*2; nx++) {
			dx = step * nx / 2;
			if (nx % 2) dx = -dx;
			if (lsm_shuffle_try(rt, l, l->x + dx, l->y + dy))
				return;
		}
	}

	/* all failed, place it back on the wrong place */
	rnd_rtree_insert(rt, (void *)l, (rnd_rtree_box_t *)l);
}


RND_INLINE void lsm_place(rnd_rtree_t *rt, pcb_smart_label_t *l)
{
	rnd_rtree_it_t it;
	pcb_smart_label_t *c;
	int n, tries = 0;

	/* check if there's any collision with a higher prio label; if so, this label has to be moved */
	for(c = rnd_rtree_first(&it, rt, (rnd_rtree_box_t *)l); c != NULL; c = rnd_rtree_next(&it)) {
		if (c->prio < l->prio) {
			lsm_shuffle(rt, l);
			return;
		}
		tries += 2;
	}

	if (tries == 0) {
		/* no collision, just place */
		rnd_rtree_insert(rt, (void *)l, (rnd_rtree_box_t *)l);
		return;
	}

	/* this seems to be the highest prio label around, shuffle the rest */
	rnd_rtree_insert(rt, (void *)l, (rnd_rtree_box_t *)l);
	for(n = 0; n < tries; n++) {
		for(c = rnd_rtree_first(&it, rt, (rnd_rtree_box_t *)l); c != NULL; c = rnd_rtree_next(&it)) {
			lsm_shuffle(rt, c);
			break;
		}
	}
}


RND_INLINE void pcb_label_smart_flush(pcb_draw_info_t *info)
{
	pcb_smart_label_t *l, *next;
	rnd_font_t *font;
	rnd_rtree_t rt;

	if (smart_labels == NULL) return;

	rnd_rtree_init(&rt);
	font = pcb_font(PCB, 0, 0);
	rnd_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.pin_name);
	if (rnd_render->gui)
		pcb_draw_force_termlab++;

	for(l = smart_labels; l != NULL; l = l->next) {
		l->bbox.X1 = l->x; l->bbox.Y1 = l->y;
		l->bbox.X2 = l->x + l->w; l->bbox.Y2 = l->y + l->h;
		lsm_place(&rt, l);
	}


	for(l = smart_labels; l != NULL; l = next) {
		next = l->next;
		pcb_text_draw_string(info, font, (unsigned const char *)l->label, l->bbox.X1, l->bbox.Y1, l->scale, l->scale, l->rot, l->mirror, conf_core.appearance.label_thickness, 0, 0, 0, 0, PCB_TXT_TINY_HIDE, 0);
		if ((l->bbox.X1 != l->x) || (l->bbox.Y1 != l->y)) {
			rnd_coord_t dx = l->w/2, dy = l->h/2;
			rnd_render->draw_line(pcb_draw_out.fgGC, l->bbox.X1+dx, l->bbox.Y1+dy, l->x+dx, l->y+dy);
		}
		free(l);
	}
	smart_labels = NULL;
	rnd_rtree_uninit(&rt);

	if (rnd_render->gui)
		pcb_draw_force_termlab--;
}
