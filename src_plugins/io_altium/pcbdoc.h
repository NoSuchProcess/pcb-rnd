int io_altium_parse_pcbdoc_ascii(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest);
int io_altium_parse_pcbdoc_bin(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest);

long io_altium_obj2netid(void *ctx, void *obj);


