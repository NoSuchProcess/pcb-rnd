#ifndef PCB_PLUG_FOOTPRINT_H
#define PCB_PLUG_FOOTPRINT_H

#include <stdio.h>
#include <genvector/vtp0.h>
#include "data.h"
#include <librnd/core/conf.h>

typedef enum {
	PCB_LIB_INVALID,
	PCB_LIB_DIR,
	PCB_LIB_FOOTPRINT
} pcb_fplibrary_type_t;

typedef enum {
	PCB_FP_INVALID,
	PCB_FP_DIR,       /* used temporarily during the mapping - a finalized tree wouldn't have this */
	PCB_FP_FILEDIR,   /* used temporarily during the mapping - a finalized tree wouldn't have this */
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} pcb_fptype_t;


typedef struct pcb_fplibrary_s pcb_fplibrary_t;

/* An element of a library: either a directory or a footprint */
struct pcb_fplibrary_s {
	char *name;            /* visible name */
	pcb_fplibrary_type_t type;
	pcb_fplibrary_t *parent;

	union {
		struct { /* type == LIB_DIR */
			vtp0_t children; /* of (pcb_fplibrary_t *) */
			void *backend; /* pcb_plug_fp_t* */
		} dir;
		struct { /* type == LIB_FOOTPRINT */
			char *loc_info;
			void *backend_data;
			pcb_fptype_t type;

			/* an array of void * tag IDs; last tag ID is NULL; the array is
			   locally allocated but values are stored in a central hash and
			   must be allocated by pcb_fp_tag() and never free'd manually */
			void **tags;
		} fp;
	} data;
} ;

typedef struct pcb_plug_fp_s pcb_plug_fp_t;

typedef struct {
	pcb_plug_fp_t *backend;
	union {
		int i;
		void *p;
	} field[4];

	const char *filename;    /* basename of the footprint file, with possible sub footprint name truncated from "::" */
	const char *subfpname;   /* if the file contains multiple footprints, this holds the footprint name to open */
	int free_filename;       /* internal */
} pcb_fp_fopen_ctx_t;

/* hook bindings, see below;  pcb_fp_fclose() must be called even if pcb_fp_fopen() returned NULL! */
FILE *pcb_fp_fopen(const rnd_conflist_t *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst);
void pcb_fp_fclose(FILE * f, pcb_fp_fopen_ctx_t *fctx);

/* duplicates the name and splits it into a basename and params;
   params is NULL if the name is not parametric (and "" if parameter list is empty)
   returns 1 if name is parametric footprint, 0 if static file footprint.
   The caller shall free only *basename at the end.
   */
int pcb_fp_dupname(const char *name, char **basename, char **params);

/**** tag management ****/
/* Resolve a tag name to an unique void * ID; create unknown tag if alloc != 0 */
const void *pcb_fp_tag(const char *tag, int alloc);

/* Resolve a tag ID to a tag name */
const char *pcb_fp_tagname(const void *tagid);

/* init/uninit the footprint lib, free tag key memory */
void pcb_fp_init();
void pcb_fp_uninit();

/**************************** API definition *********************************/
extern FILE *PCB_FP_FOPEN_IN_DST;

struct pcb_plug_fp_s {
	pcb_plug_fp_t *next;
	void *plugin_data;

	/* returns the number of footprints loaded into the library or -1 on
	   error; next in chain is run only on error. If force is 1, force doing the
	   expensive part of the load (e.g. wget) */
	int (*load_dir)(pcb_plug_fp_t *ctx, const char *path, int force);

/* Open a footprint for reading; if the footprint is parametric, it's run
   prefixed with libshell (or executed directly, if libshell is NULL).
   If name is not an absolute path, search_path is searched for the first match.
   The user has to supply a state integer that will be used by pcb_pcb_fp_fclose().
   Must fill in fctx->backend, may use any other field of fctx as well.
   If dst is non-NULL, some backends (e.g. fp_board) may decide to place the
   loaded footprint in dst and return PCB_FP_FOPEN_IN_DST.
 */
	FILE *(*fp_fopen)(pcb_plug_fp_t *ctx, const char *path, const char *name, pcb_fp_fopen_ctx_t *fctx, pcb_data_t *dst);

/* Close the footprint file opened by pcb_pcb_fp_fopen(). */
	void (*fp_fclose)(pcb_plug_fp_t *ctx, FILE * f, pcb_fp_fopen_ctx_t *fctx);
};

extern pcb_plug_fp_t *pcb_plug_fp_chain;


/* Optional pcb-rnd-side glue for some implementations */

extern pcb_fplibrary_t pcb_library; /* the footprint library */

void pcb_fp_free_children(pcb_fplibrary_t *parent);
void pcb_fp_sort_children(pcb_fplibrary_t *parent);
void pcb_fp_rmdir(pcb_fplibrary_t *dir);
pcb_fplibrary_t *pcb_fp_mkdir_p(const char *path);
pcb_fplibrary_t *pcb_fp_mkdir_len(pcb_fplibrary_t *parent, const char *name, int name_len);
pcb_fplibrary_t *pcb_fp_lib_search(pcb_fplibrary_t *dir, const char *name);

/* Append a menu entry in the tree */
pcb_fplibrary_t *pcb_fp_append_entry(pcb_fplibrary_t *parent, const char *name, pcb_fptype_t type, void *tags[], rnd_bool dup_tags);

/* walk through all lib paths and build the library menu */
int pcb_fp_read_lib_all(void);

int pcb_fp_host_uninit(void);

/* rescan/reload all footprints in the library cache */
int pcb_fp_rehash(rnd_hidlib_t *hidlib, pcb_fplibrary_t *l);

/* invoke the GUI to choose one footprint name from a footprint map; if
   there's no GUI available, throws a warning and returns the first */
const char *pcb_fp_map_choose(rnd_hidlib_t *hidlib, const pcb_plug_fp_map_t *map);

#endif
