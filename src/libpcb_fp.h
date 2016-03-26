#ifndef LIBPCB_FP_H
#define LIBPCB_FP_H

#include "plug_footprint.h"

/* List all symbols, optionally recursively, from CWD/subdir. For each symbol
   or subdir, call the callback. Ignore file names starting with
   If subdir_may_not_exist is non-zero, don't complain if the top subdir does not exist.
*/
int pcb_fp_list(const char *subdir, int recurse,
								int (*cb) (void *cookie, const char *subdir, const char *name, fp_type_t type, void *tags[]), void *cookie,
								int subdir_may_not_exist, int need_tags);

/* Decide about the type of a footprint file by reading the content and
   optionally extract tag IDs into a void *tags[] */
fp_type_t pcb_fp_file_type(const char *fn, void ***tags);

/* Open a footprint for reading; if the footprint is parametric, it's run
   prefixed with libshell (or executed directly, if libshell is NULL).
   If name is not an absolute path, search_path is searched for the first match.
   The user has to supply a state integer that will be used by pcb_fp_fclose().
 */
FILE *pcb_fp_fopen(const char *libshell, const char *search_path, const char *name, int *state);

/* Close the footprint file opened by pcb_fp_fopen(). */
void pcb_fp_fclose(FILE * f, int *st);


#endif
