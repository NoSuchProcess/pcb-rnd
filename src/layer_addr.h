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

/* parse addr into:
   - a list of layer group IDs

   - in case of a virtual group is addressed, return 0 and load vid if *vid is non-NULL
   - load xf with xf_in if there are transformations requested
   - if err_prefix is not NULL, use pcb_message() with this prefix to priont detailed error
  Input:
   - the address, without supplements or other suffixes, in addr
   - parsed supplement key/value pairs are expected in spk/spv (at most spc items)
  Returns:
  - 0 for valid syntax if no matching group is found
  - 0 for valid syntax if a virtual group is found (*vid is loaded if vid != NULL)
  - a positive integer if 1 or more real matching groups are found
  - -1 on error (typically syntax error)
*/
int pcb_layergrp_list_by_addr(pcb_board_t *pcb, char *addr, pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP], char **spk, char **spv, int spc, int *vid, pcb_xform_t **xf, pcb_xform_t *xf_in, const char *err_prefix);

#endif
