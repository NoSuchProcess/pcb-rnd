#include <stdio.h>
#include "plug_io.h"

int io_bxl_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);
int io_bxl_parse_footprint(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *fn);
pcb_plug_fp_map_t *io_bxl_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags);


