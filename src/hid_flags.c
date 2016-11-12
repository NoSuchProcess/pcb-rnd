#include "config.h"
#include "data.h"
#include "board.h"
#include "conf.h"

#include "hid_flags.h"
#include "genht/hash.h"
#include "genht/htsp.h"
#include "error.h"
#include "hid_actions.h"

int hid_get_flag(const char *name)
{
	const char *cp;

	if (name == NULL)
		return -1;

	cp = strchr(name, '(');
	if (cp == NULL) {
		conf_native_t *n = conf_get_field(name);
		if (n == NULL)
			return -1;
		if ((n->type != CFN_BOOLEAN) || (n->used != 1))
			return -1;
		return n->val.boolean[0];
	}
	else {
		const char *end, *s;
		const char *argv[2];
		if (cp != NULL) {
			const pcb_hid_action_t *a;
			char buff[256];
			int len, multiarg;
			len = cp - name;
			if (len > sizeof(buff)-1) {
				Message(PCB_MSG_DEFAULT, "hid_get_flag: action name too long: %s()\n", name);
				return -1;
			}
			memcpy(buff, name, len);
			buff[len] = '\0';
			a = hid_find_action(buff);
			if (!a) {
				Message(PCB_MSG_DEFAULT, "hid_get_flag: no action %s\n", name);
				return -1;
			}
			cp++;
			len = strlen(cp);
			end = NULL;
			multiarg = 0;
			for(s = cp; *s != '\0'; s++) {
				if (*s == ')') {
					end = s;
					break;
				}
				if (*s == ',')
					multiarg = 1;
			}
			if (!multiarg) {
				/* faster but limited way for a single arg */
				if ((len > sizeof(buff)-1) || (end == NULL)) {
					Message(PCB_MSG_DEFAULT, "hid_get_flag: action arg too long or unterminated: %s\n", name);
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
				/* slower but more generic way */
				return hid_parse_command(name);
			}
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
