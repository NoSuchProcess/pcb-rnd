#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/misc.h"

void layout_switch_to_layer(int layer)
{
	if ((layer < 0) || (layer >= MAX_LAYER+2))
		return;
	ChangeGroupVisibility(layer, true, true);
	ClearAndRedrawOutput();
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
