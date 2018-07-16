#ifndef SPOKE_H
#define SPOKE_H

#include "sktypedefs.h"
#include "obj_common.h"
#include "ewire.h"
#include "cdt/point.h"

#include <genvector/vtp0.h>


typedef enum {
  SPOKE_DIR_1PI4 = 0,
  SPOKE_DIR_3PI4 = 1,
  SPOKE_DIR_5PI4 = 2,
  SPOKE_DIR_7PI4 = 3
} spoke_dir_t;

struct spoke_s {
  spoke_dir_t dir;
	pcb_box_t bbox;
	vtp0_t slots;
  point_t *p;
};


void spoke_init(spoke_t *sp, spoke_dir_t dir, point_t *p);
void spoke_uninit(spoke_t *sp);

void spoke_pos_at_wire_point(spoke_t *sp, wire_point_t *wp, pcb_coord_t *x, pcb_coord_t *y);
void spoke_pos_at_slot(spoke_t *sp, int slot, pcb_coord_t *x, pcb_coord_t *y);

void spoke_insert_wire_at_slot(spoke_t *sp, int slot_num, ewire_t *ew);

#endif
