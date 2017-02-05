#include "unit.h"

int pcb_gtk_act_load(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

extern const char pcb_gtk_acts_save[];
extern const char pcb_gtk_acth_save[];
int pcb_gtk_act_save(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

extern const char pcb_gtk_acts_importgui[];
extern const char pcb_gtk_acth_importgui[];
int pcb_gtk_act_importgui(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
