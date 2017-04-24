/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Local functions to draw a layer group as a composite of logical layers
   using positive and negative draw operations. Included from draw.c. */

static void mask_start_sub_(int thin, const pcb_box_t *screen)
{
	if (thin)
		pcb_gui->set_color(Output.pmGC, conf_core.appearance.color.mask);
	else
		pcb_gui->use_mask(HID_MASK_CLEAR);
}

static void mask_start_add_(int thin, const pcb_box_t *screen)
{
	if (thin)
		pcb_gui->set_color(Output.pmGC, "erase");
	else
		pcb_gui->use_mask(HID_MASK_SET);
}

static void mask_start_sub(int thin, const pcb_box_t *screen)
{
	if (pcb_gui->mask_invert)
		mask_start_add_(thin, screen);
	else
		mask_start_sub_(thin, screen);
}

static void mask_start_add(int thin, const pcb_box_t *screen)
{
	if (pcb_gui->mask_invert)
		mask_start_sub_(thin, screen);
	else
		mask_start_add_(thin, screen);
}

static void mask_finish(int thin, const pcb_box_t *screen)
{
	if (!thin) {
		pcb_gui->use_mask(HID_MASK_AFTER);
		DrawMaskBoardArea(HID_MASK_AFTER, screen);
		pcb_gui->use_mask(HID_MASK_OFF);
	}
}

static void mask_init(int thin, const pcb_box_t *screen, int negative)
{
	pcb_gui->use_mask(HID_MASK_INIT);

	if (pcb_gui->mask_invert)
		negative = !negative;

	if ((!thin) && (negative)) {
		/* old way of drawing the big poly for the negative */
		DrawMaskBoardArea(HID_MASK_BEFORE, screen);

		if (!pcb_gui->poly_before) {
			/* new way */
			pcb_gui->use_mask(HID_MASK_SET);
			DrawMaskBoardArea(HID_MASK_SET, screen);
		}
	}
}
