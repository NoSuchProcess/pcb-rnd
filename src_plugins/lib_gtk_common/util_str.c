/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

/* This module, was originally written by Bill Wilson and the functions
 * here are Copyright (C) 2004 by Bill Wilson.  These functions are utility
 * functions which are taken from my other GPL'd projects gkrellm and
 * gstocks and are copied here for the Gtk PCB port.
 *
 * pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
 */

#include "config.h"
#include <string.h>
#include "util_str.h"

gboolean pcb_gtk_g_strdup(gchar ** dst, const gchar * src)
{
	if ((dst == NULL) || ((*dst == NULL) && (src == NULL)))
		return FALSE;
	if (*dst) {
		if (src && !strcmp(*dst, src))
			return FALSE;
		g_free(*dst);
	}
	*dst = g_strdup(src);
	return TRUE;
}

const gchar *pcb_str_strip_left(const gchar * s)
{
	if (s != NULL) {
		while ((*s == ' ') || (*s == '\t'))
			s++;
	}
	return s;
}
