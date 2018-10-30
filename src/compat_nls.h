/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef PCB_COMPAT_NLS
#define PCB_COMPAT_NLS

/* Internationalization support. */
#ifdef ENABLE_NLS
#	include <libintl.h>
#	define _(S) gettext(S)
#	if defined(gettext_noop)
#		define N_(S) gettext_noop(S)
#	else
#		define N_(S) S
#	endif
#else
#	define _(S) S
#	define N_(S) S
#	define textdomain(S) (S)
#	define gettext(S) (S)
#	define dgettext(D, S) (S)
#	define dcgettext(D, S, T) (S)
#	define bindtextdomain(D, Dir) (D)
#	define pcb_setlocale(a, b)
#endif /* ENABLE_NLS */

#endif
