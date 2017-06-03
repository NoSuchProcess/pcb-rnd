#ifndef PCB_TRPARSE_H
#define PCB_TRPARSE_H

typedef void trnode_t;
typedef struct trparse_s trparse_t;

typedef struct trparse_calls_s {
	int (*load)(trparse_t *pst, const char *fn);
	int (*unload)(trparse_t *pst);
	trnode_t *(*children)(trparse_t *pst, trnode_t *node);
} trparse_calls_t;


struct trparse_s {
	void *doc;
	trnode_t *root;
	const trparse_calls_t *calls;
};

#endif
