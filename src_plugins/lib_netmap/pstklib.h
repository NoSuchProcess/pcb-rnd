#include "board.h"
#include "data.h"
#include "ht_pstk.h"

/* Build a hash of subcircuit-to-prototype. Effectively a local library
   of unique footprint prototypes */

typedef struct {
	pcb_pstk_proto_t proto;  /* copy of a proto */
	long id;                 /* unique ID counted from 1 by pcb_pstklib_build */
	void *user_data;
} pcb_pstklib_entry_t;

typedef struct pcb_pstklib_s pcb_pstklib_t;
struct pcb_pstklib_s {
	htprp_t protos;   /* value is (pcb_pstklib_entry_t *) */
	pcb_board_t *pcb;
	long next_id;

	void (*on_free_entry)(pcb_pstklib_t *ctx, pcb_pstklib_entry_t *pe); /* optional: if set, called before freeing an entry on uninit */
	void *user_data;
};

void pcb_pstklib_init(pcb_pstklib_t *ctx, pcb_board_t *pcb);
void pcb_pstklib_uninit(pcb_pstklib_t *ctx);

/* Iterate all padstack prototypes within data and build ctx->protos hash */
void pcb_pstklib_build_data(pcb_pstklib_t *ctx, pcb_data_t *data);

/* return proto's prototype or NULL */
#define pcb_pstklib_get(ctx, proto)    htprp_get((ctx)->protos, (proto))
