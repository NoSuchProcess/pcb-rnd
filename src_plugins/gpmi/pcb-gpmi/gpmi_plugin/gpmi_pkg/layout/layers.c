#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/board.h"
#include "src/const.h"
#include "src/draw.h"
#include "src/conf_core.h"
#include "src/layer.h"

#define layer_check(layer) \
	if ((layer < 0) || (layer >= MAX_LAYER+2)) \
		return


void layout_switch_to_layer(int layer)
{
	layer_check(layer);
	ChangeGroupVisibility(layer, pcb_true, pcb_true);
	Redraw();
}

int layout_get_current_layer()
{
	return GetLayerNumber(PCB->Data, CURRENT);
}

int layout_resolve_layer(const char *name)
{
	int n;
	if (name == NULL)
		return -2;
	for(n = 0; n < MAX_LAYER + 2; n++)
		if ((PCB->Data->Layer[n].Name != NULL) && (strcmp(PCB->Data->Layer[n].Name, name) == 0))
			return n;
	return -1;
}

int layout_get_max_possible_layer()
{
	return MAX_LAYER+2;
}

int layout_get_max_copper_layer()
{
	return max_copper_layer;
}

int layout_get_max_layer()
{
	return max_copper_layer+2;
}


const char *layout_layer_name(int layer)
{
	layer_check(layer)("");
	return PCB->Data->Layer[layer].Name;
}

const char *layout_layer_color(int layer)
{
	layer_check(layer)("");
	return PCB->Data->Layer[layer].Color;
}

int layout_layer_field(int layer, layer_field_t fld)
{
	layer_check(layer)(-1);
	switch(fld) {
		case LFLD_NUM_LINES: return linelist_length(&(PCB->Data->Layer[layer].Line));
		case LFLD_NUM_TEXTS: return textlist_length(&(PCB->Data->Layer[layer].Text));
		case LFLD_NUM_POLYS: return polylist_length(&(PCB->Data->Layer[layer].Polygon));
		case LFLD_NUM_ARCS:  return arclist_length(&(PCB->Data->Layer[layer].Arc));
		case LFLD_VISIBLE:   return PCB->Data->Layer[layer].On;
		case LFLD_NODRC:     return PCB->Data->Layer[layer].no_drc;
	}
	return -1;
}
