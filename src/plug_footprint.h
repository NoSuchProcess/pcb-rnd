#ifndef PCB_PLUG_FOOTPRINT_H
#define PCB_PLUG_FOOTPRINT_H

#include <stdio.h>

typedef struct plug_fp_s plug_fp_t;

typedef struct {
	plug_fp_t *backend;
	union {
		int i;
		void *p;
	} field[4];
} fp_fopen_ctx_t;

/* hook bindings, see below */
FILE *fp_fopen(const char *path, const char *name, fp_fopen_ctx_t *fctx);
void fp_fclose(FILE * f, fp_fopen_ctx_t *fctx);


typedef enum {
	PCB_FP_INVALID,
	PCB_FP_DIR,
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} fp_type_t;

/* duplicates the name and splits it into a basename and params;
   params is NULL if the name is not parametric (and "" if parameter list is empty)
   returns 1 if name is parametric, 0 if file element.
   The caller shall free only *basename at the end.
   */
int fp_dupname(const char *name, char **basename, char **params);

/* walk the search_path for finding the first footprint for basename (shall not contain "(") */
////// TODO
char *fp_fs_search(const char *search_path, const char *basename, int parametric);

/**** tag management ****/
/* Resolve a tag name to an unique void * ID; create unknown tag if alloc != 0 */
const void *fp_tag(const char *tag, int alloc);

/* Resolve a tag ID to a tag name */
const char *fp_tagname(const void *tagid);

/* Uninit the footprint lib, free tag key memory */
void fp_uninit();

/**************************** API definition *********************************/
struct plug_fp_s {
	plug_fp_t *next;
	void *plugin_data;

	/* returns the number of footprints loaded into the library or -1 on
	   error; next in chain is run only on error. */
	int (*load_dir)(plug_fp_t *ctx, const char *path);

/* Open a footprint for reading; if the footprint is parametric, it's run
   prefixed with libshell (or executed directly, if libshell is NULL).
   If name is not an absolute path, search_path is searched for the first match.
   The user has to supply a state integer that will be used by pcb_fp_fclose().
   Must fill in fctx->backend, may use any other field of fctx as well.
 */
	FILE *(*fopen)(plug_fp_t *ctx, const char *path, const char *name, fp_fopen_ctx_t *fctx);

/* Close the footprint file opened by pcb_fp_fopen(). */
	void (*fclose)(plug_fp_t *ctx, FILE * f, fp_fopen_ctx_t *fctx);
};

extern plug_fp_t *plug_fp_chain;


/* Optional pcb-rnd-side glue for some implementations */

/* Return the library shell string (from Settings) */
const char *fp_get_library_shell(void);

/* walk through all lib paths and build the library menu */
int fp_read_lib_all(void);

#endif
