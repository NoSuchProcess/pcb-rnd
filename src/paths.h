
/* Allocate *out and copy the path from in to out, replacing ~ with homedir
   If extra_room is non-zero, allocate this many bytes extra for each slot;
   this leaves some room to append a file name. */
void resolve_path(const char *in, char **out, unsigned int extra_room);

/* Same as resolve_path, but it returns the pointer to the new path and calls
   free() on in */
char *resolve_path_inplace(char *in, unsigned int extra_room);


/* Resolve all paths from a in[] into out[](should be large enough) */
void resolve_paths(const char **in, char **out, int numpaths, unsigned int extra_room);

/* Resolve all paths from a char *in[] into a freshly allocated char **out */
#define resolve_all_paths(in, out, extra_room) \
do { \
	int __numpath__ = sizeof(in) / sizeof(char *); \
	if (__numpath__ > 0) { \
		out = malloc(sizeof(char *) * __numpath__); \
		resolve_paths(in, out, __numpath__, extra_room); \
	} \
} while(0)

extern char *homedir;

/* set up global var homedir */
void paths_init_homedir(void);
