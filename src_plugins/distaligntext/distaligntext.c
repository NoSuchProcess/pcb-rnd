/*
 * distaligntext plug-in for PCB.
 *
 * Copyright (C) 2012 Dan White <dan@whiteaudio.com>
 * Functions to distribute (evenly spread out) and align PCB text.
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Substantially from distalign.c
 * Copyright (C) 2007 Ben Jackson <ben@ben.com>
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016, 2018.
 *
 *
 * Modifications and internal differences are significant enough warrant
 * a new related plugin.
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "error.h"
#include "move.h"
#include "draw.h"
#include "plugins.h"
#include "actions.h"
#include "conf_core.h"
#include "box.h"
#include "macro.h"
#include "compat_misc.h"

enum {
	K_X,
	K_Y,
	K_Lefts,
	K_Rights,
	K_Tops,
	K_Bottoms,
	K_Centers,
	K_Gaps,
	K_First,
	K_Last,
	K_Average,
	K_Crosshair,
	K_Gridless,
	K_none,
	K_aligntext,
	K_distributetext
};

static const char *keywords[] = {
	/* [K_X] */ "X",
	/* [K_Y] */ "Y",
	/* [K_Lefts] */ "Lefts",
	/* [K_Rights] */ "Rights",
	/* [K_Tops] */ "Tops",
	/* [K_Bottoms] */ "Bottoms",
	/* [K_Centers] */ "Centers",
	/* [K_Gaps] */ "Gaps",
	/* [K_First] */ "First",
	/* [K_Last] */ "Last",
	/* [K_Average] */ "Average",
	/* [K_Crosshair] */ "pcb_crosshair",
	/* [K_Gridless] */ "Gridless",
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
static inline pcb_coord_t				        	\
coord ## DIR(pcb_text_t *text, int point)		        	\
{								\
	switch (point) {					\
	case K_Lefts:						\
	case K_Tops:						\
		return text->BoundingBox.DIR ## 1;		\
	case K_Rights:						\
	case K_Bottoms:						\
		return text->BoundingBox.DIR ## 2;		\
	case K_Centers:						\
	case K_Gaps:						\
		return (text->BoundingBox.DIR ## 1 +		\
		       text->BoundingBox.DIR ## 2) / 2;	        \
	}							\
	return 0;						\
}

COORD(X)
	COORD(Y)

/* Return the text coordinate associated with the given internal point. */
static pcb_coord_t coord(pcb_text_t * text, int dir, int point)
{
	if (dir == K_X)
		return coordX(text, point);
	else
		return coordY(text, point);
}

static struct text_by_pos {
	pcb_text_t *text;
	pcb_coord_t pos;
	pcb_coord_t width;
} *texts_by_pos;

static int ntexts_by_pos;

static int cmp_tbp(const void *a, const void *b)
{
	const struct text_by_pos *ta = a;
	const struct text_by_pos *tb = b;

	return ta->pos - tb->pos;
}

/* Find all selected text objects, then order them in order by coordinate in
 * the 'dir' axis.  This is used to find the "First" and "Last" elements
 * and also to choose the distribution order.
 *
 * For alignment, first and last are in the orthogonal axis (imagine if
 * you were lining up letters in a sentence, aligning *vertically* to the
 * first letter means selecting the first letter *horizontally*).
 *
 * For distribution, first and last are in the distribution axis. */
static int sort_texts_by_pos(int op, int dir, int point)
{
	int nsel = 0;

	if (ntexts_by_pos)
		return ntexts_by_pos;
	if (op == K_aligntext)
		dir = dir == K_X ? K_Y : K_X;	/* see above */

	PCB_SUBC_LOOP(PCB->Data);
	{
		PCB_TEXT_ALL_LOOP(subc->data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, text) || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, text))
				continue;
			nsel++;
		}
		PCB_ENDALL_LOOP;
	}
	PCB_END_LOOP;

	PCB_TEXT_ALL_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, text))
			continue;
		nsel++;
	}
	PCB_ENDALL_LOOP;
	if (!nsel)
		return 0;
	texts_by_pos = malloc(nsel * sizeof(*texts_by_pos));
	ntexts_by_pos = nsel;
	nsel = 0;

	PCB_SUBC_LOOP(PCB->Data);
	{
		PCB_TEXT_ALL_LOOP(subc->data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, text) || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, text))
				continue;
			texts_by_pos[nsel].text = text;
			texts_by_pos[nsel++].pos = coord(text, dir, point);
		}
		PCB_ENDALL_LOOP;
	}
	PCB_END_LOOP;

	PCB_TEXT_ALL_LOOP(PCB->Data);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, text))
			continue;
		texts_by_pos[nsel].text = text;
		texts_by_pos[nsel++].pos = coord(text, dir, point);
	}
	PCB_ENDALL_LOOP;
	qsort(texts_by_pos, ntexts_by_pos, sizeof(*texts_by_pos), cmp_tbp);
	return ntexts_by_pos;
}

static void free_texts_by_pos(void)
{
	if (ntexts_by_pos) {
		free(texts_by_pos);
		texts_by_pos = NULL;
		ntexts_by_pos = 0;
	}
}


/* Find the reference coordinate from the specified points of all selected text. */
static pcb_coord_t reference_coord(int op, int x, int y, int dir, int point, int reference)
{
	pcb_coord_t q;
	int i, nsel;

	q = 0;
	switch (reference) {
	case K_Crosshair:
		if (dir == K_X)
			q = x;
		else
			q = y;
		break;
	case K_Average:							/* the average among selected text */
		nsel = ntexts_by_pos;
		for (i = 0; i < ntexts_by_pos; ++i) {
			q += coord(texts_by_pos[i].text, dir, point);
		}
		if (nsel)
			q /= nsel;
		break;
	case K_First:								/* first or last in the orthogonal direction */
	case K_Last:
		if (!sort_texts_by_pos(op, dir, point)) {
			q = 0;
			break;
		}
		if (reference == K_First) {
			q = coord(texts_by_pos[0].text, dir, point);
		}
		else {
			q = coord(texts_by_pos[ntexts_by_pos - 1].text, dir, point);
		}
		break;
	}
	return q;
}

static const char pcb_acts_aligntext[] = "AlignText(X/Y, [Lefts/Rights/Tops/Bottoms/Centers, [First/Last/pcb_crosshair/Average[, Gridless]]])";
/* DOC: aligntext.html */
static fgw_error_t pcb_act_aligntext(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a0, *a1, *a2, *a3;
	int dir;
	int point;
	int reference;
	int gridless;
	pcb_coord_t q;
	pcb_coord_t p, dp, dx, dy;
	int changed = 0;

	if (argc < 2 || argc > 5) {
		PCB_ACT_FAIL(aligntext);
	}

	PCB_ACT_CONVARG(1, FGW_STR, aligntext, a0 = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, aligntext, a1 = argv[2].val.str);
	PCB_ACT_CONVARG(3, FGW_STR, aligntext, a2 = argv[3].val.str);
	PCB_ACT_CONVARG(4, FGW_STR, aligntext, a3 = argv[4].val.str);

	/* parse direction arg */
	switch ((dir = keyword(a0))) {
	case K_X:
	case K_Y:
		break;
	default:
		PCB_ACT_FAIL(aligntext);
	}
	/* parse point (within each element) which will be aligned */
	switch ((point = keyword(a1))) {
	case K_Centers:
		break;
	case K_Lefts:
	case K_Rights:
		if (dir == K_Y) {
			PCB_ACT_FAIL(aligntext);
		}
		break;
	case K_Tops:
	case K_Bottoms:
		if (dir == K_X) {
			PCB_ACT_FAIL(aligntext);
		}
		break;
	case K_none:									/* default value */
		if (dir == K_X) {
			point = K_Lefts;
		}
		else {
			point = K_Tops;
		}
		break;
	default:
		PCB_ACT_FAIL(aligntext);
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
		PCB_ACT_FAIL(aligntext);
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
		PCB_ACT_FAIL(aligntext);
	}
	pcb_undo_save_serial();
	/* find the final alignment coordinate using the above options */
	q = reference_coord(K_aligntext, pcb_crosshair.X, pcb_crosshair.Y, dir, point, reference);
	/* move all selected elements to the new coordinate */
	/* selected text part of an element */

	PCB_SUBC_LOOP(PCB->Data);
	{
		PCB_TEXT_ALL_LOOP(subc->data) {
		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, text) || !PCB_FLAG_TEST(PCB_FLAG_FLOATER, text))
			continue;
		/* find delta from reference point to reference point */
		p = coord(text, dir, point);
		dp = q - p;
		/* ...but if we're gridful, keep the mark on the grid */
		/* TODO re-enable for text, need textcoord()
		   if (!gridless)
		   {
		   dp -= (coord (text, dir, K_Marks) + dp) % (long) (PCB->Grid);
		   }
		 */
		if (dp) {
			/* move from generic to X or Y */
			dx = dy = dp;
			if (dir == K_X)
				dy = 0;
			else
				dx = 0;
			pcb_move_obj(PCB_OBJ_TEXT, text->parent.layer, text, text, dx, dy);
			changed = 1;
		}
		} PCB_ENDALL_LOOP;
	}
	PCB_END_LOOP;

	/* Selected bare text objects */
	PCB_TEXT_ALL_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, text)) {
			/* find delta from reference point to reference point */
			p = coord(text, dir, point);
			dp = q - p;
			/* ...but if we're gridful, keep the mark on the grid */
			/* TODO re-enable for text, need textcoord()
			   if (!gridless)
			   {
			   dp -= (coord (text, dir, K_Marks) + dp) % (long) (PCB->Grid);
			   }
			 */
			if (dp) {
				/* move from generic to X or Y */
				dx = dy = dp;
				if (dir == K_X)
					dy = 0;
				else
					dx = 0;
				pcb_move_obj(PCB_OBJ_TEXT, layer, text, text, dx, dy);
				changed = 1;
			}
		}
	}
	PCB_ENDALL_LOOP;
	if (changed) {
		pcb_undo_restore_serial();
		pcb_undo_inc_serial();
		pcb_redraw();
		pcb_board_set_changed_flag(pcb_true);
	}
	free_texts_by_pos();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_distributetext[] = "DistributeText(Y, [Lefts/Rights/Tops/Bottoms/Centers/Gaps, [First/Last/pcb_crosshair, First/Last/pcb_crosshair[, Gridless]]])";
/* DOC: distributetext.html */
static fgw_error_t pcb_act_distributetext(fgw_arg_t *res, int argc, fgw_arg_t *argv)
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

	if (argc < 2 || argc == 4 || argc > 5) {
		PCB_ACT_FAIL(distributetext);
	}

	PCB_ACT_CONVARG(1, FGW_STR, distributetext, a0 = argv[1].val.str);
	PCB_ACT_CONVARG(2, FGW_STR, distributetext, a1 = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, distributetext, a2 = argv[3].val.str);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, distributetext, a3 = argv[4].val.str);
	PCB_ACT_MAY_CONVARG(5, FGW_STR, distributetext, a4 = argv[5].val.str);

	/* parse direction arg */
	switch ((dir = keyword(a0))) {
	case K_X:
	case K_Y:
		break;
	default:
		PCB_ACT_FAIL(distributetext);
	}


	/* parse point (within each element) which will be distributed */
	switch ((point = keyword(a1))) {
	case K_Centers:
	case K_Gaps:
		break;
	case K_Lefts:
	case K_Rights:
		if (dir == K_Y) {
			PCB_ACT_FAIL(distributetext);
		}
		break;
	case K_Tops:
	case K_Bottoms:
		if (dir == K_X) {
			PCB_ACT_FAIL(distributetext);
		}
		break;
	case K_none:									/* default value */
		if (dir == K_X) {
			point = K_Lefts;
		}
		else {
			point = K_Tops;
		}
		break;
	default:
		PCB_ACT_FAIL(distributetext);
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
		PCB_ACT_FAIL(distributetext);
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
		PCB_ACT_FAIL(distributetext);
	}
	if (refa == refb) {
		PCB_ACT_FAIL(distributetext);
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
		PCB_ACT_FAIL(distributetext);
	}
	pcb_undo_save_serial();
	/* build list of texts in orthogonal axis order */
	sort_texts_by_pos(K_distributetext, dir, point);
	/* find the endpoints given the above options */
	s = reference_coord(K_distributetext, pcb_crosshair.X, pcb_crosshair.Y, dir, point, refa);
	e = reference_coord(K_distributetext, pcb_crosshair.X, pcb_crosshair.Y, dir, point, refb);
	slack = e - s;
	/* use this divisor to calculate spacing (for 1 elt, avoid 1/0) */
	divisor = (ntexts_by_pos > 1) ? (ntexts_by_pos - 1) : 1;
	/* even the gaps instead of the edges or whatnot */
	/* find the "slack" in the row */
	if (point == K_Gaps) {
		pcb_coord_t w;

		/* subtract all the "widths" from the slack */
		for (i = 0; i < ntexts_by_pos; ++i) {
			pcb_text_t *text = texts_by_pos[i].text;

			/* coord doesn't care if I mix Lefts/Tops */
			w = texts_by_pos[i].width = coord(text, dir, K_Rights) - coord(text, dir, K_Lefts);
			/* Gaps distribution is on centers, so half of
			 * first and last text don't count */
			if (i == 0 || i == ntexts_by_pos - 1) {
				w /= 2;
			}
			slack -= w;
		}
		/* slack could be negative */
	}
	/* move all selected texts to the new coordinate */
	for (i = 0; i < ntexts_by_pos; ++i) {
		pcb_text_t *text = texts_by_pos[i].text;
		pcb_coord_t p, q, dp, dx, dy;

		/* find reference point for this text */
		q = s + slack * i / divisor;
		/* find delta from reference point to reference point */
		p = coord(text, dir, point);
		dp = q - p;
		/* ...but if we're gridful, keep the mark on the grid */
		/* TODO re-enable grid
		   if (! gridless)
		   {
		   dp -= (coord (text, dir, K_Marks) + dp) % (long) (PCB->Grid);
		   }
		 */
		if (dp) {
			/* move from generic to X or Y */
			dx = dy = dp;
			if (dir == K_X)
				dy = 0;
			else
				dx = 0;
			pcb_text_move(text, dx, dy);
			pcb_undo_add_obj_to_move(PCB_OBJ_TEXT, NULL, NULL, text, dx, dy);
			changed = 1;
		}
		/* in gaps mode, accumulate part widths */
		if (point == K_Gaps) {
			/* move remaining half of our text */
			s += texts_by_pos[i].width / 2;
			/* move half of next text */
			if (i < ntexts_by_pos - 1)
				s += texts_by_pos[i + 1].width / 2;
		}
	}
	if (changed) {
		pcb_undo_restore_serial();
		pcb_undo_inc_serial();
		pcb_redraw();
		pcb_board_set_changed_flag(pcb_true);
	}
	free_texts_by_pos();
	PCB_ACT_IRES(0);
	return 0;
}

static pcb_action_t distaligntext_action_list[] = {
	{"distributetext", pcb_act_distributetext, "Distribute Text Elements", pcb_acts_distributetext},
	{"aligntext", pcb_act_aligntext, "Align Text Elements", pcb_acts_aligntext}
};

char *distaligntext_cookie = "distaligntext plugin";

PCB_REGISTER_ACTIONS(distaligntext_action_list, distaligntext_cookie)

int pplg_check_ver_distaligntext(int ver_needed) { return 0; }

void pplg_uninit_distaligntext(void)
{
	pcb_remove_actions_by_cookie(distaligntext_cookie);
}

#include "dolists.h"
int pplg_init_distaligntext(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(distaligntext_action_list, distaligntext_cookie);
	return 0;
}
