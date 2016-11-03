/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 */

#include <assert.h>
#include "config.h"
#include "fptr_cast.h"

void *pcb_cast_f2d(pcb_fptr_t f)
{
	assert(sizeof(void *) == sizeof(pcb_fptr_t));
	return (void *)f;
}

pcb_fptr_t pcb_cast_d2f(void *d)
{
	assert(sizeof(void *) == sizeof(pcb_fptr_t));
	return (pcb_fptr_t)d;
}

