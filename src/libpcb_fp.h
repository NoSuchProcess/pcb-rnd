#ifndef LIBPCB_FP_H
#define LIBPCB_FP_H
typedef enum {
	PCB_FP_INVALID,
	PCB_FP_DIR,
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} pcb_fp_type_t;

/* List all symbols, optionally recursively, from CWD/subdir. For each symbol
   or subdir, call the callback. Ignore file names starting with
   If subdir_may_not_exist is non-zero, don't complain if the top subdir does not exist.
*/
int pcb_fp_list(const char *subdir, int recurse,
								int (*cb) (void *cookie, const char *subdir, const char *name, pcb_fp_type_t type, void *tags[]), void *cookie,
								int subdir_may_not_exist, int need_tags);

/* Decide about the type of a footprint file by reading the content and
   optionally extract tag IDs into a void *tags[] */
pcb_fp_type_t pcb_fp_file_type(const char *fn, void ***tags);

/* duplicates the name and splits it into a basename and params;
   params is NULL if the name is not parametric (and "" if parameter list is empty)
   returns 1 if name is parametric, 0 if file element.
   The caller shall free only *basename at the end.
   */
int pcb_fp_dupname(const char *name, char **basename, char **params);


/* walk the search_path for finding the first footprint for basename (shall not contain "(") */
char *pcb_fp_search(const char *search_path, const char *basename, int parametric);

/* Open a footprint for reading; if the footprint is parametric, it's run
   prefixed with libshell (or executed directly, if libshell is NULL).
   If name is not an absolute path, search_path is searched for the first match.
   The user has to supply a state integer that will be used by pcb_fp_fclose().
 */
FILE *pcb_fp_fopen(const char *libshell, const char *search_path, const char *name, int *state);

/* Close the footprint file opened by pcb_fp_fopen(). */
void pcb_fp_fclose(FILE * f, int *st);

/**** tag management ****/
/* Resolve a tag name to an unique void * ID; create unknown tag if alloc != 0 */
const void *pcb_fp_tag(const char *tag, int alloc);

/* Resolve a tag ID to a tag name */
const char *pcb_fp_tagname(const void *tagid);

#endif
