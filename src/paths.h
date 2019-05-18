/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016, 2017 Tibor 'Igor2' Palinkas
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

/* Resolve paths, build paths using template */

#include "genvector/gds_char.h"
#include "global_typedefs.h"

/* Allocate *out and copy the path from in to out, replacing ~ with conf_core.rc.path.home
   If extra_room is non-zero, allocate this many bytes extra for each slot;
   this leaves some room to append a file name. If quiet is non-zero, suppress
   error messages. */
void pcb_path_resolve(pcb_hidlib_t *hidlib, const char *in, char **out, unsigned int extra_room, int quiet);

/* Same as resolve_path, but it returns the pointer to the new path and calls
   free() on in */
char *pcb_path_resolve_inplace(pcb_hidlib_t *hidlib, char *in, unsigned int extra_room, int quiet);


/* Resolve all paths from a in[] into out[](should be large enough) */
void pcb_paths_resolve(pcb_hidlib_t *hidlib, const char **in, char **out, int numpaths, unsigned int extra_room, int quiet);

/* Resolve all paths from a char *in[] into a freshly allocated char **out */
#define pcb_paths_resolve_all(hidlib, in, out, extra_room, quiet) \
do { \
	int __numpath__; \
	for(__numpath__ = 0; in[__numpath__] != NULL; __numpath__++) ; \
	if (__numpath__ > 0) { \
		out = malloc(sizeof(char *) * __numpath__); \
		pcb_paths_resolve(hidlib, in, out, __numpath__, extra_room, quiet); \
	} \
} while(0)


/* generic file name template substitution callbacks for pcb_strdup_subst:
    %P    pid
    %F    load-time file name of the current pcb
    %B    basename (load-time file name of the current pcb without path)
    %D    dirname (load-time file path of the current pcb, without file name, with trailing slash, might be ./)
    %N    name of the current pcb
    %T    wall time (Epoch)
   ctx must be the current (pcb_hidlib_t *)
*/
int pcb_build_fn_cb(void *ctx, gds_t *s, const char **input);

char *pcb_build_fn(pcb_hidlib_t *hidlib, const char *template);


/* Same as above, but also replaces lower case formatting to the members of
   the array if they are not NULL; use with pcb_build_argfn() */
typedef struct {
	const char *params['z' - 'a' + 1]; /* [0] for 'a' */
	pcb_hidlib_t *hidlib; /* if NULL, some of the substitutions (e.g. %B, %D, %N) won't be performed */
} pcb_build_argfn_t;

char *pcb_build_argfn(const char *template, pcb_build_argfn_t *arg);

int pcb_build_argfn_cb(void *ctx, gds_t *s, const char **input);

