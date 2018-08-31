typedef void pcb_clihist_append_cb_t(void *ctx, const char *cmd);
typedef void pcb_clihist_remove_cb_t(void *ctx, int idx);

void pcb_clihist_append(const char *cmd, void *ctx, pcb_clihist_append_cb_t *append, pcb_clihist_remove_cb_t *remove);

void pcb_clihist_load(void);
void pcb_clihist_sync(void *ctx, pcb_clihist_append_cb_t *append);

/*** Cursor operation ***/
const char *pcb_clihist_prev(void);
const char *pcb_clihist_next(void);
void pcb_clihist_reset(void);


void pcb_clihist_save(void);
void pcb_clihist_init(void);
void pcb_clihist_uninit(void);

