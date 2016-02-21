/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "action.h"

#define action_entry(x) { #x, F_ ## x},
static FunctionType Functions[] = {
#include "action_funclist.h"
	{"F_END", F_END}
};

/* ---------------------------------------------------------------------------
 * get function ID of passed string
 */
#define HSIZE 257
static char function_hash[HSIZE];
static int hash_initted = 0;

static int hashfunc(String s)
{
	int i = 0;
	while (*s) {
		i ^= i >> 16;
		i = (i * 13) ^ (unsigned char) tolower((int) *s);
		s++;
	}
	i = (unsigned int) i % HSIZE;
	return i;
}

int GetFunctionID(String Ident)
{
	int i, h;

	if (Ident == 0)
		return -1;

	if (!hash_initted) {
		hash_initted = 1;
		if (HSIZE < ENTRIES(Functions) * 2) {
			fprintf(stderr, _("Error: function hash size too small (%d vs %lu at %s:%d)\n"),
							HSIZE, (unsigned long) ENTRIES(Functions) * 2, __FILE__, __LINE__);
			exit(1);
		}
		if (ENTRIES(Functions) > 254) {
			/* Change 'char' to 'int' and remove this when we get to 256
			   strings to hash. */
			fprintf(stderr, _("Error: function hash type too small (%d vs %lu at %s:%d)\n"),
							256, (unsigned long) ENTRIES(Functions), __FILE__, __LINE__);
			exit(1);

		}
		for (i = ENTRIES(Functions) - 1; i >= 0; i--) {
			h = hashfunc(Functions[i].Identifier);
			while (function_hash[h])
				h = (h + 1) % HSIZE;
			function_hash[h] = i + 1;
		}
	}

	i = hashfunc(Ident);
	while (1) {
		/* We enforce the "hash table bigger than function table" rule,
		   so we know there will be at least one zero entry to find.  */
		if (!function_hash[i])
			return (-1);
		if (!strcasecmp(Ident, Functions[function_hash[i] - 1].Identifier))
			return ((int) Functions[function_hash[i] - 1].ID);
		i = (i + 1) % HSIZE;
	}
}
