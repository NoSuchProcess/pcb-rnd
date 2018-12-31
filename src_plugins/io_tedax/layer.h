#include "board.h"
#include "../src/layer.h"

int tedax_layer_save(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, const char *fn);
int tedax_layer_fsave(pcb_board_t *pcb, pcb_layergrp_id_t gid, const char *layname, FILE *f);

int tedax_layers_load(pcb_data_t *data, const char *fn);
int tedax_layers_fload(pcb_data_t *data, FILE *f);
