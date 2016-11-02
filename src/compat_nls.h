/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
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
#		define N_(S) (S)
#	endif
#else
#	define _(S) (S)
#	define N_(S) (S)
#	define textdomain(S) (S)
#	define gettext(S) (S)
#	define dgettext(D, S) (S)
#	define dcgettext(D, S, T) (S)
#	define bindtextdomain(D, Dir) (D)
#endif /* ENABLE_NLS */

#endif
