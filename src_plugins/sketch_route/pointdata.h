#ifndef POINTDATA_H
#define POINTDATA_H

#include "sktypedefs.h"
#include "spoke.h"

struct pointdata_s {
	pcb_any_obj_t *obj;
	/* these wirelists are ordered outside-to-inside */
	/* two lists are used in corner case: when two subsequent wire segments are collinear and
	 * wires are attached both on the left and on the right side of the point */
	/* uturn wirelist is used when a wire turns back at a point */
	wirelist_node_t *attached_wires[2];
	wirelist_node_t *uturn_wires;
	wirelist_node_t *terminal_wires;
	/* spokes provide sufficient spacing of wires from objects;
	 * there are 4 spokes for each of the directions: 45, 135, 225, 315 deg */
	spoke_t spokes[4];
};

void pointdata_create(point_t *p, pcb_any_obj_t *obj);
void pointdata_free(point_t *p);

#endif
