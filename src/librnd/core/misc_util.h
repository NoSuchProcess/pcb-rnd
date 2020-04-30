/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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
 *
 */

/* misc - independent of PCB data types */

#ifndef RND_MISC_UTIL_H
#define RND_MISC_UTIL_H

#include <librnd/core/unit.h>
#include <librnd/core/rnd_bool.h>

double rnd_distance(double x1, double y1, double x2, double y2);
double rnd_distance2(double x1, double y1, double x2, double y2); /* distance square */

enum rnd_unit_flags_e { RND_UNIT_PERCENT = 1 };

typedef struct {
	const char *suffix;
	double scale;
	enum rnd_unit_flags_e flags;
} rnd_unit_list_t[];

/* Convert string to coords; if units is not NULL, it's the caller supplied unit
   string; absolute is set to false if non-NULL and val starts with + or -.
   success indicates whether the conversion was successful. */
double rnd_get_value(const char *val, const char *units, rnd_bool *absolute, rnd_bool *success);
double rnd_get_value_ex(const char *val, const char *units, rnd_bool * absolute, rnd_unit_list_t extra_units, const char *default_unit, rnd_bool *success);

/* Convert a string of value+unit to coords and unit struct. Absolute is the same
   as above; if unit_strict is non-zero, require full unit name. Returns whether
   the conversion is succesful. */
rnd_bool rnd_get_value_unit(const char *val, rnd_bool *absolute, int unit_strict, double *val_out, const rnd_unit_t **unit_out);

char *rnd_concat(const char *first, ...); /* end with NULL */
int rnd_mem_any_set(unsigned char *ptr, int bytes);

char *rnd_strdup_strip_wspace(const char *S);

/* Wrap text so that each segment is at most len characters long. Insert
   sep character to break the string into segments. If nonsep >= 0, replace
   original sep chaarcters with nonsep. */
char *rnd_text_wrap(char *inp, int len, int sep, int nonsep);

/* remove leading and trailing whitespace */
char *rnd_str_strip(char *s);

#define RND_ENTRIES(x)         (sizeof((x))/sizeof((x)[0]))
#define RND_UNKNOWN(a)         ((a) && *(a) ? (a) : "(unknown)")
#define RND_NSTRCMP(a, b)      ((a) ? ((b) ? strcmp((a),(b)) : 1) : -1)
#define RND_EMPTY(a)           ((a) ? (a) : "")
#define RND_EMPTY_STRING_P(a)  ((a) ? (a)[0]==0 : 1)
#define RND_XOR(a,b)           (((a) && !(b)) || (!(a) && (b)))

#define rnd_swap(type,a,b) \
do { \
	type __tmp__ = a; \
	a = b; \
	b = __tmp__; \
} while(0)

#endif
