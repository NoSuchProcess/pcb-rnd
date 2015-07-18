#ifndef LIBPCB_FP_H
#define LIBPCB_FP_H
typedef enum {
	PCB_FP_INVALID,
	PCB_FP_DIR,
	PCB_FP_FILE,
	PCB_FP_PARAMETRIC
} pcb_fp_type_t;

/* List all symbols, optionally recursively, from CWD/subdir. For each symbol
   or subdir, call the callback. Ignore file names starting with .*/
int pcb_fp_list(const char *subdir, int recurse,  int (*cb)(void *cookie, const char *subdir, const char *name, pcb_fp_type_t type), void *cookie);

/* Decide about the type of a footprint file by reading the content*/
pcb_fp_type_t pcb_fp_file_type(const char *fn);

#endif
