
/* Allocate *out and copy the path from in to out, replacing ~ with homedir */
void resolve_path(const char *in, char **out);

/* Same as resolve_path, but it returns the pointer to the new path and calls
   free() on in */
char *resolve_path_inplace(char *in);


/* Resolve all paths from a in[] into out[](should be large enough) */
void resolve_paths(const char **in, char **out, int numpaths);

/* Resolve all paths from a char *in[] into a freshly allocated char **out */
#define resolve_all_paths(in, out) \
do { \
	int __numpath__ = sizeof(in) / sizeof(char *); \
	if (__numpath__ > 0) { \
		out = malloc(sizeof(char *) * __numpath__); \
		resolve_paths(in, out, __numpath__); \
	} \
} while(0)


