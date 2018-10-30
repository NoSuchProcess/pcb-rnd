/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_COMPAT_MISC_H
#define PCB_COMPAT_MISC_H

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
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

/* Sleep for the specified number if miliseconds */
void pcb_ms_sleep(long ms);

/* Return system time (local, time zoned) in seconds and microseconds
   since epoch. (gettimeofday() wrapper) */
void pcb_ltime(unsigned long  *secs, unsigned long *usecs);

/* Return system time (local, time zoned) since epoch. (gettimeofday() wrapper) */
double pcb_dtime(void);

int pcb_fileno(FILE *f);

#endif /* PCB_COMPAT_MISC_H */
