#include "board.h"
#include <librnd/core/conf.h>
#include "plug_io.h"

int io_eagle_test_parse_xml(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);
int io_eagle_read_pcb_xml(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest);

int io_eagle_test_parse_bin(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);
int io_eagle_read_pcb_bin(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, rnd_conf_role_t settings_dest);

