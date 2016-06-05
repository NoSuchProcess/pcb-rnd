#ifndef PCB_PLUG_FOOTPRINT_H
#define PCB_PLUG_FOOTPRINT_H

#include <stdio.h>
#include "vtlibrary.h"

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

/* init/uninit the footprint lib, free tag key memory */
void fp_init();
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

extern library_t library; /* the footprint library */

#define get_library_memory(parent) vtlib_alloc_append(((parent) == NULL ? &library.data.dir.children : &(parent)->data.dir.children), 1);

void fp_free_children(library_t *parent);
void fp_sort_children(library_t *parent);
void fp_rmdir(library_t *dir);
library_t *fp_mkdir_p(const char *path);
library_t *fp_mkdir_len(library_t *parent, const char *name, int name_len);
library_t *fp_lib_search(library_t *dir, const char *name);

#ifndef PCB_NO_GLUE
/* Return the library shell string (from Settings) */
const char *fp_get_library_shell(void);

/* Append a menu entry in the tree */
library_t *fp_append_entry(library_t *parent, const char *name, fp_type_t type, void *tags[]);

/* walk through all lib paths and build the library menu */
int fp_read_lib_all(void);


const char *fp_default_search_path(void);

int fp_host_uninit(void);

#endif
#endif
