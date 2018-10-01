#ifndef PCB_LAYER_INLINES_H
#define PCB_LAYER_INLINES_H

#include "layer.h"

PCB_INLINE pcb_layer_t *pcb_layer_silk_front(pcb_board_t *pcb)
{
	pcb_layer_id_t lid = (conf_core.editor.show_solder_side ? pcb_layer_get_bottom_silk() : pcb_layer_get_top_silk());
	if (lid < 0)
		return NULL;
	return &pcb->Data->Layer[lid];
}

PCB_INLINE pcb_layer_t *pcb_layer_silk_back(pcb_board_t *pcb)
{
	pcb_layer_id_t lid = ((!conf_core.editor.show_solder_side) ? pcb_layer_get_bottom_silk() : pcb_layer_get_top_silk());
	if (lid < 0)
		return NULL;
	return &pcb->Data->Layer[lid];
}

#endif
