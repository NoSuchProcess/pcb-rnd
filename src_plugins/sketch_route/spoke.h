#ifndef SPOKE_H
#define SPOKE_H

#include "obj_common.h"
#include "cdt/point.h"

#include <genvector/vtp0.h>


struct spoke_s {
	pcb_box_t bbox;
	vtp0_t slots;
  point_t *p;
};

#endif
