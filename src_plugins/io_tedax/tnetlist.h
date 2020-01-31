int tedax_net_load(const char *fname_net, int import_fp, const char *blk_id, int silent);
int tedax_net_fload(FILE *f, int import_fp, const char *blk_id, int silent);

int tedax_net_save(pcb_board_t *pcb, const char *netlistid, const char *fn);
int tedax_net_fsave(pcb_board_t *pcb, const char *netlistid, FILE *f);

void pcb_tedax_net_init(void);
void pcb_tedax_net_uninit(void);
