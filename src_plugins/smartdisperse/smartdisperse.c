/*!
 * \file smartdisperse.c
 *
 * \brief Smartdisperse plug-in for PCB.
 *
 * \author Copyright (C) 2007 Ben Jackson <ben@ben.com> based on
 * teardrops.c by Copyright (C) 2006 DJ Delorie <dj@delorie.com> as well
 * as the original action.c, and autoplace.c.
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 *
 * Improve the initial dispersion of elements by choosing an order based
 * on the netlist, rather than the arbitrary element order.  This isn't
 * the same as a global autoplace, it's more of a linear autoplace.  It
 * might make some useful local groupings.  For example, you should not
 * have to chase all over the board to find the resistor that goes with
 * a given LED.
 */

#include <stdio.h>
#include <math.h>

#include <genht/htpi.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "rats.h"
#include "error.h"
#include "move.h"
#include "draw.h"
#include "set.h"
#include "plugins.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "compat_nls.h"

#define GAP 10000
static pcb_coord_t minx;
static pcb_coord_t miny;
static pcb_coord_t maxx;
static pcb_coord_t maxy;

/*!
 * \brief Place one element.
 *
 * Must initialize statics above before calling for the first time.
 *
 * This is taken almost entirely from ActionDisperseElements, with cleanup
 */
static void place(pcb_element_t * element)
{
	pcb_coord_t dx, dy;

	/* figure out how much to move the element */
	dx = minx - element->BoundingBox.X1;
	dy = miny - element->BoundingBox.Y1;

	/* snap to the grid */
	dx -= (element->MarkX + dx) % (long) (PCB->Grid);
	dx += (long) (PCB->Grid);
	dy -= (element->MarkY + dy) % (long) (PCB->Grid);
	dy += (long) (PCB->Grid);

	/*
	 * and add one grid size so we make sure we always space by GAP or
	 * more
	 */
	dx += (long) (PCB->Grid);

	/* Figure out if this row has room.  If not, start a new row */
	if (minx != GAP && GAP + element->BoundingBox.X2 + dx > PCB->MaxWidth) {
		miny = maxy + GAP;
		minx = GAP;
		place(element);							/* recurse can't loop, now minx==GAP */
		return;
	}

	/* move the element */
	MoveElementLowLevel(PCB->Data, element, dx, dy);

	/* and add to the undo list so we can undo this operation */
	AddObjectToMoveUndoList(PCB_TYPE_ELEMENT, NULL, NULL, element, dx, dy);

	/* keep track of how tall this row is */
	minx += element->BoundingBox.X2 - element->BoundingBox.X1 + GAP;
	if (maxy < element->BoundingBox.Y2) {
		maxy = element->BoundingBox.Y2;
	}
}

/*!
 * \brief Return the X location of a connection's pad or pin within its
 * element.
 */
static pcb_coord_t padDX(pcb_connection_t * conn)
{
	pcb_element_t *element = (pcb_element_t *) conn->ptr1;
	pcb_any_line_t *line = (pcb_any_line_t *) conn->ptr2;

	return line->BoundingBox.X1 - (element->BoundingBox.X1 + element->BoundingBox.X2) / 2;
}

/*!
 * \brief Return true if ea,eb would be the best order, else eb,ea,
 * based on pad loc.
 */
static int padorder(pcb_connection_t * conna, pcb_connection_t * connb)
{
	pcb_coord_t dxa, dxb;

	dxa = padDX(conna);
	dxb = padDX(connb);
	/* there are other cases that merit rotation, ignore them for now */
	if (dxa > 0 && dxb < 0)
		return 1;
	return 0;
}

/* ewww, these are actually arrays */
#define ELEMENT_N(DATA,ELT)	((ELT) - (DATA)->Element)
#define VISITED(ELT)		(visited[ELEMENT_N(PCB->Data, (ELT))])
#define IS_ELEMENT(CONN)	((CONN)->type == PCB_TYPE_PAD || (CONN)->type == PCB_TYPE_PIN)

#define ARG(n) (argc > (n) ? argv[n] : 0)

static const char smartdisperse_syntax[] = "SmartDisperse([All|Selected])";

#define set_visited(element) htpi_set(&visited, ((void *)(element)), 1)
#define is_visited(element)  htpi_has(&visited, ((void *)(element)))


static int smartdisperse(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = ARG(0);
	pcb_netlist_t *Nets;
	htpi_t visited;
/*  PointerListType stack = { 0, 0, NULL };*/
	int all;
/*  int changed = 0;
  int i;*/

	if (!function) {
		all = 1;
	}
	else if (strcmp(function, "All") == 0) {
		all = 1;
	}
	else if (strcmp(function, "Selected") == 0) {
		all = 0;
	}
	else {
		PCB_AFAIL(smartdisperse);
	}

	Nets = ProcNetlist(&PCB->NetlistLib[0]);
	if (!Nets) {
		Message(PCB_MSG_ERROR, _("Can't use SmartDisperse because no netlist is loaded.\n"));
		return 0;
	}

	/* remember which elements we finish with */
	htpi_init(&visited, ptrhash, ptrkeyeq);

	/* if we're not doing all, mark the unselected elements as "visited" */
	ELEMENT_LOOP(PCB->Data);
	{
		if (!(all || TEST_FLAG(PCB_FLAG_SELECTED, element))) {
			set_visited(element);
		}
	}
	END_LOOP;

	/* initialize variables for place() */
	minx = GAP;
	miny = GAP;
	maxx = GAP;
	maxy = GAP;

	/*
	 * Pick nets with two connections.  This is the start of a more
	 * elaborate algorithm to walk serial nets, but the datastructures
	 * are too gross so I'm going with the 80% solution.
	 */
	NET_LOOP(Nets);
	{
		pcb_connection_t *conna, *connb;
		pcb_element_t *ea, *eb;
/*    pcb_element_t *epp;*/

		if (net->ConnectionN != 2)
			continue;

		conna = &net->Connection[0];
		connb = &net->Connection[1];
		if (!IS_ELEMENT(conna) || !IS_ELEMENT(conna))
			continue;

		ea = (pcb_element_t *) conna->ptr1;
		eb = (pcb_element_t *) connb->ptr1;

		/* place this pair if possible */
		if (is_visited(ea) || is_visited(eb))
			continue;
		set_visited(ea);
		set_visited(eb);

		/* a weak attempt to get the linked pads side-by-side */
		if (padorder(conna, connb)) {
			place(ea);
			place(eb);
		}
		else {
			place(eb);
			place(ea);
		}
	}
	END_LOOP;

	/* Place larger nets, still grouping by net */
	NET_LOOP(Nets);
	{
		CONNECTION_LOOP(net);
		{
			pcb_element_t *element;

			if (!IS_ELEMENT(connection))
				continue;

			element = (pcb_element_t *) connection->ptr1;

			/* place this one if needed */
			if (is_visited(element))
				continue;
			set_visited(element);
			place(element);
		}
		END_LOOP;
	}
	END_LOOP;

	/* Place up anything else */
	ELEMENT_LOOP(PCB->Data);
	{
		if (!is_visited(element)) {
			place(element);
		}
	}
	END_LOOP;

	htpi_uninit(&visited);

	IncrementUndoSerialNumber();
	Redraw();
	SetChangedFlag(1);

	return 0;
}

static pcb_hid_action_t smartdisperse_action_list[] = {
	{"smartdisperse", NULL, smartdisperse, NULL, NULL}
};

char *smartdisperse_cookie = "smartdisperse plugin";

REGISTER_ACTIONS(smartdisperse_action_list, smartdisperse_cookie)

static void hid_smartdisperse_uninit(void)
{
	hid_remove_actions_by_cookie(smartdisperse_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_smartdisperse_init()
{
	REGISTER_ACTIONS(smartdisperse_action_list, smartdisperse_cookie);
	return hid_smartdisperse_uninit;
}
