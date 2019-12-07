#ifndef PCB_LAYER_ADDR_H
#define PCB_LAYER_ADDR_H

#include "layer.h"
#include "layer_grp.h"

/*** main API ***/

/* Convert a textual layer(grp) reference into a layer ID. The text is either
   #id or a layer name. */
pcb_layer_id_t pcb_layer_str2id(pcb_data_t *data, const char *str);
pcb_layergrp_id_t pcb_layergrp_str2id(pcb_board_t *pcb, const char *str);

/* Convert a layer(grp) into a reusable address; returns NULL on error;
   caller needs to free() the returned string */
char *pcb_layergrp_to_addr(pcb_board_t *pcb, pcb_layergrp_t *grp);

/* Same, but append to dst instead of allocating new string; return 0 on
   success */
int pcb_layergrp_append_to_addr(pcb_board_t *pcb, pcb_layergrp_t *grp, gds_t *dst);

/*** for internal use ***/

/* Parse a layer group address: split at comma, addr will end up holding the
   layer name. If there were transformations in (), they are split and
   listed in tr up to at most *spc entries. Returns NULL on error or
   pointer to the next layer directive. */
char *pcb_parse_layergrp_address(char *curr, char **spk, char **spv, int *spc);
extern char *pcb_parse_layergrp_err;

void pcb_parse_layer_supplements(char **spk, char **spv, int spc,   char **purpose, pcb_xform_t **xf, pcb_xform_t *xf_);

/* parse addr into:
   - a list of layer group IDs
   - in case of a virtual group is addressed, return 0 and load vid if *vid is non-NULL
   - load xf_in fields if there are transformations requested (if xf_in is not NULL)
   - load *xf with xf_in if there are transformations requested (if xf is not NULL)
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
int pcb_layergrp_list_by_addr(pcb_board_t *pcb, const char *addr, pcb_layergrp_id_t gids[PCB_MAX_LAYERGRP], char **spk, char **spv, int spc, int *vid, pcb_xform_t **xf, pcb_xform_t *xf_in, const char *err_prefix);

#endif
