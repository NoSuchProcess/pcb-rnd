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

/* definition of types */
#ifndef	PCB_GLOBAL_H
#define	PCB_GLOBAL_H

#include "config.h"

#include "const.h"

#include "vtroutestyle.h"

/* Make sure to catch usage of non-portable functions in debug mode */
#ifndef NDEBUG
#	undef strdup
#	undef strndup
#	undef snprintf
#	undef round
#	define strdup      never_use_strdup__use_pcb_strdup
#	define strndup     never_use_strndup__use_pcb_strndup
#	define snprintf    never_use_snprintf__use_pcb_snprintf
#	define round       never_use_round__use_pcb_round
#endif

#endif /* PCB_GLOBAL_H  */
