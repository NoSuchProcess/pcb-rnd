/* Functions to distribute (evenly spread out) and align PCB subcircuits.
 *
 * Copyright (C) 2007 Ben Jackson <ben@ben.com>
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 *
 * Original source was:  http://ad7gd.net/geda/distalign.c
 *
 * Ben Jackson AD7GD <ben@ben.com>
 *
 * http://www.ben.com/
 */

#include <stdio.h>
#include <math.h>

#include "board.h"
#include "config.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "error.h"
#include "move.h"
#include "draw.h"
#include "plugins.h"
#include "macro.h"
#include "actions.h"
#include "compat_misc.h"

enum {
	K_X,
	K_Y,
	K_Lefts,
	K_Rights,
	K_Tops,
	K_Bottoms,
	K_Centers,
	K_Marks,
	K_Gaps,
	K_First,
	K_Last,
	K_Average,
	K_Crosshair,
	K_Gridless,
	K_none,
	K_align,
	K_distribute
};

static const char *keywords[] = {
	/*[K_X] */ "X",
	/*[K_Y] */ "Y",
	/*[K_Lefts] */ "Lefts",
	/*[K_Rights] */ "Rights",
	/*[K_Tops] */ "Tops",
	/*[K_Bottoms] */ "Bottoms",
	/*[K_Centers] */ "Centers",
	/*[K_Marks] */ "Marks",
	/*[K_Gaps] */ "Gaps",
	/*[K_First] */ "First",
	/*[K_Last] */ "Last",
	/*[K_Average] */ "Average",
	/*[K_Crosshair] */ "pcb_crosshair",
	/*[K_Gridless] */ "Gridless",
};

static int keyword(const char *s)
{
	int i;

	if (!s) {
		return K_none;
	}
	for (i = 0; i < PCB_ENTRIES(keywords); ++i) {
		if (keywords[i] && pcb_strcasecmp(s, keywords[i]) == 0)
			return i;
	}
	return -1;
}


/* this macro produces a function in X or Y that switches on 'point' */
#define COORD(DIR)						\
static inline pcb_coord_t		        			\
coord ## DIR(pcb_subc_t *subc, int point)			\
{								\
	pcb_coord_t oX, oY; \
	switch (point) {					\
	case K_Marks:						\
		oX = oY = 0; \
		pcb_subc_get_origin(subc, &oX, &oY); \
		return o ## DIR;			\
	case K_Lefts:						\
	case K_Tops:						\
		return subc->BoundingBox.DIR ## 1;		\
	case K_Rights:						\
	case K_Bottoms:						\
		return subc->BoundingBox.DIR ## 2;		\
	case K_Centers:						\
	case K_Gaps:						\
		return (subc->BoundingBox.DIR ## 1 +		\
		       subc->BoundingBox.DIR ## 2) / 2;	\
	}							\
	return 0;						\
}

COORD(X)
COORD(Y)

/* return the subcircuit coordinate associated with the given internal point */
static pcb_coord_t coord(pcb_subc_t *subc, int dir, int point)
{
	if (dir == K_X)
		return coordX(subc, point);
	else
		return coordY(subc, point);
}

static struct subc_by_pos {
	pcb_subc_t *subc;
	pcb_coord_t pos;
	pcb_coord_t width;
} *subcs_by_pos;

static int nsubcs_by_pos;

static int cmp_ebp(const void *a, const void *b)
{
	const struct subc_by_pos *ea = a;
	const struct subc_by_pos *eb = b;

	return ea->pos - eb->pos;
}

/*
 * Find all selected objects, then order them in order by coordinate in
 * the 'dir' axis. This is used to find the "First" and "Last" subcircuits
 * and also to choose the distribution order.
 *
 * For alignment, first and last are in the orthogonal axis (imagine if
 * you were lining up letters in a sentence, aligning *vertically* to the
 * first letter means selecting the first letter *horizontally*).
 *
 * For distribution, first and last are in the distribution axis.
 */
static int sort_subcs_by_pos(int op, int dir, int point)
{
	int nsel = 0;

	if (nsubcs_by_pos)
		return nsubcs_by_pos;
	if (op == K_align)
		dir = dir == K_X ? K_Y : K_X;	/* see above */
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc))
			continue;
		nsel++;
	}
	PCB_END_LOOP;
	if (!nsel)
		return 0;
	subcs_by_pos = malloc(nsel * sizeof(*subcs_by_pos));
	nsubcs_by_pos = nsel;
	nsel = 0;
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc))
			continue;
		subcs_by_pos[nsel].subc = subc;
		subcs_by_pos[nsel++].pos = coord(subc, dir, point);
	}
	PCB_END_LOOP;
	qsort(subcs_by_pos, nsubcs_by_pos, sizeof(*subcs_by_pos), cmp_ebp);
	return nsubcs_by_pos;
}

static void free_subcs_by_pos(void)
{
	if (nsubcs_by_pos) {
		free(subcs_by_pos);
		subcs_by_pos = NULL;
		nsubcs_by_pos = 0;
	}
}

/* Find the reference coordinate from the specified points of all
 * selected subcircuits. */
static pcb_coord_t reference_coord(int op, int x, int y, int dir, int point, int reference)
{
	pcb_coord_t q;
	int nsel;

	q = 0;
	switch (reference) {
	case K_Crosshair:
		if (dir == K_X)
			q = x;
		else
			q = y;
		break;
	case K_Average:							/* the average among selected subcircuits */
		nsel = 0;
		q = 0;
		PCB_SUBC_LOOP(PCB->Data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc))
				continue;
			q += coord(subc, dir, point);
			nsel++;
		}
		PCB_END_LOOP;
		if (nsel)
			q /= nsel;
		break;
	case K_First:								/* first or last in the orthogonal direction */
	case K_Last:
		if (!sort_subcs_by_pos(op, dir, point)) {
			q = 0;
			break;
		}
		if (reference == K_First) {
			q = coord(subcs_by_pos[0].subc, dir, point);
		}
		else {
			q = coord(subcs_by_pos[nsubcs_by_pos - 1].subc, dir, point);
		}
		break;
	}
	return q;
}

static const char pcb_acts_align[] = "Align(X/Y, [Lefts/Rights/Tops/Bottoms/Centers/Marks, [First/Last/pcb_crosshair/Average[, Gridless]]])";
/* DOC: align.html */
static fgw_error_t pcb_act_align(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a0, *a1, *a2, *a3;
	int dir;
	int point;
	int reference;
	int gridless;
	pcb_coord_t q;
	int changed = 0;

	if (argc < 2 || argc > 5) {
		PCB_ACT_FAIL(align);
	}

	PCB_ACT_CONVARG(1, FGW_STR, align, a0 = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, align, a1 = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, align, a2 = argv[3].val.str);
	PCB_ACT_CONVARG(4, FGW_STR, align, a3 = argv[4].val.str);

	/* parse direction arg */
	switch ((dir = keyword(a0))) {
	case K_X:
	case K_Y:
		break;
	default:
		PCB_ACT_FAIL(align);
	}
	/* parse point (within each subcircuit) which will be aligned */
	switch ((point = keyword(a1))) {
	case K_Centers:
	case K_Marks:
		break;
	case K_Lefts:
	case K_Rights:
		if (dir == K_Y) {
			PCB_ACT_FAIL(align);
		}
		break;
	case K_Tops:
	case K_Bottoms:
		if (dir == K_X) {
			PCB_ACT_FAIL(align);
		}
		break;
	case K_none:
		point = K_Marks;						/* default value */
		break;
	default:
		PCB_ACT_FAIL(align);
	}
	/* parse reference which will determine alignment coordinates */
	switch ((reference = keyword(a2))) {
	case K_First:
	case K_Last:
	case K_Average:
	case K_Crosshair:
		break;
	case K_none:
		reference = K_First;				/* default value */
		break;
	default:
		PCB_ACT_FAIL(align);
	}
	/* optionally work off the grid (solar cells!) */
	switch (keyword(a3)) {
	case K_Gridless:
		gridless = 1;
		break;
	case K_none:
		gridless = 0;
		break;
	default:
		PCB_ACT_FAIL(align);
	}
	/* find the final alignment coordinate using the above options */
	q = reference_coord(K_align, pcb_crosshair.X, pcb_crosshair.Y, dir, point, reference);
	/* move all selected subcircuits to the new coordinate */
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_coord_t p, dp, dx, dy;

		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc))
			continue;
		/* find delta from reference point to reference point */
		p = coord(subc, dir, point);
		dp = q - p;
		/* ...but if we're gridful, keep the mark on the grid */
		if (!gridless) {
			dp -= (coord(subc, dir, K_Marks) + dp) % (long) (PCB->hidlib.grid);
		}
		if (dp) {
			/* move from generic to X or Y */
			dx = dy = dp;
			if (dir == K_X)
				dy = 0;
			else
				dx = 0;
			pcb_subc_move(subc, dx, dy, 1);
			pcb_undo_add_obj_to_move(PCB_OBJ_SUBC, NULL, NULL, subc, dx, dy);
			changed = 1;
		}
	}
	PCB_END_LOOP;
	if (changed) {
		pcb_undo_inc_serial();
		pcb_hid_redraw();
		pcb_board_set_changed_flag(1);
	}
	free_subcs_by_pos();

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_distribute[] = "Distribute(X/Y, [Lefts/Rights/Tops/Bottoms/Centers/Marks/Gaps, [First/Last/pcb_crosshair, First/Last/pcb_crosshair[, Gridless]]])";
/* DOC: distribute.html */
static fgw_error_t pcb_act_distribute(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a0, *a1, *a2 = NULL, *a3 = NULL, *a4 = NULL;
	int dir;
	int point;
	int refa, refb;
	int gridless;
	pcb_coord_t s, e, slack;
	int divisor;
	int changed = 0;
	int i;

	if (argc < 2 || argc == 4 || argc > 6) {
		PCB_ACT_FAIL(distribute);
	}

	PCB_ACT_CONVARG(1, FGW_STR, distribute, a0 = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, distribute, a1 = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, distribute, a2 = argv[3].val.str);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, distribute, a3 = argv[4].val.str);
	PCB_ACT_MAY_CONVARG(5, FGW_STR, distribute, a4 = argv[5].val.str);


	/* parse direction arg */
	switch ((dir = keyword(a0))) {
	case K_X:
	case K_Y:
		break;
	default:
		PCB_ACT_FAIL(distribute);
	}
	/* parse point (within each subcircuit) which will be distributed */
	switch ((point = keyword(a1))) {
	case K_Centers:
	case K_Marks:
	case K_Gaps:
		break;
	case K_Lefts:
	case K_Rights:
		if (dir == K_Y) {
			PCB_ACT_FAIL(distribute);
		}
		break;
	case K_Tops:
	case K_Bottoms:
		if (dir == K_X) {
			PCB_ACT_FAIL(distribute);
		}
		break;
	case K_none:
		point = K_Marks;						/* default value */
		break;
	default:
		PCB_ACT_FAIL(distribute);
	}
	/* parse reference which will determine first distribution coordinate */
	switch ((refa = keyword(a2))) {
	case K_First:
	case K_Last:
	case K_Average:
	case K_Crosshair:
		break;
	case K_none:
		refa = K_First;							/* default value */
		break;
	default:
		PCB_ACT_FAIL(distribute);
	}
	/* parse reference which will determine final distribution coordinate */
	switch ((refb = keyword(a3))) {
	case K_First:
	case K_Last:
	case K_Average:
	case K_Crosshair:
		break;
	case K_none:
		refb = K_Last;							/* default value */
		break;
	default:
		PCB_ACT_FAIL(distribute);
	}
	if (refa == refb) {
		PCB_ACT_FAIL(distribute);
	}
	/* optionally work off the grid (solar cells!) */
	switch (keyword(a4)) {
	case K_Gridless:
		gridless = 1;
		break;
	case K_none:
		gridless = 0;
		break;
	default:
		PCB_ACT_FAIL(distribute);
	}
	/* build list of subcircuitss in orthogonal axis order */
	sort_subcs_by_pos(K_distribute, dir, point);
	/* find the endpoints given the above options */
	s = reference_coord(K_distribute, pcb_crosshair.X, pcb_crosshair.Y, dir, point, refa);
	e = reference_coord(K_distribute, pcb_crosshair.X, pcb_crosshair.Y, dir, point, refb);
	slack = e - s;
	/* use this divisor to calculate spacing (for 1 elt, avoid 1/0) */
	divisor = (nsubcs_by_pos > 1) ? (nsubcs_by_pos - 1) : 1;
	/* even the gaps instead of the edges or whatnot */
	/* find the "slack" in the row */
	if (point == K_Gaps) {
		pcb_coord_t w;

		/* subtract all the "widths" from the slack */
		for (i = 0; i < nsubcs_by_pos; ++i) {
			pcb_subc_t *subc = subcs_by_pos[i].subc;
			/* coord doesn't care if I mix Lefts/Tops */
			w = subcs_by_pos[i].width = coord(subc, dir, K_Rights) - coord(subc, dir, K_Lefts);
			/* Gaps distribution is on centers, so half of
			 * first and last subcircuit don't count */
			if (i == 0 || i == nsubcs_by_pos - 1) {
				w /= 2;
			}
			slack -= w;
		}
		/* slack could be negative */
	}
	/* move all selected subcircuits to the new coordinate */
	for (i = 0; i < nsubcs_by_pos; ++i) {
		pcb_subc_t *subc = subcs_by_pos[i].subc;
		pcb_coord_t p, q, dp, dx, dy;

		/* find reference point for this subcircuit */
		q = s + slack * i / divisor;
		/* find delta from reference point to reference point */
		p = coord(subc, dir, point);
		dp = q - p;
		/* ...but if we're gridful, keep the mark on the grid */
		if (!gridless) {
			dp -= (coord(subc, dir, K_Marks) + dp) % (long) (PCB->hidlib.grid);
		}
		if (dp) {
			/* move from generic to X or Y */
			dx = dy = dp;
			if (dir == K_X)
				dy = 0;
			else
				dx = 0;
			pcb_subc_move(subc, dx, dy, 1);
			pcb_undo_add_obj_to_move(PCB_OBJ_SUBC, NULL, NULL, subc, dx, dy);
			changed = 1;
		}
		/* in gaps mode, accumulate part widths */
		if (point == K_Gaps) {
			/* move remaining half of our subcircuits */
			s += subcs_by_pos[i].width / 2;
			/* move half of next subcircuits */
			if (i < nsubcs_by_pos - 1)
				s += subcs_by_pos[i + 1].width / 2;
		}
	}
	if (changed) {
		pcb_undo_inc_serial();
		pcb_hid_redraw();
		pcb_board_set_changed_flag(1);
	}
	free_subcs_by_pos();
	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t distalign_action_list[] = {
	{"distribute", pcb_act_distribute, "Distribute subcircuits", pcb_acts_distribute},
	{"align", pcb_act_align, "Align subcircuits", pcb_acts_align}
};

static char *distalign_cookie = "distalign plugin";

PCB_REGISTER_ACTIONS(distalign_action_list, distalign_cookie)

int pplg_check_ver_distalign(int ver_needed) { return 0; }

void pplg_uninit_distalign(void)
{
	pcb_remove_actions_by_cookie(distalign_cookie);
}

#include "dolists.h"
int pplg_init_distalign(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(distalign_action_list, distalign_cookie);
	return 0;
}
