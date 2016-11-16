/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

/* Resolve paths, build paths using template */

#include "genvector/gds_char.h"

/* Allocate *out and copy the path from in to out, replacing ~ with conf_core.rc.path.home
   If extra_room is non-zero, allocate this many bytes extra for each slot;
   this leaves some room to append a file name. */
void pcb_path_resolve(const char *in, char **out, unsigned int extra_room);

/* Same as resolve_path, but it returns the pointer to the new path and calls
   free() on in */
char *pcb_path_resolve_inplace(char *in, unsigned int extra_room);


/* Resolve all paths from a in[] into out[](should be large enough) */
void pcb_paths_resolve(const char **in, char **out, int numpaths, unsigned int extra_room);

/* Resolve all paths from a char *in[] into a freshly allocated char **out */
#define pcb_paths_resolve_all(in, out, extra_room) \
do { \
	int __numpath__ = sizeof(in) / sizeof(char *); \
	if (__numpath__ > 0) { \
		out = malloc(sizeof(char *) * __numpath__); \
		pcb_paths_resolve(in, out, __numpath__, extra_room); \
	} \
} while(0)


/* generic file name template substitution callbacks for pcb_strdup_subst:
    %P    pid
    %F    load-time file name of the current pcb
    %B    basename (load-time file name of the current pcb without path)
    %D    dirname (load-time file path of the current pcb, without file name, with trailing slash, might be ./)
    %N    name of the current pcb
    %T    wall time (Epoch)
*/
int pcb_build_fn_cb(void *ctx, gds_t *s, const char **input);

char *pcb_build_fn(const char *template);


/* Same as above, but also replaces lower case formatting to the members of
   the array if they are not NULL; use with pcb_build_argfn() */
typedef struct {
	const char *params['z' - 'a' + 1]; /* [0] for 'a' */
} pcb_build_argfn_t;

char *pcb_build_argfn(const char *template, pcb_build_argfn_t *arg);

int pcb_build_argfn_cb(void *ctx, gds_t *s, const char **input);

