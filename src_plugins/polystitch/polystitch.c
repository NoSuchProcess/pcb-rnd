/*
 * PolyStitch plug-in for PCB.
 *
 * Copyright (C) 2010 DJ Delorie <dj@delorie.com>
 *
 * Licensed under the terms of the GNU General Public 
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 * Mostly rewritten for poly holes by Tibor 'Igor2' Palinkas in 2018.
 *
 * Original source: http://www.delorie.com/pcb/polystitch.c
 *
 * Usage: PolyStitch()
 *
 * The polygon under the cursor (based on closest-corner) is stitched
 * together with the polygon surrounding it on the same layer.
 * Use with pstoedit conversions where there's a "hole" in the shape -
 * select the hole.
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include "macro.h"
#include "remove.h"
#include "hid.h"
#include "error.h"
#include "rtree.h"
#include "draw.h"
#include "polygon.h"
#include "plugins.h"
#include "actions.h"
#include "obj_poly.h"
#include "obj_poly_draw.h"

/* Given the X,Y, find the inmost (closest) polygon */
static pcb_poly_t *find_crosshair_poly(pcb_coord_t x, pcb_coord_t y)
{
	double best = 0, dist;
	pcb_poly_t *res = NULL;

	PCB_POLY_VISIBLE_LOOP(PCB->Data);
	{
		/* layer, polygon */
		PCB_POLY_POINT_LOOP(polygon);
		{
			/* point */
			pcb_coord_t dx = x - point->X;
			pcb_coord_t dy = y - point->Y;
			dist = (double)dx * dx + (double)dy * dy;
			if ((dist < best) || (res == NULL)) {
				res = polygon;
				best = dist;
			}
		}
		PCB_END_LOOP;
	}
	PCB_ENDALL_LOOP;

	if (res == NULL)
		pcb_message(PCB_MSG_ERROR, "Cannot find any polygons");

	return res;
}

/* Set outer_poly to the enclosing poly. We assume there's only one. */
static pcb_poly_t *find_enclosing_poly(pcb_poly_t *inner_poly)
{
	pcb_layer_t *poly_layer = inner_poly->parent.layer;

	PCB_POLY_LOOP(poly_layer);
	{
		if (polygon == inner_poly)
			continue;
		if (polygon->BoundingBox.X1 <= inner_poly->BoundingBox.X1
				&& polygon->BoundingBox.X2 >= inner_poly->BoundingBox.X2
				&& polygon->BoundingBox.Y1 <= inner_poly->BoundingBox.Y1 && polygon->BoundingBox.Y2 >= inner_poly->BoundingBox.Y2) {
			return polygon;
		}
	}
	PCB_END_LOOP;

	pcb_message(PCB_MSG_ERROR, "Cannot find a polygon enclosing the one you selected");
	return NULL;
}


static int pcb_act_polystitch(int oargc, const char **oargv)
{
	PCB_OLD_ACT_BEGIN;
	pcb_coord_t x, y;
	pcb_poly_t *inner_poly, *outer_poly;

	pcb_hid_get_coords("Select a corner on the inner polygon", &x, &y);
	inner_poly = find_crosshair_poly(x, y);

	if (inner_poly) {
		outer_poly = find_enclosing_poly(inner_poly);
		if (outer_poly) {
			pcb_cardinal_t n, end;

			/* figure how many contour points the inner poly has (holes are going to be ignored) */
			if (inner_poly->HoleIndexN > 0)
				end = inner_poly->HoleIndex[0];
			else
				end = inner_poly->PointN;

			/* convert the inner poly into a hole */
			pcb_poly_hole_new(outer_poly);
			for(n = 0; n < end; n++)
				pcb_poly_point_new(outer_poly, inner_poly->Points[n].X, inner_poly->Points[n].Y);
			pcb_poly_init_clip(PCB->Data, outer_poly->parent.layer, outer_poly);
			pcb_poly_bbox(outer_poly);

			pcb_poly_remove(inner_poly->parent.layer, inner_poly);
		}
	}
	return 0;
	PCB_OLD_ACT_END;
}

static pcb_hid_action_t polystitch_action_list[] = {
	{"PolyStitch", pcb_act_polystitch, NULL, NULL}
};

char *polystitch_cookie = "polystitch plugin";

PCB_REGISTER_ACTIONS(polystitch_action_list, polystitch_cookie)

int pplg_check_ver_polystitch(int ver_needed) { return 0; }

void pplg_uninit_polystitch(void)
{
	pcb_hid_remove_actions_by_cookie(polystitch_cookie);
}

#include "dolists.h"
int pplg_init_polystitch(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(polystitch_action_list, polystitch_cookie);
	return 0;
}
