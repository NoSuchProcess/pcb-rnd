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

#include "config.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "layer.h"
#include "hid_actions.h"
#include "compat_misc.h"

/*
 * Used by SaveStackAndVisibility() and
 * RestoreStackAndVisibility()
 */

static struct {
	pcb_bool ElementOn, InvisibleObjectsOn, PinOn, ViaOn, RatOn;
	int LayerStack[MAX_LAYER];
	pcb_bool LayerOn[MAX_LAYER];
	int cnt;
} SavedStack;


pcb_bool IsLayerEmpty(LayerTypePtr layer)
{
	return LAYER_IS_EMPTY(layer);
}

pcb_bool IsLayerNumEmpty(int num)
{
	return IsLayerEmpty(PCB->Data->Layer + num);
}

pcb_bool IsLayerGroupEmpty(int num)
{
	int i;
	for (i = 0; i < PCB->LayerGroups.Number[num]; i++)
		if (!IsLayerNumEmpty(PCB->LayerGroups.Entries[num][i]))
			return pcb_false;
	return pcb_true;
}

/* ----------------------------------------------------------------------
 * parses the group definition string which is a colon separated list of
 * comma separated layer numbers (1,2,b:4,6,8,t)
 */
int ParseGroupString(const char *s, LayerGroupTypePtr LayerGroup, int LayerN)
{
	int group, member, layer;
	pcb_bool c_set = pcb_false,						/* flags for the two special layers to */
		s_set = pcb_false;							/* provide a default setting for old formats */
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
				c_set = pcb_true;
				break;

			case 's':
			case 'S':
			case 'b':
			case 'B':
				layer = LayerN + SOLDER_LAYER;
				s_set = pcb_true;
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
int GetLayerNumber(pcb_data_t *Data, LayerTypePtr Layer)
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
int ChangeGroupVisibility(int Layer, pcb_bool On, pcb_bool ChangeStackOrder)
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

void LayerStringToLayerStack(const char *layer_string)
{
	static int listed_layers = 0;
	int l = strlen(layer_string);
	char **args;
	int i, argn, lno;
	int prev_sep = 1;
	char *s;

	s = pcb_strdup(layer_string);
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
		PCB->Data->Layer[i].On = pcb_false;
	}
	PCB->ElementOn = pcb_false;
	PCB->InvisibleObjectsOn = pcb_false;
	PCB->PinOn = pcb_false;
	PCB->ViaOn = pcb_false;
	PCB->RatOn = pcb_false;

	conf_set_editor(show_mask, 0);
	conf_set_editor(show_solder_side, 0);

	for (i = argn - 1; i >= 0; i--) {
		if (strcasecmp(args[i], "rats") == 0)
			PCB->RatOn = pcb_true;
		else if (strcasecmp(args[i], "invisible") == 0)
			PCB->InvisibleObjectsOn = pcb_true;
		else if (strcasecmp(args[i], "pins") == 0)
			PCB->PinOn = pcb_true;
		else if (strcasecmp(args[i], "vias") == 0)
			PCB->ViaOn = pcb_true;
		else if (strcasecmp(args[i], "elements") == 0 || strcasecmp(args[i], "silk") == 0)
			PCB->ElementOn = pcb_true;
		else if (strcasecmp(args[i], "mask") == 0)
			conf_set_editor(show_mask, 1);
		else if (strcasecmp(args[i], "solderside") == 0)
			conf_set_editor(show_solder_side, 1);
		else if (isdigit((int) args[i][0])) {
			lno = atoi(args[i]);
			ChangeGroupVisibility(lno, pcb_true, pcb_true);
		}
		else {
			int found = 0;
			for (lno = 0; lno < max_copper_layer; lno++)
				if (strcasecmp(args[i], PCB->Data->Layer[lno].Name) == 0) {
					ChangeGroupVisibility(lno, pcb_true, pcb_true);
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
int GetLayerGroupNumberByNumber(pcb_cardinal_t Layer)
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
	pcb_cardinal_t i;

	assert(PCB->Data->LayerN <= MAX_LAYER);
	for (i = 0; i < max_copper_layer + 2; i++) {
		if (i < max_copper_layer)
			LayerStack[i] = i;
		PCB->Data->Layer[i].On = pcb_true;
	}
	PCB->ElementOn = pcb_true;
	PCB->InvisibleObjectsOn = pcb_true;
	PCB->PinOn = pcb_true;
	PCB->ViaOn = pcb_true;
	PCB->RatOn = pcb_true;

	/* Bring the component group to the front and make it active.  */
	comp_group = GetLayerGroupNumberByNumber(component_silk_layer);
	ChangeGroupVisibility(PCB->LayerGroups.Entries[comp_group][0], 1, 1);
}

/* ---------------------------------------------------------------------------
 * saves the layerstack setting
 */
void SaveStackAndVisibility(void)
{
	pcb_cardinal_t i;
	static pcb_bool run = pcb_false;

	if (run == pcb_false) {
		SavedStack.cnt = 0;
		run = pcb_true;
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
	pcb_cardinal_t i;

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

pcb_bool IsPasteEmpty(int side)
{
	pcb_bool paste_empty = pcb_true;
	ALLPAD_LOOP(PCB->Data);
	{
		if (ON_SIDE(pad, side) && !TEST_FLAG(PCB_FLAG_NOPASTE, pad) && pad->Mask > 0) {
			paste_empty = pcb_false;
			break;
		}
	}
	ENDALL_LOOP;
	return paste_empty;
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

	if (layer_idx > max_copper_layer+2)
		return 0;

	if (layer_idx < max_copper_layer) {
		if (!LAYER_IS_OUTLINE(layer_idx)) {
			/* check whether it's top, bottom or internal */
			int group, entry;
			for (group = 0; group < max_group; group++) {
				if (PCB->LayerGroups.Number[group]) {
					unsigned int my_group = 0, gf = 0;
					for (entry = 0; entry < PCB->LayerGroups.Number[group]; entry++) {
						int layer = PCB->LayerGroups.Entries[group][entry];
						if (layer == layer_idx)
							my_group = 1;
						if (layer == component_silk_layer)
							gf |= PCB_LYT_TOP;
						else if (layer == solder_silk_layer)
							gf |= PCB_LYT_BOTTOM;
					}
					if (my_group) {
						res |= gf;
						if (gf == 0)
							res |= PCB_LYT_INTERN;
						break; /* stop searching groups */
					}
				}
			}
			res |= PCB_LYT_COPPER;
		}
		else
			res |= PCB_LYT_OUTLINE;
	}

	return res;
}

#define APPEND(n) \
	do { \
		if (res != NULL) { \
			if (used < res_len) { \
				res[used] = n; \
				used++; \
			} \
		} \
		else \
			used++; \
	} while(0)

int pcb_layer_list(pcb_layer_type_t mask, int *res, int res_len)
{
	int n, used = 0;

	for (n = 0; n < MAX_LAYER + 2; n++) {
		if ((pcb_layer_flags(n) & mask) == mask)
			APPEND(n);
	}
	return used;
}

int pcb_layer_group_list(pcb_layer_type_t mask, int *res, int res_len)
{
	int group, layeri, used = 0;
	for (group = 0; group < max_group; group++) {
		for (layeri = 0; layeri < PCB->LayerGroups.Number[group]; layeri++) {
			int layer = PCB->LayerGroups.Entries[group][layeri];
			if ((pcb_layer_flags(layer) & mask) == mask) {
				APPEND(group);
				goto added; /* do not add a group twice */
			}
		}
		added:;
	}
	return used;
}

int pcb_layer_by_name(const char *name)
{
	int n;
	for (n = 0; n < max_copper_layer + 2; n++)
		if (strcmp(PCB->Data->Layer[n].Name, name) == 0)
			return n;
	return -1;
}

int pcb_layer_lookup_group(int layer_id)
{
	int group, layeri;
	for (group = 0; group < max_group; group++) {
		for (layeri = 0; layeri < PCB->LayerGroups.Number[group]; layeri++) {
			int layer = PCB->LayerGroups.Entries[group][layeri];
			if (layer == layer_id)
				return group;
		}
	}
	return -1;
}

void pcb_layer_add_in_group(int layer_id, int group_id)
{
	int glen = PCB->LayerGroups.Number[group_id];
	PCB->LayerGroups.Entries[group_id][glen] = layer_id;
	PCB->LayerGroups.Number[group_id]++;
}


void pcb_layers_reset()
{
	int n;

	/* reset layer names */
	for(n = 2; n < MAX_LAYER; n++) {
		if (PCB->Data->Layer[n].Name != NULL)
			free((char *)PCB->Data->Layer[n].Name);
		PCB->Data->Layer[n].Name = pcb_strdup("<pcb_layers_reset>");
	}

	/* reset layer groups */
	for(n = 0; n < MAX_LAYER; n++)
		PCB->LayerGroups.Number[n] = 0;

	/* set up one copper layer on top and one on bottom */
	PCB->Data->LayerN = 2;
	PCB->LayerGroups.Number[SOLDER_LAYER] = 1;
	PCB->LayerGroups.Number[COMPONENT_LAYER] = 1;
	PCB->LayerGroups.Entries[SOLDER_LAYER][0] = SOLDER_LAYER;
	PCB->LayerGroups.Entries[COMPONENT_LAYER][0] = COMPONENT_LAYER;

	/* Name top and bottom layers */
	if (PCB->Data->Layer[COMPONENT_LAYER].Name != NULL)
		free((char *)PCB->Data->Layer[COMPONENT_LAYER].Name);
	PCB->Data->Layer[COMPONENT_LAYER].Name = pcb_strdup("<top>");

	if (PCB->Data->Layer[SOLDER_LAYER].Name != NULL)
		free((char *)PCB->Data->Layer[SOLDER_LAYER].Name);
	PCB->Data->Layer[SOLDER_LAYER].Name = pcb_strdup("<bottom>");
}

int pcb_layer_create(pcb_layer_type_t type, pcb_bool reuse_layer, pcb_bool_t reuse_group, const char *lname)
{
	int id, grp = -1, found;
	unsigned int loc  = type & PCB_LYT_ANYWHERE;
	unsigned int role = type & PCB_LYT_ANYTHING;

	/* look for an existing layer if reuse is enabled */
	if (reuse_layer) {
		switch(role) {
			case PCB_LYT_MASK:
			case PCB_LYT_PASTE:
			case PCB_LYT_SILK:
				return -1; /* do not create silk, paste or mask layers, they are special */

			case PCB_LYT_COPPER:
				switch(loc) {
					case PCB_LYT_TOP:    return COMPONENT_LAYER;
					case PCB_LYT_BOTTOM: return SOLDER_LAYER;
					case PCB_LYT_INTERN:
						for(grp = 2; grp < MAX_LAYER; grp++) {
							if (PCB->LayerGroups.Number[grp] > 0) {
								id = PCB->LayerGroups.Entries[grp][0];
								if (strcmp(PCB->Data->Layer[id].Name, "outline") != 0)
									return id;
							}
						}
						return -1;
				}
				break;

			case PCB_LYT_OUTLINE:
				for(grp = 2; grp < MAX_LAYER; grp++) {
					if (PCB->LayerGroups.Number[grp] > 0) {
						id = PCB->LayerGroups.Entries[grp][0];
						if (strcmp(PCB->Data->Layer[id].Name, "outline") == 0)
							return id;
					}
				}
				return -1;
		}
		return -1;
	}

	/* Need to create a new layers, look for an existing group first */
	if (role == PCB_LYT_OUTLINE) {
		lname = "outline";

		for(id = 0; id < PCB->Data->LayerN; id++)
			if (strcmp(PCB->Data->Layer[id].Name, lname) == 0)
				return id; /* force reuse outline */

		/* not found: create a new layer for outline */
		grp = -1;
		reuse_group = pcb_false;
	}

	/* there's only one top and bottom group, always reuse them */
	if (role == PCB_LYT_COPPER) {
		switch(loc) {
			case PCB_LYT_TOP:    grp = COMPONENT_LAYER; reuse_group = 0; break;
			case PCB_LYT_BOTTOM: grp = SOLDER_LAYER; reuse_group = 0; break;
		}
	}

	if (reuse_group) { /* can't use group find here, it depends on existing silks! */
		switch(role) {
			case PCB_LYT_MASK:
			case PCB_LYT_PASTE:
			case PCB_LYT_SILK:
				return -1; /* do not create silk, paste or mask layers, they are special */

			case PCB_LYT_COPPER:
				switch(loc) {
					case PCB_LYT_TOP:
					case PCB_LYT_BOTTOM:
						abort(); /* can't get here */
					case PCB_LYT_INTERN:
						/* find the first internal layer */
						for(found = 0, grp = 2; grp < MAX_LAYER; grp++) {
							if (PCB->LayerGroups.Number[grp] > 0) {
								id = PCB->LayerGroups.Entries[grp][0];
								if (strcmp(PCB->Data->Layer[id].Name, "outline") != 0) {
									found = 1;
									break;
								}
							}
						}
						if (!found)
							return -1;
						id = -1;
				}
				break;
			case PCB_LYT_OUTLINE:
				abort(); /* can't get here */
		}
	}

	if (grp < 0) {
		/* Also need to create a group */
		for(grp = 0; grp < MAX_LAYER; grp++)
			if (PCB->LayerGroups.Number[grp] == 0)
				break;
		if (grp >= MAX_LAYER)
			return -2;
	}

	id = PCB->Data->LayerN++;

	if (lname != NULL) {
		if (PCB->Data->Layer[id].Name != NULL)
			free((char *)PCB->Data->Layer[id].Name);
		PCB->Data->Layer[id].Name = pcb_strdup(lname);
	}

	/* add layer to group */
	PCB->LayerGroups.Entries[grp][PCB->LayerGroups.Number[grp]] = id;
	PCB->LayerGroups.Number[grp]++;

	return id;
}

/* Temporary hack: silk layers have to be added as the last entry in the top and bottom layer groups, if they are not already in */
static void hack_in_silks()
{
	int sl, cl, found, n;

	sl = SOLDER_LAYER + PCB->Data->LayerN;
	for(found = 0, n = 0; n < PCB->LayerGroups.Number[SOLDER_LAYER]; n++)
		if (PCB->LayerGroups.Entries[SOLDER_LAYER][n] == sl)
			found = 1;

	if (!found) {
		PCB->LayerGroups.Entries[SOLDER_LAYER][PCB->LayerGroups.Number[SOLDER_LAYER]] = sl;
		PCB->LayerGroups.Number[SOLDER_LAYER]++;
		if (PCB->Data->Layer[sl].Name != NULL)
			free((char *)PCB->Data->Layer[sl].Name);
		PCB->Data->Layer[sl].Name = pcb_strdup("silk");
	}


	cl = COMPONENT_LAYER + PCB->Data->LayerN;
	for(found = 0, n = 0; n < PCB->LayerGroups.Number[COMPONENT_LAYER]; n++)
		if (PCB->LayerGroups.Entries[COMPONENT_LAYER][n] == cl)
			found = 1;

	if (!found) {
		PCB->LayerGroups.Entries[COMPONENT_LAYER][PCB->LayerGroups.Number[COMPONENT_LAYER]] = cl;
		PCB->LayerGroups.Number[COMPONENT_LAYER]++;
		if (PCB->Data->Layer[cl].Name != NULL)
			free((char *)PCB->Data->Layer[cl].Name);
		PCB->Data->Layer[cl].Name = pcb_strdup("silk");
	}
}

int pcb_layer_rename(int layer, const char *lname)
{
	if (PCB->Data->Layer[layer].Name != NULL)
		free((char *)PCB->Data->Layer[layer].Name);
	PCB->Data->Layer[layer].Name = pcb_strdup(lname);
	return 0;
}

void pcb_layers_finalize()
{
	hack_in_silks();
}

#undef APPEND
