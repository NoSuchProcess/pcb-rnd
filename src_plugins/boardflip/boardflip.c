/*!
 * \file boardflip.c
 *
 * \brief BoardFlip plug-in for PCB.
 *
 * \author Copyright (C) 2008 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 *
 * Original source was: http://www.delorie.com/pcb/boardflip.c
 *
 * Usage: Boardflip()
 *
 * All objects on the board are up-down flipped.
 *
 * Command line board flipping:
 *
 * pcb --action-string "BoardFlip() SaveTo(LayoutAs,$OUTFILE) Quit()" $INFILE
 *
 * To flip the board physically, use BoardFlip(sides)
 *
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "board.h"
#include "rats.h"
#include "polygon.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "plugins.h"
#include "obj_all.h"
#include "hid_actions.h"
#include "compat_misc.h"


/* Things that need to be flipped:

  lines
  text
  polygons
  arcs
  vias
  elements
    elementlines
    elementarcs
    pins
    pads
  rats
*/

#define XFLIP(v)  v = ((flip_x ? -(v) : (v)) + xo)
#define YFLIP(v)  v = ((flip_y ? -(v) : (v)) + yo)
#define AFLIP(a) (a) = - (a)
#define ONLY1 ((flip_x || flip_y) && (!flip_x && !flip_y))

void pcb_flip_data(pcb_data_t *data, pcb_bool flip_x, pcb_bool flip_y, pcb_coord_t xo, pcb_coord_t yo, pcb_bool elem_swap_sides)
{
	LAYER_LOOP(data, pcb_max_layer);
	{
		PCB_LINE_LOOP(layer);
		{
			XFLIP(line->Point1.X);
			XFLIP(line->Point2.X);
			YFLIP(line->Point1.Y);
			YFLIP(line->Point2.Y);
		}
		PCB_END_LOOP;
		PCB_TEXT_LOOP(layer);
		{
			XFLIP(text->X);
			YFLIP(text->Y);
			if (ONLY1)
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
		}
		PCB_END_LOOP;
		PCB_POLY_LOOP(layer);
		{
			int i, j;
			PCB_POLY_POINT_LOOP(polygon);
			{
				XFLIP(point->X);
				YFLIP(point->Y);
			}
			PCB_END_LOOP;
			if (ONLY1) {
				i = 0;
				j = polygon->PointN - 1;
				while (i < j) {
					pcb_point_t p = polygon->Points[i];
					polygon->Points[i] = polygon->Points[j];
					polygon->Points[j] = p;
					i++;
					j--;
				}
			}
			pcb_poly_init_clip(data, layer, polygon);
		}
		PCB_END_LOOP;
		PCB_ARC_LOOP(layer);
		{
			XFLIP(arc->X);
			YFLIP(arc->Y);
			if (ONLY1) {
				AFLIP(arc->StartAngle);
				AFLIP(arc->Delta);
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	PCB_VIA_LOOP(data);
	{
		XFLIP(via->X);
		YFLIP(via->Y);
	}
	PCB_END_LOOP;
	PCB_ELEMENT_LOOP(data);
	{
		XFLIP(element->MarkX);
		YFLIP(element->MarkY);
		if ((elem_swap_sides) && (ONLY1))
			PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, element);
		PCB_ELEMENT_PCB_TEXT_LOOP(element);
		{
			XFLIP(text->X);
			YFLIP(text->Y);
			if (ONLY1)
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
		}
		PCB_END_LOOP;
		PCB_ELEMENT_PCB_LINE_LOOP(element);
		{
			XFLIP(line->Point1.X);
			XFLIP(line->Point2.X);
			YFLIP(line->Point1.Y);
			YFLIP(line->Point2.Y);
		}
		PCB_END_LOOP;
		PCB_ELEMENT_ARC_LOOP(element);
		{
			XFLIP(arc->X);
			YFLIP(arc->Y);
			if (ONLY1) {
				AFLIP(arc->StartAngle);
				AFLIP(arc->Delta);
			}
		}
		PCB_END_LOOP;
		PCB_PIN_LOOP(element);
		{
			XFLIP(pin->X);
			YFLIP(pin->Y);
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			XFLIP(pad->Point1.X);
			XFLIP(pad->Point2.X);
			YFLIP(pad->Point1.Y);
			YFLIP(pad->Point2.Y);
			if ((elem_swap_sides) && (ONLY1))
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, pad);
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	PCB_RAT_LOOP(data);
	{
		XFLIP(line->Point1.X);
		XFLIP(line->Point2.X);
		YFLIP(line->Point1.Y);
		YFLIP(line->Point2.Y);
	}
	PCB_END_LOOP;
	return 0;
}

static int boardflip(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int h = PCB->MaxHeight;
	int sides = 0;

	if (argc > 0 && pcb_strcasecmp(argv[0], "sides") == 0)
		sides = 1;
	printf("argc %d argv %s sides %d\n", argc, argc > 0 ? argv[0] : "", sides);
	pcb_flip_data(PCB->Data, pcb_false, pcb_true, 0, h, sides);
	return 0;
}


static pcb_hid_action_t boardflip_action_list[] = {
	{"BoardFlip", NULL, boardflip, NULL, NULL}
};

char *boardflip_cookie = "boardflip plugin";

PCB_REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie)

static void hid_boardflip_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(boardflip_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_boardflip_init()
{
	PCB_REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie);
	return hid_boardflip_uninit;
}

