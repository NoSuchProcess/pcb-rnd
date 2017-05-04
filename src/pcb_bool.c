/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "pcb_bool.h"
#include "compat_misc.h"

pcb_bool_op_t pcb_str2boolop(const char *s)
{
	if (pcb_strcasecmp(s, "set"))       return PCB_BOOL_SET;
	if (pcb_strcasecmp(s, "clear"))     return PCB_BOOL_CLEAR;
	if (pcb_strcasecmp(s, "toggle"))    return PCB_BOOL_TOGGLE;
	if (pcb_strcasecmp(s, "preserve"))  return PCB_BOOL_PRESERVE;
	return PCB_BOOL_INVALID;
}
