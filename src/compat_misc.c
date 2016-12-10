/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2004, 2006 Dan McMahill
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

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <math.h>
#include "compat_misc.h"
#include "compat_inc.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* On some old systems random() works better than rand(). Unfortunately
random() is less portable than rand(), which is C89. By default, just
use rand(). Later on: scconfig should detect and enable random() if
we find a system where it really breaks. */
#ifdef HAVE_RANDOM
long pcb_rand(void)
{
	return (long) random();
}
#else
long pcb_rand(void)
{
	return (long) rand();
}
#endif

const char *pcb_get_user_name(void)
{
#ifdef HAVE_GETPWUID
	static struct passwd *pwentry;

	int len;
	char *comma, *gecos, *fab_author;

	/* ID the user. */
	pwentry = getpwuid(getuid());
	gecos = pwentry->pw_gecos;
	comma = strchr(gecos, ',');
	if (comma)
		len = comma - gecos;
	else
		len = strlen(gecos);
	fab_author = (char *) malloc(len + 1);
	if (!fab_author) {
		perror("pcb: out of memory.\n");
		exit(-1);
	}
	memcpy(fab_author, gecos, len);
	fab_author[len] = 0;
	return fab_author;
#else
	return "Unknown";
#endif
}

int pcb_getpid(void)
{
	return getpid();
}

char *pcb_strndup(const char *s, int len)
{
	int a, l = strlen(s);
	char *o;

	a = (len < l) ? len : l;
	o = malloc(a+1);
	memcpy(o, s, a);
	o[a] = '\0';
	return o;
}

char *pcb_strdup(const char *s)
{
	int l = strlen(s);
	char *o;
	o = malloc(l+1);
	memcpy(o, s, l+1);
	return o;
}

#ifdef HAVE_ROUND
#undef round
extern double round(double x);
double pcb_round(double x)
{
	return round(x);
}
#else

/* Implementation idea borrowed from an old gcc (GPL'd) */
double pcb_round(double x)
{
	double t;

/* We should check for inf here, but inf is not in C89; if we'd have isinf(),
   we'd have round() as well and we wouldn't be here at all. */

	if (x >= 0.0) {
		t = ceil(x);
		if (t - x > 0.5)
			t -= 1.0;
		return t;
	}

	t = ceil(-x);
	if ((t + x) > 0.5)
		t -= 1.0;
	return -t;
}
#endif

int pcb_strcasecmp(const char *s1, const char *s2)
{
	while(tolower(*s1) == tolower(*s2)) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return tolower(*s1) - tolower(*s2);
}

int pcb_strncasecmp(const char *s1, const char *s2, size_t n)
{
	do {
		if (n == 0)
			return 0;
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
		n--;
	} while(tolower(*s1) == tolower(*s2));
	return tolower(*s1) - tolower(*s2);
}
