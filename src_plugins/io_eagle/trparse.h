#ifndef PCB_TRPARSE_H
#define PCB_TRPARSE_H

typedef void trnode_t;
typedef struct trparse_s trparse_t;

typedef struct trparse_calls_s {
	int (*load)(trparse_t *pst, const char *fn);
	int (*unload)(trparse_t *pst);

	trnode_t *(*parent)(trparse_t *pst, trnode_t *node);
	trnode_t *(*children)(trparse_t *pst, trnode_t *node);
	trnode_t *(*next)(trparse_t *pst, trnode_t *node);

	const char *(*nodename)(trnode_t *node);
	const char *(*prop)(trparse_t *pst, trnode_t *node, const char *key);
	const char *(*text)(trparse_t *pst, trnode_t *node);

	int (*strcmp)(const char *s1, const char *s2);
	int (*is_text)(trparse_t *pst, trnode_t *node);
} trparse_calls_t;


struct trparse_s {
	void *doc;
	trnode_t *root;
	const trparse_calls_t *calls;
};

#endif
