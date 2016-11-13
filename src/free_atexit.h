/* This tiny library is to assist cleaning up harmless memory leaks caused
   by (growing) buffers allocated in static variables in functions. The
   library provides pcb_leaky_ prefixed variants of the common allocation
   routines. These wrappers will remember all pointers they return and
   can free all memory used, at the end of the application.
*/

#include <stdlib.h>

#ifdef NDEBUG
#define pcb_leaky_init()
#define pcb_leaky_uninit()
#define pcb_leaky_malloc(size) malloc(size)
#define pcb_leaky_calloc(nmemb, size) calloc(nmemb, size)
#define pcb_leaky_realloc(old_memory, size) realloc(old_memory, size)
#define pcb_leaky_strdup(str) strdup(str)
#else

/* set up atexit() hook - can be avoided if pcb_leaky_uninit() is called by hand */
void pcb_leaky_init(void);

/* free all allocations */
void pcb_leaky_uninit(void);

/* allocate memory, remember the pointer and free it after exit from the application */
void *pcb_leaky_malloc(size_t size);

/* same as leaky_malloc but this one wraps calloc() */
void *pcb_leaky_calloc(size_t nmemb, size_t size);

/* reallocate memory, remember the new pointer and free it after exit from the application */
void *pcb_leaky_realloc(void *old_memory, size_t size);

/* strdup() using pcb_leaky_malloc() */
char *pcb_leaky_strdup(const char *src);


#endif
