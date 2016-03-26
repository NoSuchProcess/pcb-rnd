#ifndef PCB_PLUG_FOOTPRINT_H
#define PCB_PLUG_FOOTPRINT_H
/* walk through all lib paths and build the library menu */
int fp_read_lib_all(void);

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
typedef struct plug_fp_s plug_fp_t;
struct plug_fp_s {
	plug_fp_t *next;
	void *plugin_data;

	/* returns the number of footprints loaded into the library or -1 on
	   error; next in chain is run only on error. */
	int (*load_dir)(plug_fp_t *ctx, const char *path);
};

extern plug_fp_t *plug_fp_chain;


#endif
