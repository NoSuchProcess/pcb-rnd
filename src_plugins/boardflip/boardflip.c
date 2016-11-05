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
#include "misc.h"
#include "rtree.h"
#include "undo.h"
#include "plugins.h"
#include "obj_all.h"
#include "hid_actions.h"


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

#define FLIP(y) (y) = h - (y)
#define NEG(y) (y) = - (y)

static int boardflip(int argc, const char **argv, Coord x, Coord y)
{
	int h = PCB->MaxHeight;
	int sides = 0;

	if (argc > 0 && strcasecmp(argv[0], "sides") == 0)
		sides = 1;
	printf("argc %d argv %s sides %d\n", argc, argc > 0 ? argv[0] : "", sides);
	LAYER_LOOP(PCB->Data, max_copper_layer + 2);
	{
		LINE_LOOP(layer);
		{
			FLIP(line->Point1.Y);
			FLIP(line->Point2.Y);
		}
		END_LOOP;
		TEXT_LOOP(layer);
		{
			FLIP(text->Y);
			TOGGLE_FLAG(PCB_FLAG_ONSOLDER, text);
		}
		END_LOOP;
		POLYGON_LOOP(layer);
		{
			int i, j;
			POLYGONPOINT_LOOP(polygon);
			{
				FLIP(point->Y);
			}
			END_LOOP;
			i = 0;
			j = polygon->PointN - 1;
			while (i < j) {
				PointType p = polygon->Points[i];
				polygon->Points[i] = polygon->Points[j];
				polygon->Points[j] = p;
				i++;
				j--;
			}
			InitClip(PCB->Data, layer, polygon);
		}
		END_LOOP;
		ARC_LOOP(layer);
		{
			FLIP(arc->Y);
			NEG(arc->StartAngle);
			NEG(arc->Delta);
		}
		END_LOOP;
	}
	END_LOOP;
	VIA_LOOP(PCB->Data);
	{
		FLIP(via->Y);
	}
	END_LOOP;
	ELEMENT_LOOP(PCB->Data);
	{
		FLIP(element->MarkY);
		if (sides)
			TOGGLE_FLAG(PCB_FLAG_ONSOLDER, element);
		ELEMENTTEXT_LOOP(element);
		{
			FLIP(text->Y);
			TOGGLE_FLAG(PCB_FLAG_ONSOLDER, text);
		}
		END_LOOP;
		ELEMENTLINE_LOOP(element);
		{
			FLIP(line->Point1.Y);
			FLIP(line->Point2.Y);
		}
		END_LOOP;
		ELEMENTARC_LOOP(element);
		{
			FLIP(arc->Y);
			NEG(arc->StartAngle);
			NEG(arc->Delta);
		}
		END_LOOP;
		PIN_LOOP(element);
		{
			FLIP(pin->Y);
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			FLIP(pad->Point1.Y);
			FLIP(pad->Point2.Y);
			if (sides)
				TOGGLE_FLAG(PCB_FLAG_ONSOLDER, pad);
		}
		END_LOOP;
	}
	END_LOOP;
	RAT_LOOP(PCB->Data);
	{
		FLIP(line->Point1.Y);
		FLIP(line->Point2.Y);
	}
	END_LOOP;
	return 0;
}

static HID_Action boardflip_action_list[] = {
	{"BoardFlip", NULL, boardflip, NULL, NULL}
};

char *boardflip_cookie = "boardflip plugin";

REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie)

static void hid_boardflip_uninit(void)
{
	hid_remove_actions_by_cookie(boardflip_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_boardflip_init()
{
	REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie);
	return hid_boardflip_uninit;
}

