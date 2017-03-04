#include <genht/htsp.h>

typedef struct {
	htsp_t id2refdes; /* element ID -> refdes */
	int alloced;
} nethlp_ctx_t;

typedef struct {
	htsp_t attr;
	char *id;
	nethlp_ctx_t *nhctx;
	int alloced;
} nethlp_elem_ctx_t;


nethlp_ctx_t *nethlp_new(nethlp_ctx_t *prealloc);
void nethlp_destroy(nethlp_ctx_t *nhctx);

nethlp_elem_ctx_t *nethlp_elem_new(nethlp_ctx_t *nhctx, nethlp_elem_ctx_t *prealloc, const char *id);
void nethlp_elem_refdes(nethlp_elem_ctx_t *ectx, const char *refdes);
void nethlp_elem_attr(nethlp_elem_ctx_t *ectx, const char *key, const char *val);
void nethlp_elem_done(nethlp_elem_ctx_t *ectx);

