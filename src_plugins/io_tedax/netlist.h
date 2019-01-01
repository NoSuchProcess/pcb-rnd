int tedax_net_load(const char *fname_net);
int tedax_net_fload(FILE *f);

int tedax_net_save(pcb_board_t *pcb, const char *netlistid, const char *fn);
int tedax_net_fsave(pcb_board_t *pcb, const char *netlistid, FILE *f);
