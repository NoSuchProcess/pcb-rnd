#include <stdio.h>
#include <librnd/core/pcb_bool.h>
#include "plug_io.h"

int io_hyp_write_pcb(pcb_plug_io_t * ctx, FILE * f, const char *old_filename, const char *new_filename, pcb_bool emergency);
