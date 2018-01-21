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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "elem_rot.h"
#include "conf_core.h"

static const int verbose_rot = 0;

/* In order of preference.
 * Includes numbered and BGA pins.
 * Possibly BGA pins can be missing, so we add a few to try. */
#define MAXREFPINS 32 /* max length of following list */
static char *reference_pin_names[] = {"1", "2", "A1", "A2", "B1", "B2", 0};

static double xyToAngle(double x, double y, pcb_bool morethan2pins)
{
	double d = atan2(-y, x) * 180.0 / M_PI;

		if (verbose_rot)
			pcb_trace(" xyToAngle: %f %f %d ->%f\n", x, y, morethan2pins, d);

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

void pcb_elem_xy_rot(pcb_element_t *element, pcb_coord_t *cx, pcb_coord_t *cy, double *theta, double *xray_theta)
{
	double padcentrex, padcentrey;
	double centroidx, centroidy;
	int found_any_not_at_centroid, found_any, rpindex;
	double sumx, sumy;
	double pin1x = 0.0, pin1y = 0.0;
	int pin_cnt;
	int pinfound[MAXREFPINS];
	double pinx[MAXREFPINS];
	double piny[MAXREFPINS];
	double pinangle[MAXREFPINS];
	const char *fixed_rotation;

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
	PCB_PIN_LOOP(element);
	{
		sumx += (double) pin->X;
		sumy += (double) pin->Y;
		pin_cnt++;

		for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
			if (PCB_NSTRCMP(pin->Number, reference_pin_names[rpindex]) == 0) {
				pinx[rpindex] = (double) pin->X;
				piny[rpindex] = (double) pin->Y;
				pinangle[rpindex] = 0.0;	/* pins have no notion of angle */
				pinfound[rpindex] = 1;
			}
		}
	}
	PCB_END_LOOP;

	PCB_PAD_LOOP(element);
	{
		sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
		sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
		pin_cnt++;

		for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
			if (PCB_NSTRCMP(pad->Number, reference_pin_names[rpindex]) == 0) {
				padcentrex = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
				padcentrey = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
				pinx[rpindex] = padcentrex;
				piny[rpindex] = padcentrey;
				/*
				 * NOTE: We swap the Y points because in PCB, the Y-axis
				 * is inverted.  Increasing Y moves down.  We want to deal
				 * in the usual increasing Y moves up coordinates though.
				 */
				pinangle[rpindex] = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y, pad->Point2.X - pad->Point1.X);
				pinfound[rpindex] = 1;
			}
		}
	}
	PCB_END_LOOP;

	if (pin_cnt > 0) {
		centroidx = sumx / (double) pin_cnt;
		centroidy = sumy / (double) pin_cnt;

		if (PCB_NSTRCMP(pcb_attribute_get(&element->Attributes, "xy-centre"), "origin") == 0) {
			*cx = element->MarkX;
			*cy = element->MarkY;
		}
		else {
			*cx = centroidx;
			*cy = centroidy;
		}

		fixed_rotation = pcb_attribute_get(&element->Attributes, "xy-fixed-rotation");
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
						pcb_trace("\npcb_elem_xy_rot: %s pin_cnt=%d pin1x=%d pin1y=%d\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), pin_cnt, pin1x, pin1y);

					/* if only 1 pin, use pin 1's angle */
					if (pin_cnt == 1) {
						*theta = pinangle[rpindex];
						found_any_not_at_centroid = 1;
					}
					else {
						if ((pin1x != 0.0) || (pin1y != 0.0))
							*xray_theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);

						/* flip x, to reverse rotation for elements on back */
						if (PCB_FRONT(element) != 1)
							pin1x = -pin1x;

						if ((pin1x != 0.0) || (pin1y != 0.0)) {
							*theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);
							found_any_not_at_centroid = 1;
						}
					}
					if (verbose_rot)
						pcb_trace(" ->theta=%f\n", *theta);
				}
			}

			if (!found_any) {
				pcb_message
					(PCB_MSG_WARNING, "pcb_elem_xy_rot: unable to figure out angle because I could\n"
					 "     not find a suitable reference pin of element %s\n"
					 "     Setting to %g degrees\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), *theta);
			}
			else if (!found_any_not_at_centroid) {
				pcb_message
					(PCB_MSG_WARNING, "pcb_elem_xy_rot: unable to figure out angle of element\n"
					 "     %s because the reference pin(s) are at the centroid of the part.\n"
					 "     Setting to %g degrees\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), *theta);
			}
		}
	}
}

void pcb_subc_xy_rot(pcb_element_t *element, pcb_coord_t *cx, pcb_coord_t *cy, double *theta, double *xray_theta)
{
	double padcentrex, padcentrey;
	double centroidx, centroidy;
	int found_any_not_at_centroid, found_any, rpindex;
	double sumx, sumy;
	double pin1x = 0.0, pin1y = 0.0;
	int pin_cnt;
	int pinfound[MAXREFPINS];
	double pinx[MAXREFPINS];
	double piny[MAXREFPINS];
	double pinangle[MAXREFPINS];
	const char *fixed_rotation;

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
	PCB_PIN_LOOP(element);
	{
		sumx += (double) pin->X;
		sumy += (double) pin->Y;
		pin_cnt++;

		for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
			if (PCB_NSTRCMP(pin->Number, reference_pin_names[rpindex]) == 0) {
				pinx[rpindex] = (double) pin->X;
				piny[rpindex] = (double) pin->Y;
				pinangle[rpindex] = 0.0;	/* pins have no notion of angle */
				pinfound[rpindex] = 1;
			}
		}
	}
	PCB_END_LOOP;

	PCB_PAD_LOOP(element);
	{
		sumx += (pad->Point1.X + pad->Point2.X) / 2.0;
		sumy += (pad->Point1.Y + pad->Point2.Y) / 2.0;
		pin_cnt++;

		for (rpindex = 0; reference_pin_names[rpindex]; rpindex++) {
			if (PCB_NSTRCMP(pad->Number, reference_pin_names[rpindex]) == 0) {
				padcentrex = (double) (pad->Point1.X + pad->Point2.X) / 2.0;
				padcentrey = (double) (pad->Point1.Y + pad->Point2.Y) / 2.0;
				pinx[rpindex] = padcentrex;
				piny[rpindex] = padcentrey;
				/*
				 * NOTE: We swap the Y points because in PCB, the Y-axis
				 * is inverted.  Increasing Y moves down.  We want to deal
				 * in the usual increasing Y moves up coordinates though.
				 */
				pinangle[rpindex] = (180.0 / M_PI) * atan2(pad->Point1.Y - pad->Point2.Y, pad->Point2.X - pad->Point1.X);
				pinfound[rpindex] = 1;
			}
		}
	}
	PCB_END_LOOP;

	if (pin_cnt > 0) {
		centroidx = sumx / (double) pin_cnt;
		centroidy = sumy / (double) pin_cnt;

		if (PCB_NSTRCMP(pcb_attribute_get(&element->Attributes, "xy-centre"), "origin") == 0) {
			*cx = element->MarkX;
			*cy = element->MarkY;
		}
		else {
			*cx = centroidx;
			*cy = centroidy;
		}

		fixed_rotation = pcb_attribute_get(&element->Attributes, "xy-fixed-rotation");
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
						pcb_trace("\npcb_elem_xy_rot: %s pin_cnt=%d pin1x=%d pin1y=%d\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), pin_cnt, pin1x, pin1y);

					/* if only 1 pin, use pin 1's angle */
					if (pin_cnt == 1) {
						*theta = pinangle[rpindex];
						found_any_not_at_centroid = 1;
					}
					else {
						if ((pin1x != 0.0) || (pin1y != 0.0))
							*xray_theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);

						/* flip x, to reverse rotation for elements on back */
						if (PCB_FRONT(element) != 1)
							pin1x = -pin1x;

						if ((pin1x != 0.0) || (pin1y != 0.0)) {
							*theta = xyToAngle(pin1x, pin1y, pin_cnt > 2);
							found_any_not_at_centroid = 1;
						}
					}
					if (verbose_rot)
						pcb_trace(" ->theta=%f\n", *theta);
				}
			}

			if (!found_any) {
				pcb_message
					(PCB_MSG_WARNING, "pcb_elem_xy_rot: unable to figure out angle because I could\n"
					 "     not find a suitable reference pin of element %s\n"
					 "     Setting to %g degrees\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), *theta);
			}
			else if (!found_any_not_at_centroid) {
				pcb_message
					(PCB_MSG_WARNING, "pcb_elem_xy_rot: unable to figure out angle of element\n"
					 "     %s because the reference pin(s) are at the centroid of the part.\n"
					 "     Setting to %g degrees\n", PCB_UNKNOWN(PCB_ELEM_NAME_REFDES(element)), *theta);
			}
		}
	}
}
