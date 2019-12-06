#ifndef PCB_LAYER_ADDR_H
#define PCB_LAYER_ADDR_H

#include "layer.h"
#include "layer_grp.h"

/*** main API ***/

/* Convert a textual layer(grp) reference into a layer ID. The text is either
   #id or a layer name. */
pcb_layer_id_t pcb_layer_str2id(pcb_data_t *data, const char *str);
pcb_layergrp_id_t pcb_layergrp_str2id(pcb_board_t *pcb, const char *str);

/*** for internal use ***/

void pcb_parse_layer_supplements(char **spk, char **spv, int spc,   char **purpose, pcb_xform_t **xf, pcb_xform_t *xf_);
int pcb_layergrp_list_by_addr(pcb_board_t *pcb, char *curr, pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP], char **spk, char **spv, int spc, int *vid, pcb_xform_t **xf, pcb_xform_t *xf_in, const char *err_prefix);

#endif
