/* walk through all lib paths and build the library menu */
int fp_read_lib_all(void);


/* API definition */
typedef struct plug_fp_s plug_fp_t;
struct plug_fp_s {
	plug_fp_t *next;
	void *plugin_data;

	/* returns the number of footprints loaded into the library or -1 on
	   error; next in chain is run only on error. */
	int (*load_dir)(plug_fp_t *ctx, const char *path);
};

extern plug_fp_t *plug_fp_chain;


