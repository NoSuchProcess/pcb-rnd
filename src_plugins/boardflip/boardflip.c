/*
 * BoardFlip plug-in for PCB.
 *
 * Copyright (C) 2008 DJ Delorie <dj@delorie.com>
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016 and generalized in 2017, 2018.
 *
 * Original source was: http://www.delorie.com/pcb/boardflip.c
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
#include "actions.h"
#include "compat_misc.h"
#include "boardflip.h"
#include "funchash_core.h"

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

void pcb_flip_data(pcb_data_t *data, pcb_bool flip_x, pcb_bool flip_y, pcb_coord_t xo, pcb_coord_t yo, pcb_bool et_swap_sides)
{
	LAYER_LOOP(data, pcb_max_layer);
	{
		PCB_LINE_LOOP(layer);
		{
			pcb_poly_restore_to_poly(data, PCB_OBJ_LINE, layer, line);
			pcb_r_delete_entry(layer->line_tree, (pcb_box_t *)line);
			XFLIP(line->Point1.X);
			XFLIP(line->Point2.X);
			YFLIP(line->Point1.Y);
			YFLIP(line->Point2.Y);
			pcb_line_bbox(line);
			pcb_r_insert_entry(layer->line_tree, (pcb_box_t *)line);
			pcb_poly_clear_from_poly(data, PCB_OBJ_LINE, layer, line);
		}
		PCB_END_LOOP;
		PCB_TEXT_LOOP(layer);
		{
			pcb_poly_restore_to_poly(data, PCB_OBJ_TEXT, layer, text);
			pcb_r_delete_entry(layer->text_tree, (pcb_box_t *)text);
			XFLIP(text->X);
			YFLIP(text->Y);
			if (et_swap_sides && ONLY1)
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
			pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
			pcb_r_insert_entry(layer->text_tree, (pcb_box_t *)text);
			pcb_poly_clear_from_poly(data, PCB_OBJ_TEXT, layer, text);
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
			pcb_r_insert_entry(layer->polygon_tree, (pcb_box_t *)polygon);
			pcb_poly_init_clip(data, layer, polygon);
		}
		PCB_END_LOOP;
		PCB_ARC_LOOP(layer);
		{
			pcb_poly_restore_to_poly(data, PCB_OBJ_ARC, layer, arc);
			pcb_r_delete_entry(layer->arc_tree, (pcb_box_t *)arc);
			XFLIP(arc->X);
			YFLIP(arc->Y);
			AFLIP(arc->StartAngle);
			if (ONLY1)
				NEG(arc->Delta);
			pcb_arc_bbox(arc);
			pcb_r_insert_entry(layer->arc_tree, (pcb_box_t *)arc);
			pcb_poly_clear_from_poly(data, PCB_OBJ_ARC, layer, arc);
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;
	PCB_PADSTACK_LOOP(data);
	{
		pcb_r_delete_entry(data->padstack_tree, (pcb_box_t *)padstack);
		pcb_poly_restore_to_poly(data, PCB_OBJ_PSTK, padstack, padstack);
		XFLIP(padstack->x);
		YFLIP(padstack->y);
		pcb_pstk_bbox(padstack);
		pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)padstack);
		pcb_poly_clear_from_poly(data, PCB_OBJ_PSTK, padstack, padstack);
	}
	PCB_END_LOOP;
	PCB_SUBC_LOOP(data);
	{
		pcb_flip_data(subc->data, flip_x, flip_y, xo, yo, et_swap_sides);
		if (et_swap_sides) {
#warning subc TODO: layer mirror, also test one-sided padstack
		}
	}
	PCB_END_LOOP;
	PCB_RAT_LOOP(data);
	{
		pcb_r_delete_entry(data->rat_tree, (pcb_box_t *)line);
		XFLIP(line->Point1.X);
		XFLIP(line->Point2.X);
		YFLIP(line->Point1.Y);
		YFLIP(line->Point2.Y);
		pcb_r_insert_entry(data->rat_tree, (pcb_box_t *)line);
	}
	PCB_END_LOOP;
}

static const char pcb_acts_boardflip[] = "BoardFlip([sides])";
static const char pcb_acth_boardflip[] = "Mirror the board over the x axis, optionally mirroring sides as well.";
/* DOC: boardflip.html */
static fgw_error_t pcb_act_boardflip(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = -2;
	int h = PCB->MaxHeight;

	PCB_ACT_MAY_CONVARG(1, FGW_KEYWORD, boardflip, op = fgw_keyword(&argv[1]));

	pcb_flip_data(PCB->Data, pcb_false, pcb_true, 0, h, (op == F_Sides));
	PCB_ACT_IRES(0);
	return 0;
}


static pcb_action_t boardflip_action_list[] = {
	{"BoardFlip", pcb_act_boardflip, pcb_acth_boardflip, pcb_acts_boardflip}
};

char *boardflip_cookie = "boardflip plugin";

PCB_REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie)

int pplg_check_ver_boardflip(int ver_needed) { return 0; }

void pplg_uninit_boardflip(void)
{
	pcb_remove_actions_by_cookie(boardflip_cookie);
}

#include "dolists.h"
int pplg_init_boardflip(void)
{
	PCB_API_CHK_VER;

	PCB_REGISTER_ACTIONS(boardflip_action_list, boardflip_cookie);
	return 0;
}

