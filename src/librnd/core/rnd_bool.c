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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <librnd/config.h>
#include <librnd/core/rnd_bool.h>
#include <librnd/core/compat_misc.h>

rnd_bool_op_t rnd_str2boolop(const char *s)
{
	if (rnd_strcasecmp(s, "set") == 0)       return RND_BOOL_SET;
	if (rnd_strcasecmp(s, "clear") == 0)     return RND_BOOL_CLEAR;
	if (rnd_strcasecmp(s, "toggle") == 0)    return RND_BOOL_TOGGLE;
	if (rnd_strcasecmp(s, "preserve") == 0)  return RND_BOOL_PRESERVE;
	return RND_BOOL_INVALID;
}
