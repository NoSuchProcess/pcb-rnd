typedef void pcb_clihist_append_cb_t(void *ctx, const char *cmd);
typedef void pcb_clihist_remove_cb_t(void *ctx, int idx);

void pcb_clihist_append(const char *cmd, pcb_clihist_append_cb_t *append, pcb_clihist_remove_cb_t *remove);

void pcb_clihist_load(void);
void pcb_clihist_save(void);
