/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2011 Andrew Poelstra
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
 *  Contact addresses for paper mail and Email:
 *  Andrew Poelstra, 16966 60A Ave, V3S 8X5 Surrey, BC, Canada
 *  asp11@sfu.ca
 *
 */

/* This file defines a wrapper around sprintf, that
 *  defines new specifiers that take pcb Coord objects
 *  as input.
 *
 * There is a fair bit of nasty (repetitious) code in
 *  here, but I feel the gain in clarity for output
 *  code elsewhere in the project will make it worth
 *  it.
 *
 * The new specifiers are:
 *   %mI    outout a raw internal coordinate without any suffix
 *   %mm    output a measure in mm
 *   %mM    output a measure in scaled (mm/um) metric
 *   %ml    output a measure in mil
 *   %mk    output a measure in decimil (kicad legacy format)
 *   %mL    output a measure in scaled (mil/in) imperial
 *   %ms    output a measure in most natural mm/mil units
 *   %mS    output a measure in most natural scaled units
 *   %mH    output a measure in most human readable scaled units
 *   %md    output a pair of measures in most natural mm/mil units
 *   %mD    output a pair of measures in most natural scaled units
 *   %m3    output 3 measures in most natural scaled units
 *     ...
 *   %m9    output 9 measures in most natural scaled units
 *   %m*    output a measure with unit given as an additional
 *          const char* parameter
 *   %m+    accepts an e_allow parameter that masks all subsequent
 *          "natural" (S/D/3/.../9) specifiers to only use certain
 *          units
 *   %mr    output a measure in a unit readable by parse_l.l
 *          (outputs in centimil without units - compatibility with mainline)
 *   %ma    output an angle in degrees (expects degrees)
 *
 * These accept the usual printf modifiers for %f, as well as
 *     $    output a unit suffix after the measure (with space between measure and unit)
 *     $$   output a unit suffix after the measure (without space)
 *     .n   number of digits after the decimal point (the usual %f modifier)
 *     .0n  where n is a digit; same as %.n, but truncates trailing zeros
 *     [n]  use stored format n
 *     #    prevents all scaling for %mS/D/1/.../9 (this should
 *          ONLY be used for debug code since its output exposes
 *          pcb's base units).
 *
 * The usual printf(3) precision and length modifiers should work with
 * any format specifier that outputs coords, e.g. %.3mm will output in
 * mm up to 3 decimal digits after the decimal point.
 *
 * KNOWN ISSUES:
 *   No support for %zu size_t printf spec
 */

#ifndef	PCB_PCB_PRINTF_H
#define	PCB_PCB_PRINTF_H

#include <stdio.h>
#include <stdarg.h>
#include <genvector/gds_char.h>
#include "unit.h"

void initialize_units();

int pcb_fprintf(FILE * f, const char *fmt, ...);
int pcb_vfprintf(FILE * f, const char *fmt, va_list args);
int pcb_sprintf(char *string, const char *fmt, ...);
int pcb_snprintf(char *string, size_t len, const char *fmt, ...);
int pcb_vsnprintf(char *string, size_t len, const char *fmt, va_list args);
int pcb_printf(const char *fmt, ...);
char *pcb_strdup_printf(const char *fmt, ...);
char *pcb_strdup_vprintf(const char *fmt, va_list args);

int pcb_append_printf(gds_t *str, const char *fmt, ...);
int pcb_append_vprintf(gds_t *string, const char *fmt, va_list args);

/* Predefined slots (macros): %[n] will use the nth string from this list.
   The first 8 are user-definable. */
typedef enum {
	PCB_PRINTF_SLOT_USER0,
	PCB_PRINTF_SLOT_USER1,
	PCB_PRINTF_SLOT_USER2,
	PCB_PRINTF_SLOT_USER3,
	PCB_PRINTF_SLOT_USER4,
	PCB_PRINTF_SLOT_USER5,
	PCB_PRINTF_SLOT_USER6,
	PCB_PRINTF_SLOT_USER7,
	PCB_PRINTF_SLOT_FF_ORIG_COORD, /* %[8] original .pcb file format coord prints */
	PCB_PRINTF_SLOT_FF_SAFE_COORD, /* %[9] safe .pcb file format coord print that doesn't lose precision */
	PCB_PRINTF_SLOT_max
} pcb_printf_slot_idx_t;
extern const char *pcb_printf_slot[PCB_PRINTF_SLOT_max];

/* strdup and return a template; also attempt to replace printf-like formatting
   directives (e.g. %P) using an user provided callback function. The callback
   function needs to recognize the directive at **input (pointing to the byte
   after the %) and append the substitution to s and increase *input to point
   beyond the format directive. The callback returns 0 on success or -1
   on unknown directive (whcih will be copied verbatim). %% will always
   be translated into a single %, without calling cb.
*/
char *pcb_strdup_subst(const char *template, int (*cb)(void *ctx, gds_t *s, const char **input), void *ctx);

#endif
