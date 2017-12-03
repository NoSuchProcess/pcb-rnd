/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2004 Dan McMahill
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
 */

#ifndef PCB_COMPAT_MISC_H
#define PCB_COMPAT_MISC_H

#include "config.h"

#include <stdlib.h>
#include <math.h>
#include <time.h>

long pcb_rand(void);

/* Gets the user's real name when available; normally shouldn't be used,
   consider using pcb_author() that allows config override */
const char *pcb_get_user_name(void);

int pcb_getpid(void);

char *pcb_strndup(const char *s, int len);
char *pcb_strdup(const char *s);

#define pcb_strdup_null(x) (((x) != NULL) ? pcb_strdup (x) : NULL)

double pcb_round(double x);

int pcb_strcasecmp(const char *s1, const char *s2);
int pcb_strncasecmp(const char *s1, const char *s2, size_t n);

int pcb_setenv(const char *name, const char *val, int overwrite);

/* Print a date in UTC; if when is 0, print current date */
size_t pcb_print_utc(char *out, size_t out_len, time_t when);

#endif /* PCB_COMPAT_MISC_H */
