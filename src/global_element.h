#ifndef PCB_ELEMENT_H
#define PCB_ELEMENT_H

#include "list_pin.h"
#include "list_pad.h"
#include "obj_all_list.h"

struct element_st {
	ANYOBJECTFIELDS;
	TextType Name[MAX_ELEMENTNAMES];	/* the elements names; */
	/* description text */
	/* name on PCB second, */
	/* value third */
	/* see macro.h */
	Coord MarkX, MarkY;						/* position mark */
	pinlist_t Pin;
	padlist_t Pad;
	linelist_t Line;
	arclist_t Arc;
	BoxType VBox;
	gdl_elem_t link;
};

#include "list_element.h"

#endif
