#include "board.h"
#include <librnd/core/conf.h>
#include "plug_io.h"

int io_eagle_test_parse_xml(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);
int io_eagle_read_pcb_xml(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest);
pcb_plug_fp_map_t *io_eagle_map_footprint_xml(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags);
int io_eagle_parse_footprint_xml(pcb_plug_io_t *ctx, pcb_data_t *data, const char *filename, const char *subfpname);

int io_eagle_test_parse_bin(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);
int io_eagle_read_pcb_bin(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, rnd_conf_role_t settings_dest);
pcb_plug_fp_map_t *io_eagle_map_footprint_bin(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags);
int io_eagle_parse_footprint_bin(pcb_plug_io_t *ctx, pcb_data_t *data, const char *filename, const char *subfpname);
