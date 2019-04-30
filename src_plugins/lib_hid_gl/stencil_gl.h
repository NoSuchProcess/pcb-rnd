/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2017 PCB Contributers (See ChangeLog for details)
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

#ifndef STENCIL_GL_H
#define STENCIL_GL_H
/*
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
*/

#include "opengl.h"

void stencilgl_init();
int stencilgl_bit_count();
void stencilgl_clear_stencil_bits(int bits);
void stencilgl_clear_unassigned_stencil();
int stencilgl_allocate_clear_stencil_bit();
void stencilgl_return_stencil_bit(int bit);
void stencilgl_reset_stencil_usage();

/* stencilgl_mode_write_set
 * Setup the stencil buffer so that writes will set stencil bits
 */
static inline void stencilgl_mode_write_set(int bits)
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(bits);
	glStencilFunc(GL_ALWAYS, bits, bits);
}

/* stencilgl_mode_write_clear
 * Setup the stencil buffer so that writes will clear stencil bits
 */
static inline void stencilgl_mode_write_clear(int bits)
{
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
	glStencilMask(bits);
	glStencilFunc(GL_ALWAYS, bits, bits);
}

#endif /* !defined STENCIL_GL_H */
