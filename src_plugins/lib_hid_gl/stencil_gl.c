/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2011 PCB Contributers (See ChangeLog for details)
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
 */

#include <stdio.h>
#include <stdlib.h>

#include "stencil_gl.h"

static GLint stencil_bits = 0;
static int dirty_bits = 0;
static int assigned_bits = 0;


int stencilgl_bit_count()
{
	return stencil_bits;
}

void stencilgl_clear_stencil_bits(int bits)
{
	glPushAttrib(GL_STENCIL_BUFFER_BIT);
	glStencilMask(bits);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glPopAttrib();

	dirty_bits = (dirty_bits & ~bits) | assigned_bits;
}

void stencilgl_clear_unassigned_stencil()
{
	stencilgl_clear_stencil_bits(~assigned_bits);
}

int stencilgl_allocate_clear_stencil_bit(void)
{
	int stencil_bitmask = (1 << stencil_bits) - 1;
	int test;
	int first_dirty = 0;

	if (assigned_bits == stencil_bitmask) {
		printf("No more stencil bits available, total of %i already assigned\n", stencil_bits);
		return 0;
	}

	/* Look for a bitplane we don't have to clear */
	for(test = 1; test & stencil_bitmask; test <<= 1) {
		if (!(test & dirty_bits)) {
			assigned_bits |= test;
			dirty_bits |= test;
			return test;
		}
		else if (!first_dirty && !(test & assigned_bits)) {
			first_dirty = test;
		}
	}

	/* Didn't find any non dirty planes. Clear those dirty ones which aren't in use */
	stencilgl_clear_unassigned_stencil();
	assigned_bits |= first_dirty;
	dirty_bits = assigned_bits;

	return first_dirty;
}

void stencilgl_return_stencil_bit(int bit)
{
	assigned_bits &= ~bit;
}

void stencilgl_reset_stencil_usage()
{
	assigned_bits = 0;
}

void stencilgl_init()
{
	glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);

	if (stencil_bits == 0) {
		printf("No stencil bits available.\n" "Cannot mask polygon holes or subcomposite layers\n");
		/* TODO: Flag this to the HID so it can revert to the dicer? */
	}
	else if (stencil_bits == 1) {
		printf("Only one stencil bitplane avilable\n" "Cannot use stencil buffer to sub-composite layers.\n");
		/* Do we need to disable that somewhere? */
	}

	stencilgl_reset_stencil_usage();
	stencilgl_clear_unassigned_stencil();
}
