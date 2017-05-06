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
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016; generalized in 2017.
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
#include "boardflip.h"


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

#define XFLIP(v) (v) = ((flip_x ? -(v) : (v)) + xo)
#define YFLIP(v) (v) = ((flip_y ? -(v) : (v)) + yo)
#define AFLIP(a) (a) = aflip((a), flip_x, flip_y)
#define NEG(a) (a) = -(a)
#define ONLY1 ((flip_x || flip_y) && !(flip_x && flip_y))

static double aflip(double a, pcb_bool flip_x, pcb_bool flip_y)
{
	if (flip_x) {
		while(a > 360.0) a -= 360.0;
		while(a < 0.0) a += 360.0;

		if ((0.0 <= a) && (a <= 90.0)) a = (90.0 - a) + 90.0;
		else if ((0 < 90.0) && (a <= 180.0)) a = 90.0 - (a - 90.0);
		else if ((0 < 180.0) && (a <= 270.0)) a = 270.0 + (270.0 - a);
		else if ((0 < 270.0) && (a <= 360.0)) a = 270.0 - (a - 270.0);

		while(a > 360.0) a -= 360.0;
		while(a < 0.0) a += 360.0;
	}
	if (flip_y)
		a = -a;
	return a;
}

void pcb_flip_data(pcb_data_t *data, pcb_bool flip_x, pcb_bool flip_y, pcb_coord_t xo, pcb_coord_t yo, pcb_bool elem_swap_sides)
{
	LAYER_LOOP(data, pcb_max_layer);
	{
		PCB_LINE_LOOP(layer);
		{
			pcb_poly_restore_to_poly(data, PCB_TYPE_LINE, layer, line);
			pcb_r_delete_entry(layer->line_tree, (pcb_box_t *)line);
			XFLIP(line->Point1.X);
			XFLIP(line->Point2.X);
			YFLIP(line->Point1.Y);
			YFLIP(line->Point2.Y);
			pcb_line_bbox(line);
			pcb_r_insert_entry(layer->line_tree, (pcb_box_t *)line, 0);
			pcb_poly_clear_from_poly(data, PCB_TYPE_LINE, layer, line);
		}
		PCB_END_LOOP;
		PCB_TEXT_LOOP(layer);
		{
			pcb_poly_restore_to_poly(data, PCB_TYPE_TEXT, layer, text);
			pcb_r_delete_entry(layer->text_tree, (pcb_box_t *)text);
			XFLIP(text->X);
			YFLIP(text->Y);
			if (ONLY1)
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
			pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
			pcb_r_insert_entry(layer->text_tree, (pcb_box_t *)text, 0);
			pcb_poly_clear_from_poly(data, PCB_TYPE_TEXT, layer, text);
		}
		PCB_END_LOOP;
		PCB_POLY_LOOP(layer);
		{
			int i, j;
			pcb_r_delete_entry(layer->polygon_tree, (pcb_box_t *)polygon);
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
			pcb_poly_bbox(polygon);
			pcb_r_insert_entry(layer->polygon_tree, (pcb_box_t *)polygon, 0);
			pcb_poly_init_clip(data, layer, polygon);
		}
		PCB_END_LOOP;
		PCB_ARC_LOOP(layer);
		{
			pcb_poly_restore_to_poly(data, PCB_TYPE_ARC, layer, arc);
			pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *)arc);
			XFLIP(arc->X);
			YFLIP(arc->Y);
			AFLIP(arc->StartAngle);
			if (ONLY1)
				NEG(arc->Delta);
			pcb_arc_bbox(arc);
			pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *)arc, 0);
			pcb_poly_clear_from_poly(data, PCB_TYPE_ARC, layer, arc);
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	PCB_VIA_LOOP(data);
	{
		pcb_r_delete_entry(data->via_tree, (pcb_box_t *)via);
		pcb_poly_restore_to_poly(data, PCB_TYPE_VIA, via, via);
		XFLIP(via->X);
		YFLIP(via->Y);
		pcb_pin_bbox(via);
		pcb_r_insert_entry(data->via_tree, (pcb_box_t *) via, 0);
		pcb_poly_clear_from_poly(data, PCB_TYPE_VIA, via, via);
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
			pcb_r_delete_entry(data->name_tree[n], (pcb_box_t *)text);
			XFLIP(text->X);
			YFLIP(text->Y);
			if (elem_swap_sides)
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
			pcb_r_insert_entry(data->name_tree[n], (pcb_box_t *)text, 0);
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
			AFLIP(arc->StartAngle);
			if (ONLY1)
				NEG(arc->Delta);
		}
		PCB_END_LOOP;
		PCB_PIN_LOOP(element);
		{
			pcb_r_delete_entry(data->pin_tree, (pcb_box_t *)pin);
			pcb_poly_restore_to_poly(data, PCB_TYPE_PIN, element, pin);
			XFLIP(pin->X);
			YFLIP(pin->Y);
			pcb_pin_bbox(pin);
			pcb_r_insert_entry(data->pin_tree, (pcb_box_t *)pin, 0);
			pcb_poly_clear_from_poly(data, PCB_TYPE_PIN, element, pin);
		}
		PCB_END_LOOP;
		PCB_PAD_LOOP(element);
		{
			pcb_r_delete_entry(data->pad_tree, (pcb_box_t *)pad);
			pcb_poly_restore_to_poly(data, PCB_TYPE_PAD, element, pad);
			XFLIP(pad->Point1.X);
			XFLIP(pad->Point2.X);
			YFLIP(pad->Point1.Y);
			YFLIP(pad->Point2.Y);
			if ((elem_swap_sides) && (ONLY1))
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, pad);
			pcb_pad_bbox(pad);
			pcb_r_insert_entry(data->pad_tree, (pcb_box_t *)pad, 0);
			pcb_poly_clear_from_poly(data, PCB_TYPE_PAD, element, pad);
		}
		PCB_END_LOOP;
		pcb_element_bbox(data, element, pcb_font(PCB, 0, 1)); /* also does the rtree administration */
	}
	PCB_END_LOOP;
	PCB_RAT_LOOP(data);
	{
		pcb_r_delete_entry(data->rat_tree, (pcb_box_t *)line);
		XFLIP(line->Point1.X);
		XFLIP(line->Point2.X);
		YFLIP(line->Point1.Y);
		YFLIP(line->Point2.Y);
		pcb_r_insert_entry(data->rat_tree, (pcb_box_t *)line, 0);
	}
	PCB_END_LOOP;
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

int pplg_check_ver_boardflip(int ver_needed) { return 0; }

void pplg_uninit_boardflip(void)
{
	pcb_hid_remove_actions_by_cookie(boardflip_cookie);
}

#include "dolists.h"
int pplg_init_boardflip(void)
{
	PCB_REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie);
	return 0;
}

