typedef enum {
	PCB_FP_DIR,
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} pcb_fp_type_t;

int pcb_fp_list(char *subdir, int recurse,  int (*cb)(void *cookie, const char *subdir, const char *name, pcb_fp_type_t type), void *cookie);

