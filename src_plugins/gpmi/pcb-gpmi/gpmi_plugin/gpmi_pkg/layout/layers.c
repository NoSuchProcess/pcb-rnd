#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/board.h"
#include "src/draw.h"
#include "src/conf_core.h"
#include "src/layer.h"
#include "src/layer_ui.h"
#include "src/layer_vis.h"
#include "src/compat_misc.h"

#define LAYER_SEARCH_MAX 32

#define layer_check(layer) \
	if ((layer < 0) || (layer >= PCB_MAX_LAYER+2)) \
		return


void layout_switch_to_layer(int layer)
{
	layer_check(layer);
	pcb_layervis_change_group_vis(layer, pcb_true, pcb_true);
	pcb_redraw();
}

layer_id_t layout_get_current_layer()
{
	return pcb_layer_id(PCB->Data, CURRENT);
}

int layout_resolve_layer(const char *name)
{
	int n;
	if (name == NULL)
		return -2;
	for(n = 0; n < PCB_MAX_LAYER + 2; n++)
		if ((PCB->Data->Layer[n].name != NULL) && (strcmp(PCB->Data->Layer[n].name, name) == 0))
			return n;
	return -1;
}

int layout_get_max_possible_layer()
{
	return PCB_MAX_LAYER+2;
}

int layout_get_max_layer()
{
	return pcb_max_layer;
}

const char *layout_layer_name(int layer)
{
	layer_check(layer)("");
	return PCB->Data->Layer[layer].name;
}

const char *layout_layer_color(int layer)
{
	layer_check(layer)("");
	return PCB->Data->Layer[layer].meta.real.color;
}

int layout_layer_field(int layer, layer_field_t fld)
{
	layer_check(layer)(-1);
	switch(fld) {
		case LFLD_NUM_LINES: return linelist_length(&(PCB->Data->Layer[layer].Line));
		case LFLD_NUM_TEXTS: return textlist_length(&(PCB->Data->Layer[layer].Text));
		case LFLD_NUM_POLYS: return polylist_length(&(PCB->Data->Layer[layer].Polygon));
		case LFLD_NUM_ARCS:  return arclist_length(&(PCB->Data->Layer[layer].Arc));
		case LFLD_VISIBLE:   return PCB->Data->Layer[layer].meta.real.vis;
		case LFLD_NODRC:     return PCB->Data->Layer[layer].meta.real.no_drc;
	}
	return -1;
}

int layer_flag_is_set(unsigned int flags, multiple layer_type_t flg)
{
	return !!(flags & flg);
}

layer_id_t layer_list(multiple layer_type_t flags, int idx)
{
	pcb_layer_id_t ids[LAYER_SEARCH_MAX];
	int len = pcb_layer_list(PCB, flags, ids, LAYER_SEARCH_MAX);
	if (idx < 0)
		return len;
	if (idx >= len)
		return -1;
	return ids[idx];
}

layer_id_t layer_list_any(multiple layer_type_t flags, int idx)
{
	pcb_layer_id_t ids[LAYER_SEARCH_MAX];
	int len = pcb_layer_list_any(PCB, flags, ids, LAYER_SEARCH_MAX);
	if (idx < 0)
		return len;
	if (idx >= len)
		return -1;
	return ids[idx];
}

layer_id_t uilayer_alloc(const char *name, const char *color)
{
	pcb_layer_t *l = pcb_uilayer_alloc("script", pcb_strdup(name), pcb_strdup(color));
	if (l == NULL)
		return -1;
	return pcb_layer_id(PCB->Data, l);
}
