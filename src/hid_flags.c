/* $Id$ */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "misc.h"
#include "conf.h"

#include "hid.h"
#include "hid_flags.h"
#include "genht/hash.h"
#include "genht/htsp.h"
#include "error.h"
#include "hid_actions.h"

RCSID("$Id$");

int hid_get_flag(const char *name)
{
	const char *cp;

	if (name == NULL)
		return -1;

	cp = strchr(name, '/');
	if (cp) {
		conf_native_t *n = conf_get_field(name);
		if (n == NULL)
			return -1;
		if ((n->type != CFN_BOOLEAN) || (n->used != 1))
			return -1;
		return n->val.boolean[0];
	}
	else {
		char *end;
		char *argv[2];
		cp = strchr(name, '(');
		if (cp != NULL) {
			const HID_Action *a;
			char buff[256];
			int len;
			len = cp - name;
			if (len > sizeof(buff)-1) {
				Message("hid_get_flag: action name too long: %s()\n", name);
				return -1;
			}
			memcpy(buff, name, len);
			buff[len] = '\0';
			a = hid_find_action(buff);
			if (!a) {
				Message("hid_get_flag: no action %s\n", name);
				return -1;
			}
			cp++;
			len = strlen(cp);
			end = strchr(cp, ')');
			if ((len > sizeof(buff)-1) || (end == NULL)) {
				Message("hid_get_flag: action arg too long or unterminated: %s\n", name);
				return -1;
			}
			len = end - cp;
			memcpy(buff, cp, len);
			buff[len] = '\0';
			argv[0] = buff;
			argv[1] = NULL;
			return hid_actionv_(a, len > 0, argv);
		}
		else {
			fprintf(stderr, "ERROR: hid_get_flag(%s) - not a path or an action\n", name);
		}
	}
	return -1;
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

