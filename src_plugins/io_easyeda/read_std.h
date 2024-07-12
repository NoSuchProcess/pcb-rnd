#include <stdio.h>
#include "plug_io.h"

int io_easyeda_std_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);
int io_easyeda_std_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest);
int io_easyeda_std_parse_footprint(pcb_plug_io_t *ctx, pcb_data_t *data, const char *filename, const char *subfpname);
pcb_plug_fp_map_t *io_easyeda_std_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags);



