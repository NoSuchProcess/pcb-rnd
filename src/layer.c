/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas (pcb-rnd extensions)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "layer.h"

bool IsLayerEmpty(LayerTypePtr layer)
{
	return LAYER_IS_EMPTY(layer);
}

bool IsLayerNumEmpty(int num)
{
	return IsLayerEmpty(PCB->Data->Layer + num);
}

bool IsLayerGroupEmpty(int num)
{
	int i;
	for (i = 0; i < PCB->LayerGroups.Number[num]; i++)
		if (!IsLayerNumEmpty(PCB->LayerGroups.Entries[num][i]))
			return false;
	return true;
}

/* ----------------------------------------------------------------------
 * parses the group definition string which is a colon separated list of
 * comma separated layer numbers (1,2,b:4,6,8,t)
 */
int ParseGroupString(char *s, LayerGroupTypePtr LayerGroup, int LayerN)
{
	int group, member, layer;
	bool c_set = false,						/* flags for the two special layers to */
		s_set = false;							/* provide a default setting for old formats */
	int groupnum[MAX_LAYER + 2];

	/* clear struct */
	memset(LayerGroup, 0, sizeof(LayerGroupType));

	/* Clear assignments */
	for (layer = 0; layer < MAX_LAYER + 2; layer++)
		groupnum[layer] = -1;

	/* loop over all groups */
	for (group = 0; s && *s && group < LayerN; group++) {
		while (*s && isspace((int) *s))
			s++;

		/* loop over all group members */
		for (member = 0; *s; s++) {
			/* ignore white spaces and get layernumber */
			while (*s && isspace((int) *s))
				s++;
			switch (*s) {
			case 'c':
			case 'C':
			case 't':
			case 'T':
				layer = LayerN + COMPONENT_LAYER;
				c_set = true;
				break;

			case 's':
			case 'S':
			case 'b':
			case 'B':
				layer = LayerN + SOLDER_LAYER;
				s_set = true;
				break;

			default:
				if (!isdigit((int) *s))
					goto error;
				layer = atoi(s) - 1;
				break;
			}
			if (layer > LayerN + MAX(SOLDER_LAYER, COMPONENT_LAYER) || member >= LayerN + 1)
				goto error;
			groupnum[layer] = group;
			LayerGroup->Entries[group][member++] = layer;
			while (*++s && isdigit((int) *s));

			/* ignore white spaces and check for separator */
			while (*s && isspace((int) *s))
				s++;
			if (!*s || *s == ':')
				break;
			if (*s != ',')
				goto error;
		}
		LayerGroup->Number[group] = member;
		if (*s == ':')
			s++;
	}
	if (!s_set)
		LayerGroup->Entries[SOLDER_LAYER][LayerGroup->Number[SOLDER_LAYER]++] = LayerN + SOLDER_LAYER;
	if (!c_set)
		LayerGroup->Entries[COMPONENT_LAYER][LayerGroup->Number[COMPONENT_LAYER]++] = LayerN + COMPONENT_LAYER;

	for (layer = 0; layer < LayerN && group < LayerN; layer++)
		if (groupnum[layer] == -1) {
			LayerGroup->Entries[group][0] = layer;
			LayerGroup->Number[group] = 1;
			group++;
		}
	return (0);

	/* reset structure on error */
error:
	memset(LayerGroup, 0, sizeof(LayerGroupType));
	return (1);
}



/* ---------------------------------------------------------------------------
 * returns the layer number for the passed pointer
 */
int GetLayerNumber(DataTypePtr Data, LayerTypePtr Layer)
{
	int i;

	for (i = 0; i < MAX_LAYER + 2; i++)
		if (Layer == &Data->Layer[i])
			break;
	return (i);
}

/* ---------------------------------------------------------------------------
 * move layer (number is passed in) to top of layerstack
 */
static void PushOnTopOfLayerStack(int NewTop)
{
	int i;

	/* ignore silk layers */
	if (NewTop < max_copper_layer) {
		/* first find position of passed one */
		for (i = 0; i < max_copper_layer; i++)
			if (LayerStack[i] == NewTop)
				break;

		/* bring this element to the top of the stack */
		for (; i; i--)
			LayerStack[i] = LayerStack[i - 1];
		LayerStack[0] = NewTop;
	}
}


/* ----------------------------------------------------------------------
 * changes the visibility of all layers in a group
 * returns the number of changed layers
 */
int ChangeGroupVisibility(int Layer, bool On, bool ChangeStackOrder)
{
	int group, i, changed = 1;		/* at least the current layer changes */

	/* Warning: these special case values must agree with what gui-top-window.c
	   |  thinks the are.
	 */

	if (conf_core.rc.verbose)
		printf("ChangeGroupVisibility(Layer=%d, On=%d, ChangeStackOrder=%d)\n", Layer, On, ChangeStackOrder);

	/* decrement 'i' to keep stack in order of layergroup */
	if ((group = GetGroupOfLayer(Layer)) < max_group)
		for (i = PCB->LayerGroups.Number[group]; i;) {
			int layer = PCB->LayerGroups.Entries[group][--i];

			/* don't count the passed member of the group */
			if (layer != Layer && layer < max_copper_layer) {
				PCB->Data->Layer[layer].On = On;

				/* push layer on top of stack if switched on */
				if (On && ChangeStackOrder)
					PushOnTopOfLayerStack(layer);
				changed++;
			}
		}

	/* change at least the passed layer */
	PCB->Data->Layer[Layer].On = On;
	if (On && ChangeStackOrder)
		PushOnTopOfLayerStack(Layer);

	/* update control panel and exit */
	hid_action("LayersChanged");
	return (changed);
}

/* ----------------------------------------------------------------------
 * Given a string description of a layer stack, adjust the layer stack
 * to correspond.
*/

void LayerStringToLayerStack(char *s)
{
	static int listed_layers = 0;
	int l = strlen(s);
	char **args;
	int i, argn, lno;
	int prev_sep = 1;

	s = pcb_strdup(s);
	args = (char **) malloc(l * sizeof(char *));
	argn = 0;

	for (i = 0; i < l; i++) {
		switch (s[i]) {
		case ' ':
		case '\t':
		case ',':
		case ';':
		case ':':
			prev_sep = 1;
			s[i] = '\0';
			break;
		default:
			if (prev_sep)
				args[argn++] = s + i;
			prev_sep = 0;
			break;
		}
	}

	for (i = 0; i < max_copper_layer + 2; i++) {
		if (i < max_copper_layer)
			LayerStack[i] = i;
		PCB->Data->Layer[i].On = false;
	}
	PCB->ElementOn = false;
	PCB->InvisibleObjectsOn = false;
	PCB->PinOn = false;
	PCB->ViaOn = false;
	PCB->RatOn = false;

	conf_set_editor(show_mask, 0);
	conf_set_editor(show_solder_side, 0);

	for (i = argn - 1; i >= 0; i--) {
		if (strcasecmp(args[i], "rats") == 0)
			PCB->RatOn = true;
		else if (strcasecmp(args[i], "invisible") == 0)
			PCB->InvisibleObjectsOn = true;
		else if (strcasecmp(args[i], "pins") == 0)
			PCB->PinOn = true;
		else if (strcasecmp(args[i], "vias") == 0)
			PCB->ViaOn = true;
		else if (strcasecmp(args[i], "elements") == 0 || strcasecmp(args[i], "silk") == 0)
			PCB->ElementOn = true;
		else if (strcasecmp(args[i], "mask") == 0)
			conf_set_editor(show_mask, 1);
		else if (strcasecmp(args[i], "solderside") == 0)
			conf_set_editor(show_solder_side, 1);
		else if (isdigit((int) args[i][0])) {
			lno = atoi(args[i]);
			ChangeGroupVisibility(lno, true, true);
		}
		else {
			int found = 0;
			for (lno = 0; lno < max_copper_layer; lno++)
				if (strcasecmp(args[i], PCB->Data->Layer[lno].Name) == 0) {
					ChangeGroupVisibility(lno, true, true);
					found = 1;
					break;
				}
			if (!found) {
				fprintf(stderr, "Warning: layer \"%s\" not known\n", args[i]);
				if (!listed_layers) {
					fprintf(stderr, "Named layers in this board are:\n");
					listed_layers = 1;
					for (lno = 0; lno < max_copper_layer; lno++)
						fprintf(stderr, "\t%s\n", PCB->Data->Layer[lno].Name);
					fprintf(stderr, "Also: component, solder, rats, invisible, pins, vias, elements or silk, mask, solderside.\n");
				}
			}
		}
	}
}

/* ----------------------------------------------------------------------
 * lookup the group to which a layer belongs to
 * returns max_group if no group is found, or is
 * passed Layer is equal to max_copper_layer
 */
int GetGroupOfLayer(int Layer)
{
	int group, i;

	if (Layer == max_copper_layer)
		return max_group;
	for (group = 0; group < max_group; group++)
		for (i = 0; i < PCB->LayerGroups.Number[group]; i++)
			if (PCB->LayerGroups.Entries[group][i] == Layer)
				return (group);
	return max_group;
}


/* ---------------------------------------------------------------------------
 * returns the layergroup number for the passed pointer
 */
int GetLayerGroupNumberByPointer(LayerTypePtr Layer)
{
	return (GetLayerGroupNumberByNumber(GetLayerNumber(PCB->Data, Layer)));
}

/* ---------------------------------------------------------------------------
 * returns the layergroup number for the passed layernumber
 */
int GetLayerGroupNumberByNumber(Cardinal Layer)
{
	int group, entry;

	for (group = 0; group < max_group; group++)
		for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++)
			if (PCB->LayerGroups.Entries[group][entry] == Layer)
				return (group);

	/* since every layer belongs to a group it is safe to return
	 * the value without boundary checking
	 */
	return (group);
}


/* ---------------------------------------------------------------------------
 * resets the layerstack setting
 */
void ResetStackAndVisibility(void)
{
	int comp_group;
	Cardinal i;

	for (i = 0; i < max_copper_layer + 2; i++) {
		if (i < max_copper_layer)
			LayerStack[i] = i;
		PCB->Data->Layer[i].On = true;
	}
	PCB->ElementOn = true;
	PCB->InvisibleObjectsOn = true;
	PCB->PinOn = true;
	PCB->ViaOn = true;
	PCB->RatOn = true;

	/* Bring the component group to the front and make it active.  */
	comp_group = GetLayerGroupNumberByNumber(component_silk_layer);
	ChangeGroupVisibility(PCB->LayerGroups.Entries[comp_group][0], 1, 1);
}

/* ---------------------------------------------------------------------------
 * saves the layerstack setting
 */
void SaveStackAndVisibility(void)
{
	Cardinal i;
	static bool run = false;

	if (run == false) {
		SavedStack.cnt = 0;
		run = true;
	}

	if (SavedStack.cnt != 0) {
		fprintf(stderr,
						"SaveStackAndVisibility()  layerstack was already saved and not" "yet restored.  cnt = %d\n", SavedStack.cnt);
	}

	for (i = 0; i < max_copper_layer + 2; i++) {
		if (i < max_copper_layer)
			SavedStack.LayerStack[i] = LayerStack[i];
		SavedStack.LayerOn[i] = PCB->Data->Layer[i].On;
	}
	SavedStack.ElementOn = PCB->ElementOn;
	SavedStack.InvisibleObjectsOn = PCB->InvisibleObjectsOn;
	SavedStack.PinOn = PCB->PinOn;
	SavedStack.ViaOn = PCB->ViaOn;
	SavedStack.RatOn = PCB->RatOn;
	SavedStack.cnt++;
}

/* ---------------------------------------------------------------------------
 * restores the layerstack setting
 */
void RestoreStackAndVisibility(void)
{
	Cardinal i;

	if (SavedStack.cnt == 0) {
		fprintf(stderr, "RestoreStackAndVisibility()  layerstack has not" " been saved.  cnt = %d\n", SavedStack.cnt);
		return;
	}
	else if (SavedStack.cnt != 1) {
		fprintf(stderr, "RestoreStackAndVisibility()  layerstack save count is" " wrong.  cnt = %d\n", SavedStack.cnt);
	}

	for (i = 0; i < max_copper_layer + 2; i++) {
		if (i < max_copper_layer)
			LayerStack[i] = SavedStack.LayerStack[i];
		PCB->Data->Layer[i].On = SavedStack.LayerOn[i];
	}
	PCB->ElementOn = SavedStack.ElementOn;
	PCB->InvisibleObjectsOn = SavedStack.InvisibleObjectsOn;
	PCB->PinOn = SavedStack.PinOn;
	PCB->ViaOn = SavedStack.ViaOn;
	PCB->RatOn = SavedStack.RatOn;

	SavedStack.cnt--;
}

/***********************************************************************
 * Layer Group Functions
 */

int MoveLayerToGroup(int layer, int group)
{
	int prev, i, j;

	if (layer < 0 || layer > max_copper_layer + 1)
		return -1;
	prev = GetLayerGroupNumberByNumber(layer);
	if ((layer == solder_silk_layer && group == GetLayerGroupNumberByNumber(component_silk_layer))
			|| (layer == component_silk_layer && group == GetLayerGroupNumberByNumber(solder_silk_layer))
			|| (group < 0 || group >= max_group) || (prev == group))
		return prev;

	/* Remove layer from prev group */
	for (j = i = 0; i < PCB->LayerGroups.Number[prev]; i++)
		if (PCB->LayerGroups.Entries[prev][i] != layer)
			PCB->LayerGroups.Entries[prev][j++] = PCB->LayerGroups.Entries[prev][i];
	PCB->LayerGroups.Number[prev]--;

	/* Add layer to new group.  */
	i = PCB->LayerGroups.Number[group]++;
	PCB->LayerGroups.Entries[group][i] = layer;

	return group;
}

char *LayerGroupsToString(LayerGroupTypePtr lg)
{
#if MAX_LAYER < 9998
	/* Allows for layer numbers 0..9999 */
	static char buf[(MAX_LAYER + 2) * 5 + 1];
#endif
	char *cp = buf;
	char sep = 0;
	int group, entry;
	for (group = 0; group < max_group; group++)
		if (PCB->LayerGroups.Number[group]) {
			if (sep)
				*cp++ = ':';
			sep = 1;
			for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++) {
				int layer = PCB->LayerGroups.Entries[group][entry];
				if (layer == component_silk_layer) {
					*cp++ = 'c';
				}
				else if (layer == solder_silk_layer) {
					*cp++ = 's';
				}
				else {
					sprintf(cp, "%d", layer + 1);
					while (*++cp);
				}
				if (entry != PCB->LayerGroups.Number[group] - 1)
					*cp++ = ',';
			}
		}
	*cp++ = 0;
	return buf;
}

unsigned int pcb_layer_flags(int layer_idx)
{
	unsigned int res = 0;


	if (layer_idx == solder_silk_layer)
		return PCB_LYT_SILK | PCB_LYT_BOTTOM;

	if (layer_idx == component_silk_layer)
		return PCB_LYT_SILK | PCB_LYT_TOP;

	if (layer_idx < max_copper_layer) {
		res |= PCB_LYT_COPPER;
	}

}


int pcb_layer_list(pcb_layer_type_t mask, int *res, int res_len)
{
	int n, used = 0;

	for (n = 0; n < MAX_LAYER + 2; n++) {
		if ((pcb_layer_flags(n) & mask) == mask) {
			if (res != NULL) {
				if (used < res_len) {
					res[used] = n;
					used++;
				}
			}
			else
				used++;
		}
	}
	return used;
}

