#include "board.h"
#include "layer.h"

int tedax_layer_save(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, const char *fn);
int tedax_layer_fsave(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, FILE *f);

int tedax_layers_load(pcb_data_t *data, const char *fn, const char *blk_id, int silent);
int tedax_layers_fload(pcb_data_t *data, FILE *f, const char *blk_id, int silent);
