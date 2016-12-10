/*!
 * \file distalign.c
 *
 * \brief distalign plug-in for PCB.
 * Functions to distribute (evenly spread out) and align PCB elements.
 *
 * \author Copyright (C) 2007 Ben Jackson <ben@ben.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 *
 * From: Ben Jackson <bjj@saturn.home.ben.com>
 * To: geda-user@moria.seul.org
 * Date: Sat, 24 Feb 2007 22:13:51 -0800
 * Subject: The amazing Align/Distribute plugin
 *
 * Ok, having "finished" the smartdisperse plugin, I moved on to phase 2
 * of my "I'm sick of moving LEDs by hand" master plan.
 *
 * Introducing Align() and Distribute(), which work much like the
 * similarly named functions in Visio.
 * Given that PCB does not have the concept of "first selected object"
 * to draw on, the reference points can be selected by arguments.
 *
 * Both functions always work on all selected elements[*].
 *
 * Some examples:
 * <table noborder>
 * <tr><td>
 * :Align(X)
 * </td><td>
 * Objects are moved left/right to align their Marks with the Mark of
 * the topmost selected object. All objects remain on or move to the
 * current grid.
 * </td></tr><tr><td>
 * :Align(Y,Centers)
 * </td><td>
 * Now objects are aligned up/down to their geometric centers rather
 * than their marks. The reference point is the center of the leftmost
 * object.
 * </td></tr><tr><td>
 * :Align(Y,Tops,Average,Gridless)
 * </td><td>
 * Feeling bold, you align the tops all selected objects to the average
 * of their top edges and let them wander off the grid to exactly the
 * average of their tops.
 * </td></tr><tr><td>
 * :Align(X,Marks,pcb_crosshair)
 * </td><td>
 * None of the objects are where you want them, so you move the
 * crosshair to a likely spot and cause them all to move their X
 * coordinates to your new X location.
 * </td></tr><tr><td>
 * :Align(X,Marks,Last)
 * </td><td>
 * As above, but instead of the crosshair you just override the default
 * "First" to "Last" so the reference object is bottommost instead of
 * topmost.
 * </table>
 *
 * Now you have them in a neat line, but they're all clumped!
 *
 * <table noborder>
 * <tr><td>
 * :Distribute(Y)
 * </td><td>
 * Objects are spread out evenly from the first (uppermost) to last
 * using their marks as the reference point.
 * </td></tr><tr><td>
 * :Distribute(X,Lefts,pcb_crosshair,Last)
 * </td><td>
 * You move your crosshair to the left edge of the target area, which
 * will be the leftmost edge of the leftmost object. The objects are
 * spread between your crosshair and the original location of the Last
 * (rightmost) object.
 * </td></tr><tr><td>
 * :Distribute(Y,Gaps)
 * </td><td>
 * You have chosen to select many oddly sized objects, and instead of
 * their centers being spread out evenly, you want the space (or "gaps")
 * to be even.
 *
 * You get tricky and bind `Align(X,Marks,pcb_crosshair)' to a key.
 * Now you can select an object and hit your key and the object will
 * warp to the same X coordinate as your cursor.
 *
 * Original source was:  http://ad7gd.net/geda/distalign.c
 *
 * [*] If it has any flaws, it is that you can't operate non-element
 * objects, though some melding of autocrop (which knows how to do such
 * things) and distalign could produce such a capability.
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
#include "rats.h"
#include "error.h"
#include "move.h"
#include "draw.h"
#include "plugins.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "compat_misc.h"

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char align_syntax[] =
	"Align(X/Y, [Lefts/Rights/Tops/Bottoms/Centers/Marks, [First/Last/pcb_crosshair/Average[, Gridless]]])";

static const char distribute_syntax[] =
	"Distribute(Y, [Lefts/Rights/Tops/Bottoms/Centers/Marks/Gaps, [First/Last/pcb_crosshair, First/Last/pcb_crosshair[, Gridless]]])";

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
coord ## DIR(pcb_element_t *element, int point)			\
{								\
	switch (point) {					\
	case K_Marks:						\
		return element->Mark ## DIR;			\
	case K_Lefts:						\
	case K_Tops:						\
		return element->BoundingBox.DIR ## 1;		\
	case K_Rights:						\
	case K_Bottoms:						\
		return element->BoundingBox.DIR ## 2;		\
	case K_Centers:						\
	case K_Gaps:						\
		return (element->BoundingBox.DIR ## 1 +		\
		       element->BoundingBox.DIR ## 2) / 2;	\
	}							\
	return 0;						\
}

COORD(X)
	COORD(Y)

/* return the element coordinate associated with the given internal point */
		 static pcb_coord_t coord(pcb_element_t * element, int dir, int point)
{
	if (dir == K_X)
		return coordX(element, point);
	else
		return coordY(element, point);
}

static struct element_by_pos {
	pcb_element_t *element;
	pcb_coord_t pos;
	pcb_coord_t width;
} *elements_by_pos;

static int nelements_by_pos;

static int cmp_ebp(const void *a, const void *b)
{
	const struct element_by_pos *ea = a;
	const struct element_by_pos *eb = b;

	return ea->pos - eb->pos;
}

/*!
 * Find all selected objects, then order them in order by coordinate in
 * the 'dir' axis. This is used to find the "First" and "Last" elements
 * and also to choose the distribution order.
 *
 * For alignment, first and last are in the orthogonal axis (imagine if
 * you were lining up letters in a sentence, aligning *vertically* to the
 * first letter means selecting the first letter *horizontally*).
 *
 * For distribution, first and last are in the distribution axis.
 */
static int sort_elements_by_pos(int op, int dir, int point)
{
	int nsel = 0;

	if (nelements_by_pos)
		return nelements_by_pos;
	if (op == K_align)
		dir = dir == K_X ? K_Y : K_X;	/* see above */
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))
			continue;
		nsel++;
	}
	PCB_END_LOOP;
	if (!nsel)
		return 0;
	elements_by_pos = malloc(nsel * sizeof(*elements_by_pos));
	nelements_by_pos = nsel;
	nsel = 0;
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))
			continue;
		elements_by_pos[nsel].element = element;
		elements_by_pos[nsel++].pos = coord(element, dir, point);
	}
	PCB_END_LOOP;
	qsort(elements_by_pos, nelements_by_pos, sizeof(*elements_by_pos), cmp_ebp);
	return nelements_by_pos;
}

static void free_elements_by_pos(void)
{
	if (nelements_by_pos) {
		free(elements_by_pos);
		elements_by_pos = NULL;
		nelements_by_pos = 0;
	}
}

/*!
 * \brief Find the reference coordinate from the specified points of all
 * selected elements.
 */
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
	case K_Average:							/* the average among selected elements */
		nsel = 0;
		q = 0;
		PCB_ELEMENT_LOOP(PCB->Data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))
				continue;
			q += coord(element, dir, point);
			nsel++;
		}
		PCB_END_LOOP;
		if (nsel)
			q /= nsel;
		break;
	case K_First:								/* first or last in the orthogonal direction */
	case K_Last:
		if (!sort_elements_by_pos(op, dir, point)) {
			q = 0;
			break;
		}
		if (reference == K_First) {
			q = coord(elements_by_pos[0].element, dir, point);
		}
		else {
			q = coord(elements_by_pos[nelements_by_pos - 1].element, dir, point);
		}
		break;
	}
	return q;
}

/*!
 * Align(X, [Lefts/Rights/Centers/Marks, [First/Last/pcb_crosshair/Average[, Gridless]]])\n
 * Align(Y, [Tops/Bottoms/Centers/Marks, [First/Last/pcb_crosshair/Average[, Gridless]]])
 *
 * X or Y - Select which axis will move, other is untouched. \n
 * Lefts, Rights, \n
 * Tops, Bottoms, \n
 * Centers, Marks - Pick alignment point within each element. \n
 * First, Last, \n
 * pcb_crosshair, \n
 * Average - Alignment reference, First=Topmost/Leftmost, \n
 * Last=Bottommost/Rightmost, Average or pcb_crosshair point \n
 * Gridless - Do not force results to align to prevailing grid. \n
 *
 * Defaults are Marks, First.
 */
static int align(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int dir;
	int point;
	int reference;
	int gridless;
	pcb_coord_t q;
	int changed = 0;

	if (argc < 1 || argc > 4) {
		PCB_AFAIL(align);
	}
	/* parse direction arg */
	switch ((dir = keyword(ARG(0)))) {
	case K_X:
	case K_Y:
		break;
	default:
		PCB_AFAIL(align);
	}
	/* parse point (within each element) which will be aligned */
	switch ((point = keyword(ARG(1)))) {
	case K_Centers:
	case K_Marks:
		break;
	case K_Lefts:
	case K_Rights:
		if (dir == K_Y) {
			PCB_AFAIL(align);
		}
		break;
	case K_Tops:
	case K_Bottoms:
		if (dir == K_X) {
			PCB_AFAIL(align);
		}
		break;
	case K_none:
		point = K_Marks;						/* default value */
		break;
	default:
		PCB_AFAIL(align);
	}
	/* parse reference which will determine alignment coordinates */
	switch ((reference = keyword(ARG(2)))) {
	case K_First:
	case K_Last:
	case K_Average:
	case K_Crosshair:
		break;
	case K_none:
		reference = K_First;				/* default value */
		break;
	default:
		PCB_AFAIL(align);
	}
	/* optionally work off the grid (solar cells!) */
	switch (keyword(ARG(3))) {
	case K_Gridless:
		gridless = 1;
		break;
	case K_none:
		gridless = 0;
		break;
	default:
		PCB_AFAIL(align);
	}
	/* find the final alignment coordinate using the above options */
	q = reference_coord(K_align, pcb_crosshair.X, pcb_crosshair.Y, dir, point, reference);
	/* move all selected elements to the new coordinate */
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		pcb_coord_t p, dp, dx, dy;

		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))
			continue;
		/* find delta from reference point to reference point */
		p = coord(element, dir, point);
		dp = q - p;
		/* ...but if we're gridful, keep the mark on the grid */
		if (!gridless) {
			dp -= (coord(element, dir, K_Marks) + dp) % (long) (PCB->Grid);
		}
		if (dp) {
			/* move from generic to X or Y */
			dx = dy = dp;
			if (dir == K_X)
				dy = 0;
			else
				dx = 0;
			pcb_element_move(PCB->Data, element, dx, dy);
			pcb_undo_add_obj_to_move(PCB_TYPE_ELEMENT, NULL, NULL, element, dx, dy);
			changed = 1;
		}
	}
	PCB_END_LOOP;
	if (changed) {
		pcb_undo_inc_serial();
		pcb_redraw();
		pcb_board_set_changed_flag(1);
	}
	free_elements_by_pos();
	return 0;
}

/*!
 * Distribute(X, [Lefts/Rights/Centers/Marks/Gaps, [First/Last/pcb_crosshair, First/Last/pcb_crosshair[, Gridless]]]) \n
 * Distribute(Y, [Tops/Bottoms/Centers/Marks/Gaps, [First/Last/pcb_crosshair, First/Last/pcb_crosshair[, Gridless]]]) \n
 * \n
 * As with align, plus: \n
 * \n
 * Gaps - Make gaps even rather than spreading points evenly. \n
 * First, Last, \n
 * pcb_crosshair - Two arguments specifying both ends of the distribution,
 * they can't both be the same. \n
 * \n
 * Defaults are Marks, First, Last \n
 * \n
 * Distributed elements always retain the same relative order they had
 * before they were distributed. \n
 */
static int distribute(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int dir;
	int point;
	int refa, refb;
	int gridless;
	pcb_coord_t s, e, slack;
	int divisor;
	int changed = 0;
	int i;

	if (argc < 1 || argc == 3 || argc > 4) {
		PCB_AFAIL(distribute);
	}
	/* parse direction arg */
	switch ((dir = keyword(ARG(0)))) {
	case K_X:
	case K_Y:
		break;
	default:
		PCB_AFAIL(distribute);
	}
	/* parse point (within each element) which will be distributed */
	switch ((point = keyword(ARG(1)))) {
	case K_Centers:
	case K_Marks:
	case K_Gaps:
		break;
	case K_Lefts:
	case K_Rights:
		if (dir == K_Y) {
			PCB_AFAIL(distribute);
		}
		break;
	case K_Tops:
	case K_Bottoms:
		if (dir == K_X) {
			PCB_AFAIL(distribute);
		}
		break;
	case K_none:
		point = K_Marks;						/* default value */
		break;
	default:
		PCB_AFAIL(distribute);
	}
	/* parse reference which will determine first distribution coordinate */
	switch ((refa = keyword(ARG(2)))) {
	case K_First:
	case K_Last:
	case K_Average:
	case K_Crosshair:
		break;
	case K_none:
		refa = K_First;							/* default value */
		break;
	default:
		PCB_AFAIL(distribute);
	}
	/* parse reference which will determine final distribution coordinate */
	switch ((refb = keyword(ARG(3)))) {
	case K_First:
	case K_Last:
	case K_Average:
	case K_Crosshair:
		break;
	case K_none:
		refb = K_Last;							/* default value */
		break;
	default:
		PCB_AFAIL(distribute);
	}
	if (refa == refb) {
		PCB_AFAIL(distribute);
	}
	/* optionally work off the grid (solar cells!) */
	switch (keyword(ARG(4))) {
	case K_Gridless:
		gridless = 1;
		break;
	case K_none:
		gridless = 0;
		break;
	default:
		PCB_AFAIL(distribute);
	}
	/* build list of elements in orthogonal axis order */
	sort_elements_by_pos(K_distribute, dir, point);
	/* find the endpoints given the above options */
	s = reference_coord(K_distribute, x, y, dir, point, refa);
	e = reference_coord(K_distribute, x, y, dir, point, refb);
	slack = e - s;
	/* use this divisor to calculate spacing (for 1 elt, avoid 1/0) */
	divisor = (nelements_by_pos > 1) ? (nelements_by_pos - 1) : 1;
	/* even the gaps instead of the edges or whatnot */
	/* find the "slack" in the row */
	if (point == K_Gaps) {
		pcb_coord_t w;

		/* subtract all the "widths" from the slack */
		for (i = 0; i < nelements_by_pos; ++i) {
			pcb_element_t *element = elements_by_pos[i].element;
			/* coord doesn't care if I mix Lefts/Tops */
			w = elements_by_pos[i].width = coord(element, dir, K_Rights) - coord(element, dir, K_Lefts);
			/* Gaps distribution is on centers, so half of
			 * first and last element don't count */
			if (i == 0 || i == nelements_by_pos - 1) {
				w /= 2;
			}
			slack -= w;
		}
		/* slack could be negative */
	}
	/* move all selected elements to the new coordinate */
	for (i = 0; i < nelements_by_pos; ++i) {
		pcb_element_t *element = elements_by_pos[i].element;
		pcb_coord_t p, q, dp, dx, dy;

		/* find reference point for this element */
		q = s + slack * i / divisor;
		/* find delta from reference point to reference point */
		p = coord(element, dir, point);
		dp = q - p;
		/* ...but if we're gridful, keep the mark on the grid */
		if (!gridless) {
			dp -= (coord(element, dir, K_Marks) + dp) % (long) (PCB->Grid);
		}
		if (dp) {
			/* move from generic to X or Y */
			dx = dy = dp;
			if (dir == K_X)
				dy = 0;
			else
				dx = 0;
			pcb_element_move(PCB->Data, element, dx, dy);
			pcb_undo_add_obj_to_move(PCB_TYPE_ELEMENT, NULL, NULL, element, dx, dy);
			changed = 1;
		}
		/* in gaps mode, accumulate part widths */
		if (point == K_Gaps) {
			/* move remaining half of our element */
			s += elements_by_pos[i].width / 2;
			/* move half of next element */
			if (i < nelements_by_pos - 1)
				s += elements_by_pos[i + 1].width / 2;
		}
	}
	if (changed) {
		pcb_undo_inc_serial();
		pcb_redraw();
		pcb_board_set_changed_flag(1);
	}
	free_elements_by_pos();
	return 0;
}

static pcb_hid_action_t distalign_action_list[] = {
	{"distribute", NULL, distribute, "Distribute Elements", distribute_syntax},
	{"align", NULL, align, "Align Elements", align_syntax}
};

static char *distalign_cookie = "distalign plugin";

PCB_REGISTER_ACTIONS(distalign_action_list, distalign_cookie)

static void hid_distalign_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(distalign_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_distalign_init()
{
	PCB_REGISTER_ACTIONS(distalign_action_list, distalign_cookie);
	return hid_distalign_uninit;
}


