/* $Id$ */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "data.h"
#include "misc.h"

#include "hid.h"
#include "../hidint.h"
#include "genht/hash.h"
#include "genht/htsp.h"

RCSID("$Id$");

typedef struct HID_FlagNode {
	HID_Flag *flags;
	int n;
	const char *cookie;
} HID_FlagNode;

static htsp_t *hid_flags = NULL;

static int keyeq(char *a, char *b)
{
	return !strcmp(a, b);
}

void hid_register_flags(HID_Flag * a, int numact, const char *cookie)
{
	HID_FlagNode *ha;
	HID_Flag *f;
	int n;

	if (hid_flags == NULL)
		hid_flags = htsp_alloc(strhash, keyeq);

	for(f = a, n = 0; n < numact; n++, f++) {
		if (htsp_get(hid_flags, f->name) != NULL) {
			fprintf(stderr, "ERROR: can't register flag %s for cookie %s: name already in use\n", f->name, cookie);
			return;
		}

		/* printf("%d flag%s registered\n", n, n==1 ? "" : "s"); */
		ha = (HID_FlagNode *) malloc(sizeof(HID_FlagNode));
		ha->flags = f;
		ha->n = n;
		ha->cookie = cookie;

		htsp_set(hid_flags, f->name, ha);
	}
}

void hid_remove_flags_by_cookie(const char *cookie)
{
	htsp_entry_t *e;
	HID_FlagNode *ha;

	if (hid_flags == NULL)
		return NULL;

	for(e = htsp_first(hid_flags); e; e = htsp_next(hid_flags, e)) {
		ha = e->value;
		if (ha->cookie == cookie) {
			htsp_pop(hid_flags, e->key);
			free(ha);
		}
	}
}


void hid_flags_uninit(void)
{
	if (hid_flags != NULL) {
		htsp_entry_t *e;
		for(e = htsp_first(hid_flags); e; e = htsp_next(hid_flags, e)) {
			HID_FlagNode *ha;
			ha = e->value;
			if (ha->cookie != NULL)
				fprintf(stderr, "Warning: uninitialized flag in hid_flags_uninit: %s by %s; check your plugins' uninit!\n", e->key, ha->cookie);
			htsp_pop(hid_flags, e->key);
			free(ha);
		}
		htsp_free(hid_flags);
		hid_flags = NULL;
	}
}

HID_Flag *hid_find_flag(const char *name)
{
	HID_FlagNode *ha;

	if (hid_flags == NULL)
		return NULL;

	ha = htsp_get(hid_flags, (char *)name);
	if (ha == NULL) {
		fprintf(stderr, "ERROR: hid_find_flag(): flag not found '%s'\n", name);
		return NULL;
	}

	return ha->flags;
}

int hid_get_flag(const char *name)
{
	static char *buf = 0;
	static int nbuf = 0;
	const char *cp;
	HID_Flag *f;

	cp = strchr(name, ',');
	if (cp) {
		int wv;

		if (nbuf < (cp - name + 1)) {
			nbuf = cp - name + 10;
			buf = (char *) realloc(buf, nbuf);
		}
		memcpy(buf, name, cp - name);
		buf[cp - name] = 0;
		/* A number without units is just a number.  */
		wv = GetValueEx(cp + 1, NULL, NULL, NULL, NULL);
		f = hid_find_flag(buf);
		if (!f)
			return 0;
		return f->function(f->parm) == wv;
	}

	f = hid_find_flag(name);
	if (!f)
		return 0;
	return f->function(f->parm);
}


void hid_save_and_show_layer_ons(int *save_array)
{
	int i;
	for (i = 0; i < max_copper_layer + 2; i++) {
		save_array[i] = PCB->Data->Layer[i].On;
		PCB->Data->Layer[i].On = 1;
	}
}

void hid_restore_layer_ons(int *save_array)
{
	int i;
	for (i = 0; i < max_copper_layer + 2; i++)
		PCB->Data->Layer[i].On = save_array[i];
}

const char *layer_type_to_file_name(int idx, int style)
{
	int group;
	int nlayers;
	const char *single_name;

	switch (idx) {
	case SL(SILK, TOP):
		return "topsilk";
	case SL(SILK, BOTTOM):
		return "bottomsilk";
	case SL(MASK, TOP):
		return "topmask";
	case SL(MASK, BOTTOM):
		return "bottommask";
	case SL(PDRILL, 0):
		return "plated-drill";
	case SL(UDRILL, 0):
		return "unplated-drill";
	case SL(PASTE, TOP):
		return "toppaste";
	case SL(PASTE, BOTTOM):
		return "bottompaste";
	case SL(INVISIBLE, 0):
		return "invisible";
	case SL(FAB, 0):
		return "fab";
	case SL(ASSY, TOP):
		return "topassembly";
	case SL(ASSY, BOTTOM):
		return "bottomassembly";
	default:
		group = GetLayerGroupNumberByNumber(idx);
		nlayers = PCB->LayerGroups.Number[group];
		single_name = PCB->Data->Layer[idx].Name;
		if (group == GetLayerGroupNumberByNumber(component_silk_layer)) {
			if (style == FNS_first || (style == FNS_single && nlayers == 2))
				return single_name;
			return "top";
		}
		else if (group == GetLayerGroupNumberByNumber(solder_silk_layer)) {
			if (style == FNS_first || (style == FNS_single && nlayers == 2))
				return single_name;
			return "bottom";
		}
		else if (nlayers == 1
						 && (strcmp(PCB->Data->Layer[idx].Name, "route") == 0 || strcmp(PCB->Data->Layer[idx].Name, "outline") == 0)) {
			return "outline";
		}
		else {
			static char buf[20];
			if (style == FNS_first || (style == FNS_single && nlayers == 1))
				return single_name;
			sprintf(buf, "group%d", group);
			return buf;
		}
		break;
	}
}
