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

#include <sys/types.h>
#include <math.h>
#include "compat_misc.h"
#include "global.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef HAVE_EXPF
float expf(float x)
{
	return (float) exp((double) x);
}
#endif

#ifndef HAVE_LOGF
float logf(float x)
{
	return (float) log((double) x);
}
#endif

/* On some old systems random() works better than rand(). Unfrtunately
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

const char *get_user_name(void)
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
