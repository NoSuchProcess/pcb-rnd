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

#ifndef PCB_COMPAT_CC_H
#define PCB_COMPAT_CC_H

/* ---------------------------------------------------------------------------
 * Macros to annotate branch-prediction information.
 * Taken from GLib 2.16.3 (LGPL 2).G_ / g_ prefixes have
 * been removed to avoid namespace clashes.
 */

/* The LIKELY and UNLIKELY macros let the programmer give hints to
 * the compiler about the expected result of an expression. Some compilers
 * can use this information for optimizations.
 *
 * The PCB_BOOLEAN_EXPR macro is intended to trigger a gcc warning when
 * putting assignments inside the test.
 */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#	define PCB_BOOLEAN_EXPR(expr)                   \
 __extension__ ({                             \
   int _boolean_var_;                         \
   if (expr)                                  \
      _boolean_var_ = 1;                      \
   else                                       \
      _boolean_var_ = 0;                      \
   _boolean_var_;                             \
})
#	define PCB_LIKELY(expr) (__builtin_expect (PCB_BOOLEAN_EXPR(expr), 1))
#	define PCB_UNLIKELY(expr) (__builtin_expect (PCB_BOOLEAN_EXPR(expr), 0))
#else
#	define PCB_LIKELY(expr) (expr)
#	define PCB_UNLIKELY(expr) (expr)
#endif

#endif
