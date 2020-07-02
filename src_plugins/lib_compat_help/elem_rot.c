/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *
 *  (This file was part of the original PCB source on the import, as bom.c,
 *  but did not have a copyright banner. The names of the original authors
 *  potentially can be found in the old CVS.)
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
 *
 */

#include "elem_rot.h"
#include "conf_core.h"
#include "data_it.h"

static const int verbose_rot = 0;

/* In order of preference.
 * Includes numbered and BGA pins.
 * Possibly BGA pins can be missing, so we add a few to try. */
#define MAXREFPINS 32 /* max length of following list */
static char *reference_pin_names[] = {"1", "2", "A1", "A2", "B1", "B2", 0};

static double xyToAngle(double x, double y, rnd_bool morethan2pins)
{
	double d = atan2(-y, x) * 180.0 / M_PI;

		if (verbose_rot)
			rnd_trace(" xyToAngle: %f %f %d ->%f\n", x, y, morethan2pins, d);

	/* IPC 7351 defines different rules for 2 pin elements */
	if (morethan2pins) {
		/* Multi pin case:
		 * Output 0 degrees if pin1 in is top left or top, i.e. between angles of
		 * 80 to 170 degrees.
		 * Pin #1 can be at dead top (e.g. certain PLCCs) or anywhere in the top
		 * left.
		 */
		if (d < -100)
			return 90;								/* -180 to -100 */
		else if (d < -10)
			return 180;								/* -100 to -10 */
		else if (d < 80)
			return 270;								/* -10 to 80 */
		else if (d < 170)
			return 0;									/* 80 to 170 */
		else
			return 90;								/* 170 to 180 */
	}
	else {
		/* 2 pin element:
		 * Output 0 degrees if pin #1 is in top left or left, i.e. in sector
		 * between angles of 95 and 185 degrees.
		 */
		if (d < -175)
			return 0;									/* -180 to -175 */
		else if (d < -85)
			return 90;								/* -175 to -85 */
		else if (d < 5)
			return 180;								/* -85 to 5 */
		else if (d < 95)
			return 270;								/* 5 to 95 */
		else
			return 0;									/* 95 to 180 */
	}
}

void pcb_subc_xy_rot(pcb_subc_t *subc, rnd_coord_t *cx, rnd_coord_t *cy, double *theta, double *xray_theta, rnd_bool autodetect)
{
	double centroidx, centroidy;
	int found_any_not_at_centroid, found_any, rpindex;
	double sumx, sumy;
	double pin1x = 0.0, pin1y = 0.0;
	int pin_cnt, bott = 0;
	int pinfound[MAXREFPINS];
	double pinx[MAXREFPINS];
	double piny[MAXREFPINS];
	const char *fixed_rotation;
	rnd_coord_t ox = 0, oy = 0;
	pcb_any_obj_t *o;
	pcb_data_it_t it;

	memset(pinfound, 0, sizeof(pinfound));

	if (!autodetect) {
		if (pcb_subc_get_origin(subc, &ox, &oy) != 0)
			rnd_message(RND_MSG_ERROR, "pcb_subc_xy_rot(): can't get subc origin for %s\n", subc->refdes);

		if (pcb_subc_get_side(subc, &bott) != 0)
			rnd_message(RND_MSG_ERROR, "pcb_subc_xy_rot(): can't get subc side for %s\n", subc->refdes);
	}

	/* initialize our pin count and our totals for finding the
	   centriod */
	pin_cnt = 0;
	sumx = 0.0;
	sumy = 0.0;

	/*
	 * iterate over the pins and pads keeping a running count of how
	 * many pins/pads total and the sum of x and y coordinates
	 *
	 * While we're at it, store the location of pin/pad #1 and #2 if
	 * we can find them
	 */
	for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		rnd_coord_t px, py;

		if (o->term == NULL)
			continue;

		if (o->type == PCB_OBJ_PSTK) {
			px = ((pcb_pstk_t *)o)->x;
			py = ((pcb_pstk_t *)o)->y;
		}
		else {
			px = (o->BoundingBox.X1 + o->BoundingBox.X2) / 2;
			py = (o->BoundingBox.Y1 + o->BoundingBox.Y2) / 2;
		}

		sumx += (double)px;
		sumy += (double)py;
		pin_cnt++;

		for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
			if (RND_NSTRCMP(o->term, reference_pin_names[rpindex]) == 0) {
				pinx[rpindex] = (double)px;
				piny[rpindex] = (double)py;
				pinfound[rpindex] = 1;
			}
		}
	}

	if (pin_cnt > 0) {
		centroidx = sumx / (double) pin_cnt;
		centroidy = sumy / (double) pin_cnt;

		if (!autodetect && (RND_NSTRCMP(pcb_attribute_get(&subc->Attributes, "xy-centre"), "origin") == 0)) {
			*cx = ox;
			*cy = oy;
		}
		else {
			*cx = centroidx;
			*cy = centroidy;
		}

		fixed_rotation = pcb_attribute_get(&subc->Attributes, "xy-fixed-rotation");
		if (fixed_rotation != NULL) {
			/* The user specified a fixed rotation */
			*theta = atof(fixed_rotation);
			found_any_not_at_centroid = 1;
			found_any = 1;
		}
		else {
			/* Find first reference pin not at the  centroid  */
			found_any_not_at_centroid = 0;
			found_any = 0;
			*theta = 0.0;
			for (rpindex = 0; reference_pin_names[rpindex] && !found_any_not_at_centroid; rpindex++) {
				if (pinfound[rpindex]) {
					found_any = 1;

					/* Recenter pin "#1" onto the axis which cross at the part
					   centroid */
					pin1x = pinx[rpindex] - *cx;
					pin1y = piny[rpindex] - *cy;

					if (verbose_rot)
						rnd_trace("\npcb_subc_xy_rot: %s pin_cnt=%d pin1x=%d pin1y=%d\n", RND_UNKNOWN(subc->refdes), pin_cnt, pin1x, pin1y);

					/* if only 1 pin, we are doomed */
					if (pin_cnt == 1) {
						*theta = 0;
						found_any_not_at_centroid = 1;
					}
					else {
						if ((pin1x != 0.0) || (pin1y != 0.0))
							*xray_theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);

						/* flip x, to reverse rotation for elements on back */
						if (bott)
							pin1x = -pin1x;

						if ((pin1x != 0.0) || (pin1y != 0.0)) {
							*theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);
							found_any_not_at_centroid = 1;
						}
					}
					if (verbose_rot)
						rnd_trace(" ->theta=%f\n", *theta);
				}
			}

			if (!found_any) {
				rnd_message
					(RND_MSG_WARNING, "pcb_subc_xy_rot: unable to figure out angle because I could\n"
					 "     not find a suitable reference pin of element %s\n"
					 "     Setting to %g degrees\n", RND_UNKNOWN(subc->refdes), *theta);
			}
			else if (!found_any_not_at_centroid) {
				rnd_message
					(RND_MSG_WARNING, "pcb_subc_xy_rot: unable to figure out angle of element\n"
					 "     %s because the reference pin(s) are at the centroid of the part.\n"
					 "     Setting to %g degrees\n", RND_UNKNOWN(subc->refdes), *theta);
			}
		}
	}
}

void pcb_subc_xy_rot_pnp(pcb_subc_t *subc, rnd_coord_t subc_ox, rnd_coord_t subc_oy, rnd_bool on_bottom)
{
	rnd_coord_t cx = subc_ox, cy = subc_oy;
	double rot = 0.0, tmp;
	const char *cent;

	pcb_subc_xy_rot(subc, &cx, &cy, &rot, &tmp, 1);

	/* unless xy-centre or pnp-centre is set to origin, place a pnp origin mark */
	cent = pcb_attribute_get(&subc->Attributes, "xy-centre");
	if (cent == NULL)
		cent = pcb_attribute_get(&subc->Attributes, "pnp-centre");
	if ((cent == NULL) || (strcmp(cent, "origin") != 0))
		pcb_subc_create_aux_point(subc, cx, cy, "pnp-origin");

	/* add the base vector at the origin imported, but with the rotation
	   reverse engineered: the original element format does have an explicit
	   origin but no rotation info */
	pcb_subc_create_aux(subc, subc_ox, subc_oy, rot, on_bottom);
}

