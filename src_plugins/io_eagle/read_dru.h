#ifndef PCB_IO_EAGLE_READ_DRU_H
#define PCB_IO_EAGLE_READ_DRU_H

#include <genvector/gds_char.h>
#include <stdio.h>

#ifndef PCB_EAGLE_DRU_PARSER_TEST
#include "plug_io.h"
int io_eagle_test_parse_dru(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);
int io_eagle_read_pcb_dru(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *Filename, rnd_conf_role_t settings_dest);
#endif

/*** low level ***/

/* Read the first bytes of a file (and rewind) to determine if the file is
   an eagle dru file; returns 1 if it is, 0 if not. */
int pcb_eagle_dru_test_parse(FILE *f);

/* Load the next item from the dru and set key and value to point to the
   corresponding fields within the buff (or NULL if not present). */
void pcb_eagle_dru_parse_line(FILE *f, gds_t *buff, char **key, char **value);

#endif
